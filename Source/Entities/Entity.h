#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <SDL.h>

#include "../Animation.h"
#include "../Hexagons.h"

#define ENTITY_TYPE_COUNT 2ULL
enum EntityType {
        ENTITY_PLAYER,
        ENTITY_BLOCK
};

struct Level;
struct Entity {
        struct Level *level;
        enum EntityType type;
        uint8_t current_column;
        uint8_t current_row;
        uint8_t previous_column;
        uint8_t previous_row;
        SDL_FPoint position;
        const struct GridMetrics *grid_metrics;
        struct Animation movement;
        enum Orientation orientation;
        SDL_FPoint shake_offset;
        float shake_power;
        void *payload;
        size_t flag;
};

struct TileState;
enum MoveType {
        MOVE_ORIGINAL,
        MOVE_UNDO,
        MOVE_REDO
};

struct EntityAPI {
        void (*setup)(struct Entity *);
        void (*moved)(struct Entity *, enum Orientation, struct TileState, bool, enum MoveType);
        void (*receive_event)(struct Entity *, const SDL_Event *);
        void (*update)(struct Entity *, double);
        void (*destroy)(struct Entity *);
};

struct Entity *create_entity(struct Level *const level, const enum EntityType type, const uint8_t column, const uint8_t row, const enum Orientation orientation);
void destroy_entity(struct Entity *const entity);

bool move_entity(struct Entity *const entity, const enum Orientation orientation, const bool pushed, const bool pull, const enum MoveType move_type);
void set_entity_grid_metrics(struct Entity *const entity, const struct GridMetrics *const grid_metrics);
void entity_receive_event(struct Entity *const entity, const SDL_Event *const event);
void update_entity(struct Entity *const entity, const double delta_time);

const struct EntityAPI *get_player_entity_API(void);
const struct EntityAPI *get_block_entity_API(void);