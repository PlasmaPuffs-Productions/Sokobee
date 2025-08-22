#include "Entity.h"

#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>

#include "../Assets.h"
#include "../Level.h"
#include "../Hexagons.h"
#include "../Geometry.h"
#include "../Animation.h"
#include "../Utilities.h"

#define SCALE_ALPHA 0.005f

static void setup_block_entity(struct Entity *);
static void block_entity_moved(struct Entity *, enum Orientation, struct TileState, bool, enum MoveType);
static void update_block_entity(struct Entity *, double);
static void destroy_block_entity(struct Entity *);

static const struct EntityAPI block_entity_API = (struct EntityAPI){
        .setup = setup_block_entity,
        .moved = block_entity_moved,
        .receive_event = NULL,
        .update = update_block_entity,
        .destroy = destroy_block_entity
};

const struct EntityAPI *get_block_entity_API(void) {
        return &block_entity_API;
}

struct Block {
        struct Geometry *geometry;
        bool is_locked_in;
        float scale;
};

static void setup_block_entity(struct Entity *const entity) {
        entity->payload = xcalloc(1ULL, sizeof(struct Block));

        struct Block *const block = (struct Block *)entity->payload;
        block->geometry = create_geometry(4ULL + HEXAGON_VERTEX_COUNT * 2ULL, 6ULL + HEXAGON_INDEX_COUNT * 2ULL);
        block->scale = 1.0f;
}

static void block_entity_moved(struct Entity *const entity, const enum Orientation orientation, const struct TileState tile_state, const bool pull_recursion, const enum MoveType move_type) {
        const bool placed = tile_state.tile_type == SPOT && move_type == MOVE_ORIGINAL;
        entity->flag = (size_t)placed;

        struct Block *const block = (struct Block *)entity->payload;
        block->scale = placed ? 1.1f : 1.2f;

        if (placed) {
                play_sound(SOUND_PLACED);
        }
}

static void update_block_entity(struct Entity *const entity, const double delta_time) {
        struct Block *const block = (struct Block *)entity->payload;
        block->scale += (1.0f - block->scale) * SCALE_ALPHA * delta_time;

        const float tile_radius = entity->grid_metrics->tile_radius * block->scale;
        const float thickness = tile_radius / 5.0f;
        const SDL_FPoint position = (SDL_FPoint){
                .x = entity->position.x + entity->shake_offset.x, 
                .y = entity->position.y + entity->shake_offset.y
        };

        clear_geometry(block->geometry);

        set_geometry_color(block->geometry, COLOR_GOLD);
        write_rectangle_geometry(block->geometry, (SDL_FRect){position.x, position.y, tile_radius, thickness}, 0.0f);
        write_hexagon_geometry(block->geometry, (SDL_FPoint){position.x, position.y + thickness / 2.0f}, tile_radius / 2.0f, 0.0f);

        set_geometry_color(block->geometry, COLOR_LIGHT_YELLOW);
        write_hexagon_geometry(block->geometry, (SDL_FPoint){position.x, position.y - thickness / 2.0f}, tile_radius / 2.0f, 0.0f);

        render_geometry(block->geometry);
}

static void destroy_block_entity(struct Entity *const entity) {
        struct Block *const block = (struct Block *)entity->payload;
        destroy_geometry(block->geometry);
        xfree(block);
}