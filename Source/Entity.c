#include "Entity.h"

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>

#include "Assets.h"
#include "Level.h"
#include "Hexagons.h"
#include "Animation.h"
#include "Audio.h"

struct Entity {
        struct Level *level;
        enum EntityType type;
        struct Geometry *geometry;
        uint16_t last_tile_index;
        uint16_t next_tile_index;
        enum Orientation last_orientation;
        enum Orientation next_orientation;
        struct Animation recoiling;
        struct Animation moving;
        struct Animation turning;
        struct Animation scaling;
        struct Animation shaking;
        SDL_FPoint position;
        float angle;
        float scale;
        float radius;
        union {
                struct Player {
                        float wings_angle;
                        struct Animation flapping;
                        struct Animation bouncing;
                        SDL_FPoint antenna_offset;
                        float float_time;
                } player;

                struct Block {
                } block;
        } as;
};

#define PLAYER_CLOSED_WINGS_ANGLE (-(float)M_PI * 5.0f / 6.0f);
#define PLAYER_OPEN_WINGS_ANGLE   (-(float)M_PI * 4.0f / 6.0f);

#define PULSE_ENTITY_SCALE(entity, scale)                                   \
        do {                                                                \
                (entity)->scaling.actions[0].keyframes.floats[1] = (scale); \
                restart_animation(&(entity)->scaling, 0ULL);                \
        } while (0);                                                        \

struct Entity *create_entity(struct Level *const level, const enum EntityType type, const uint16_t tile_index, const enum Orientation orientation) {
        struct Entity *const entity = (struct Entity *)xmalloc(sizeof(struct Entity));
        entity->type = type;
        entity->level = level;
        entity->geometry = create_geometry();
        entity->last_tile_index = tile_index;
        entity->next_tile_index = tile_index;
        entity->last_orientation = orientation;
        entity->next_orientation = orientation;
        entity->angle = orientation_angle(orientation);
        entity->scale = 1.0f;
        entity->radius = 0.0f;

        initialize_animation(&entity->recoiling, 2ULL);

        struct Action *const move_away = &entity->recoiling.actions[0];
        move_away->target.point_pointer = &entity->position;
        move_away->type = ACTION_POINT;
        move_away->easing = QUAD_OUT;
        move_away->lazy_start = true;
        move_away->duration = 150.0f;

        struct Action *const move_back = &entity->recoiling.actions[1];
        move_back->target.point_pointer = &entity->position;
        move_back->type = ACTION_POINT;
        move_back->easing = QUAD_IN;
        move_back->lazy_start = true;
        move_back->duration = 150.0f;

        initialize_animation(&entity->moving, 1ULL);
        struct Action *const moving_action = &entity->moving.actions[0];
        moving_action->target.point_pointer = &entity->position;
        moving_action->type = ACTION_POINT;
        moving_action->lazy_start = true;
        moving_action->duration = 100.0f;

        initialize_animation(&entity->turning, 1ULL);
        struct Action *const turning_action = &entity->turning.actions[0];
        turning_action->target.float_pointer = &entity->angle;
        turning_action->type = ACTION_FLOAT;
        turning_action->easing = SINE_OUT;
        turning_action->lazy_start = true;
        turning_action->duration = 100.0f;
        turning_action->offset = true;

        initialize_animation(&entity->scaling, 2ULL);

        struct Action *const scale_up = &entity->scaling.actions[0];
        scale_up->target.float_pointer = &entity->scale;
        scale_up->type = ACTION_FLOAT;
        scale_up->easing = QUAD_OUT;
        scale_up->lazy_start = true;
        scale_up->duration = 50.0f;

        struct Action *const scale_down = &entity->scaling.actions[1];
        scale_down->target.float_pointer = &entity->scale;
        scale_down->keyframes.floats[1] = 1.0f;
        scale_down->type = ACTION_FLOAT;
        scale_down->easing = SINE_IN;
        scale_down->lazy_start = true;
        scale_down->duration = 200.0f;

        if (entity->type == ENTITY_PLAYER) {
                struct Player *const player = &entity->as.player;
                player->wings_angle = PLAYER_CLOSED_WINGS_ANGLE;
                player->antenna_offset.x = 0.0f;
                player->antenna_offset.y = 0.0f;

                initialize_animation(&player->flapping, 2ULL);

                struct Action *const wings_opening = &player->flapping.actions[0];
                wings_opening->target.float_pointer = &player->wings_angle;
                wings_opening->keyframes.floats[0] = PLAYER_CLOSED_WINGS_ANGLE;
                wings_opening->keyframes.floats[1] = PLAYER_OPEN_WINGS_ANGLE;
                wings_opening->type = ACTION_FLOAT;
                wings_opening->easing = SINE_IN;
                wings_opening->duration = 60.0f;

                struct Action *const wings_closing = &player->flapping.actions[1];
                wings_closing->target.float_pointer = &player->wings_angle;
                wings_closing->keyframes.floats[0] = PLAYER_OPEN_WINGS_ANGLE;
                wings_closing->keyframes.floats[1] = PLAYER_CLOSED_WINGS_ANGLE;
                wings_closing->type = ACTION_FLOAT;
                wings_closing->easing = SINE_OUT;
                wings_closing->duration = 60.0f;
                wings_closing->delay = 30.0f;

                initialize_animation(&player->bouncing, 2ULL);

                struct Action *const bounce_away = &player->bouncing.actions[0];
                bounce_away->target.point_pointer = &player->antenna_offset;
                bounce_away->type = ACTION_POINT;
                bounce_away->easing = SINE_OUT;
                bounce_away->lazy_start = true;
                bounce_away->duration = 100.0f;

                struct Action *const bounce_back = &player->bouncing.actions[1];
                bounce_back->target.point_pointer = &player->antenna_offset;
                bounce_back->keyframes.points[1].x = 0.0f;
                bounce_back->keyframes.points[1].y = 0.0f;
                bounce_back->type = ACTION_POINT;
                bounce_back->easing = SINE_IN_OUT;
                bounce_back->lazy_start = true;
                bounce_back->duration = 100.0f;
        }

        return entity;
}

void destroy_entity(struct Entity *const entity) {
        if (!entity) {
                send_message(WARNING, "Given entity to destroy is NULL");
                return;
        }

        if (entity->type == ENTITY_PLAYER) {
                struct Player *const player = &entity->as.player;
                deinitialize_animation(&player->flapping);
                deinitialize_animation(&player->bouncing);
        }

        deinitialize_animation(&entity->moving);
        deinitialize_animation(&entity->turning);
        deinitialize_animation(&entity->scaling);
        deinitialize_animation(&entity->recoiling);
        destroy_geometry(entity->geometry);
        xfree(entity);
}

void update_entity(struct Entity *const entity, const double delta_time) {
        update_animation(&entity->moving, delta_time);
        update_animation(&entity->turning, delta_time);
        update_animation(&entity->scaling, delta_time);
        update_animation(&entity->recoiling, delta_time);

        const float radius = entity->radius * entity->scale;

        if (entity->type == ENTITY_PLAYER) {
                struct Player *const player = &entity->as.player;

                update_animation(&player->flapping, delta_time);
                update_animation(&player->bouncing, delta_time);

                float x = entity->position.x;
                float y = entity->position.y;

                player->float_time += delta_time / 500.0f;
                const float float_x = cosf(player->float_time) / 5.0f;
                const float float_y = sinf(player->float_time) / 5.0f;
                const float float_angle = (float_x + float_y) / 2.5f;

                const float wings_angle = player->wings_angle + float_angle;
                const float rotation = entity->angle + float_angle;

                x += float_x * radius / 5.0f;
                y += float_y * radius / 5.0f;

                const float body_length = radius * 1.25f;
                const float body_thickness = radius / 1.5f;
                const float line_width = radius / 10.0f;

                SDL_FPoint back_circle_position = (SDL_FPoint){x - body_length / 2.0f + body_thickness / 2.0f, y};
                SDL_FPoint front_circle_position = (SDL_FPoint){x + body_length / 2.0f - body_thickness / 2.0f, y};

                const float outer_circle_radius = body_thickness / 2.0f + line_width / 2.0f;
                const float inner_circle_radius = body_thickness / 2.0f - line_width / 2.0f;

                const SDL_FRect main_body_segment = (SDL_FRect){
                        .x = x,
                        .y = y,
                        .w = body_length - body_thickness,
                        .h = body_thickness + line_width
                };

                const SDL_FPoint left_antenna_tip_position = (SDL_FPoint){
                        .x = front_circle_position.x + radius / 1.5f,
                        .y = y - radius / 1.5f
                };

                const SDL_FPoint right_antenna_tip_position = (SDL_FPoint){
                        .x = front_circle_position.x + radius / 1.5f,
                        .y = y + radius / 1.5f
                };

                SDL_FPoint left_antenna_endpoints[] = {
                        (SDL_FPoint){front_circle_position.x + body_thickness / 3.0f, y - body_thickness / 3.0f},
                        (SDL_FPoint){left_antenna_tip_position.x, left_antenna_tip_position.y}
                };

                SDL_FPoint left_antenna_control_points[] = {
                        (SDL_FPoint){left_antenna_tip_position.x - line_width * 1.5f, left_antenna_tip_position.y + body_thickness / 1.5f},
                        (SDL_FPoint){left_antenna_tip_position.x - line_width * 0.0f, left_antenna_tip_position.y + body_thickness / 2.5f}
                };

                SDL_FPoint right_antenna_endpoints[] = {
                        (SDL_FPoint){front_circle_position.x + body_thickness / 3.0f, y + body_thickness / 3.0f},
                        (SDL_FPoint){right_antenna_tip_position.x, right_antenna_tip_position.y}
                };

                SDL_FPoint right_antenna_control_points[] = {
                        (SDL_FPoint){right_antenna_tip_position.x - line_width * 1.5f, right_antenna_tip_position.y - body_thickness / 1.5f},
                        (SDL_FPoint){right_antenna_tip_position.x - line_width * 0.0f, right_antenna_tip_position.y - body_thickness / 2.5f}
                };

                left_antenna_endpoints[1].x       += radius * player->antenna_offset.x;
                left_antenna_endpoints[1].y       += radius * player->antenna_offset.y;
                right_antenna_endpoints[1].x      += radius * player->antenna_offset.x;
                right_antenna_endpoints[1].y      += radius * player->antenna_offset.y;
                left_antenna_control_points[1].x  += radius * player->antenna_offset.x / 2.0f;
                left_antenna_control_points[1].y  += radius * player->antenna_offset.y / 2.0f;
                right_antenna_control_points[1].x += radius * player->antenna_offset.x / 2.0f;
                right_antenna_control_points[1].y += radius * player->antenna_offset.y / 2.0f;

                SDL_FPoint stinger[] = {
                        (SDL_FPoint){x - body_length / 2.0f,                      y + line_width * 1.5f},
                        (SDL_FPoint){x - body_length / 2.0f,                      y - line_width * 1.5f},
                        (SDL_FPoint){x - body_length / 2.0f - line_width * 1.25f, y                    }
                };

                const float wings_length = body_thickness - line_width;
                const float wings_thickness = (wings_length - line_width) / 2.0f;
                const SDL_FPoint wings_border_radii = (SDL_FPoint){wings_length + line_width / 2.0f, wings_thickness + line_width / 2.0f};
                const SDL_FPoint wings_filled_radii = (SDL_FPoint){wings_length - line_width / 2.0f, wings_thickness - line_width / 2.0f};

                const float left_wing_angle = wings_angle;
                const float right_wing_angle = 2.0f * (float)M_PI - left_wing_angle;
                const float wings_anchor_x = front_circle_position.x - line_width * 1.5f;
                const float wings_anchor_y = y;

                const SDL_FPoint wing_center = (SDL_FPoint){
                        .x = wings_anchor_x + wings_length / 1.5f,
                        .y = wings_anchor_y
                };

                SDL_FPoint left_wing_center = wing_center;
                rotate_point(&left_wing_center.x, &left_wing_center.y, wings_anchor_x, wings_anchor_y, left_wing_angle);
                left_wing_center.y -= line_width;

                SDL_FPoint right_wing_center = wing_center;
                rotate_point(&right_wing_center.x, &right_wing_center.y, wings_anchor_x, wings_anchor_y, right_wing_angle);
                right_wing_center.y += line_width;

                SDL_FPoint *const points[] = {
                        &back_circle_position,
                        &front_circle_position,
                        &left_antenna_endpoints[0],
                        &left_antenna_endpoints[1],
                        &left_antenna_control_points[0],
                        &left_antenna_control_points[1],
                        &right_antenna_endpoints[0],
                        &right_antenna_endpoints[1],
                        &right_antenna_control_points[0],
                        &right_antenna_control_points[1],
                        &stinger[0],
                        &stinger[1],
                        &stinger[2],
                        &left_wing_center,
                        &right_wing_center
                };

                const size_t point_count = sizeof(points) / sizeof(points[0]);
                for (size_t point_index = 0ULL; point_index < point_count; ++point_index) {
                        rotate_point(&points[point_index]->x, &points[point_index]->y, x, y, -rotation);
                }

                clear_geometry(entity->geometry);

                set_geometry_color(entity->geometry, COLOR_DARK_BROWN, COLOR_OPAQUE);
                write_circle_geometry(entity->geometry, back_circle_position.x, back_circle_position.y, outer_circle_radius);
                write_circle_geometry(entity->geometry, front_circle_position.x, front_circle_position.y, outer_circle_radius);

                set_geometry_color(entity->geometry, COLOR_YELLOW, COLOR_OPAQUE);
                write_circle_geometry(entity->geometry, back_circle_position.x, back_circle_position.y, inner_circle_radius);
                write_circle_geometry(entity->geometry, front_circle_position.x, front_circle_position.y, inner_circle_radius);

                set_geometry_color(entity->geometry, COLOR_DARK_BROWN, COLOR_OPAQUE);
                write_rectangle_geometry(entity->geometry, main_body_segment.x, main_body_segment.y, main_body_segment.w, main_body_segment.h, -rotation);
                write_circle_geometry(entity->geometry, left_antenna_endpoints[1].x, left_antenna_endpoints[1].y, line_width);
                write_circle_geometry(entity->geometry, right_antenna_endpoints[1].x, right_antenna_endpoints[1].y, line_width);

                write_bezier_curve_geometry(
                        entity->geometry,
                        left_antenna_endpoints[0].x,
                        left_antenna_endpoints[0].y,
                        left_antenna_endpoints[1].x,
                        left_antenna_endpoints[1].y,
                        left_antenna_control_points[0].x,
                        left_antenna_control_points[0].y,
                        left_antenna_control_points[1].x,
                        left_antenna_control_points[1].y,
                        line_width
                );

                write_bezier_curve_geometry(
                        entity->geometry,
                        right_antenna_endpoints[0].x,
                        right_antenna_endpoints[0].y,
                        right_antenna_endpoints[1].x,
                        right_antenna_endpoints[1].y,
                        right_antenna_control_points[0].x,
                        right_antenna_control_points[0].y,
                        right_antenna_control_points[1].x,
                        right_antenna_control_points[1].y,
                        line_width
                );

                write_triangle_geometry(entity->geometry, stinger[0].x, stinger[0].y, stinger[1].x, stinger[1].y, stinger[2].x, stinger[2].y);

                set_geometry_color(entity->geometry, COLOR_DARK_BROWN, COLOR_OPAQUE);
                write_ellipse_geometry(entity->geometry, left_wing_center.x, left_wing_center.y, wings_border_radii.x, wings_border_radii.y, -rotation + left_wing_angle);

                set_geometry_color(entity->geometry, COLOR_LIGHT_YELLOW, COLOR_OPAQUE);
                write_ellipse_geometry(entity->geometry, left_wing_center.x, left_wing_center.y, wings_filled_radii.x, wings_filled_radii.y, -rotation + left_wing_angle);

                set_geometry_color(entity->geometry, COLOR_DARK_BROWN, COLOR_OPAQUE);
                write_ellipse_geometry(entity->geometry, right_wing_center.x, right_wing_center.y, wings_border_radii.x, wings_border_radii.y, -rotation + right_wing_angle);

                set_geometry_color(entity->geometry, COLOR_LIGHT_YELLOW, COLOR_OPAQUE);
                write_ellipse_geometry(entity->geometry, right_wing_center.x, right_wing_center.y, wings_filled_radii.x, wings_filled_radii.y, -rotation + right_wing_angle);

                render_geometry(entity->geometry);
        }

        if (entity->type == ENTITY_BLOCK) {
                const float thickness = radius / 5.0f;
                const float x = entity->position.x;
                const float y = entity->position.y - thickness / 2.0f;

                clear_geometry(entity->geometry);

                set_geometry_color(entity->geometry, COLOR_GOLD, COLOR_OPAQUE);
                write_hexagon_thickness_geometry(entity->geometry, x, y, radius / 2.0f, thickness, HEXAGON_THICKNESS_MASK_ALL);

                set_geometry_color(entity->geometry, COLOR_LIGHT_YELLOW, COLOR_OPAQUE);
                write_hexagon_geometry(entity->geometry, x, y, radius / 2.0f, 0.0f);

                render_geometry(entity->geometry);
        }
}

void resize_entity(struct Entity *const entity, const float radius) {
        entity->radius = radius;

        query_level_tile(entity->level, entity->next_tile_index, NULL, NULL, &entity->position.x, &entity->position.y);

        if (entity->moving.active) {
                // If there is a moving animation, update the positions of the animation's start and end keyframes
                struct Action *const moving_action = &entity->moving.actions[0];
                query_level_tile(entity->level, entity->last_tile_index, NULL, NULL, &moving_action->keyframes.points[0].x, &moving_action->keyframes.points[0].y);
                query_level_tile(entity->level, entity->next_tile_index, NULL, NULL, &moving_action->keyframes.points[1].x, &moving_action->keyframes.points[1].y);
        }
}

enum EntityType get_entity_type(const struct Entity *const entity) {
        return entity->type;
}

uint16_t get_entity_tile_index(const struct Entity *const entity) {
        return entity->next_tile_index;
}

enum Orientation get_entity_orientation(const struct Entity *const entity) {
        return entity->next_orientation;
}

bool entity_can_change(const struct Entity *const entity) {
        return !entity->moving.active && !entity->turning.active && !entity->recoiling.active;
}

void entity_handle_change(struct Entity *const entity, const struct Change *const change) {
        if (change->type == CHANGE_TURN) {
                entity->last_orientation = change->turn.last_orientation;
                entity->next_orientation = change->turn.next_orientation;

                entity->turning.actions[0].keyframes.floats[1] = (change->input == INPUT_RIGHT ? -1.0f : 1.0f) * (float)M_PI * 2.0f / 6.0f;
                start_animation(&entity->turning, 0ULL);
                PULSE_ENTITY_SCALE(entity, 1.1f);

                if (entity->type == ENTITY_PLAYER) {
                        struct Player *const player = &entity->as.player;

                        struct Action *const bounce_away = &player->bouncing.actions[0];
                        bounce_away->keyframes.points[1].x = 0.125f;
                        bounce_away->keyframes.points[1].y = change->input == INPUT_RIGHT ? 0.125f : -0.125f;
                        start_animation(&player->bouncing, 0ULL);
                }

                return;
        }

        if (change->type == CHANGE_INVALID) {
                float x, y;
                query_level_tile(entity->level, entity->next_tile_index, NULL, NULL, &x, &y);

                const float angle = -orientation_angle(change->face.direction);

                struct Action *const move_away = &entity->recoiling.actions[0];
                move_away->keyframes.points[1].x = x + cosf(angle) * entity->radius / 5.0f;
                move_away->keyframes.points[1].y = y + sinf(angle) * entity->radius / 5.0f;

                struct Action *const move_back = &entity->recoiling.actions[1];
                move_back->keyframes.points[1].x = x;
                move_back->keyframes.points[1].y = y;

                start_animation(&entity->recoiling, 0ULL);
                PULSE_ENTITY_SCALE(entity, 1.1f);

                if (entity->type == ENTITY_PLAYER) {
                        struct Player *const player = &entity->as.player;
                        start_animation(&player->flapping, 0ULL);

                        struct Action *const bounce_away = &player->bouncing.actions[0];
                        bounce_away->keyframes.points[1].x = change->input == INPUT_FORWARD ? -0.125f : 0.125f;
                        bounce_away->keyframes.points[1].y = 0.0f;
                        start_animation(&player->bouncing, 0ULL);
                }

                return;
        }

        entity->last_tile_index = change->move.last_tile_index;
        entity->next_tile_index = change->move.next_tile_index;

        struct Action *const moving_action = &entity->moving.actions[0];
        query_level_tile(entity->level, entity->next_tile_index, NULL, NULL, &moving_action->keyframes.points[1].x, &moving_action->keyframes.points[1].y);

        switch (change->type) {
                case CHANGE_WALK: {
                        moving_action->easing = QUAD_IN_OUT;
                        break;
                }

                case CHANGE_PUSH: {
                        moving_action->easing = QUAD_OUT;
                        break;
                }

                case CHANGE_PUSHED: {
                        moving_action->easing = QUAD_IN;
                        break;
                }

                default: {
                        break;
                }
        }

        start_animation(&entity->moving, 0ULL);
        PULSE_ENTITY_SCALE(entity, 1.2f);

        if (entity->type == ENTITY_PLAYER) {
                struct Player *const player = &entity->as.player;
                start_animation(&player->flapping, 0ULL);

                struct Action *const bounce_away = &player->bouncing.actions[0];
                bounce_away->keyframes.points[1].x = change->input == INPUT_FORWARD ? -0.25f : 0.25f;
                bounce_away->keyframes.points[1].y = 0.0f;
                start_animation(&player->bouncing, 0ULL);
        }
}