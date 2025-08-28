#include "Level.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <SDL.h>

#include "Audio.h"
#include "cJSON.h"
#include "Assets.h"
#include "Context.h"
#include "Hexagons.h"
#include "Entities/Entity.h"
#include "Geometry.h"

#define LEVEL_DIMENSION_LIMIT 20

struct LevelImplementation {
        enum TileType *tiles;
        size_t tile_count;
        struct Entity **entities;
        size_t entity_count;
        struct GridMetrics grid_metrics;
        struct Geometry *grid_geometry;
};

static bool parse_level(const cJSON *const json, struct Level *const level);

static void resize_level(struct Level *const level);

struct Level *load_level(const struct LevelMetadata *const metadata) {
        struct Level *const level = (struct Level *)xcalloc(1, sizeof(struct Level));
        if (!initialize_level(level, metadata)) {
                send_message(ERROR, "Failed to load level: Failed to initialize level");
                destroy_level(level);
                return NULL;
        }

        return level;
}

void destroy_level(struct Level *const level) {
        if (!level) {
                send_message(WARNING, "Level given to destroy is NULL");
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

        char *const json_string = load_text_file(metadata->path);
        if (!json_string) {
                send_message(ERROR, "Failed to initialize level \"%s\": Failed to load level data file \"%s\"", metadata->title, metadata->path);
                deinitialize_level(level);
                return false;
        }

        cJSON *const json = cJSON_Parse(json_string);
        xfree(json_string);

        if (!json) {
                send_message(ERROR, "Failed to initialize level \"%s\": Failed to parse level data file \"%s\": %s", metadata->title, metadata->path, cJSON_GetErrorPtr());
                deinitialize_level(level);
                return false;
        }

        if (!parse_level(json, level)) {
                send_message(ERROR, "Failed to initialize level \"%s\": Failed to parse level", metadata->title);
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
                send_message(WARNING, "Level given to deinitialize is NULL");
                return;
        }

        xfree(level->title);
        level->title = NULL;

        struct LevelImplementation *const implementation = level->implementation;
        if (!implementation) {
                return;
        }

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

struct TileState get_level_tile_state(const struct Level *const level, const int8_t column, const int8_t row) {
        if (column < 0 || column >= (int8_t)level->columns || row < 0 || row >= (int8_t)level->rows) {
                return (struct TileState){
                        .tile_type = EMPTY,
                        .entity = NULL
                };
        }

        struct LevelImplementation *const implementation = level->implementation;

        struct Entity *entity = NULL;
        for (size_t entity_index = 0ULL; entity_index < implementation->entity_count; ++entity_index) {
                entity = implementation->entities[entity_index];
                if (entity->current_column == (uint8_t)column && entity->current_row == (uint8_t)row) {
                        break;
                }

                entity = NULL;
        }

        return (struct TileState){
                .tile_type = implementation->tiles[(size_t)row * (size_t)level->columns + (size_t)column],
                .entity = entity
        };
}

void level_receive_event(struct Level *const level, const SDL_Event *const event) {
        struct LevelImplementation *const implementation = level->implementation;

        if (event->type == SDL_WINDOWEVENT) {
                const Uint8 window_event = event->window.event;
                if (window_event == SDL_WINDOWEVENT_RESIZED || window_event == SDL_WINDOWEVENT_MAXIMIZED || window_event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        resize_level(level);
                }

                return;
        }

        for (size_t entity_index = 0ULL; entity_index < implementation->entity_count; ++entity_index) {
                entity_receive_event(implementation->entities[entity_index], event);
        }

        for (size_t entity_index = 0ULL; entity_index < implementation->entity_count; ++entity_index) {
                struct Entity *const entity = implementation->entities[entity_index];
                if (entity->type == ENTITY_BLOCK && !entity->flag) {
                        return;
                }
        }

        play_sound(SOUND_WIN);
        if (level->completion_callback) {
                level->completion_callback(level->completion_callback_data);
        }
}

void update_level(struct Level *const level, const double delta_time) {
        struct LevelImplementation *const implementation = level->implementation;
        render_geometry(implementation->grid_geometry);

        for (size_t entity_index = 0ULL; entity_index < implementation->entity_count; ++entity_index) {
                struct Entity *const entity = implementation->entities[entity_index];
                if (entity->type != ENTITY_PLAYER) {
                        update_entity(entity, delta_time);
                }
        }

        for (size_t entity_index = 0ULL; entity_index < implementation->entity_count; ++entity_index) {
                struct Entity *const entity = implementation->entities[entity_index];
                if (entity->type == ENTITY_PLAYER) {
                        update_entity(entity, delta_time);
                }
        }
}

static bool parse_level(const cJSON *const json, struct Level *const level) {
        if (!cJSON_IsObject(json)) {
                send_message(ERROR, "Failed to parse level: JSON data is invalid");
                return false;
        }

        const cJSON *const columns_json  = cJSON_GetObjectItemCaseSensitive(json, "columns");
        const cJSON *const rows_json     = cJSON_GetObjectItemCaseSensitive(json, "rows");
        const cJSON *const tiles_json    = cJSON_GetObjectItemCaseSensitive(json, "tiles");
        const cJSON *const entities_json = cJSON_GetObjectItemCaseSensitive(json, "entities");

        if (!cJSON_IsNumber(columns_json) || !cJSON_IsNumber(rows_json) || !cJSON_IsArray(tiles_json) || !cJSON_IsArray(entities_json)) {
                send_message(ERROR, "Failed to parse level: JSON data is invalid");
                return false;
        }

        const double columns = columns_json->valuedouble;
        if (floor(columns) != columns || columns <= 0.0 || columns > (double)LEVEL_DIMENSION_LIMIT) {
                send_message(ERROR, "Failed to parse level: The grid columns %lf is invalid, it should be an integer between 0 and %u", columns, LEVEL_DIMENSION_LIMIT);
                return false;
        }

        const double rows = rows_json->valuedouble;
        if (floor(rows) != rows || rows <= 0.0 || rows > (double)LEVEL_DIMENSION_LIMIT) {
                send_message(ERROR, "Failed to parse level: The grid rows %lf is invalid, it should be an integer between 0 and %u", rows, LEVEL_DIMENSION_LIMIT);
                return false;
        }

        struct LevelImplementation *const implementation = level->implementation;
        level->columns = (uint8_t)columns;
        level->rows = (uint8_t)rows;

        const size_t tile_count = (size_t)cJSON_GetArraySize(tiles_json);
        implementation->tile_count = (size_t)level->columns * (size_t)level->rows;
        if (tile_count != implementation->tile_count) {
                send_message(ERROR, "Failed to parse level: The tile count of %zu does not match the expected tile count of %zu (%u * %u)", tile_count, implementation->tile_count, level->columns, level->rows);
                return false;
        }

        implementation->tiles = (enum TileType *)xmalloc(implementation->tile_count * sizeof(enum TileType));

        size_t tile_index = 0ULL;
        const cJSON *tile_json = NULL;
        cJSON_ArrayForEach(tile_json, tiles_json) {
                if (!cJSON_IsNumber(tile_json)) {
                        send_message(ERROR, "Failed to parse level: JSON data is invalid");
                        return false;
                }

                const double tile = tile_json->valuedouble;
                if (floor(tile) != tile || tile < 0.0 || (size_t)tile > (size_t)TILE_TYPE_MAXIMUM) {
                        send_message(ERROR, "Failed to parse level: The tile #%zu of %lf is invalid, it should be an integer between 0 and %d", tile_index, tile, (int)TILE_TYPE_MAXIMUM);
                        return false;
                }

                implementation->tiles[tile_index++] = (enum TileType)(uint8_t)tile;
        }

        const int entities_length = cJSON_GetArraySize(entities_json);
        if (entities_length % 4) {
                send_message(ERROR, "Failed to parse level: Entities array length of %d is not a multiple of 4", entities_length);
                return false;
        }

        implementation->entity_count = (size_t)entities_length / 4ULL;
        implementation->entities = (struct Entity **)xcalloc(implementation->entity_count, sizeof(struct Entity *));

        const cJSON *entity_part_json = entities_json->child;
        for (size_t entity_index = 0ULL; entity_index < implementation->entity_count; ++entity_index) {
                const cJSON *const entity_type_json = entity_part_json;
                const cJSON *const entity_column_json = entity_type_json->next;
                const cJSON *const entity_row_json = entity_column_json->next;
                const cJSON *const entity_orientation_json = entity_row_json->next;
                entity_part_json = entity_orientation_json->next;

                const enum EntityType entity_type = (enum EntityType)(uint8_t)entity_type_json->valuedouble;
                const uint8_t entity_column = (uint8_t)entity_column_json->valuedouble;
                const uint8_t entity_row = (uint8_t)entity_row_json->valuedouble;
                const enum Orientation entity_orientation = (enum Orientation)(uint8_t)entity_orientation_json->valuedouble;
                implementation->entities[entity_index] = create_entity(level, entity_type, entity_column, entity_row, entity_orientation);
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
                        if (tile_type == EMPTY) {
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
                                if (neighbor_tile_type != EMPTY) {
                                        thickness_mask &= ~HEXAGON_THICKNESS_MASK_BOTTOM;
                                }
                        }

                        if (get_hexagon_neighbor(&level->implementation->grid_metrics, (size_t)column, (size_t)row, HEXAGON_NEIGHBOR_BOTTOM_LEFT, &neighbor_column, &neighbor_row)) {
                                const size_t neighbor_index = neighbor_row * implementation->grid_metrics.columns + neighbor_column;
                                const enum TileType neighbor_tile_type = implementation->tiles[neighbor_index];
                                if (neighbor_tile_type != EMPTY) {
                                        thickness_mask &= ~HEXAGON_THICKNESS_MASK_LEFT;
                                }
                        }

                        if (get_hexagon_neighbor(&level->implementation->grid_metrics, (size_t)column, (size_t)row, HEXAGON_NEIGHBOR_BOTTOM_RIGHT, &neighbor_column, &neighbor_row)) {
                                const size_t neighbor_index = neighbor_row * implementation->grid_metrics.columns + neighbor_column;
                                const enum TileType neighbor_tile_type = implementation->tiles[neighbor_index];
                                if (neighbor_tile_type != EMPTY) {
                                        thickness_mask &= ~HEXAGON_THICKNESS_MASK_RIGHT;
                                }
                        }

                        write_hexagon_thickness_geometry(implementation->grid_geometry, x, y, tile_radius + line_width / 2.0f, thickness, thickness_mask);
                }
        }

        for (uint8_t row = 0; row < level->rows; ++row) {
                for (uint8_t column = 0; column < level->columns; ++column) {
                        const enum TileType tile_type = implementation->tiles[(size_t)row * (size_t)level->columns + (size_t)column];
                        if (tile_type == EMPTY) {
                                continue;
                        }

                        float x, y;
                        get_grid_tile_position(grid_metrics, (size_t)column, (size_t)row, &x, &y);

                        set_geometry_color(implementation->grid_geometry, COLOR_LIGHT_YELLOW, COLOR_OPAQUE);
                        write_hexagon_geometry(implementation->grid_geometry, x, y, tile_radius + line_width / 2.0f, 0.0f);

                        // Don't use the color macros in expressions
                        if (tile_type == SPOT) {
                                set_geometry_color(implementation->grid_geometry, COLOR_GOLD, COLOR_OPAQUE);
                        } else {
                                set_geometry_color(implementation->grid_geometry, COLOR_YELLOW, COLOR_OPAQUE);
                        }

                        write_hexagon_geometry(implementation->grid_geometry, x, y, tile_radius - line_width / 2.0f, 0.0f);
                }
        }

        for (size_t entity_index = 0ULL; entity_index < implementation->entity_count; ++entity_index) {
                set_entity_grid_metrics(implementation->entities[entity_index], grid_metrics);
        }
}