#include "Button.h"

#include <assert.h>
#include <stdint.h>

#include <SDL.h>

#include "Audio.h"
#include "Cursor.h"
#include "Assets.h"
#include "Context.h"
#include "Geometry.h"
#include "Animation.h"
#include "Hexagons.h"
#include "Icons.h"
#include "Text.h"

#define PADDING_FACTOR 0.02f
#define MINIMUM_PADDING 20.0f
#define MAXIMUM_PADDING 100.0f

#define BUTTON_STATE_COUNT 3ULL
enum ButtonState {
        BUTTON_IDLE,
        BUTTON_HOVER,
        BUTTON_PRESSED
};

struct ButtonImplementation {
        enum ButtonState state;
        struct Geometry *geometry;
        struct Animation animations;
        float animation_offset;
        float computed_radius;
        struct GridMetrics *grid_metrics;
        struct Text *surface_text;
        struct Icon *surface_icon;
        char *tooltip_text;
        bool hovering;
};

static void resize_button(struct Button *);

struct Button *create_button(const bool grid_slot_positioning) {
        struct Button *const button = (struct Button *)xmalloc(sizeof(struct Button));
        initialize_button(button, grid_slot_positioning);
        return button;
}

void destroy_button(struct Button *const button) {
        if (!button) {
                send_message(MESSAGE_WARNING, "Button given to destroy is NULL");
                return;
        }

        deinitialize_button(button);
        xfree(button);
}

void initialize_button(struct Button *const button, const bool grid_slot_positioning) {
        button->scale = 1.0f;
        button->screen_position_x = 0.0f;
        button->screen_position_y = 0.0f;
        button->relative_offset_x = 0.0f;
        button->relative_offset_y = 0.0f;
        button->absolute_offset_x = 0.0f;
        button->absolute_offset_y = 0.0f;
        button->grid_anchor_x = 0.0f;
        button->grid_anchor_y = 0.0f;
        button->tile_offset_column = 0;
        button->tile_offset_row = 0;
        button->callback = NULL;
        button->callback_data = NULL;
        button->thickness_mask = HEXAGON_THICKNESS_MASK_ALL;

        button->implementation = (struct ButtonImplementation *)xmalloc(sizeof(struct ButtonImplementation));
        button->implementation->geometry = create_geometry();
        button->implementation->state = BUTTON_IDLE;
        button->implementation->hovering = false;
        button->implementation->tooltip_text = NULL;
        button->implementation->animation_offset = 0.0f;
        button->implementation->computed_radius = 0.0f;
        button->implementation->surface_icon = NULL;
        button->implementation->surface_text = NULL;
        initialize_animation(&button->implementation->animations, BUTTON_STATE_COUNT);

        struct Action *const idle = &button->implementation->animations.actions[BUTTON_IDLE];
        idle->target.float_pointer = &button->implementation->animation_offset;
        idle->keyframes.floats[1] = 0.0f;
        idle->type = ACTION_FLOAT;
        idle->duration = 150.0f;
        idle->easing = SINE_OUT;
        idle->lazy_start = true;
        idle->pause = true;

        struct Action *const hover = &button->implementation->animations.actions[BUTTON_HOVER];
        hover->target.float_pointer = &button->implementation->animation_offset;
        hover->keyframes.floats[1] = 0.1f;
        hover->type = ACTION_FLOAT;
        hover->duration = 100.0f;
        hover->easing = BACK_OUT;
        hover->lazy_start = true;
        hover->pause = true;

        struct Action *const pressed = &button->implementation->animations.actions[BUTTON_PRESSED];
        pressed->target.float_pointer = &button->implementation->animation_offset;
        pressed->keyframes.floats[1] = -0.25f;
        pressed->type = ACTION_FLOAT;
        pressed->duration = 50.0f;
        pressed->easing = QUAD_IN;
        pressed->lazy_start = true;
        pressed->pause = true;

        button->implementation->grid_metrics = grid_slot_positioning ? (struct GridMetrics *)xmalloc(sizeof(struct GridMetrics)) : NULL;
        resize_button(button);
}

void deinitialize_button(struct Button *const button) {
        if (!button) {
                send_message(MESSAGE_ERROR, "Button given to deinitialize is NULL");
                return;
        }

        if (button->implementation) {
                deinitialize_animation(&button->implementation->animations);
                destroy_geometry(button->implementation->geometry);

                if (button->implementation->surface_icon) {
                        destroy_icon(button->implementation->surface_icon);
                }

                if (button->implementation->surface_text) {
                        destroy_text(button->implementation->surface_text);
                }

                if (button->implementation->grid_metrics) {
                        xfree(button->implementation->grid_metrics);
                }

                if (button->implementation->tooltip_text) {
                        xfree(button->implementation->tooltip_text);
                }

                xfree(button->implementation);
                button->implementation = NULL;
        }
}

void get_button_metrics(const struct Button *const button, float *const out_x, float *const out_y, float *const out_radius) {
        if (out_x == NULL && out_y == NULL && out_radius == NULL) {
                return;
        }

        const float radius = button->implementation->computed_radius * button->scale;

        if (out_radius != NULL) {
                *out_radius = radius;
        }

        if (button->implementation->grid_metrics) {
                const int8_t column = (int8_t)lroundf(button->grid_anchor_x * (float)(button->implementation->grid_metrics->columns - 1ULL)) + button->tile_offset_column;
                const int8_t row    = (int8_t)lroundf(button->grid_anchor_y * (float)(button->implementation->grid_metrics->rows    - 1ULL)) + button->tile_offset_row;
                get_grid_tile_position(button->implementation->grid_metrics, column, row, out_x, out_y);
                return;
        }

        int drawable_width;
        int drawable_height;
        SDL_GetRendererOutputSize(get_context_renderer(), &drawable_width, &drawable_height);

        if (out_x != NULL) {
                *out_x =
                        button->screen_position_x * (float)drawable_width +
                        button->relative_offset_x * radius +
                        button->absolute_offset_x;
        }

        if (out_y != NULL) {
                *out_y =
                        button->screen_position_y * (float)drawable_height +
                        button->relative_offset_y * radius +
                        button->absolute_offset_y;
        }
}

void set_button_surface_icon(struct Button *const button, const enum IconType icon_type) {
        if (button->implementation->surface_icon) {
                set_icon_type(button->implementation->surface_icon, icon_type);
                return;
        }

        button->implementation->surface_icon = create_icon(icon_type);
}

bool set_button_surface_text(struct Button *const button, char *const surface_text) {
        if (button->implementation->surface_text) {
                set_text_string(button->implementation->surface_text, surface_text);
                return true;
        }

        button->implementation->surface_text = create_text(surface_text, FONT_HEADER_1);
        button->implementation->surface_text->relative_offset_x = -0.5f;
        button->implementation->surface_text->relative_offset_y = -0.5f;
        set_text_color(button->implementation->surface_text, COLOR_BROWN, 255);

        return true;
}

void set_button_tooltip_text(struct Button *const button, char *const tooltip_text) {
        if (button->implementation->tooltip_text && !strcmp(button->implementation->tooltip_text, tooltip_text)) {
                return;
        }

        if (button->implementation->tooltip_text) {
                xfree(button->implementation->tooltip_text);
        }

        button->implementation->tooltip_text = xstrdup(tooltip_text);
}

bool button_receive_event(struct Button *const button, const SDL_Event *const event) {
        if (event->type == SDL_WINDOWEVENT) {
                const Uint8 window_event = event->window.event;
                if (window_event == SDL_WINDOWEVENT_RESIZED || window_event == SDL_WINDOWEVENT_MAXIMIZED || window_event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        resize_button(button);
                }

                return false;
        }

        if (
                event->type == SDL_MOUSEBUTTONDOWN ||
                event->type == SDL_MOUSEMOTION ||
                event->type == SDL_MOUSEBUTTONUP ||
                event->type == SDL_FINGERDOWN ||
                event->type == SDL_FINGERMOTION ||
                event->type == SDL_FINGERUP
        ) {
                int drawable_width;
                int drawable_height;
                SDL_GetRendererOutputSize(get_context_renderer(), &drawable_width, &drawable_height);

                float target_x;
                float target_y;
                if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEMOTION || event->type == SDL_MOUSEBUTTONUP) {
                        int window_width;
                        int window_height;
                        SDL_GetWindowSize(get_context_window(), &window_width, &window_height);
                        target_x = event->button.x * (float)drawable_width / (float)window_width;
                        target_y = event->button.y * (float)drawable_height / (float)window_height;
                } else {
                        target_x = event->tfinger.x * (float)drawable_width;
                        target_y = event->tfinger.y * (float)drawable_height;
                }

                float x;
                float y;
                float radius;
                get_button_metrics(button, &x, &y, &radius);
                button->implementation->hovering = powf(x - target_x, 2.0f) + powf(y - target_y, 2.0f) <= powf(radius * 0.8f, 2.0f);

                bool event_consumed = false;
                enum ButtonState next_button_state = button->implementation->state;
                const enum ButtonState current_button_state = next_button_state;
                switch (event->type) {
                        case SDL_MOUSEBUTTONDOWN: case SDL_FINGERDOWN: {
                                if (button->implementation->hovering && current_button_state != BUTTON_PRESSED) {
                                        next_button_state = BUTTON_PRESSED;
                                        event_consumed = true;
                                }

                                break;
                        }

                        case SDL_MOUSEMOTION: case SDL_FINGERMOTION: {
                                if (button->implementation->hovering && current_button_state == BUTTON_IDLE) {
                                        next_button_state = BUTTON_HOVER;
                                        event_consumed = true;
                                } else if (!button->implementation->hovering && current_button_state == BUTTON_HOVER) {
                                        next_button_state = BUTTON_IDLE;
                                        event_consumed = true;
                                }

                                break;
                        }

                        case SDL_MOUSEBUTTONUP: case SDL_FINGERUP: {
                                if (!button->implementation->hovering && current_button_state == BUTTON_PRESSED) {
                                        event_consumed = true;
                                } else if (button->implementation->hovering && current_button_state == BUTTON_PRESSED) {
                                        next_button_state = BUTTON_HOVER;
                                        event_consumed = true;
                                        if (button->callback) {
                                                button->callback(button->callback_data);
                                        }

                                        play_sound(SOUND_CLICK);
                                }

                                break;
                        }
                }

                if (next_button_state != current_button_state) {
                        button->implementation->state = next_button_state;
                        restart_animation(&button->implementation->animations, (size_t)next_button_state);
                }

                return event_consumed;
        }

        return false;
}

bool update_button(struct Button *const button, const double delta_time) {
        update_animation(&button->implementation->animations, delta_time);

        clear_geometry(button->implementation->geometry);

        float x;
        float y;
        float radius;
        get_button_metrics(button, &x, &y, &radius);

        const float thickness = radius / 2.0f;
        const float line_width = radius / 5.0f;
        const float height_offset = button->implementation->animation_offset * thickness;

        const float surface_x = x;
        const float surface_y = y - height_offset;

        set_geometry_color(button->implementation->geometry, COLOR_GOLD, COLOR_OPAQUE);
        write_hexagon_thickness_geometry(button->implementation->geometry, surface_x, surface_y, radius + line_width / 2.0f, thickness, button->thickness_mask);

        set_geometry_color(button->implementation->geometry, COLOR_LIGHT_YELLOW, COLOR_OPAQUE);
        write_hexagon_geometry(button->implementation->geometry, surface_x, surface_y, radius + line_width / 2.0f, 0.0f);

        set_geometry_color(button->implementation->geometry, COLOR_YELLOW, COLOR_OPAQUE);
        write_hexagon_geometry(button->implementation->geometry, surface_x, surface_y, radius - line_width / 2.0f, 0.0f);

        render_geometry(button->implementation->geometry);

        if (button->implementation->surface_icon) {
                set_icon_position(button->implementation->surface_icon, surface_x, surface_y);
                set_icon_size(button->implementation->surface_icon, radius);
                update_icon(button->implementation->surface_icon);
        }

        if (button->implementation->surface_text) {
                button->implementation->surface_text->absolute_offset_x = surface_x;
                button->implementation->surface_text->absolute_offset_y = surface_y;
                button->implementation->surface_text->scale_x = button->scale * radius / 100.0f;
                button->implementation->surface_text->scale_y = button->scale * radius / 100.0f;
                update_text(button->implementation->surface_text);
        }

        if (button->implementation->hovering) {
                request_cursor(CURSOR_POINTER);

                if (button->implementation->tooltip_text) {
                        set_tooltip_text(button->implementation->tooltip_text);
                        request_tooltip(true);
                }
        }

        return true;
}

static void resize_button(struct Button *const button) {
        int drawable_width;
        int drawable_height;
        SDL_GetRendererOutputSize(get_context_renderer(), &drawable_width, &drawable_height);

        const float padding = CLAMP_VALUE(fmaxf((float)drawable_width, (float)drawable_height) * PADDING_FACTOR, MINIMUM_PADDING, MAXIMUM_PADDING);
        button->implementation->computed_radius = padding;

        if (button->implementation->grid_metrics) {
                button->implementation->grid_metrics->bounding_width  = (float)drawable_width  - padding * 2.0f;
                button->implementation->grid_metrics->bounding_height = (float)drawable_height - padding * 2.0f;
                button->implementation->grid_metrics->bounding_x = padding;
                button->implementation->grid_metrics->bounding_y = padding;
                button->implementation->grid_metrics->tile_radius = padding;
                populate_grid_metrics_from_radius(button->implementation->grid_metrics);
        }
}