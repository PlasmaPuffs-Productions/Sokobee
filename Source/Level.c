#include "Level.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "SDL.h"

#include "Audio.h"
#include "cJSON.h"
#include "Assets.h"
#include "Context.h"
#include "Hexagons.h"
#include "Entity.h"
#include "Geometry.h"
#include "Gesture.h"

#define LEVEL_DIMENSION_LIMIT 20

#define STEP_HISTORY_INITIAL_CAPACITY 64ULL

struct StepHistory {
        struct Change *changes;
        size_t change_capacity;
        size_t change_count;

        size_t *step_offsets;
        size_t step_capacity;
        size_t step_count;
};

static inline void initialize_step_history(struct StepHistory *const step_history) {
        step_history->changes = (struct Change *)xmalloc(STEP_HISTORY_INITIAL_CAPACITY * sizeof(struct Change));
        step_history->change_capacity = STEP_HISTORY_INITIAL_CAPACITY;
        step_history->change_count = 0ULL;

        step_history->step_offsets = (size_t *)xmalloc(STEP_HISTORY_INITIAL_CAPACITY * sizeof(size_t));
        step_history->step_capacity = STEP_HISTORY_INITIAL_CAPACITY;
        step_history->step_count = 0ULL;
}

static inline void destroy_step_history(struct StepHistory *const step_history) {
        xfree(step_history->changes);
        step_history->changes = NULL;
        step_history->change_capacity = 0ULL;
        step_history->change_count = 0ULL;

        xfree(step_history->step_offsets);
        step_history->step_offsets = NULL;
        step_history->step_capacity = 0ULL;
        step_history->step_count = 0ULL;
}

static inline void empty_step_history(struct StepHistory *const step_history) {
        step_history->change_count = 0ULL;
        step_history->step_count = 0ULL;
}

static inline void step_history_swap_step(struct StepHistory *const source, struct StepHistory *const destination) {
        if (source->step_count == 0ULL) {
                return;
        }

        const size_t step_end     = source->step_offsets[source->step_count - 1ULL];
        const size_t step_start   = source->step_count > 1ULL ? source->step_offsets[source->step_count - 2ULL] : 0ULL;
        const size_t step_changes = step_end - step_start;

        for (size_t change_index = 0ULL; change_index < step_changes; ++change_index) {
                const struct Change *change = &source->changes[step_start + change_index];

                struct Change reversed = *change;

                switch (change->type) {
                        case CHANGE_WALK: case CHANGE_PUSH: case CHANGE_PUSHED: {
                                if (change->type == CHANGE_WALK) {
                                        play_sound(SOUND_MOVE);
                                }

                                if (change->type == CHANGE_PUSH) {
                                        play_sound(SOUND_PUSH);
                                }

                                reversed.input = reversed.input == INPUT_FORWARD ? INPUT_BACKWARD : INPUT_FORWARD;

                                uint16_t tile_index = reversed.move.last_tile_index;
                                reversed.move.last_tile_index = reversed.move.next_tile_index;
                                reversed.move.next_tile_index = tile_index;
                                break;
                        }

                        case CHANGE_TURN: {
                                play_sound(SOUND_TURN);

                                reversed.input = reversed.input == INPUT_LEFT ? INPUT_RIGHT : INPUT_LEFT;

                                enum Orientation orientation = reversed.turn.last_orientation;
                                reversed.turn.last_orientation = reversed.turn.next_orientation;
                                reversed.turn.next_orientation = orientation;
                                break;
                        }

                        case CHANGE_INVALID: {
                                continue;
                        }
                }

                entity_handle_change(reversed.entity, &reversed);

                if (destination->change_count >= destination->change_capacity) {
                        destination->change_capacity *= 2ULL;
                        destination->changes = (struct Change *)xrealloc(destination->changes, destination->change_capacity * sizeof(struct Change));
                }

                destination->changes[destination->change_count++] = reversed;
        }

        if (destination->step_count >= destination->step_capacity) {
                destination->step_capacity *= 2ULL;
                destination->step_offsets = (size_t *)xrealloc(destination->step_offsets, destination->step_capacity * sizeof(size_t));
        }

        destination->step_offsets[destination->step_count++] = destination->change_count;

        source->change_count = step_start;
        --source->step_count;
}

static inline struct Change *get_next_change_slot(struct StepHistory *const step_history) {
        if (step_history->change_count >= step_history->change_capacity) {
                step_history->change_capacity *= 2ULL;
                step_history->changes = (struct Change *)xrealloc(step_history->changes, step_history->change_capacity * sizeof(struct Change));
        }

        return &step_history->changes[step_history->change_count++];
}

static inline void commit_pending_step(struct StepHistory *const step_history) {
        const size_t last_offset = step_history->step_count ? step_history->step_offsets[step_history->step_count - 1ULL] : 0ULL;
        const size_t step_changes = step_history->change_count - last_offset;
        if (step_changes == 0ULL) {
                return;
        }

        if (step_history->step_count >= step_history->step_capacity) {
                step_history->step_capacity *= 2ULL;
                step_history->step_offsets = (size_t *)xrealloc(step_history->step_offsets, step_history->step_capacity * sizeof(size_t));
        }

        step_history->step_offsets[step_history->step_count++] = step_history->change_count;

        for (size_t index = 0ULL; index < step_changes; ++index) {
                struct Change *const change = &step_history->changes[step_history->change_count - 1ULL - index];
                entity_handle_change(change->entity, change);
        }
}

static inline void discard_pending_step(struct StepHistory *const step_history, const enum Orientation direction) {
        const size_t last_offset = step_history->step_count ? step_history->step_offsets[step_history->step_count - 1ULL] : 0ULL;
        const size_t step_changes = step_history->change_count - last_offset;
        if (step_changes == 0ULL) {
                return;
        }

        for (size_t index = 0ULL; index < step_changes; ++index) {
                struct Change *const change = &step_history->changes[step_history->change_count - 1ULL - index];
                change->face.direction = direction;
                change->type = CHANGE_INVALID;

                entity_handle_change(change->entity, change);
        }

        step_history->change_count -= step_changes;
}
struct LevelImplementation {
        enum TileType *tiles;
        uint16_t tile_count;
        struct Entity **entities;
        uint16_t entity_count;
        struct Entity *current_player;
        struct GridMetrics grid_metrics;
        struct Geometry *grid_geometry;
        struct StepHistory step_history;
        struct StepHistory undo_history;
        enum Input buffered_input;
        bool has_buffered_input;
};

static inline void level_move_step(struct Level *const level, struct Entity *const entity, const enum Input input) {
        if (!entity_can_change(entity)) {
                if (!level->implementation->has_buffered_input) {
                        level->implementation->has_buffered_input = true;
                        level->implementation->buffered_input = input;
                }

                return;
        }

        uint16_t tile_index = get_entity_tile_index(entity);
        enum Orientation direction = get_entity_orientation(entity);
        if (input == INPUT_BACKWARD) {
                direction = orientation_reverse(direction);
        }

        struct Entity *next_entity = entity;
        while (true) {
                const bool first_change = next_entity == entity;

                struct Change *const change = get_next_change_slot(&level->implementation->step_history);
                change->input = input;
                change->type = CHANGE_PUSH;
                change->entity = next_entity;
                change->move.last_tile_index = tile_index;

                if (!orientation_advance_index(direction, level->columns, level->rows, &tile_index)) {
                        discard_pending_step(&level->implementation->step_history, direction);
                        return;
                }

                change->move.next_tile_index = tile_index;

                enum TileType tile_type;
                query_level_tile(level, tile_index, &tile_type, &next_entity, NULL, NULL);

                if (tile_type == TILE_EMPTY) {
                        discard_pending_step(&level->implementation->step_history, direction);
                        play_sound(SOUND_HIT);
                        return;
                }

                // Players can walk on slab tiles but blocks can't get pushed onto them
                if (tile_type == TILE_SLAB && get_entity_type(change->entity) == ENTITY_BLOCK) {
                        discard_pending_step(&level->implementation->step_history, direction);
                        play_sound(SOUND_HIT);
                        return;
                }

                if (next_entity == NULL) {
                        empty_step_history(&level->implementation->undo_history);

                        if (first_change) {
                                // This means that the next tile is valid but nothing will be pushed
                                change->type = CHANGE_WALK;
                                commit_pending_step(&level->implementation->step_history);
                                play_sound(SOUND_MOVE);
                                return;
                        }

                        commit_pending_step(&level->implementation->step_history);

                        bool did_win = true;
                        for (uint16_t tile_index = 0; tile_index < level->implementation->tile_count; ++tile_index) {
                                enum TileType tile_type;
                                struct Entity *entity;
                                query_level_tile(level, tile_index, &tile_type, &entity, NULL, NULL);

                                if (tile_type == TILE_SPOT) {
                                        if (entity == NULL) {
                                                did_win = false;
                                                break;
                                        }

                                        if (get_entity_type(entity) != ENTITY_BLOCK) {
                                                did_win = false;
                                                break;
                                        }
                                }
                        }

                        if (did_win) {
                                level->completion_callback(level->completion_callback_data);
                                play_sound(SOUND_WIN);
                                return;
                        }

                        play_sound(SOUND_PUSH);
                        return;
                }
        }
}

static inline void level_turn_step(struct Level *const level, struct Entity *const entity, const enum Input input) {
        if (!entity_can_change(entity)) {
                if (!level->implementation->has_buffered_input) {
                        level->implementation->has_buffered_input = true;
                        level->implementation->buffered_input = input;
                }

                return;
        }

        struct Change *const change = get_next_change_slot(&level->implementation->step_history);
        change->input = input;
        change->entity = entity;
        change->type = CHANGE_TURN;
        change->turn.last_orientation = get_entity_orientation(entity);
        change->turn.next_orientation = input == INPUT_RIGHT
                ? orientation_turn_right(change->turn.last_orientation)
                : orientation_turn_left(change->turn.last_orientation);

        commit_pending_step(&level->implementation->step_history);
        empty_step_history(&level->implementation->undo_history);
        play_sound(SOUND_TURN);
}

static bool parse_level(const cJSON *const json, struct Level *const level);

static void resize_level(struct Level *const level);

struct Level *load_level(const struct LevelMetadata *const metadata) {
        struct Level *const level = (struct Level *)xcalloc(1, sizeof(struct Level));
        if (!initialize_level(level, metadata)) {
                send_message(MESSAGE_ERROR, "Failed to load level: Failed to initialize level");
                destroy_level(level);
                return NULL;
        }

        return level;
}

void destroy_level(struct Level *const level) {
        if (!level) {
                send_message(MESSAGE_WARNING, "Level given to destroy is NULL");
                return;
        }

        deinitialize_level(level);
        xfree(level);
}

bool initialize_level(struct Level *const level, const struct LevelMetadata *const metadata) {
        level->columns = 0;
        level->rows = 0;
        level->move_count = 0ULL;
        level->completion_callback = NULL;
        level->completion_callback_data = NULL;
        level->title = xstrdup(metadata->title);

        level->implementation = (struct LevelImplementation *)xmalloc(sizeof(struct LevelImplementation));
        level->implementation->grid_geometry = create_geometry();
        level->implementation->current_player = NULL;
        level->implementation->has_buffered_input = false;

        initialize_step_history(&level->implementation->step_history);
        initialize_step_history(&level->implementation->undo_history);

        char *const json_string = load_text_file(metadata->path);
        if (!json_string) {
                send_message(MESSAGE_ERROR, "Failed to initialize level \"%s\": Failed to load level data file \"%s\"", metadata->title, metadata->path);
                deinitialize_level(level);
                return false;
        }

        cJSON *const json = cJSON_Parse(json_string);
        xfree(json_string);

        if (!json) {
                send_message(MESSAGE_ERROR, "Failed to initialize level \"%s\": Failed to parse level data file \"%s\": %s", metadata->title, metadata->path, cJSON_GetMESSAGE_ERRORPtr());
                deinitialize_level(level);
                return false;
        }

        if (!parse_level(json, level)) {
                send_message(MESSAGE_ERROR, "Failed to initialize level \"%s\": Failed to parse level", metadata->title);
                deinitialize_level(level);
                cJSON_Delete(json);
                return NULL;
        }

        cJSON_Delete(json);

        struct GridMetrics *const grid_metrics = &level->implementation->grid_metrics;
        grid_metrics->columns = (size_t)level->columns;
        grid_metrics->rows = (size_t)level->rows;

        resize_level(level);
        return true;
}

void deinitialize_level(struct Level *const level) {
        if (!level) {
                send_message(MESSAGE_WARNING, "Level given to deinitialize is NULL");
                return;
        }

        xfree(level->title);
        level->title = NULL;

        struct LevelImplementation *const implementation = level->implementation;
        if (!implementation) {
                return;
        }

        destroy_step_history(&level->implementation->step_history);
        destroy_step_history(&level->implementation->undo_history);

        destroy_geometry(implementation->grid_geometry);

        if (implementation->entities) {
                for (size_t entity_index = 0ULL; entity_index < implementation->entity_count; ++entity_index) {
                        destroy_entity(implementation->entities[entity_index]);
                }

                xfree(implementation->entities);
        }

        if (implementation->tiles) {
                xfree(implementation->tiles);
        }

        level->implementation = NULL;
        xfree(implementation);
}

bool query_level_tile(
        const struct Level *const level,
        const uint16_t tile_index,
        enum TileType *const out_tile_type,
        struct Entity **const out_entity,
        float *const out_x,
        float *const out_y
) {
        if (tile_index >= level->implementation->tile_count) {
                return false;
        }

        const enum TileType tile_type = level->implementation->tiles[tile_index];
        if (out_tile_type) {
                *out_tile_type = tile_type;
        }

        const struct GridMetrics *const grid_metrics = &level->implementation->grid_metrics;
        const uint16_t columns = (uint16_t)grid_metrics->columns;

        float x, y;
        get_grid_tile_position(grid_metrics, (size_t)(tile_index % columns), (size_t)(tile_index / columns), &x, &y);

        if (out_x) {
                *out_x = x;
        }

        if (out_y) {
                *out_y = tile_type == TILE_SLAB ? y - grid_metrics->tile_radius / 4.0f : y;
        }

        if (out_entity) {
                *out_entity = NULL;

                for (uint16_t entity_index = 0; entity_index < level->implementation->entity_count; ++entity_index) {
                        struct Entity *const entity = level->implementation->entities[entity_index];
                        if (get_entity_tile_index(entity) == tile_index) {
                                *out_entity = entity;
                                break;
                        }
                }
        }

        return true;
}

bool level_receive_event(struct Level *const level, const SDL_Event *const event) {
        if (event->type == SDL_WINDOWEVENT) {
                const Uint8 window_event = event->window.event;
                if (window_event == SDL_WINDOWEVENT_RESIZED || window_event == SDL_WINDOWEVENT_MAXIMIZED || window_event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        resize_level(level);
                }

                return false;
        }

        if (event->type == SDL_KEYDOWN && event->key.repeat == 0) {
                const SDL_Keycode key = event->key.keysym.sym;

                if (key == SDLK_LEFT || key == SDLK_a) {
                        level_turn_step(level, level->implementation->current_player, INPUT_LEFT);
                        return true;
                }

                if (key == SDLK_RIGHT || key == SDLK_d) {
                        level_turn_step(level, level->implementation->current_player, INPUT_RIGHT);
                        return true;
                }

                if (key == SDLK_UP || key == SDLK_w) {
                        level_move_step(level, level->implementation->current_player, INPUT_FORWARD);
                        return true;
                }

                if (key == SDLK_DOWN || key == SDLK_s) {
                        level_move_step(level, level->implementation->current_player, INPUT_BACKWARD);
                        return true;
                }

                if (key == SDLK_z) {
                        if (!entity_can_change(level->implementation->current_player)) {
                                if (!level->implementation->has_buffered_input) {
                                        level->implementation->has_buffered_input = true;
                                        level->implementation->buffered_input = INPUT_UNDO;
                                }

                                return true;
                        }

                        step_history_swap_step(&level->implementation->step_history, &level->implementation->undo_history);
                        return true;
                }

                if (key == SDLK_x || key == SDLK_y) {
                        if (!entity_can_change(level->implementation->current_player)) {
                                if (!level->implementation->has_buffered_input) {
                                        level->implementation->has_buffered_input = true;
                                        level->implementation->buffered_input = INPUT_UNDO;
                                }

                                return true;
                        }

                        step_history_swap_step(&level->implementation->undo_history, &level->implementation->step_history);
                        return true;
                }
        }

        const enum Input gesture_input = handle_gesture_event(event);
        if (gesture_input != INPUT_NONE) {
                if (!level->implementation->has_buffered_input) {
                        level->implementation->has_buffered_input = true;
                        level->implementation->buffered_input = gesture_input;
                }

                return true;
        }

        return false;
}

void update_level(struct Level *const level, const double delta_time) {
        if (level->implementation->has_buffered_input && entity_can_change(level->implementation->current_player)) {
                level->implementation->has_buffered_input = false;

                switch (level->implementation->buffered_input) {
                        case INPUT_BACKWARD: case INPUT_FORWARD: {
                                level_move_step(level, level->implementation->current_player, level->implementation->buffered_input);
                                break;
                        }

                        case INPUT_LEFT: case INPUT_RIGHT: {
                                level_turn_step(level, level->implementation->current_player, level->implementation->buffered_input);
                                break;
                        }

                        case INPUT_UNDO: {
                                step_history_swap_step(&level->implementation->step_history, &level->implementation->undo_history);
                                break;
                        }

                        case INPUT_REDO: {
                                step_history_swap_step(&level->implementation->undo_history, &level->implementation->step_history);
                                break;
                        }

                        default: {
                                break;
                        }
                }
        }

        render_geometry(level->implementation->grid_geometry);

        for (size_t entity_index = 0ULL; entity_index < level->implementation->entity_count; ++entity_index) {
                struct Entity *const entity = level->implementation->entities[entity_index];
                if (get_entity_type(entity) != ENTITY_PLAYER) {
                        update_entity(entity, delta_time);
                }
        }

        // Add another pass to update players last (I forgot why though)
        for (size_t entity_index = 0ULL; entity_index < level->implementation->entity_count; ++entity_index) {
                struct Entity *const entity = level->implementation->entities[entity_index];
                if (get_entity_type(entity) == ENTITY_PLAYER) {
                        update_entity(entity, delta_time);
                }
        }
}

static bool parse_level(const cJSON *const json, struct Level *const level) {
        if (!cJSON_IsObject(json)) {
                send_message(MESSAGE_ERROR, "Failed to parse level: JSON data is invalid");
                return false;
        }

        const cJSON *const columns_json  = cJSON_GetObjectItemCaseSensitive(json, "columns");
        const cJSON *const rows_json     = cJSON_GetObjectItemCaseSensitive(json, "rows");
        const cJSON *const tiles_json    = cJSON_GetObjectItemCaseSensitive(json, "tiles");
        const cJSON *const entities_json = cJSON_GetObjectItemCaseSensitive(json, "entities");

        if (!cJSON_IsNumber(columns_json) || !cJSON_IsNumber(rows_json) || !cJSON_IsArray(tiles_json) || !cJSON_IsArray(entities_json)) {
                send_message(MESSAGE_ERROR, "Failed to parse level: JSON data is invalid");
                return false;
        }

        const double columns = columns_json->valuedouble;
        if (floor(columns) != columns || columns <= 0.0 || columns > (double)LEVEL_DIMENSION_LIMIT) {
                send_message(MESSAGE_ERROR, "Failed to parse level: The grid columns %lf is invalid, it should be an integer between 0 and %u", columns, LEVEL_DIMENSION_LIMIT);
                return false;
        }

        const double rows = rows_json->valuedouble;
        if (floor(rows) != rows || rows <= 0.0 || rows > (double)LEVEL_DIMENSION_LIMIT) {
                send_message(MESSAGE_ERROR, "Failed to parse level: The grid rows %lf is invalid, it should be an integer between 0 and %u", rows, LEVEL_DIMENSION_LIMIT);
                return false;
        }

        struct LevelImplementation *const implementation = level->implementation;
        level->columns = (uint8_t)columns;
        level->rows = (uint8_t)rows;

        const size_t tile_count = (size_t)cJSON_GetArraySize(tiles_json);
        implementation->tile_count = (size_t)level->columns * (size_t)level->rows;
        if (tile_count != implementation->tile_count) {
                send_message(MESSAGE_ERROR, "Failed to parse level: The tile count of %zu does not match the expected tile count of %zu (%u * %u)", tile_count, implementation->tile_count, level->columns, level->rows);
                return false;
        }

        implementation->tiles = (enum TileType *)xmalloc(implementation->tile_count * sizeof(enum TileType));

        size_t tile_index = 0ULL;
        const cJSON *tile_json = NULL;
        cJSON_ArrayForEach(tile_json, tiles_json) {
                if (!cJSON_IsNumber(tile_json)) {
                        send_message(MESSAGE_ERROR, "Failed to parse level: JSON data is invalid");
                        return false;
                }

                const double tile = tile_json->valuedouble;
                if (floor(tile) != tile || tile < 0.0 || (size_t)tile > (size_t)TILE_COUNT) {
                        send_message(MESSAGE_ERROR, "Failed to parse level: The tile #%zu of %lf is invalid, it should be an integer between 0 and %d", tile_index, tile, (int)TILE_COUNT);
                        return false;
                }

                implementation->tiles[tile_index++] = (enum TileType)(uint8_t)tile;
        }

        const int entities_length = cJSON_GetArraySize(entities_json);
        if (entities_length % 4) {
                send_message(MESSAGE_ERROR, "Failed to parse level: Entities array length of %d is not a multiple of 4", entities_length);
                return false;
        }

        implementation->entity_count = (uint16_t)entities_length / 4;
        implementation->entities = (struct Entity **)xcalloc(implementation->entity_count, sizeof(struct Entity *));

        const cJSON *entity_part_json = entities_json->child;
        for (uint16_t entity_index = 0; entity_index < implementation->entity_count; ++entity_index) {
                const cJSON *const entity_type_json = entity_part_json;
                const cJSON *const entity_column_json = entity_type_json->next;
                const cJSON *const entity_row_json = entity_column_json->next;
                const cJSON *const entity_orientation_json = entity_row_json->next;
                entity_part_json = entity_orientation_json->next;

                const enum EntityType entity_type = (enum EntityType)(uint8_t)entity_type_json->valuedouble;
                const uint8_t entity_column = (uint8_t)entity_column_json->valuedouble;
                const uint8_t entity_row = (uint8_t)entity_row_json->valuedouble;
                const uint16_t entity_tile_index = (uint16_t)entity_row * (uint16_t)level->columns + (uint16_t)entity_column;
                const enum Orientation entity_orientation = (enum Orientation)(uint8_t)entity_orientation_json->valuedouble;
                implementation->entities[entity_index] = create_entity(level, entity_type, entity_tile_index, entity_orientation);

                if (implementation->current_player == NULL) {
                        implementation->current_player = implementation->entities[entity_index];
                }
        }

        return true;
}

static void resize_level(struct Level *const level) {
        struct LevelImplementation *const implementation = level->implementation;
        clear_geometry(implementation->grid_geometry);

        int drawable_width;
        int drawable_height;
        SDL_GetRendererOutputSize(get_context_renderer(), &drawable_width, &drawable_height);

        const float grid_padding = fminf((float)drawable_width, (float)drawable_height) / 10.0f;

        struct GridMetrics *const grid_metrics = &implementation->grid_metrics;
        grid_metrics->bounding_x = grid_padding;
        grid_metrics->bounding_y = grid_padding;
        grid_metrics->bounding_width  = (float)drawable_width  - grid_padding * 2.0f;
        grid_metrics->bounding_height = (float)drawable_height - grid_padding * 2.0f;
        populate_grid_metrics_from_size(grid_metrics);

        const float tile_radius = grid_metrics->tile_radius;
        const float thickness = tile_radius / 2.0f;
        const float line_width = tile_radius / 5.0f;
        grid_metrics->bounding_y -= thickness / 2.0f;
        grid_metrics->grid_y -= thickness / 2.0f;

        const size_t tile_count = (size_t)level->columns * (size_t)level->rows;

        set_geometry_color(implementation->grid_geometry, COLOR_GOLD, COLOR_OPAQUE);
        for (uint8_t row = 0; row < level->rows; ++row) {
                for (uint8_t column = 0; column < level->columns; ++column) {
                        const enum TileType tile_type = implementation->tiles[(size_t)row * (size_t)level->columns + (size_t)column];
                        if (tile_type == TILE_EMPTY || tile_type == TILE_SLAB) {
                                continue;
                        }

                        float x, y;
                        get_grid_tile_position(grid_metrics, (size_t)column, (size_t)row, &x, &y);

                        enum HexagonThicknessMask thickness_mask = HEXAGON_THICKNESS_MASK_ALL;
                        size_t neighbor_column;
                        size_t neighbor_row;

                        if (get_hexagon_neighbor(&implementation->grid_metrics, (size_t)column, (size_t)row, HEXAGON_NEIGHBOR_BOTTOM, &neighbor_column, &neighbor_row)) {
                                const size_t neighbor_index = neighbor_row * implementation->grid_metrics.columns + neighbor_column;
                                const enum TileType neighbor_tile_type = implementation->tiles[neighbor_index];
                                if (neighbor_tile_type != TILE_EMPTY) {
                                        thickness_mask &= ~HEXAGON_THICKNESS_MASK_BOTTOM;
                                }
                        }

                        if (get_hexagon_neighbor(&level->implementation->grid_metrics, (size_t)column, (size_t)row, HEXAGON_NEIGHBOR_BOTTOM_LEFT, &neighbor_column, &neighbor_row)) {
                                const size_t neighbor_index = neighbor_row * implementation->grid_metrics.columns + neighbor_column;
                                const enum TileType neighbor_tile_type = implementation->tiles[neighbor_index];
                                if (neighbor_tile_type != TILE_EMPTY) {
                                        thickness_mask &= ~HEXAGON_THICKNESS_MASK_LEFT;
                                }
                        }

                        if (get_hexagon_neighbor(&level->implementation->grid_metrics, (size_t)column, (size_t)row, HEXAGON_NEIGHBOR_BOTTOM_RIGHT, &neighbor_column, &neighbor_row)) {
                                const size_t neighbor_index = neighbor_row * implementation->grid_metrics.columns + neighbor_column;
                                const enum TileType neighbor_tile_type = implementation->tiles[neighbor_index];
                                if (neighbor_tile_type != TILE_EMPTY) {
                                        thickness_mask &= ~HEXAGON_THICKNESS_MASK_RIGHT;
                                }
                        }

                        write_hexagon_thickness_geometry(implementation->grid_geometry, x, y, tile_radius + line_width / 2.0f, thickness, thickness_mask);
                }
        }

        for (uint8_t row = 0; row < level->rows; ++row) {
                for (uint8_t column = 0; column < level->columns; ++column) {
                        const enum TileType tile_type = implementation->tiles[(size_t)row * (size_t)level->columns + (size_t)column];
                        if (tile_type == TILE_EMPTY || tile_type == TILE_SLAB) {
                                continue;
                        }

                        float x, y;
                        get_grid_tile_position(grid_metrics, (size_t)column, (size_t)row, &x, &y);

                        set_geometry_color(implementation->grid_geometry, COLOR_LIGHT_YELLOW, COLOR_OPAQUE);
                        write_hexagon_geometry(implementation->grid_geometry, x, y, tile_radius + line_width / 2.0f, 0.0f);

                        // Don't use the color macros in expressions
                        if (tile_type == TILE_SPOT) {
                                set_geometry_color(implementation->grid_geometry, COLOR_GOLD, COLOR_OPAQUE);
                        } else {
                                set_geometry_color(implementation->grid_geometry, COLOR_YELLOW, COLOR_OPAQUE);
                        }

                        write_hexagon_geometry(implementation->grid_geometry, x, y, tile_radius - line_width / 2.0f, 0.0f);
                }
        }

        const float slab_thickness = thickness / 2.0f;
        const float slab_radius = tile_radius - line_width;

        for (uint8_t row = 0; row < level->rows; ++row) {
                for (uint8_t column = 0; column < level->columns; ++column) {
                        const enum TileType tile_type = implementation->tiles[(size_t)row * (size_t)level->columns + (size_t)column];
                        if (tile_type != TILE_SLAB) {
                                continue;
                        }

                        float x, y;
                        get_grid_tile_position(grid_metrics, (size_t)column, (size_t)row, &x, &y);

                        y -= slab_thickness;

                        set_geometry_color(implementation->grid_geometry, COLOR_GOLD, COLOR_OPAQUE);
                        write_hexagon_thickness_geometry(implementation->grid_geometry, x, y, slab_radius + line_width / 2.0f, slab_thickness, HEXAGON_THICKNESS_MASK_ALL);

                        set_geometry_color(implementation->grid_geometry, COLOR_LIGHT_YELLOW, COLOR_OPAQUE);
                        write_hexagon_geometry(implementation->grid_geometry, x, y, slab_radius + line_width / 2.0f, 0.0f);

                        set_geometry_color(implementation->grid_geometry, COLOR_YELLOW, COLOR_OPAQUE);
                        write_hexagon_geometry(implementation->grid_geometry, x, y, slab_radius - line_width / 2.0f, 0.0f);
                }
        }

        for (uint16_t entity_index = 0; entity_index < implementation->entity_count; ++entity_index) {
                resize_entity(implementation->entities[entity_index], implementation->grid_metrics.tile_radius);
        }
}