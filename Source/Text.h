#pragma once

#include <SDL.h>

#include <stdint.h>
#include <stdbool.h>

#include "Assets.h"

enum TextAlignment {
        TEXT_ALIGNMENT_LEFT,
        TEXT_ALIGNMENT_CENTER,
        TEXT_ALIGNMENT_RIGHT
};

struct TextImplementation;
struct Text {
        struct TextImplementation *implementation;
        float screen_position_x;
        float screen_position_y;
        float relative_offset_x;
        float relative_offset_y;
        float absolute_offset_x;
        float absolute_offset_y;
        float scale_x;
        float scale_y;
        float rotation;
        bool visible;
};

typedef struct SDL_Color SDL_Color;

struct Text *create_text(const char *const string, const enum Font font);
void destroy_text(struct Text *const text);

void initialize_text(struct Text *const text, const char *const string, const enum Font font);
void deinitialize_text(struct Text *const text);

void update_text(struct Text *const text);
void get_text_dimensions(struct Text *const text, size_t *const width, size_t *const height);

void set_text_string(struct Text *const text, const char *const string);
void set_text_font(struct Text *const text, const enum Font font);
void set_text_alignment(struct Text *const text, const enum TextAlignment alignment);
void set_text_maximum_width(struct Text *const text, const float maximum_width);
void set_text_line_spacing(struct Text *const text, const float line_spacing);
void set_text_color(struct Text *const text, const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a);