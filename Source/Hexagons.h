#pragma once

#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "Utilities.h"
#include "Geometry.h"

#include <SDL.h>

// ================================================================================================
// Hexagon Orientation
// ================================================================================================

#define ORIENTATION_MAXIMUM LOWER_RIGHT

enum Orientation {
        UPPER_RIGHT,
        UPPER_MIDDLE,
        UPPER_LEFT,
        LOWER_LEFT,
        LOWER_MIDDLE,
        LOWER_RIGHT
};

static inline float orientation_angle(const enum Orientation orientation) {
        switch (orientation) {
                case UPPER_RIGHT:  return (float)M_PI * 1.0f  / 6.0f;
                case UPPER_MIDDLE: return (float)M_PI * 3.0f  / 6.0f;
                case UPPER_LEFT:   return (float)M_PI * 5.0f  / 6.0f;
                case LOWER_LEFT:   return (float)M_PI * 7.0f  / 6.0f;
                case LOWER_MIDDLE: return (float)M_PI * 9.0f  / 6.0f;
                case LOWER_RIGHT:  return (float)M_PI * 11.0f / 6.0f;
        }
}

static inline enum Orientation orientation_turn_left(const enum Orientation orientation) {
        switch (orientation) {
                case UPPER_RIGHT:  return UPPER_MIDDLE;
                case UPPER_MIDDLE: return UPPER_LEFT;
                case UPPER_LEFT:   return LOWER_LEFT;
                case LOWER_LEFT:   return LOWER_MIDDLE;
                case LOWER_MIDDLE: return LOWER_RIGHT;
                case LOWER_RIGHT:  return UPPER_RIGHT;
        }
}

static inline enum Orientation orientation_turn_right(const enum Orientation orientation) {
        switch (orientation) {
                case UPPER_RIGHT:  return LOWER_RIGHT;
                case UPPER_MIDDLE: return UPPER_RIGHT;
                case UPPER_LEFT:   return UPPER_MIDDLE;
                case LOWER_LEFT:   return UPPER_LEFT;
                case LOWER_MIDDLE: return LOWER_LEFT;
                case LOWER_RIGHT:  return LOWER_MIDDLE;
        }
}

static inline enum Orientation orientation_reverse(const enum Orientation orientation) {
        switch (orientation) {
                case UPPER_RIGHT:  return LOWER_LEFT;
                case UPPER_MIDDLE: return LOWER_MIDDLE;
                case UPPER_LEFT:   return LOWER_RIGHT;
                case LOWER_LEFT:   return UPPER_RIGHT;
                case LOWER_MIDDLE: return UPPER_MIDDLE;
                case LOWER_RIGHT:  return UPPER_LEFT;
        }
}

static inline bool orientation_advance_index(const enum Orientation orientation, const uint8_t columns, const uint8_t rows, uint16_t *const out_index) {
        int8_t column = (int8_t)(*out_index % (uint16_t)columns);
        int8_t row    = (int8_t)(*out_index / (uint16_t)columns);

        int8_t next_column = column;
        int8_t next_row    = row;

        switch (orientation) {
                case UPPER_RIGHT: {
                        ++next_column;

                        if (!(column & 1)) {
                                --next_row;
                        }

                        break;
                }

                case UPPER_MIDDLE: {
                        --next_row;
                        break;
                }

                case UPPER_LEFT: {
                        --next_column;

                        if (!(column & 1)) {
                                --next_row;
                        }

                        break;
                }

                case LOWER_LEFT: {
                        --next_column;

                        if (column & 1) {
                                ++next_row;
                        }

                        break;
                }

                case LOWER_MIDDLE: {
                        ++next_row;
                        break;
                }

                case LOWER_RIGHT: {
                        ++next_column;

                        if (column & 1) {
                                ++next_row;
                        }

                        break;
                }
        }

        if (next_column < 0 || next_row < 0 || next_column >= columns || next_row >= rows) {
                return false;
        }

        *out_index = (uint16_t)(next_row * (int8_t)columns + next_column);
        return true;
}

// ================================================================================================
// Hexagon Grid Metrics
// ================================================================================================

struct GridMetrics {
        size_t columns;
        size_t rows;
        size_t tile_count;
        float tile_radius;
        float bounding_width;
        float bounding_height;
        float bounding_x;
        float bounding_y;
        float grid_width;
        float grid_height;
        float grid_x;
        float grid_y;
        float tile_distance_x;
        float tile_distance_y;
        float first_tile_x;
        float first_tile_y;
};

enum GridAxis {
        GRID_AXIS_HORIZONTAL,
        GRID_AXIS_VERTICAL
};

static inline void get_grid_tile_position(const struct GridMetrics *const grid_metrics, const size_t column, const size_t row, float *const x, float *const y) {
        if (x != NULL) {
                *x = grid_metrics->grid_x + grid_metrics->tile_radius + column * grid_metrics->tile_distance_x;
        }

        if (y != NULL) {
                *y = grid_metrics->grid_y + grid_metrics->tile_distance_y / 2.0f + row * grid_metrics->tile_distance_y + ((column & 1ULL) ? grid_metrics->tile_distance_y / 2.0f : 0.0f);
        }
}

enum HexagonNeighbor {
        HEXAGON_NEIGHBOR_TOP = 0,
        HEXAGON_NEIGHBOR_BOTTOM,
        HEXAGON_NEIGHBOR_TOP_LEFT,
        HEXAGON_NEIGHBOR_TOP_RIGHT,
        HEXAGON_NEIGHBOR_BOTTOM_LEFT,
        HEXAGON_NEIGHBOR_BOTTOM_RIGHT,
        HEXAGON_NEIGHBOR_COUNT
};

struct HexagonNeighborOffset {
        int8_t column;
        int8_t row;
};

static const struct HexagonNeighborOffset even_hexagon_neighbor_offsets[HEXAGON_NEIGHBOR_COUNT] = {
        (struct HexagonNeighborOffset){.column =  0, .row = -1}, // TOP
        (struct HexagonNeighborOffset){.column =  0, .row = +1}, // BOTTOM
        (struct HexagonNeighborOffset){.column = -1, .row = -1}, // TOP_LEFT
        (struct HexagonNeighborOffset){.column = +1, .row = -1}, // TOP_RIGHT
        (struct HexagonNeighborOffset){.column = -1, .row =  0}, // BOTTOM_LEFT
        (struct HexagonNeighborOffset){.column = +1, .row =  0}, // BOTTOM_RIGHT
};

static const struct HexagonNeighborOffset odd_hexagon_neighbor_offsets[HEXAGON_NEIGHBOR_COUNT] = {
        (struct HexagonNeighborOffset){.column =  0, .row = -1}, // TOP
        (struct HexagonNeighborOffset){.column =  0, .row = +1}, // BOTTOM
        (struct HexagonNeighborOffset){.column = -1, .row =  0}, // TOP_LEFT
        (struct HexagonNeighborOffset){.column = +1, .row =  0}, // TOP_RIGHT
        (struct HexagonNeighborOffset){.column = -1, .row = +1}, // BOTTOM_LEFT
        (struct HexagonNeighborOffset){.column = +1, .row = +1}, // BOTTOM_RIGHT
};

static inline bool get_hexagon_neighbor(const struct GridMetrics *const grid_metrics, const size_t column, const size_t row, const enum HexagonNeighbor neighbor, size_t *const out_column, size_t *const out_row) {
        const struct HexagonNeighborOffset *const offsets = (column & 1ULL) ? odd_hexagon_neighbor_offsets : even_hexagon_neighbor_offsets;
        const size_t neighbor_row    = row    + (size_t)offsets[neighbor].row;
        const size_t neighbor_column = column + (size_t)offsets[neighbor].column;

        if (neighbor_row < 0ULL || neighbor_column < 0ULL || neighbor_row >= grid_metrics->rows || neighbor_column >= grid_metrics->columns) {
                return false;
        }

        if (out_row != NULL) {
                *out_row = neighbor_row;
        }

        if (out_column != NULL) {
                *out_column = neighbor_column;
        }

        return true;
}

static inline void populate_grid_metrics_from_radius(struct GridMetrics *const grid_metrics) {
        grid_metrics->tile_distance_x = grid_metrics->tile_radius * 1.5f;
        grid_metrics->tile_distance_y = grid_metrics->tile_radius * sqrtf(3.0f);

        grid_metrics->columns = (size_t)((grid_metrics->bounding_width - grid_metrics->tile_radius * 0.5f) / grid_metrics->tile_distance_x);
        grid_metrics->rows    = (size_t)((grid_metrics->bounding_height - 0.0f) / grid_metrics->tile_distance_y);

        if (grid_metrics->columns == 0ULL) {
                grid_metrics->columns = 1ULL;
        }

        if (grid_metrics->rows == 0ULL) {
                grid_metrics->rows = 1ULL;
        }

        grid_metrics->tile_count = grid_metrics->columns * grid_metrics->rows;

        grid_metrics->grid_width  = grid_metrics->tile_distance_x * (grid_metrics->columns - 1ULL) + grid_metrics->tile_radius * 2.0f;
        grid_metrics->grid_height = grid_metrics->tile_distance_y *  grid_metrics->rows;

        grid_metrics->grid_x = grid_metrics->bounding_x + (grid_metrics->bounding_width  - grid_metrics->grid_width)  / 2.0f;
        grid_metrics->grid_y = grid_metrics->bounding_y + (grid_metrics->bounding_height - grid_metrics->grid_height) / 2.0f;
}

static inline void populate_grid_metrics_from_size(struct GridMetrics *const grid_metrics) {
        const float maximum_tile_radius_from_width  = grid_metrics->bounding_width  / (1.5f * grid_metrics->columns + 0.5f);
        const float maximum_tile_radius_from_height = grid_metrics->bounding_height / (sqrtf(3.0f) * (grid_metrics->rows + 0.5f));

        grid_metrics->tile_radius = fminf(maximum_tile_radius_from_width, maximum_tile_radius_from_height);
        grid_metrics->tile_count = grid_metrics->columns * grid_metrics->rows;
        grid_metrics->tile_distance_x = grid_metrics->tile_radius * 1.5f;
        grid_metrics->tile_distance_y = grid_metrics->tile_radius * sqrt(3.0f);
        grid_metrics->grid_width  = grid_metrics->tile_radius * 2.0f + grid_metrics->tile_distance_x * (grid_metrics->columns - 1ULL);
        grid_metrics->grid_height = grid_metrics->tile_distance_y * grid_metrics->rows + ((grid_metrics->columns > 1ULL) ? grid_metrics->tile_distance_y / 2.0f : 0.0f);
        grid_metrics->grid_x = grid_metrics->bounding_x + (grid_metrics->bounding_width  - grid_metrics->grid_width)  / 2.0f;
        grid_metrics->grid_y = grid_metrics->bounding_y + (grid_metrics->bounding_height - grid_metrics->grid_height) / 2.0f;
}

static inline void populate_scrolling_grid_metrics(struct GridMetrics *const grid_metrics, const enum GridAxis grid_scroll_axis) {
        grid_metrics->tile_distance_x = grid_metrics->tile_radius * 1.5f;
        grid_metrics->tile_distance_y = grid_metrics->tile_radius * sqrtf(3.0f);

        if (grid_scroll_axis == GRID_AXIS_VERTICAL) {
                grid_metrics->columns = (size_t)((grid_metrics->bounding_width - grid_metrics->tile_radius * 0.5f) / grid_metrics->tile_distance_x);
                if (grid_metrics->columns == 0) {
                        grid_metrics->columns = 1ULL;
                }

                grid_metrics->rows = (grid_metrics->tile_count + grid_metrics->columns - 1ULL) / grid_metrics->columns;
                grid_metrics->grid_width  = grid_metrics->tile_distance_x * (grid_metrics->columns - 1ULL) + grid_metrics->tile_radius * 2.0f;
                grid_metrics->grid_height = grid_metrics->tile_distance_y *  grid_metrics->rows;
                grid_metrics->bounding_height = grid_metrics->grid_height;
        }

        if (grid_scroll_axis == GRID_AXIS_HORIZONTAL) {
                grid_metrics->rows = (size_t)((grid_metrics->bounding_height - 0.0f) / grid_metrics->tile_distance_y);
                if (grid_metrics->rows == 0) {
                        grid_metrics->rows = 1ULL;
                }

                grid_metrics->columns = (grid_metrics->tile_count + grid_metrics->rows - 1ULL) / grid_metrics->rows;
                grid_metrics->grid_width  = grid_metrics->tile_distance_x * (grid_metrics->columns - 1ULL) + grid_metrics->tile_radius * 2.0f;
                grid_metrics->grid_height = grid_metrics->tile_distance_y *  grid_metrics->rows;
                grid_metrics->bounding_width = grid_metrics->grid_width;
        }

        grid_metrics->grid_x = grid_metrics->bounding_x + (grid_metrics->bounding_width  - grid_metrics->grid_width)  / 2.0f;
        grid_metrics->grid_y = grid_metrics->bounding_y + (grid_metrics->bounding_height - grid_metrics->grid_height) / 2.0f;
}

// ================================================================================================
// Extruded Hexagon Thickness
// ================================================================================================

enum HexagonThicknessMask {
        HEXAGON_THICKNESS_MASK_NONE   = 0,
        HEXAGON_THICKNESS_MASK_LEFT   = 1 << 0,
        HEXAGON_THICKNESS_MASK_BOTTOM = 1 << 1,
        HEXAGON_THICKNESS_MASK_RIGHT  = 1 << 2,
        HEXAGON_THICKNESS_MASK_ALL    = HEXAGON_THICKNESS_MASK_LEFT | HEXAGON_THICKNESS_MASK_BOTTOM | HEXAGON_THICKNESS_MASK_RIGHT
};

static inline void write_hexagon_thickness_geometry(
        struct Geometry *const geometry,
        const float x, const float y,
        const float radius,
        const float thickness,
        const enum HexagonThicknessMask thickness_mask
) {
        if (thickness_mask == HEXAGON_THICKNESS_MASK_NONE) {
                return;
        }

        const float ax1 = x - radius,        ay1 = y;
        const float ax2 = x - radius / 2.0f, ay2 = y + radius * sqrtf(3.0f) / 2.0f;
        const float ax3 = x + radius / 2.0f, ay3 = y + radius * sqrtf(3.0f) / 2.0f;
        const float ax4 = x + radius,        ay4 = y;

#define THICKER thickness +

        if ((thickness_mask & HEXAGON_THICKNESS_MASK_LEFT) == HEXAGON_THICKNESS_MASK_LEFT) {
                write_quadrilateral_geometry(geometry, ax1, ay1, ax2, ay2, ax2, THICKER ay2, ax1, THICKER ay1);
        }

        if ((thickness_mask & HEXAGON_THICKNESS_MASK_BOTTOM) == HEXAGON_THICKNESS_MASK_BOTTOM) {
                write_quadrilateral_geometry(geometry, ax2, ay2, ax3, ay3, ax3, THICKER ay3, ax2, THICKER ay2);
        }

        if ((thickness_mask & HEXAGON_THICKNESS_MASK_RIGHT) == HEXAGON_THICKNESS_MASK_RIGHT) {
                write_quadrilateral_geometry(geometry, ax3, ay3, ax4, ay4, ax4, THICKER ay4, ax3, THICKER ay3);
        }

#undef THICKER

}