#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "SDL.h"

void track_geometry_data(void);

void get_tracked_geometry_data(size_t *const out_vertex_count, size_t *const out_index_count);

struct Geometry;

struct Geometry *create_geometry(void);

void destroy_geometry(struct Geometry *const geometry);

void clear_geometry(struct Geometry *const geometry);

void set_geometry_color(
        struct Geometry *const geometry,
        const uint16_t r,
        const uint16_t g,
        const uint16_t b,
        const uint16_t a
);

void render_geometry(const struct Geometry *const geometry);

enum LineCap {
        LINE_CAP_NONE  = 0,
        LINE_CAP_START = 1 << 0,
        LINE_CAP_END   = 1 << 1,
        LINE_CAP_BOTH  = LINE_CAP_START | LINE_CAP_END
};

void write_triangle_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float x3, const float y3
);

void write_line_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float line_width,
        const enum LineCap rounded_caps
);

void write_rectangle_geometry(
        struct Geometry *const geometry,
        const float x, const float y,
        const float w, const float h,
        const float rotation
);

void write_quadrilateral_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float x3, const float y3,
        const float x4, const float y4
);

void write_circle_geometry(
        struct Geometry *const geometry,
        const float x, const float y,
        const float radius
);

void write_ellipse_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float rx, const float ry,
        const float rotation
);

void write_circular_arc_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float radius,
        const float start_angle,
        const float end_angle,
        const bool clockwise
);

void write_elliptical_arc_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float rx, const float ry,
        const float rotation,
        const float start_angle,
        const float end_angle,
        const bool clockwise
);

void write_circle_outline_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float radius,
        const float line_width
);

void write_ellipse_outline_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float rx, const float ry,
        const float line_width
);

void write_circular_arc_outline_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float radius,
        const float line_width,
        const float start_angle,
        const float end_angle,
        const bool clockwise,
        const enum LineCap rounded_caps
);

void write_elliptical_arc_outline_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float rx, const float ry,
        const float rotation,
        const float line_width,
        const float start_angle,
        const float end_angle,
        const bool clockwise,
        const enum LineCap rounded_caps
);

void write_hexagon_geometry(
        struct Geometry *const geometry,
        const float x, const float y,
        const float radius,
        const float rotation
);

void write_bezier_curve_geometry(
        struct Geometry *const geometry,
        const float px1,    const float py1, // Endpoint A
        const float px2,    const float py2, // Endpoint B
        const float cx1,    const float cy1, // Control Point A
        const float cx2,    const float cy2, // Control Point B
        const float line_width
);

void write_rounded_triangle_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float x3, const float y3,
        const float rounded_radius
);

void write_rounded_rectangle_geometry(
        struct Geometry *const geometry,
        const float x, const float y,
        const float w, const float h,
        const float rounded_radius,
        const float rotation
);

void write_rounded_quadrilateral_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float x3, const float y3,
        const float x4, const float y4,
        const float rounded_radius
);