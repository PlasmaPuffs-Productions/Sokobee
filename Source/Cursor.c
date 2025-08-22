#include "Cursor.h"

#include <stdbool.h>

#include "Utilities.h"

#if !PLATFORM_HAS_MOUSE

bool initialize_cursor(void) {
        return true;
}

void terminate_cursor(void) {
        return;
}

void request_cursor(const enum CursorType type) {
        return;
}

void request_tooltip(const bool active) {
        return;
}

void set_tooltip_text(char *const text) {
        return;
}

void update_cursor(const double delta_time) {
        return;
}

#else

#include <string.h>

#include <SDL.h>

#include "Text.h"
#include "Geometry.h"
#include "Animation.h"
#include "Context.h"

#define TOOLTIP_PADDING 5.0f
#define TOOLTIP_CURSOR_OFFSET 10.0f

static enum CursorType current_cursor;
static enum CursorType requested_cursor;

static SDL_Cursor *cursors[CURSOR_COUNT];
static const SDL_SystemCursor cursor_flags[CURSOR_COUNT] = {
        [CURSOR_ARROW] = SDL_SYSTEM_CURSOR_ARROW,
        [CURSOR_POINTER] = SDL_SYSTEM_CURSOR_HAND
};

static bool tooltip_currently_active = false;
static bool tooltip_requested_active = false;

static char *tooltip_string = NULL;
static struct Text tooltip_text;
static struct Geometry *tooltip_geometry;
static float current_tooltip_alpha = 0.0f;
static float animated_tooltip_alpha = 0.0f;
static struct Animation tooltip_fade;

bool initialize_cursor(void) {
        current_cursor = CURSOR_ARROW;
        requested_cursor = CURSOR_ARROW;
        for (size_t cursor_index = 0ULL; cursor_index < CURSOR_COUNT; ++cursor_index) {
                cursors[cursor_index] = SDL_CreateSystemCursor(cursor_flags[cursor_index]);
        }

        SDL_SetCursor(cursors[current_cursor]);

        tooltip_geometry = create_geometry(0ULL, 0ULL);

        if (!initialize_text(&tooltip_text, "[tooltip]", FONT_CAPTION, COLOR_TRANSPARENT)) {
                send_message(ERROR, "Failed to initialize cursor: Failed to initialize tooltip text");
                terminate_cursor();
                return false;
        }

        initialize_animation(&tooltip_fade, 2ULL);

        struct Action *const fade_in = &tooltip_fade.actions[0];
        fade_in->type = ACTION_FLOAT;
        fade_in->target.float_pointer = &animated_tooltip_alpha;
        fade_in->keyframes.floats[1] = 1.0f;
        fade_in->easing = QUAD_OUT;
        fade_in->duration = 250.0f;
        fade_in->lazy_start = true;
        fade_in->pause = true;

        struct Action *const fade_out = &tooltip_fade.actions[1];
        fade_out->type = ACTION_FLOAT;
        fade_out->target.float_pointer = &animated_tooltip_alpha;
        fade_out->keyframes.floats[1] = 0.0f;
        fade_out->easing = QUAD_IN;
        fade_out->duration = 100.0f;
        fade_out->lazy_start = true;
        fade_out->pause = true;

        return true;
}

void terminate_cursor(void) {
        deinitialize_animation(&tooltip_fade);
        deinitialize_text(&tooltip_text);
        destroy_geometry(tooltip_geometry);

        if (tooltip_string) {
                free(tooltip_string);
                tooltip_string = NULL;
        }

        for (size_t cursor_index = 0ULL; cursor_index < CURSOR_COUNT; ++cursor_index) {
                SDL_FreeCursor(cursors[cursor_index]);
                cursors[cursor_index] = NULL;
        }
}

void request_cursor(const enum CursorType type) {
        requested_cursor = type;
}

void request_tooltip(const bool active) {
        tooltip_requested_active = active;
}

void set_tooltip_text(char *const text) {
        if (tooltip_string && !strcmp(text, tooltip_string)) {
                return;
        }

        if (tooltip_string) {
                free(tooltip_string);
        }

        tooltip_string = strdup(text);
        set_text_string(&tooltip_text, tooltip_string);
}

void update_cursor(const double delta_time) {
        update_animation(&tooltip_fade, delta_time);

        if (current_cursor != requested_cursor) {
                current_cursor = requested_cursor;
                SDL_SetCursor(cursors[current_cursor]);
        }

        if (tooltip_requested_active && !tooltip_currently_active) {
                tooltip_currently_active = tooltip_requested_active;
                restart_animation(&tooltip_fade, 0ULL);
        }

        if (!tooltip_requested_active && tooltip_currently_active) {
                tooltip_currently_active = tooltip_requested_active;
                restart_animation(&tooltip_fade, 1ULL);
        }

        if (current_tooltip_alpha != animated_tooltip_alpha) {
                current_tooltip_alpha = animated_tooltip_alpha;

                SDL_Color text_color = COLOR_YELLOW;
                text_color.a = (int)lroundf(current_tooltip_alpha * 255.0f);

                set_text_color(&tooltip_text, text_color);
        }

        int mouse_x;
        int mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);

        int window_width;
        int window_height;
        SDL_GetWindowSize(get_context_window(), &window_width, &window_height);

        int drawable_width;
        int drawable_height;
        SDL_GetRendererOutputSize(get_context_renderer(), &drawable_width, &drawable_height);

        const float cursor_x = (float)mouse_x * (float)drawable_width / (float)window_width;
        const float cursor_y = (float)mouse_y * (float)drawable_height / (float)window_height;

        const float padding = TOOLTIP_PADDING * drawable_height / window_height;
        const float width   = get_text_width(&tooltip_text)  + padding * 2.0f;
        const float height  = get_text_height(&tooltip_text) + padding * 2.0f;

        float tooltip_center_x = cursor_x + TOOLTIP_CURSOR_OFFSET + width * 0.5f;
        float tooltip_center_y = cursor_y + TOOLTIP_CURSOR_OFFSET + height * 0.5f;

        if (tooltip_center_x + width * 0.5f > drawable_width) {
                tooltip_center_x = cursor_x - TOOLTIP_CURSOR_OFFSET - width * 0.5f;
        }

        if (tooltip_center_y + height * 0.5f > drawable_height) {
                tooltip_center_y = cursor_y - TOOLTIP_CURSOR_OFFSET - height * 0.5f;
        }

        if (tooltip_center_x - width * 0.5f < 0.0f) {
                tooltip_center_x = width * 0.5f;
        }

        if (tooltip_center_y - height * 0.5f < 0.0f) {
                tooltip_center_y = height * 0.5f;
        }

        const SDL_FRect tooltip_rectangle = {
                .x = tooltip_center_x,
                .y = tooltip_center_y,
                .w = width,
                .h = height
        };

        clear_geometry(tooltip_geometry);
        set_geometry_color(tooltip_geometry, (SDL_Color){0, 0, 0, (int)lroundf(current_tooltip_alpha * 255.0f * 0.75f)});
        write_rounded_rectangle_geometry(tooltip_geometry, tooltip_rectangle, padding / 4.0f, 8ULL);
        render_geometry(tooltip_geometry);

        tooltip_text.absolute_offset_x = tooltip_center_x - width  / 2.0f + padding;
        tooltip_text.absolute_offset_y = tooltip_center_y - height / 2.0f + padding;
        update_text(&tooltip_text);
}

#endif