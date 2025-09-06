#include "Gesture.h"

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "SDL.h"

#include "Debug.h"
#include "Context.h"
#include "Level.h"

struct GestureState {
        bool active;
        float start_x;
        float start_y;
        uint32_t start_time;
};

static struct GestureState gesture = {0};

#define TAP_DISTANCE_THRESHOLD     0.05f // 5% of screen
#define TAP_TIME_THRESHOLD         300   // max time for tap
#define SWIPE_DISTANCE_THRESHOLD   0.15f // 15% of screen
#define SWIPE_TIME_THRESHOLD       500   // max time for swipe

static inline void get_event_position(const SDL_Event *const event, const int screen_width, const int screen_height, float *const x, float *const y) {
#ifndef NDEBUG
        if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP || event->type == SDL_MOUSEMOTION) {
                *x = (float)event->button.x / (float)screen_width;
                *y = (float)event->button.y / (float)screen_height;
                return;
        }
#endif

        if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERUP || event->type == SDL_FINGERMOTION) {
                *x = event->tfinger.x;
                *y = event->tfinger.y;
        }
}

enum Input handle_gesture_event(const SDL_Event *const event) {
        int screen_width, screen_height;
        SDL_GetWindowSize(get_context_window(), &screen_width, &screen_height);

        float x, y;

#ifndef NDEBUG
        if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_FINGERDOWN) {
#else
        if (event->type == SDL_FINGERDOWN) {
#endif
                get_event_position(event, screen_width, screen_height, &x, &y);
                gesture.start_x = x;
                gesture.start_y = y;
                gesture.start_time = (uint32_t)SDL_GetTicks();
                gesture.active = true;
                return INPUT_NONE;
        }

#ifndef NDEBUG
        if (event->type == SDL_MOUSEBUTTONUP || event->type == SDL_FINGERUP) {
#else
        if (event->type == SDL_FINGERUP) {
#endif

                if (!gesture.active) {
                        return INPUT_NONE;
                }

                get_event_position(event, screen_width, screen_height, &x, &y);
                const uint32_t end_time = (uint32_t)SDL_GetTicks();
                const uint32_t delta_time = end_time - gesture.start_time;

                const float dx = x - gesture.start_x;
                const float dy = y - gesture.start_y;
                const float distance = sqrtf(dx * dx + dy * dy);

                gesture.active = false;

                if (distance < TAP_DISTANCE_THRESHOLD && delta_time < TAP_TIME_THRESHOLD) {
                        return INPUT_FORWARD;
                }

                if (distance > SWIPE_DISTANCE_THRESHOLD && delta_time < SWIPE_TIME_THRESHOLD) {
                        if (fabsf(dx) > fabsf(dy)) {
                                return dx > 0 ? INPUT_RIGHT : INPUT_LEFT;
                        } else {
                                return dy > 0 ? INPUT_BACKWARD : INPUT_FORWARD;
                        }
                }

                return INPUT_NONE;
        }

        return INPUT_NONE;
}