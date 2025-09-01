#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Hexagons.h"

enum TileType {
        TILE_EMPTY,
        TILE_CELL,
        TILE_SPOT,
        TILE_SLAB,
        TILE_COUNT
};

struct LevelImplementation;
struct Level {
        char *title;
        uint8_t columns;
        uint8_t rows;
        size_t move_count;
        void (*completion_callback)(void *);
        void *completion_callback_data;
        struct LevelImplementation *implementation;
};

struct LevelMetadata;

struct Level *load_level(const struct LevelMetadata *const metadata);
void destroy_level(struct Level *const level);

bool initialize_level(struct Level *const level, const struct LevelMetadata *const metadata);
void deinitialize_level(struct Level *const level);

typedef union SDL_Event SDL_Event;
void level_receive_event(struct Level *const level, const SDL_Event *const event);
void update_level(struct Level *const level, const double delta_time);

struct Entity;
bool query_level_tile(
        const struct Level *const level,
        const uint16_t tile_index,
        enum TileType *const out_tile_type,
        struct Entity **const out_entity,
        float *const out_x,
        float *const out_y
);

enum Input {
        INPUT_FORWARD,
        INPUT_BACKWARD,
        INPUT_LEFT,
        INPUT_RIGHT,
        INPUT_SWITCH,
        INPUT_UNDO,
        INPUT_REDO,
        INPUT_NONE
};

enum ChangeType {
        CHANGE_WALK,
        CHANGE_TURN,
        CHANGE_PUSH,
        CHANGE_PUSHED,
        CHANGE_INVALID
};

struct Change {
        enum Input input;
        enum ChangeType type;
        struct Entity *entity;
        union {
                struct {
                        uint16_t last_tile_index;
                        uint16_t next_tile_index;
                } move;
                struct {
                        enum Orientation last_orientation;
                        enum Orientation next_orientation;
                } turn;
                struct {
                        enum Orientation direction;
                } face;
        };
};