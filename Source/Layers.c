#include "Layers.h"

#include <math.h>
#include <stdbool.h>

#include <SDL.h>

#include "Context.h"
#include "Geometry.h"
#include "Hexagons.h"

#define LAYER_GRID_COLUMNS 10ULL
#define LAYER_GRID_ROWS 10ULL

#define ROTATION_SPEED 0.01f
#define ROTATION_CYCLE (float)M_PI * 2.0f

static float grid_rotation = 0.0f;
static struct GridMetrics grid_metrics = {};
static float layers_width = 0.0f;
static float layers_height = 0.0f;

static struct Geometry *background_geometry = NULL;
static struct Geometry *transition_geometry = NULL;

#define TRANSITION_DURATION 3000.0f
static bool transitionning = false;
static void (*transition_callback)(void *) = NULL;
static void *transition_callback_data;
static float transition_time = 0.0f;
static bool transition_direction = false;

static void resize_layers(void);

void initialize_layers(void) {
        background_geometry = create_geometry();
        transition_geometry = create_geometry();
        grid_metrics.columns = LAYER_GRID_COLUMNS;
        grid_metrics.rows = LAYER_GRID_ROWS;
        grid_rotation = RANDOM_NUMBER(0.0f, ROTATION_CYCLE);

        set_geometry_color(transition_geometry, COLOR_YELLOW, 255);
        resize_layers();
}

void terminate_layers(void) {
        destroy_geometry(background_geometry);
        background_geometry = NULL;

        destroy_geometry(transition_geometry);
        transition_geometry = NULL;
}

void layers_receive_event(const SDL_Event *const event) {
        if (event->type == SDL_WINDOWEVENT) {
                const Uint8 window_event = event->window.event;
                if (window_event == SDL_WINDOWEVENT_RESIZED || window_event == SDL_WINDOWEVENT_MAXIMIZED || window_event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        resize_layers();
                }
        }
}

static inline float transition_easing(const float t) {
        const float u = 1.0f - t; // Cubic Bezier: [0, 0.5, 1, 0.5]
        return (3.0f * u * u * t * 0.5f) + (3.0f * u * t * t * 0.5f) + (t * t * t);
}

void update_layers(const double delta_time) {
        grid_rotation += ROTATION_SPEED * (float)delta_time / 1000.0f;
        while (grid_rotation >= ROTATION_CYCLE) {
                grid_rotation -= ROTATION_CYCLE;
        }

        if (transitionning) {
                const float previous_transition_time = transition_time;
                transition_time += delta_time / TRANSITION_DURATION;

                if (previous_transition_time < 0.5f && transition_time >= 0.5f) {
                        if (transition_callback) {
                                transition_callback(transition_callback_data);
                        }

                        transition_direction = !transition_direction;
                }

                if (transition_time >= 1.0f) {
                        transition_time = 0.0f;
                        transitionning = false;
                        return;
                }
        }

        clear_geometry(background_geometry);
        clear_geometry(transition_geometry);

        set_geometry_color(background_geometry, COLOR_DARK_BROWN, 255);
        write_rectangle_geometry(background_geometry, layers_width / 2.0f, layers_height / 2.0f, layers_width, layers_height, 0.0f);

        const float rotation_pivot_x = grid_metrics.grid_x + grid_metrics.grid_width  / 2.0f;
        const float rotation_pivot_y = grid_metrics.grid_y + grid_metrics.grid_height / 2.0f;

        set_geometry_color(background_geometry, COLOR_BROWN, 255);
        const float time = transition_easing(1.0f - fabsf(2.0f * transition_time - 1.0f)) * 2.0f;
        for (size_t row = 0ULL; row < LAYER_GRID_ROWS; ++row) {
                const size_t row_number = transition_direction ? row + 1ULL : (LAYER_GRID_ROWS - (row + 1ULL));
                const float row_time = fminf(fmaxf(time - (float)row_number / (float)LAYER_GRID_ROWS, 0.0f), 1.0f);

                for (size_t column = 0ULL; column < LAYER_GRID_COLUMNS; ++column) {
                        float x;
                        float y;
                        get_grid_tile_position(&grid_metrics, column, row, &x, &y);
                        rotate_point(&x, &y, rotation_pivot_x, rotation_pivot_y, grid_rotation);

                        write_hexagon_geometry(background_geometry, x, y, grid_metrics.tile_radius * 0.9f, grid_rotation);
                        if (row_time != 0.0f) {
                                write_hexagon_geometry(transition_geometry, x, y, grid_metrics.tile_radius * row_time * 2.0f, grid_rotation);
                        }
                }
        }
}

void render_background_layer(void) {
        render_geometry(background_geometry);
}

void render_transition_layer(void) {
        render_geometry(transition_geometry);
}

bool is_transition_triggered(void) {
        return transitionning;
}

bool trigger_transition_layer(void (*const callback)(void *), void *const callback_data) {
        if (transitionning) {
                send_message(ERROR, "Failed to trigger transition: The current transition is still in use");
                return false;
        }

        transition_callback = callback;
        transition_callback_data = callback_data;
        transition_direction = (bool)RANDOM_INTEGER(0ULL, 1ULL);
        transitionning = true;
        return true;
}

static void resize_layers(void) {
        int drawable_width;
        int drawable_height;
        SDL_GetRendererOutputSize(get_context_renderer(), &drawable_width, &drawable_height);

        layers_width = (float)drawable_width;
        layers_height = (float)drawable_height;

        const float side_length = drawable_width + drawable_height;
        grid_metrics.bounding_x = (drawable_width - side_length) / 2.0f;
        grid_metrics.bounding_y = (drawable_height - side_length) / 2.0f;
        grid_metrics.bounding_width  = side_length;
        grid_metrics.bounding_height = side_length;

        populate_grid_metrics_from_size(&grid_metrics);
}
