#include "Entity.h"

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include "../Assets.h"
#include "../Level.h"
#include "../Hexagons.h"
#include "../Animation.h"
#include "../Audio.h"

#define SHAKE_DECAY 0.01f
#define HIT_SHAKE_POWER 0.1f

static const struct EntityAPI *(*entity_API_getters[ENTITY_TYPE_COUNT])(void) = {
        [ENTITY_PLAYER] = get_player_entity_API,
        [ENTITY_BLOCK] = get_block_entity_API
};

#define INVOKE_ENTITY_METHOD(entity, method, ...)                                                \
        do {                                                                                     \
                const struct EntityAPI *const entity_API = entity_API_getters[(entity)->type](); \
                if (entity_API->method) {                                                        \
                        entity_API->method(__VA_ARGS__);                                         \
                }                                                                                \
        } while (0)

struct Entity *create_entity(struct Level *const level, const enum EntityType type, const uint8_t column, const uint8_t row, const enum Orientation orientation) {
        struct Entity *const entity = (struct Entity *)xmalloc(sizeof(struct Entity));
        entity->level = level;
        entity->type = type;
        entity->current_column = column;
        entity->current_row = row;
        entity->previous_column = column;
        entity->previous_row = row;
        entity->position = (SDL_FPoint){0.0f, 0.0f};
        entity->grid_metrics = NULL;
        entity->orientation = orientation;
        entity->shake_offset = (SDL_FPoint){0.0f, 0.0f};
        entity->shake_power = 0.0f;
        entity->payload = NULL;
        entity->flag = 0ULL;

        initialize_animation(&entity->movement, 1ULL);
        struct Action *const moving = &entity->movement.actions[0];
        moving->target.point_pointer = &entity->position;
        moving->type = ACTION_POINT;
        moving->lazy_start = true;
        moving->duration = 150.0f;

        INVOKE_ENTITY_METHOD(entity, setup, entity);
        return entity;
}

void destroy_entity(struct Entity *const entity) {
        if (!entity) {
                send_message(WARNING, "Given entity to destroy is NULL");
                return;
        }

        INVOKE_ENTITY_METHOD(entity, destroy, entity);
        deinitialize_animation(&entity->movement);
        xfree(entity);
}

void set_entity_grid_metrics(struct Entity *const entity, const struct GridMetrics *const grid_metrics) {
        entity->grid_metrics = grid_metrics;

        if (!entity->movement.active) {
                get_grid_tile_position(grid_metrics, entity->current_column, entity->current_row, &entity->position.x, &entity->position.y);
                return;
        }

        SDL_FPoint *const start_keyframe = &entity->movement.actions->keyframes.points[0];
        get_grid_tile_position(grid_metrics, entity->previous_column, entity->previous_row, &start_keyframe->x, &start_keyframe->y);
}

bool move_entity(struct Entity *const entity, const enum Orientation orientation, const bool initiator, const bool pull_recursion, const enum MoveType move_type) {
        if (entity->movement.active) {
                return false;
        }

        int8_t next_column = (int8_t)entity->current_column;
        int8_t next_row = (int8_t)entity->current_row;
        int8_t contact_column = (int8_t)entity->current_column;
        int8_t contact_row = (int8_t)entity->current_row;

        if (move_type != MOVE_UNDO || !pull_recursion) {
                orientation_advance(orientation, &contact_column, &contact_row);
                next_column = contact_column;
                next_row = contact_row;
        } else {
                orientation_advance(orientation, &next_column, &next_row);
                orientation_advance(orientation_reverse(orientation), &contact_column, &contact_row);
        }

        const struct TileState tile_state = get_level_tile_state(entity->level, contact_column, contact_row);
        if ((move_type != MOVE_UNDO && tile_state.tile_type == EMPTY) || (tile_state.entity && !move_entity(tile_state.entity, orientation, false, pull_recursion, move_type))) {
                entity->shake_power = entity->shake_power > HIT_SHAKE_POWER ? entity->shake_power : HIT_SHAKE_POWER;
                if (tile_state.tile_type == EMPTY) {
                        play_sound(SOUND_HIT);
                }

                return false;
        }

        entity->previous_column = entity->current_column;
        entity->previous_row = entity->current_row;
        entity->current_column = (uint8_t)next_column;
        entity->current_row = (uint8_t)next_row;

        struct Action *const moving = &entity->movement.actions[0];
        moving->easing = initiator ? (tile_state.entity ? QUAD_OUT : QUAD_IN_OUT) : QUAD_IN;
        get_grid_tile_position(entity->grid_metrics, entity->current_column, entity->current_row, &moving->keyframes.points[1].x, &moving->keyframes.points[1].y);
        start_animation(&entity->movement, 0ULL);

        if (moving->easing == QUAD_IN_OUT) {
                play_sound(SOUND_MOVE);
        } else if (!tile_state.entity) {
                play_sound(SOUND_PUSH);
        }

        INVOKE_ENTITY_METHOD(entity, moved, entity, orientation, tile_state, pull_recursion, move_type);
        return true;
}

void entity_receive_event(struct Entity *const entity, const SDL_Event *const event) {
        INVOKE_ENTITY_METHOD(entity, receive_event, entity, event);
}

void update_entity(struct Entity *const entity, const double delta_time) {
        entity->shake_offset = (SDL_FPoint){0.0f, 0.0f};
        if (entity->shake_power != 0.0f) {
                // TODO: Use an animationn instead of randomizing the shake position since it causes distortion due to renderer vsync
                const float angle = RANDOM_NUMBER(0.0f, 2.0f * (float)M_PI);
                entity->shake_offset.x += cosf(angle) * entity->shake_power * entity->grid_metrics->tile_radius;
                entity->shake_offset.y += sinf(angle) * entity->shake_power * entity->grid_metrics->tile_radius;
                entity->shake_power -= entity->shake_power * SHAKE_DECAY * delta_time;
                entity->shake_power = entity->shake_power > 0.0f ? entity->shake_power : 0.0f;
        }

        update_animation(&entity->movement, delta_time);
        INVOKE_ENTITY_METHOD(entity, update, entity, delta_time);
}