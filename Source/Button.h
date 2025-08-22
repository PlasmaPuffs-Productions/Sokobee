#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "Hexagons.h"

enum IconType;
struct ButtonImplementation;
struct Button {
        float scale;
        float screen_position_x;
        float screen_position_y;
        float relative_offset_x;
        float relative_offset_y;
        float absolute_offset_x;
        float absolute_offset_y;
        float grid_anchor_x;
        float grid_anchor_y;
        int8_t tile_offset_column;
        int8_t tile_offset_row;
        void (*callback)(void *);
        void *callback_data;
        enum HexagonThicknessMask thickness_mask;
        struct ButtonImplementation *implementation;
};

struct Button *create_button(const bool grid_slot_positioning);
void destroy_button(struct Button *const button);

void initialize_button(struct Button *const button, const bool grid_slot_positioning);
void deinitialize_button(struct Button *const button);

typedef union SDL_Event SDL_Event;

void get_button_metrics(const struct Button *const button, float *const out_x, float *const out_y, float *const out_radius);
void set_button_surface_icon(struct Button *const button, const enum IconType icon_type);
bool set_button_surface_text(struct Button *const button, char *const surface_text);
void set_button_tooltip_text(struct Button *const button, char *const tooltip_text);
void button_receive_event(struct Button *const button, const SDL_Event *const event);
bool update_button(struct Button *const button, const double delta_time);