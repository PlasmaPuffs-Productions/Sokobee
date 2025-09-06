#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "SDL.h"

#include "Animation.h"
#include "Hexagons.h"

enum EntityType {
        ENTITY_PLAYER,
        ENTITY_BLOCK,
        ENTITY_COUNT
};

struct Level;
struct Entity;

struct Entity *create_entity(struct Level *const level, const enum EntityType type, const uint16_t tile_index, const enum Orientation orientation);
void destroy_entity(struct Entity *const entity);

void update_entity(struct Entity *const entity, const double delta_time);
void resize_entity(struct Entity *const entity, const float radius);

enum EntityType get_entity_type(const struct Entity *const entity);
uint16_t get_entity_tile_index(const struct Entity *const entity);
enum Orientation get_entity_orientation(const struct Entity *const entity);

struct Change;

bool entity_can_change(const struct Entity *const entity);
void entity_handle_change(struct Entity *const entity, const struct Change *const change);