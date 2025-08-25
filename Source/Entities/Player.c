#include "Entity.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL.h>

#include "../Assets.h"
#include "../Level.h"
#include "../Hexagons.h"
#include "../Geometry.h"
#include "../Context.h"
#include "../Animation.h"
#include "../Utilities.h"

#define SCALE_ALPHA 0.005f

#define ROTATION_DURATION 100.0f
#define ROTATION_ANGLE (float)M_PI * (2.0f / 6.0f)

#define ANTENNA_WAVE_ALPHA 0.01f
#define ANTENNA_WAVE_DISTANCE 0.25f
#define ANTENNA_WAVE_EPSILON 0.01f

#define CLOSED_WINGS_ANGLE -(float)M_PI * 5.0f / 6.0f
#define OPEN_WINGS_ANGLE -(float)M_PI * 3.5f / 6.0f

#define HISTORY_LIMIT 100ULL

static void setup_player_entity(struct Entity *);
static void player_entity_moved(struct Entity *, enum Orientation, struct TileState, bool, enum MoveType);
static void player_entity_receive_event(struct Entity *, const SDL_Event *);
static void update_player_entity(struct Entity *, double);
static void destroy_player_entity(struct Entity *);

static const struct EntityAPI player_entity_API = (struct EntityAPI){
        .setup = setup_player_entity,
        .moved = player_entity_moved,
        .receive_event = player_entity_receive_event,
        .update = update_player_entity,
        .destroy = destroy_player_entity
};

const struct EntityAPI *get_player_entity_API(void) {
        return &player_entity_API;
}

struct Step {
        enum {
                STEP_MOVE,
                STEP_TURN
        } type;
        struct {
                struct {
                        bool pushed;
                        bool forward;
                } move;
                struct {
                        bool direction;
                } turn;
        };
};

struct Player {
        struct Geometry *geometry;
        float rotation;
        float wings_angle;
        bool has_queued_key;
        SDL_KeyCode queued_key;
        struct Animation flapping;
        struct Animation turning;
        SDL_FPoint current_antenna_offset;
        SDL_FPoint target_antenna_offset;
        struct Step step_history[HISTORY_LIMIT];
        struct Step undo_history[HISTORY_LIMIT];
        size_t step_index;
        size_t step_count;
        size_t undo_count; 
        float float_time;
        float scale;
};

static void setup_player_entity(struct Entity *const entity) {
        entity->payload = xmalloc(sizeof(struct Player));

        struct Player *const player = (struct Player *)entity->payload;
        player->rotation = orientation_angle(entity->orientation);
        player->geometry = create_geometry();
        player->wings_angle = CLOSED_WINGS_ANGLE;
        player->has_queued_key = false;
        player->step_index = 0ULL;
        player->step_count = 0ULL;
        player->undo_count = 0ULL;
        player->float_time = 0.0f;
        player->scale = 1.0f;

        player->current_antenna_offset = (SDL_FPoint){0.0f, 0.0f};
        player->target_antenna_offset = (SDL_FPoint){0.0f, 0.0f};

        initialize_animation(&player->flapping, 2ULL);
        initialize_animation(&player->turning, 1ULL);

        struct Action *const wings_opening = &player->flapping.actions[0];
        wings_opening->target.float_pointer = &player->wings_angle;
        wings_opening->keyframes.floats[0] = CLOSED_WINGS_ANGLE;
        wings_opening->keyframes.floats[1] = OPEN_WINGS_ANGLE;
        wings_opening->type = ACTION_FLOAT;
        wings_opening->easing = SINE_IN;
        wings_opening->duration = 60.0f;

        struct Action *const wings_closing = &player->flapping.actions[1];
        wings_closing->target.float_pointer = &player->wings_angle;
        wings_closing->keyframes.floats[0] = OPEN_WINGS_ANGLE;
        wings_closing->keyframes.floats[1] = CLOSED_WINGS_ANGLE;
        wings_closing->type = ACTION_FLOAT;
        wings_closing->easing = SINE_OUT;
        wings_closing->duration = 60.0f;
        wings_closing->delay = 30.0f;

        struct Action *const turning = &player->turning.actions[0];
        turning->target.float_pointer = &player->rotation;
        turning->easing = SINE_OUT;
        turning->duration = 100.0f;
        turning->lazy_start = true;
        turning->offset = true;
}

static void player_entity_moved(struct Entity *const entity, const enum Orientation orientation, const struct TileState tile_state, const bool pull_recursion, const enum MoveType move_type) {
        entity->level->move_count += move_type == MOVE_UNDO ? -1 : 1;

        struct Player *const player = (struct Player *)entity->payload;
        start_animation(&player->flapping, 0ULL);
        player->scale = 1.2f;
        player->target_antenna_offset = (SDL_FPoint){
                .x = orientation == entity->orientation ? -ANTENNA_WAVE_DISTANCE : ANTENNA_WAVE_DISTANCE,
                .y = 0.0f
        };

        if (move_type == MOVE_ORIGINAL) {
                player->undo_count = 0ULL;
                player->step_history[player->step_index].type = STEP_MOVE;
                player->step_history[player->step_index].move.pushed = (bool)tile_state.entity;
                player->step_history[player->step_index].move.forward = orientation == entity->orientation;
                player->step_index = ++player->step_index % HISTORY_LIMIT;
                if (player->step_count < HISTORY_LIMIT) {
                        ++player->step_count;
                }
        }
}

static void turn_player_entity(struct Entity *, bool, bool);
static void player_entity_receive_event(struct Entity *const entity, const SDL_Event *const event) {
        struct Player *const player = (struct Player *)entity->payload;

        if (event->type == SDL_KEYDOWN) {
                const SDL_Keycode key = event->key.keysym.sym;

                if (key == SDLK_LEFT || key == SDLK_a) {
                        if (entity->movement.active || player->turning.active) {
                                if (!player->has_queued_key) {
                                        player->has_queued_key = true;
                                        player->queued_key = key;
                                }

                                return;
                        }

                        turn_player_entity(entity, false, true);
                }

                if (key == SDLK_RIGHT || key == SDLK_d) {
                        if (entity->movement.active || player->turning.active) {
                                if (!player->has_queued_key) {
                                        player->has_queued_key = true;
                                        player->queued_key = key;
                                }

                                return;
                        }

                        turn_player_entity(entity, true, true);
                }

                if (key == SDLK_UP || key == SDLK_w) {
                        if (entity->movement.active || player->turning.active) {
                                if (!player->has_queued_key) {
                                        player->has_queued_key = true;
                                        player->queued_key = key;
                                }

                                return;
                        }

                        move_entity(entity, entity->orientation, true, false, MOVE_ORIGINAL);
                }

                if (key == SDLK_DOWN || key == SDLK_s) {
                        if (entity->movement.active || player->turning.active) {
                                if (!player->has_queued_key) {
                                        player->has_queued_key = true;
                                        player->queued_key = key;
                                }

                                return;
                        }

                        move_entity(entity, orientation_reverse(entity->orientation), true, false, MOVE_ORIGINAL);
                }

                if (key == SDLK_z) {
                        if (entity->movement.active || player->turning.active) {
                                if (!player->has_queued_key) {
                                        player->has_queued_key = true;
                                        player->queued_key = key;
                                }

                                return;
                        }

                        if (!player->step_count) {
                                // TODO: DO something to show that there are no steps left to undo
                                return;
                        }

                        const size_t next_index = (player->step_index ? player->step_index : HISTORY_LIMIT) - 1ULL;
                        const struct Step *const step = &player->step_history[next_index];

                        if (step->type == STEP_MOVE) {
                                if (!move_entity(entity, step->move.forward ? orientation_reverse(entity->orientation) : entity->orientation, true, step->move.pushed, MOVE_UNDO)) {
                                        return;
                                }
                        }

                        if (step->type == STEP_TURN) {
                                turn_player_entity(entity, !step->turn.direction, false);
                        }

                        player->undo_history[player->undo_count++] = *step;
                        player->step_index = next_index;
                        --player->step_count;
                }

                if (key == SDLK_x || key == SDLK_y) {
                        if (entity->movement.active || player->turning.active) {
                                if (!player->has_queued_key) {
                                        player->has_queued_key = true;
                                        player->queued_key = key;
                                }

                                return;
                        }

                        if (!player->undo_count) {
                                // TODO: Do something to show that there are no steps left to redo
                                return;
                        }

                        const struct Step *const step = &player->undo_history[player->undo_count - 1ULL];

                        if (step->type == STEP_MOVE) {
                                if (!move_entity(entity, step->move.forward ? entity->orientation : orientation_reverse(entity->orientation), true, false, MOVE_REDO)) {
                                        return;
                                }
                        }

                        if (step->type == STEP_TURN) {
                                turn_player_entity(entity, step->turn.direction, false);
                        }

                        player->step_history[player->step_count++] = *step;
                        player->step_index = player->step_index == HISTORY_LIMIT ? 0ULL : player->step_index + 1ULL;
                        --player->undo_count;
                }
        }
}

static void update_player_entity(struct Entity *const entity, const double delta_time) {
        struct Player *const player = (struct Player *)entity->payload;
        player->scale += (1.0f - player->scale) * SCALE_ALPHA * delta_time;

        if (!entity->movement.active && !player->turning.active && player->has_queued_key) {
                player->has_queued_key = false;

                SDL_Event event = {};
                event.type = SDL_KEYDOWN;
                event.key.type = SDL_KEYDOWN;
                event.key.keysym.sym = player->queued_key;
                player_entity_receive_event(entity, &event);
        }

        if (player->target_antenna_offset.x != 0.0f || player->target_antenna_offset.y != 0.0f) {
                const float alpha = fminf(ANTENNA_WAVE_ALPHA * (float)delta_time, 1.0f);
                player->current_antenna_offset.x += (player->target_antenna_offset.x - player->current_antenna_offset.x) * alpha;
                player->current_antenna_offset.y += (player->target_antenna_offset.y - player->current_antenna_offset.y) * alpha;

                const float distance_x = player->target_antenna_offset.x - player->current_antenna_offset.x;
                const float distance_y = player->target_antenna_offset.y - player->current_antenna_offset.y;
                if (powf(distance_x, 2.0f) + powf(distance_y, 2.0f) < powf(ANTENNA_WAVE_EPSILON, 2.0f)) {
                        player->target_antenna_offset.x = 0.0f;
                        player->target_antenna_offset.y = 0.0f;
                }
        } else if (player->current_antenna_offset.x != 0.0f || player->current_antenna_offset.y != 0.0f) {
                const float alpha = fminf(ANTENNA_WAVE_ALPHA * (float)delta_time, 1.0f);
                player->current_antenna_offset.x += (player->target_antenna_offset.x - player->current_antenna_offset.x) * alpha;
                player->current_antenna_offset.y += (player->target_antenna_offset.y - player->current_antenna_offset.y) * alpha;

                const float distance_x = player->target_antenna_offset.x - player->current_antenna_offset.x;
                const float distance_y = player->target_antenna_offset.y - player->current_antenna_offset.y;
                if (powf(distance_x, 2.0f) + powf(distance_y, 2.0f) < powf(ANTENNA_WAVE_EPSILON, 2.0f)) {
                        player->current_antenna_offset.x = 0.0f;
                        player->current_antenna_offset.y = 0.0f;
                }
        }

        update_animation(&player->flapping, delta_time);
        update_animation(&player->turning, delta_time);

        const float tile_radius = entity->grid_metrics->tile_radius * player->scale;
        SDL_FPoint position = (SDL_FPoint){
                .x = entity->position.x + entity->shake_offset.x,
                .y = entity->position.y + entity->shake_offset.y
        };

        player->float_time += delta_time / 500.0f;
        const SDL_FPoint float_animation = (SDL_FPoint){cosf(player->float_time) / 5.0f, sinf(player->float_time) / 5.0f};
        const float wings_angle = player->wings_angle + (float_animation.x + float_animation.y) / 2.5f;
        const float rotation = player->rotation + (float_animation.x + float_animation.y) / 2.5f;
        position.x += float_animation.x * tile_radius / 5.0f;
        position.y += float_animation.y * tile_radius / 5.0f;

        const float body_length = tile_radius * 1.25f;
        const float body_thickness = tile_radius / 1.5f;
        const float line_width = tile_radius / 10.0f;

        SDL_FPoint back_circle_position = (SDL_FPoint){
                .x = position.x - body_length / 2.0f + body_thickness / 2.0f,
                .y = position.y
        };

        SDL_FPoint front_circle_position = (SDL_FPoint){
                .x = position.x + body_length / 2.0f - body_thickness / 2.0f,
                .y = position.y
        };

        const float outer_circle_radius = body_thickness / 2.0f + line_width / 2.0f;
        const float inner_circle_radius = body_thickness / 2.0f - line_width / 2.0f;

        const SDL_FRect main_body_segment = (SDL_FRect){
                .x = position.x,
                .y = position.y,
                .w = body_length - body_thickness,
                .h = body_thickness + line_width
        };

        const SDL_FPoint left_antenna_tip_position = (SDL_FPoint){
                .x = front_circle_position.x + tile_radius / 1.5f,
                .y = position.y - tile_radius / 1.5f
        };

        const SDL_FPoint right_antenna_tip_position = (SDL_FPoint){
                .x = front_circle_position.x + tile_radius / 1.5f,
                .y = position.y + tile_radius / 1.5f
        };

        SDL_FPoint left_antenna_endpoints[] = {
                (SDL_FPoint){front_circle_position.x + body_thickness / 3.0f, position.y - body_thickness / 3.0f},
                (SDL_FPoint){left_antenna_tip_position.x, left_antenna_tip_position.y}
        };

        SDL_FPoint left_antenna_control_points[] = {
                (SDL_FPoint){left_antenna_tip_position.x - line_width * 1.5f, left_antenna_tip_position.y + body_thickness / 1.5f},
                (SDL_FPoint){left_antenna_tip_position.x - line_width * 0.0f, left_antenna_tip_position.y + body_thickness / 2.5f}
        };

        SDL_FPoint right_antenna_endpoints[] = {
                (SDL_FPoint){front_circle_position.x + body_thickness / 3.0f, position.y + body_thickness / 3.0f},
                (SDL_FPoint){right_antenna_tip_position.x, right_antenna_tip_position.y}
        };

        SDL_FPoint right_antenna_control_points[] = {
                (SDL_FPoint){right_antenna_tip_position.x - line_width * 1.5f, right_antenna_tip_position.y - body_thickness / 1.5f},
                (SDL_FPoint){right_antenna_tip_position.x - line_width * 0.0f, right_antenna_tip_position.y - body_thickness / 2.5f}
        };

        left_antenna_endpoints[1].x       += tile_radius * player->current_antenna_offset.x;
        left_antenna_endpoints[1].y       += tile_radius * player->current_antenna_offset.y;
        right_antenna_endpoints[1].x      += tile_radius * player->current_antenna_offset.x;
        right_antenna_endpoints[1].y      += tile_radius * player->current_antenna_offset.y;
        left_antenna_control_points[1].x  += tile_radius * player->current_antenna_offset.x / 2.0f;
        left_antenna_control_points[1].y  += tile_radius * player->current_antenna_offset.y / 2.0f;
        right_antenna_control_points[1].x += tile_radius * player->current_antenna_offset.x / 2.0f;
        right_antenna_control_points[1].y += tile_radius * player->current_antenna_offset.y / 2.0f;

        SDL_FPoint stinger[] = {
                (SDL_FPoint){position.x - body_length / 2.0f,                      position.y + line_width * 1.5f},
                (SDL_FPoint){position.x - body_length / 2.0f,                      position.y - line_width * 1.5f},
                (SDL_FPoint){position.x - body_length / 2.0f - line_width * 1.25f, position.y                    }
        };

        const float wings_length = body_thickness - line_width;
        const float wings_thickness = (wings_length - line_width) / 2.0f;
        const SDL_FPoint wings_border_radii = (SDL_FPoint){wings_length + line_width / 2.0f, wings_thickness + line_width / 2.0f};
        const SDL_FPoint wings_filled_radii = (SDL_FPoint){wings_length - line_width / 2.0f, wings_thickness - line_width / 2.0f};

        const float left_wing_angle = wings_angle;
        const float right_wing_angle = 2.0f * (float)M_PI - left_wing_angle;
        const float wings_anchor_x = front_circle_position.x - line_width * 1.5f;
        const float wings_anchor_y = position.y;

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
                rotate_point(&points[point_index]->x, &points[point_index]->y, position.x, position.y, -rotation);
        }

        clear_geometry(player->geometry);

        set_geometry_color(player->geometry, COLOR_DARK_BROWN, COLOR_OPAQUE);
        write_circle_geometry(player->geometry, back_circle_position.x, back_circle_position.y, outer_circle_radius);
        write_circle_geometry(player->geometry, front_circle_position.x, front_circle_position.y, outer_circle_radius);

        set_geometry_color(player->geometry, COLOR_YELLOW, COLOR_OPAQUE);
        write_circle_geometry(player->geometry, back_circle_position.x, back_circle_position.y, inner_circle_radius);
        write_circle_geometry(player->geometry, front_circle_position.x, front_circle_position.y, inner_circle_radius);

        set_geometry_color(player->geometry, COLOR_DARK_BROWN, COLOR_OPAQUE);
        write_rectangle_geometry(player->geometry, main_body_segment.x, main_body_segment.y, main_body_segment.w, main_body_segment.h, -rotation);
        write_circle_geometry(player->geometry, left_antenna_endpoints[1].x, left_antenna_endpoints[1].y, line_width);
        write_circle_geometry(player->geometry, right_antenna_endpoints[1].x, right_antenna_endpoints[1].y, line_width);

        write_bezier_curve_geometry(
                player->geometry,
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
                player->geometry,
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

        write_triangle_geometry(player->geometry, stinger[0].x, stinger[0].y, stinger[1].x, stinger[1].y, stinger[2].x, stinger[2].y);

        set_geometry_color(player->geometry, COLOR_DARK_BROWN, COLOR_OPAQUE);
        write_ellipse_geometry(player->geometry, left_wing_center.x, left_wing_center.y, wings_border_radii.x, wings_border_radii.y, -rotation + left_wing_angle);

        set_geometry_color(player->geometry, COLOR_LIGHT_YELLOW, COLOR_OPAQUE);
        write_ellipse_geometry(player->geometry, left_wing_center.x, left_wing_center.y, wings_filled_radii.x, wings_filled_radii.y, -rotation + left_wing_angle);

        set_geometry_color(player->geometry, COLOR_DARK_BROWN, COLOR_OPAQUE);
        write_ellipse_geometry(player->geometry, right_wing_center.x, right_wing_center.y, wings_border_radii.x, wings_border_radii.y, -rotation + right_wing_angle);

        set_geometry_color(player->geometry, COLOR_LIGHT_YELLOW, COLOR_OPAQUE);
        write_ellipse_geometry(player->geometry, right_wing_center.x, right_wing_center.y, wings_filled_radii.x, wings_filled_radii.y, -rotation + right_wing_angle);

        render_geometry(player->geometry);
}

static void destroy_player_entity(struct Entity *const entity) {
        struct Player *const player = (struct Player *)entity->payload;
        destroy_geometry(player->geometry);
        deinitialize_animation(&player->flapping);
        deinitialize_animation(&player->turning);
        xfree(player);
}

static void turn_player_entity(struct Entity *const entity, const bool direction, const bool original) {
        struct Player *const player = (struct Player *)entity->payload;
        if (original) {
                player->undo_count = 0ULL;
                player->step_history[player->step_index].type = STEP_TURN;
                player->step_history[player->step_index].turn.direction = direction;
                player->step_index = ++player->step_index % HISTORY_LIMIT;
                if (player->step_count < HISTORY_LIMIT) {
                        ++player->step_count;
                }
        }

        entity->orientation = (direction ? orientation_turn_right : orientation_turn_left)(entity->orientation);
        player->target_antenna_offset = (SDL_FPoint){ANTENNA_WAVE_DISTANCE / -2.0f, (direction ? ANTENNA_WAVE_DISTANCE : -ANTENNA_WAVE_DISTANCE) / 2.0f};
        player->turning.actions[0].keyframes.floats[1] = ROTATION_ANGLE * (direction ? -1.0f : 1.0f);
        player->scale = 1.1f;

        start_animation(&player->flapping, 0ULL);
        start_animation(&player->turning, 0ULL);
        play_sound(SOUND_TURN);
}