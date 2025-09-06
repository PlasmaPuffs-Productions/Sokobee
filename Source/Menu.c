#include "Scenes.h"

#include <math.h>
#include <stdio.h>

#include <SDL.h>

#include "Text.h"
#include "Layers.h"
#include "Button.h"
#include "Context.h"
#include "Geometry.h"
#include "Utilities.h"
#include "Hexagons.h"
#include "Icons.h"

static bool initialize_main_menu_scene(void);
static bool main_menu_scene_receive_event(const SDL_Event *const event);
static void update_main_menu_scene(const double delta_time);
static void terminate_main_menu_scene(void);

static const struct SceneAPI main_menu_scene_API = (struct SceneAPI){
        .initialize = initialize_main_menu_scene,
        .present = NULL,
        .receive_event = main_menu_scene_receive_event,
        .update = update_main_menu_scene,
        .dismiss = NULL,
        .terminate = terminate_main_menu_scene
};

const struct SceneAPI *get_main_menu_scene_API(void) {
        return &main_menu_scene_API;
}

#define LEVEL_BUTTON_SCALE 1.5f

static struct GridMetrics levels_grid_metrics = {};
static struct Button *buttons = NULL;

static void start_level(void *const data) {
        scene_manager_present_scene(SCENE_PLAYING);
}

static void level_button_callback(void *const data) {
        current_level_number = (size_t)(uintptr_t)data;
        trigger_transition_layer(start_level, NULL);
}

static void resize_main_menu_scene(void);

static bool initialize_main_menu_scene(void) {
        const size_t level_count = get_level_count();
        buttons = (struct Button *)xmalloc(level_count * sizeof(struct Button));

        char string[16];
        for (size_t level_index = 0ULL; level_index < level_count; ++level_index) {
                struct Button *const button = &buttons[level_index];
                initialize_button(button, false);

                const size_t level_number = level_index + 1ULL;

                button->callback = level_button_callback;
                button->callback_data = (void *)(uintptr_t)level_number;
                button->scale = LEVEL_BUTTON_SCALE;

                snprintf(string, sizeof(string), "%zu", level_number);
                if (!set_button_surface_text(button, string)) {
                        send_message(MESSAGE_ERROR, "Failed to initialize main menu screen: Failed to set button (index %zu) surface text", level_index);
                        terminate_main_menu_scene();
                        return false;
                }

                string[0] = '\0';
                snprintf(string, sizeof(string), "Play level %zu", level_number);
                set_button_tooltip_text(button, string);
        }

        resize_main_menu_scene();
        return true;
}

static bool main_menu_scene_receive_event(const SDL_Event *const event) {
        if (event->type == SDL_WINDOWEVENT) {
                const Uint8 window_event = event->window.event;
                if (window_event == SDL_WINDOWEVENT_RESIZED || window_event == SDL_WINDOWEVENT_MAXIMIZED || window_event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        resize_main_menu_scene();
                }

                return false;
        }

        const size_t level_count = get_level_count();
        for (size_t level_index = 0ULL; level_index < level_count; ++level_index) {
                if (button_receive_event(&buttons[level_index], event)) {
                        return true;
                }
        }

        return false;
}

static void update_main_menu_scene(const double delta_time) {
        const size_t level_count = get_level_count();
        for (size_t level_index = 0ULL; level_index < level_count; ++level_index) {
                update_button(&buttons[level_index], delta_time);
        }
}

static void terminate_main_menu_scene(void) {
        const size_t level_count = get_level_count();

        if (buttons) {
                for (size_t level_index = 0ULL; level_index < level_count; ++level_index) {
                        deinitialize_button(&buttons[level_index]);
                }

                xfree(buttons);
                buttons = NULL;
        }
}

static void resize_main_menu_scene(void) {
        int drawable_width;
        int drawable_height;
        SDL_GetRendererOutputSize(get_context_renderer(), &drawable_width, &drawable_height);

        const float grid_padding = fminf((float)drawable_width, (float)drawable_height) / 10.0f;

        levels_grid_metrics.bounding_x = grid_padding;
        levels_grid_metrics.bounding_y = grid_padding;
        levels_grid_metrics.bounding_width = (float)drawable_width - grid_padding * 2.0f;

        levels_grid_metrics.tile_count = get_level_count();
        get_button_metrics(&buttons[0], NULL, NULL, &levels_grid_metrics.tile_radius);
        populate_scrolling_grid_metrics(&levels_grid_metrics, GRID_AXIS_VERTICAL);

        const size_t level_count = get_level_count();
        for (size_t row = 0ULL; row < levels_grid_metrics.rows; ++row) {
                for (size_t column = 0ULL; column < levels_grid_metrics.columns; ++column) {
                        const size_t index = row * levels_grid_metrics.columns + column;
                        if (index >= level_count) {
                                continue;
                        }

                        struct Button *const button = &buttons[index];
                        get_grid_tile_position(&levels_grid_metrics, column, row, &button->absolute_offset_x, &button->absolute_offset_y);

                        button->thickness_mask = HEXAGON_THICKNESS_MASK_ALL;
                        size_t neighbor_column;
                        size_t neighbor_row;

                        if (get_hexagon_neighbor(&levels_grid_metrics, column, row, HEXAGON_NEIGHBOR_BOTTOM, &neighbor_column, &neighbor_row)) {
                                if (neighbor_row * levels_grid_metrics.columns + neighbor_column < level_count) {
                                        button->thickness_mask &= ~HEXAGON_THICKNESS_MASK_BOTTOM;
                                }
                        }

                        if (get_hexagon_neighbor(&levels_grid_metrics, column, row, HEXAGON_NEIGHBOR_BOTTOM_LEFT, &neighbor_column, &neighbor_row)) {
                                if (neighbor_row * levels_grid_metrics.columns + neighbor_column < level_count) {
                                        button->thickness_mask &= ~HEXAGON_THICKNESS_MASK_LEFT;
                                }
                        }

                        if (get_hexagon_neighbor(&levels_grid_metrics, column, row, HEXAGON_NEIGHBOR_BOTTOM_RIGHT, &neighbor_column, &neighbor_row)) {
                                if (neighbor_row * levels_grid_metrics.columns + neighbor_column < level_count) {
                                        button->thickness_mask &= ~HEXAGON_THICKNESS_MASK_RIGHT;
                                }
                        }
                }
        }
}