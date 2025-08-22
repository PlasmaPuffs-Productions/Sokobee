#include "Animation.h"

#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL.h>

#include "Utilities.h"

// https://easings.net/

#define C1 1.70158f
#define C2 C1 * 1.525f
#define C3 C1 + 1.0f
#define C4 2.0f * (float)M_PI / 3.0f
#define C5 2.0f * (float)M_PI / 4.5f

float ease(const float time, const enum Easing easing);

struct Animation *create_animation(const size_t action_count) {
        struct Animation *const animation = (struct Animation *)xmalloc(sizeof(struct Animation));
        initialize_animation(animation, action_count);
        return animation;
}

void destroy_animation(struct Animation *const animation) {
        if (!animation) {
                send_message(WARNING, "Animation given to destroy is NULL");
                return;
        }

        deinitialize_animation(animation);
        xfree(animation);
}

void initialize_animation(struct Animation *const animation, const size_t action_count) {
        animation->actions = (struct Action *)xcalloc(action_count, sizeof(struct Action));
        animation->action_count = action_count;
        animation->action_index = SIZE_MAX;
        animation->active = false;
}

void deinitialize_animation(struct Animation *const animation) {
        if (!animation) {
                send_message(WARNING, "Animation given to deinitialize is NULL");
                return;
        }

        xfree(animation->actions);
        animation->actions = NULL;
}

static void start_action(struct Action *const action);

void start_animation(struct Animation *const animation, const size_t action_index) {
        animation->active = true;

        if (animation->action_index == SIZE_MAX) {
                animation->action_index = action_index;
                start_action(&animation->actions[action_index]);
        }
}

void stop_animation(struct Animation *const animation) {
        animation->active = false;
}

void reset_animation(struct Animation *const animation) {
        stop_animation(animation);

        if (animation->action_index == SIZE_MAX) {
                return;
        }

        for (size_t action_index = 0ULL; action_index < animation->action_index; ++action_index) {
                animation->actions[action_index].elapsed = 0.0f;
        }

        animation->action_index = SIZE_MAX;
}

void restart_animation(struct Animation *const animation, const size_t action_index) {
        reset_animation(animation);
        start_animation(animation, action_index);
}

static void apply_action(struct Action *const action, const float value);

void update_animation(struct Animation *const animation, const double delta_time) {
        if (!animation->active) {
                return;
        }

        struct Action *const current_action = &animation->actions[animation->action_index];

        current_action->elapsed += (float)delta_time;
        if (current_action->elapsed <= current_action->delay) {
                return;
        }

        const float elapsed = current_action->elapsed - current_action->delay;
        if (elapsed > current_action->duration) {
                apply_action(current_action, 1.0f);
                if (current_action->completion_callback) {
                        current_action->completion_callback(current_action->completion_callback_data);
                }

                if (++animation->action_index >= animation->action_count) {
                        reset_animation(animation);
                        return;
                }

                if (current_action->pause) {
                        stop_animation(animation);
                        return;
                }

                start_action(&animation->actions[animation->action_index]);
                update_animation(animation, current_action->duration - elapsed);
                return;
        }

        const float progress = elapsed / current_action->duration;
        const float value = ease(progress, current_action->easing);
        apply_action(current_action, value);
}

static void start_action(struct Action *const action) {
        if (!action->lazy_start) {
                return;
        }

        switch (action->type) {
                case ACTION_FLOAT: {
                        if (!action->target.float_pointer) {
                                send_message(ERROR, "Failed to start action: The float pointer referencing the start keyframe is NULL");
                                return;
                        }

                        action->keyframes.floats[0] = *action->target.float_pointer;
                        break;
                }

                case ACTION_POINT: {
                        if (!action->target.point_pointer) {
                                send_message(ERROR, "Failed to start action: The point pointer referencing the start keyframe is NULL");
                                return;
                        }

                        action->keyframes.points[0] = *action->target.point_pointer;
                        break;
                }

                case ACTION_COLOR: {
                        if (!action->target.color_pointer) {
                                send_message(ERROR, "Failed to start action: The color pointer referencing the start keyframe is NULL");
                                return;
                        }

                        action->keyframes.colors[0] = *action->target.color_pointer;
                        break;
                }
        }
}

static void apply_action(struct Action *const action, const float value) {
        switch (action->type) {
                case ACTION_FLOAT: {
                        const float a = action->keyframes.floats[0];
                        const float b = action->keyframes.floats[1];
                        *action->target.float_pointer = a + value * (action->offset ? b : b - a);
                        break;
                }

                case ACTION_POINT: {
                        const SDL_FPoint a = action->keyframes.points[0];
                        const SDL_FPoint b = action->keyframes.points[1];
                        (*action->target.point_pointer).x = a.x + value * (action->offset ? b.x : b.x - a.x);
                        (*action->target.point_pointer).y = a.y + value * (action->offset ? b.y : b.y - a.y);
                        break;
                }

                case ACTION_COLOR: {
                        const SDL_Color a = action->keyframes.colors[0];
                        const SDL_Color b = action->keyframes.colors[1];
                        (*action->target.color_pointer).r = a.r + (Uint8)lroundf(value * (float)(action->offset ? b.r : b.r - a.r));
                        (*action->target.color_pointer).g = a.g + (Uint8)lroundf(value * (float)(action->offset ? b.g : b.g - a.g));
                        (*action->target.color_pointer).b = a.b + (Uint8)lroundf(value * (float)(action->offset ? b.b : b.b - a.b));
                        (*action->target.color_pointer).a = a.a + (Uint8)lroundf(value * (float)(action->offset ? b.a : b.a - a.a));
                        break;
                }
        }
}

float ease(const float time, const enum Easing easing) {
        const float t = time;
        switch (easing) {
                case LINEAR: {
                        return t;
                }

                case QUAD_IN: {
                        return t * t;
                }

                case QUAD_OUT: {
                        return t * (2.0f - t);
                }

                case QUAD_IN_OUT: {
                        if (t < 0.5f) {
                                return 2.0f * t * t;
                        }

                        return -1.0f + (4.0f - t * 2.0f) * t;
                }

                case CUBE_IN: {
                        return t * t * t;
                }

                case CUBE_OUT: {
                        const float f = t - 1.0f;
                        return f * f * f + 1.0f;
                }

                case CUBE_IN_OUT: {
                        if (t < 0.5f) {
                                return t * t * t * 4.0f;
                        }

                        const float f = t * 2.0f - 2.0f;
                        return f * f * f / 2.0f + 1.0f;
                }

                case SINE_IN: {
                        return 1.0f - cosf(t * (float)M_PI / 2.0f);
                }

                case SINE_OUT: {
                        return sinf(t * (float)M_PI / 2.0f);
                }

                case SINE_IN_OUT: {
                        return -(cosf(t * (float)M_PI) - 1.0f) / 2.0f;
                }

                // For the formulas that were commented out:
                // They don't work for some reason even though they seem mathematically identical to the current formula used

                case BACK_IN: {
                        const float c1 = 1.70158f;
                        const float c3 = c1 + 1.0f;
                        const float t2 = t * t;
                        const float t3 = t2 * t;
                        return c3 * t3 - c1 * t2;

                        // return C3 * powf(t, 3.0f) - C1 * powf(t, 2.0f);
                }

                case BACK_OUT: {
                        const float c1 = 1.70158f;
                        const float c3 = c1 + 1.0f;
                        const float y = t - 1.0f;
                        return 1.0f + c3 * powf(y, 3.0f) + c1 * powf(y, 2.0f);

                        // return 1.0f + C3 * powf(t - 1.0f, 3.0f) + C1 * powf(t - 1.0f, 2.0f);
                }

                case BACK_IN_OUT: {
                        if (t < 0.5f) {
                                return powf(t * 2.0f, 2.0f) * ((C2 + 1.0f) * 2.0f * t - C2) / 2.0f;
                        }

                        const float f = t * 2.0f - 2.0f;
                        return (powf(f, 2.0f) * ((C2 + 1.0f) * f + C2) + 2.0f) / 2.0f;
                }
        }
}