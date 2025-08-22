#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

enum TileType {
        EMPTY,
        CELL,
        SPOT
};

#define TILE_TYPE_MAXIMUM SPOT

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
struct TileState {
        const enum TileType tile_type;
        struct Entity *const entity;
};

struct TileState get_level_tile_state(const struct Level *const level, const int8_t column, const int8_t row);