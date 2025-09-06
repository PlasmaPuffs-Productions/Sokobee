#pragma once

#include <stdlib.h>
#include <stdbool.h>

#include "SDL.h"

enum Easing {
        LINEAR,
        QUAD_IN,
        QUAD_OUT,
        QUAD_IN_OUT,
        CUBE_IN,
        CUBE_OUT,
        CUBE_IN_OUT,
        SINE_IN,
        SINE_OUT,
        SINE_IN_OUT,
        BACK_IN,
        BACK_OUT,
        BACK_IN_OUT
};

float ease(const float time, const enum Easing easing);

enum ActionType {
        ACTION_FLOAT,
        ACTION_POINT,
        ACTION_COLOR
};

struct Action {
        enum ActionType type;
        void (*completion_callback)(void *);
        void *completion_callback_data;
        enum Easing easing;
        bool lazy_start;
        bool offset;
        bool pause;
        float duration;
        float elapsed;
        float delay;
        union {
                float *float_pointer;
                SDL_FPoint *point_pointer;
                SDL_Color *color_pointer;
        } target;
        union {
                float floats[2];
                SDL_FPoint points[2];
                SDL_Color colors[2];
        } keyframes;
};

struct Animation {
        struct Action *actions;
        size_t action_count;
        size_t action_index;
        bool active;
};

struct Animation *create_animation(const size_t action_count);
void destroy_animation(struct Animation *const animation);

void initialize_animation(struct Animation *const animation, const size_t action_count);
void deinitialize_animation(struct Animation *const animation);

void start_animation(struct Animation *const animation, const size_t action_index);
void stop_animation(struct Animation *const animation);
void reset_animation(struct Animation *const animation);
void update_animation(struct Animation *const animation, const double delta_time);
void restart_animation(struct Animation *const animation, const size_t action_index);