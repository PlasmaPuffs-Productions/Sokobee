#include "Geometry.h"

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include "SDL.h"

#include "Utilities.h"
#include "Context.h"

#define INITIAL_VERTEX_CAPACITY 64ULL
#define INITIAL_INDEX_CAPACITY  64ULL

#define INDEX_STRIDE           (sizeof(uint16_t))
#define VERTEX_POSITION_STRIDE (sizeof(float) * 2ULL)
#define VERTEX_COLOR_STRIDE    (sizeof(uint8_t) * 4ULL)

#define SEGMENT_LENGTH 4.0f

static size_t tracked_vertex_count = 0ULL;
static size_t tracked_index_count = 0ULL;

void track_geometry_data(void) {
        tracked_vertex_count = 0ULL;
        tracked_index_count = 0ULL;
}

void get_tracked_geometry_data(size_t *const out_vertex_count, size_t *const out_index_count) {
        if (out_vertex_count != NULL) {
                *out_vertex_count = tracked_vertex_count;
        }

        if (out_index_count != NULL) {
                *out_index_count = tracked_index_count;
        }
}

struct Geometry {
        float *positions;
        uint8_t *colors;
        size_t vertex_count;
        size_t vertex_capacity;
        uint16_t *indices;
        size_t index_count;
        size_t index_capacity;
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
};

struct Geometry *create_geometry(void) {
        struct Geometry *const geometry = (struct Geometry *)xmalloc(sizeof(struct Geometry));
        geometry->vertex_capacity       = INITIAL_VERTEX_CAPACITY;
        geometry->index_capacity        = INITIAL_INDEX_CAPACITY;
        geometry->positions             = (float *)xmalloc(VERTEX_POSITION_STRIDE * geometry->vertex_capacity);
        geometry->colors                = (uint8_t *)xmalloc(VERTEX_COLOR_STRIDE * geometry->vertex_capacity);
        geometry->indices               = (uint16_t *)xmalloc(INDEX_STRIDE * geometry->index_capacity);
        geometry->vertex_count          = 0ULL;
        geometry->index_count           = 0ULL;
        geometry->r                     = 255;
        geometry->g                     = 255;
        geometry->b                     = 255;
        geometry->a                     = 255;
        return geometry;
}

void destroy_geometry(struct Geometry *const geometry) {
        if (geometry == NULL) {
                send_message(MESSAGE_WARNING, "Geometry given to destroy is NULL");
                return;
        }

        xfree(geometry->positions);
        xfree(geometry->colors);
        xfree(geometry->indices);
        xfree(geometry);
}

void clear_geometry(struct Geometry *const geometry) {
        geometry->index_count = 0ULL;
        geometry->vertex_count = 0ULL;
}

void set_geometry_color(struct Geometry *const geometry, const uint16_t r, const uint16_t g, const uint16_t b, const uint16_t a) {
        geometry->r = r;
        geometry->g = g;
        geometry->b = b;
        geometry->a = a;
}

void render_geometry(const struct Geometry *const geometry) {
        tracked_vertex_count += geometry->vertex_count;
        tracked_index_count += geometry->index_count;
        SDL_RenderGeometryRaw(
                get_context_renderer(),        // Renderer
                NULL,                          // Texture
                geometry->positions,           // Positions
                (int)VERTEX_POSITION_STRIDE,   // Position Stride
                (SDL_Color *)geometry->colors, // Colors
                (int)VERTEX_COLOR_STRIDE,      // Color Stride
                NULL,                          // Texcoords
                0,                             // Texcoord Stride
                (int)geometry->vertex_count,   // Vertex Count
                geometry->indices,             // Indices
                (int)geometry->index_count,    // Index Count
                (int)INDEX_STRIDE              // Index Size (Stride)
        );
}

static inline void secure_geometry_vertex_capacity(struct Geometry *const geometry, const size_t required_vertex_capacity) {
        if (required_vertex_capacity <= geometry->vertex_capacity) {
                return;
        }

        while (geometry->vertex_capacity < required_vertex_capacity) {
                geometry->vertex_capacity *= 2ULL;
        }

        geometry->positions = (float *)xrealloc(geometry->positions, VERTEX_POSITION_STRIDE * geometry->vertex_capacity);
        geometry->colors    = (uint8_t *)xrealloc(geometry->colors, VERTEX_COLOR_STRIDE * geometry->vertex_capacity);
}

static inline void secure_geometry_index_capacity(struct Geometry *const geometry, const size_t required_index_capacity) {
        if (required_index_capacity <= geometry->index_capacity) {
                return;
        }

        while (geometry->index_capacity < required_index_capacity) {
                geometry->index_capacity *= 2ULL;
        }

        geometry->indices = (uint16_t *)xrealloc(geometry->indices, INDEX_STRIDE * geometry->index_capacity);
}

static inline uint16_t add_geometry_vertex(struct Geometry *const geometry, const float x, const float y) {
        const size_t color_index = geometry->vertex_count * 4ULL;
        geometry->colors[color_index + 0ULL] = geometry->r;
        geometry->colors[color_index + 1ULL] = geometry->g;
        geometry->colors[color_index + 2ULL] = geometry->b;
        geometry->colors[color_index + 3ULL] = geometry->a;

        const size_t position_index = geometry->vertex_count * 2ULL;
        geometry->positions[position_index + 0ULL] = x;
        geometry->positions[position_index + 1ULL] = y;

        return (uint16_t)geometry->vertex_count++;
}

void write_triangle_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float x3, const float y3
) {
        secure_geometry_vertex_capacity(geometry, geometry->vertex_count + 3ULL);
        secure_geometry_index_capacity(geometry, geometry->index_count + 3ULL);

        geometry->indices[geometry->index_count++] = add_geometry_vertex(geometry, x1, y1);
        geometry->indices[geometry->index_count++] = add_geometry_vertex(geometry, x2, y2);
        geometry->indices[geometry->index_count++] = add_geometry_vertex(geometry, x3, y3);
}

void write_line_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float line_width,
        const enum LineCap rounded_caps
) {
        const float dx = x2 - x1;
        const float dy = y2 - y1;
        const float length = sqrtf(dx * dx + dy * dy);
        if (length == 0.0f) {
                return;
        }

        const float nx = (-dy / length) * line_width / 2.0f;
        const float ny = ( dx / length) * line_width / 2.0f;
        write_quadrilateral_geometry(
                geometry,
                x1 + nx, y1 + ny,
                x2 + nx, y2 + ny,
                x2 - nx, y2 - ny,
                x1 - nx, y1 - ny
        );

        const float rotation = atan2f(y2 - y1, x2 - x1);

        if ((rounded_caps & LINE_CAP_START) == LINE_CAP_START || (rounded_caps & LINE_CAP_BOTH) == LINE_CAP_BOTH) {
                write_circular_arc_geometry(geometry, x1, y1, line_width / 2.0f, rotation + (float)M_PI_2, rotation - (float)M_PI_2, false);
        }

        if ((rounded_caps & LINE_CAP_END) == LINE_CAP_END || (rounded_caps & LINE_CAP_BOTH) == LINE_CAP_BOTH) {
                write_circular_arc_geometry(geometry, x2, y2, line_width / 2.0f, rotation + (float)M_PI_2, rotation - (float)M_PI_2, true);
        }
}

void write_rectangle_geometry(
        struct Geometry *const geometry,
        const float x, const float y,
        const float w, const float h,
        const float rotation
) {
        const float half_w = w / 2.0f;
        const float half_h = h / 2.0f;

        // Top Left
        float x1 = x - half_w, y1 = y - half_h;
        rotate_point(&x1, &y1, x, y, rotation);

        // Top Right
        float x2 = x + half_w, y2 = y - half_h;
        rotate_point(&x2, &y2, x, y, rotation);

        // Bottom Right
        float x3 = x + half_w, y3 = y + half_h;
        rotate_point(&x3, &y3, x, y, rotation);

        // Bottom Left
        float x4 = x - half_w, y4 = y + half_h;
        rotate_point(&x4, &y4, x, y, rotation);

        write_quadrilateral_geometry(geometry, x1, y1, x2, y2, x3, y3, x4, y4);
}

void write_quadrilateral_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float x3, const float y3,
        const float x4, const float y4
) {
        secure_geometry_vertex_capacity(geometry, geometry->vertex_count + 4ULL);
        secure_geometry_index_capacity(geometry, geometry->index_count + 6ULL);

        const uint16_t index1 = add_geometry_vertex(geometry, x1, y1);
        const uint16_t index2 = add_geometry_vertex(geometry, x2, y2);
        const uint16_t index3 = add_geometry_vertex(geometry, x3, y3);
        const uint16_t index4 = add_geometry_vertex(geometry, x4, y4);

        geometry->indices[geometry->index_count++] = index1;
        geometry->indices[geometry->index_count++] = index2;
        geometry->indices[geometry->index_count++] = index3;

        geometry->indices[geometry->index_count++] = index1;
        geometry->indices[geometry->index_count++] = index3;
        geometry->indices[geometry->index_count++] = index4;
}

void write_circle_geometry(
        struct Geometry *const geometry,
        const float x, const float y,
        const float radius
) {
        write_ellipse_geometry(geometry, x, y, radius, radius, 0.0f);
}

void write_ellipse_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float rx, const float ry,
        const float rotation
) {
        write_elliptical_arc_geometry(geometry, cx, cy, rx, ry, rotation, 0.0f, 2.0f * (float)M_PI, false);
}

void write_circular_arc_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float radius,
        const float start_angle,
        const float end_angle,
        const bool clockwise
) {
        write_elliptical_arc_geometry(geometry, cx, cy, radius, radius, 0.0f, start_angle, end_angle, clockwise);
}

void write_elliptical_arc_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float rx, const float ry,
        const float rotation,
        const float start_angle,
        const float end_angle,
        const bool clockwise
) {
        if (rx <= 0.0f || ry <= 0.0f || start_angle == end_angle) {
                return;
        }

        float angle_span = end_angle - start_angle;
        if (clockwise && angle_span > 0.0f) {
                angle_span -= 2.0f * (float)M_PI;
        } else if (!clockwise && angle_span < 0.0f) {
                angle_span += 2.0f * (float)M_PI;
        }

        const float circumference = M_PI * (3.0f * (rx + ry) - sqrtf((3.0f * rx + ry) * (rx + 3.0f * ry)));
        const float arc_length = circumference * fabsf(angle_span) / (2.0f * (float)M_PI);

        size_t resolution = (size_t)ceilf(arc_length / SEGMENT_LENGTH);
        if (resolution < 3ULL) {
                resolution = 3ULL;
        }

        secure_geometry_vertex_capacity(geometry, geometry->vertex_count + resolution + 2ULL);
        secure_geometry_index_capacity(geometry, geometry->index_count + (resolution + 1ULL) * 3ULL);

        const uint16_t center_index = add_geometry_vertex(geometry, cx, cy);
        for (size_t index = 0ULL; index <= resolution; ++index) {
                float interpolation = (float)index / (float)resolution;
                float angle = start_angle + interpolation * angle_span;

                float x = rx * cosf(angle);
                float y = ry * sinf(angle);

                if (rotation != 0.0f) {
                        const float sin_r = sinf(rotation);
                        const float cos_r = cosf(rotation);
                        float vx = x;
                        x = vx * cos_r - y * sin_r;
                        y = vx * sin_r + y * cos_r;
                }

                add_geometry_vertex(geometry, cx + x, cy + y);
        }

        for (size_t index = 0ULL; index < resolution; ++index) {
                geometry->indices[geometry->index_count++] = center_index;
                geometry->indices[geometry->index_count++] = center_index + 1 + (uint16_t)index;
                geometry->indices[geometry->index_count++] = center_index + 1 + (uint16_t)((index + 1ULL));
        }
}

void write_circle_outline_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float radius,
        const float line_width
) {
        write_ellipse_outline_geometry(geometry, cx, cy, radius, radius, line_width);
}

void write_ellipse_outline_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float rx, const float ry,
        const float line_width
) {
        write_elliptical_arc_outline_geometry(geometry, cx, cy, rx, ry, 0.0f, line_width, 0.0f, 2.0f * (float)M_PI, false, LINE_CAP_NONE);
}

void write_circular_arc_outline_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float radius,
        const float line_width,
        const float start_angle,
        const float end_angle,
        const bool clockwise,
        const enum LineCap rounded_caps
) {
        write_elliptical_arc_outline_geometry(geometry, cx, cy, radius, radius, 0.0f, line_width, start_angle, end_angle, clockwise, rounded_caps);
}

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
) {
        if (rx <= 0.0f || ry <= 0.0f || start_angle == end_angle) {
                return;
        }

        float angle_span = end_angle - start_angle;
        if (clockwise && angle_span > 0.0f) {
                angle_span -= 2.0f * (float)M_PI;
        } else if (!clockwise && angle_span < 0.0f) {
                angle_span += 2.0f * (float)M_PI;
        }

        const float circumference = M_PI * (3.0f * (rx + ry) - sqrtf((3.0f * rx + ry) * (rx + 3.0f * ry)));
        const float arc_length = circumference * fabsf(angle_span) / (2.0f * (float)M_PI);

        size_t resolution = (size_t)ceilf(arc_length / SEGMENT_LENGTH);
        if (resolution < 3ULL) {
                resolution = 3ULL;
        }

        secure_geometry_vertex_capacity(geometry, geometry->vertex_count + (resolution + 1ULL) * 2ULL);
        secure_geometry_index_capacity(geometry, geometry->index_count + resolution * 6ULL);

        float inner_radius_x = rx - line_width / 2.0f;
        float inner_radius_y = ry - line_width / 2.0f;

        if (inner_radius_x < 0.0f) {
                inner_radius_x = 0.0f;
        }

        if (inner_radius_y < 0.0f) {
                inner_radius_y = 0.0f;
        }

        const float outer_radius_x = rx + line_width / 2.0f;
        const float outer_radius_y = ry + line_width / 2.0f;

        const float cos = cosf(rotation);
        const float sin = sinf(rotation);
        const uint16_t start_index = (uint16_t)geometry->vertex_count;

        // HACK: Keeping track of the triangle vertices at the start and end of the triangle strip to accurately calculate the
        // positions (centers) of the line cap arcs since I can't get it to look aligned visually.
        float inner_x1, inner_y1, inner_x2, inner_y2;
        float outer_x1, outer_y1, outer_x2, outer_y2;

        for (size_t index = 0ULL; index <= resolution; ++index) {
                const float angle = start_angle + angle_span * (float)index / (float)resolution;

                const float x_outer = outer_radius_x * cosf(angle);
                const float y_outer = outer_radius_y * sinf(angle);

                const float x_inner = inner_radius_x * cosf(angle);
                const float y_inner = inner_radius_y * sinf(angle);

                const float rx_outer = x_outer * cos - y_outer * sin;
                const float ry_outer = x_outer * sin + y_outer * cos;

                const float rx_inner = x_inner * cos - y_inner * sin;
                const float ry_inner = x_inner * sin + y_inner * cos;

                if (index == 0LL) {
                        inner_x1 = cx + rx_inner;
                        inner_y1 = cy + ry_inner;
                        outer_x1 = cx + rx_outer;
                        outer_y1 = cy + ry_outer;
                }

                if (index == resolution) {
                        inner_x2 = cx + rx_inner;
                        inner_y2 = cy + ry_inner;
                        outer_x2 = cx + rx_outer;
                        outer_y2 = cy + ry_outer;
                }

                add_geometry_vertex(geometry, cx + rx_outer, cy + ry_outer);
                add_geometry_vertex(geometry, cx + rx_inner, cy + ry_inner);

                if (index < resolution) {
                        const uint16_t base = start_index + (uint16_t)index * 2ULL;

                        geometry->indices[geometry->index_count++] = base + 0;
                        geometry->indices[geometry->index_count++] = base + 1;
                        geometry->indices[geometry->index_count++] = base + 2;

                        geometry->indices[geometry->index_count++] = base + 1;
                        geometry->indices[geometry->index_count++] = base + 3;
                        geometry->indices[geometry->index_count++] = base + 2;
                }
        }

        const float cap_radius = line_width / 2.0f;

        // HACK: Using an arbitrary offset to add to the angle to the arcs since the angle of both arcs are just slightly off for some reason.
        static const float angle_offset = M_PI_4 / 4.0f;

        if ((rounded_caps & LINE_CAP_START) == LINE_CAP_START || (rounded_caps & LINE_CAP_BOTH) == LINE_CAP_BOTH) {
                const float x = outer_radius_x * cosf(start_angle);
                const float y = outer_radius_y * sinf(start_angle);

                const float rx = x * cos - y * sin;
                const float ry = x * sin + y * cos;

                const float dx0 = -outer_radius_x * sinf(start_angle);
                const float dy0 =  outer_radius_y * cosf(start_angle);

                const float dx = dx0 * cos - dy0 * sin;
                const float dy = dx0 * sin + dy0 * cos;

                const float tangent_angle = atan2f(dy, dx);

                write_circular_arc_geometry(
                        geometry,
                        (inner_x1 + outer_x1) / 2.0f,
                        (inner_y1 + outer_y1) / 2.0f,
                        cap_radius,
                        tangent_angle - M_PI_2 - angle_offset,
                        tangent_angle + M_PI_2 + angle_offset,
                        false
                );
        }

        if ((rounded_caps & LINE_CAP_END) == LINE_CAP_END || (rounded_caps & LINE_CAP_BOTH) == LINE_CAP_BOTH) {
                const float x = outer_radius_x * cosf(end_angle);
                const float y = outer_radius_y * sinf(end_angle);

                const float rx = x * cos - y * sin;
                const float ry = x * sin + y * cos;

                float dx0 = -outer_radius_x * sinf(end_angle);
                float dy0 =  outer_radius_y * cosf(end_angle);

                float dx = dx0 * cos - dy0 * sin;
                float dy = dx0 * sin + dy0 * cos;

                const float tangent_angle = atan2f(dy, dx);

                write_circular_arc_geometry(
                        geometry,
                        (inner_x2 + outer_x2) / 2.0f,
                        (inner_y2 + outer_y2) / 2.0f,
                        cap_radius,
                        tangent_angle + M_PI_2 - angle_offset,
                        tangent_angle - M_PI_2 + angle_offset,
                        false
                );
        }
}

void write_hexagon_geometry(
        struct Geometry *const geometry,
        const float x, const float y,
        const float radius,
        const float rotation
) {
        secure_geometry_vertex_capacity(geometry, geometry->vertex_count + 6ULL);
        secure_geometry_index_capacity(geometry, geometry->index_count + 12ULL);

        uint16_t vertices[6];
        static const float step = (float)M_PI / 3.0f;
        for (uint8_t index = 0; index < 6; index++) {
                const float angle = rotation + step * (float)index;
                vertices[index] = add_geometry_vertex(geometry, x + cosf(angle) * radius, y + sinf(angle) * radius);
        }

        geometry->indices[geometry->index_count++] = vertices[1];
        geometry->indices[geometry->index_count++] = vertices[2];
        geometry->indices[geometry->index_count++] = vertices[3];

        geometry->indices[geometry->index_count++] = vertices[1];
        geometry->indices[geometry->index_count++] = vertices[3];
        geometry->indices[geometry->index_count++] = vertices[4];

        geometry->indices[geometry->index_count++] = vertices[1];
        geometry->indices[geometry->index_count++] = vertices[4];
        geometry->indices[geometry->index_count++] = vertices[5];

        geometry->indices[geometry->index_count++] = vertices[1];
        geometry->indices[geometry->index_count++] = vertices[5];
        geometry->indices[geometry->index_count++] = vertices[0];
}

static inline void compute_bezier_point(
        const float interpolation,
        const float px1,    const float py1, // Endpoint A
        const float px2,    const float py2, // Endpoint B
        const float cx1,    const float cy1, // Control Point A
        const float cx2,    const float cy2, // Control Point B
        float *const out_x, float *const out_y
) {
        const float t = interpolation;
        const float u = 1.0f - t;
        const float tt = t * t;
        const float uu = u * u;
        const float uuu = uu * u;
        const float ttt = tt * t;

        *out_x = uuu * px1 + 3.0f * uu * t * cx1 + 3.0f * u * tt * cx2 + ttt * px2;
        *out_y = uuu * py1 + 3.0f * uu * t * cy1 + 3.0f * u * tt * cy2 + ttt * py2;
}

static inline void compute_bezier_tangent(
        const float interpolation,
        const float px1, const float py1,
        const float px2, const float py2,
        const float cx1, const float cy1,
        const float cx2, const float cy2,
        float *const out_dx, float *const out_dy
) {
        const float t = interpolation;
        const float u = 1.0f - t;
        *out_dx = 3.0f * u * u * (cx1 - px1)
                + 6.0f * u * t * (cx2 - cx1)
                + 3.0f * t * t * (px2 - cx2);
        *out_dy = 3.0f * u * u * (cy1 - py1)
                + 6.0f * u * t * (cy2 - cy1)
                + 3.0f * t * t * (py2 - cy2);
}

void write_bezier_curve_geometry(
        struct Geometry *const geometry,
        const float px1,    const float py1, // Endpoint A
        const float px2,    const float py2, // Endpoint B
        const float cx1,    const float cy1, // Control Point A
        const float cx2,    const float cy2, // Control Point B
        const float line_width
) {

#define DISTANCE(x1, y1, x2, y2) (sqrtf(((x2) - (x1)) * ((x2) - (x1)) + ((y2) - (y1)) * ((y2) - (y1))))

        const float curvature
                = DISTANCE(px1, py1, cx1, cy1)
                + DISTANCE(cx1, cy1, cx2, cy2)
                + DISTANCE(cx2, cy2, px2, py2)
                / DISTANCE(px1, py1, px2, py2);
        size_t samples = (size_t)(curvature * 5.0f);
        if (samples < 5ULL) {
                samples = 5ULL;
        }

        float estimated_length = 0.0f;
        float x1 = px1, y1 = py1;
        for (size_t index = 1ULL; index <= samples; ++index) {
                const float interpolation = (float)index / (float)samples;

                float x2, y2;
                compute_bezier_point(interpolation, px1, py1, cx1, cy1, cx2, cy2, px2, py2, &x2, &y2);
                estimated_length += DISTANCE(x1, y1, x2, y2);
                x1 = x2;
                y1 = y2;
        }

#undef DISTANCE

        const size_t resolution = (size_t)ceilf(estimated_length / SEGMENT_LENGTH);
        if (resolution == 0ULL) {
                return;
        }

        secure_geometry_vertex_capacity(geometry, geometry->vertex_count + (resolution + 1ULL) * 2ULL);
        secure_geometry_index_capacity(geometry, geometry->index_count + resolution * 6ULL);

        float tx, ty;
        compute_bezier_tangent(0.0f, px1, py1, px2, py2, cx1, cy1, cx2, cy2, &tx, &ty);
        compute_bezier_point(0.0f, px1, py1, px2, py2, cx1, cy1, cx2, cy2, &x1, &y1);

        const float half_width = line_width / 2.0f;

        float length = sqrtf(tx * tx + ty * ty);
        float nx = (length > 0.0f) ? (-ty / length) * half_width : 0.0f;
        float ny = (length > 0.0f) ? ( tx / length) * half_width : 0.0f;

        uint16_t left1  = add_geometry_vertex(geometry, x1 - nx, y1 - ny);
        uint16_t right1 = add_geometry_vertex(geometry, x1 + nx, y1 + ny);

        for (size_t index = 1ULL; index <= resolution; ++index) {
                const float interpolation = (float)index / (float)resolution;

                float x2, y2;
                compute_bezier_point(  interpolation, px1, py1, px2, py2, cx1, cy1, cx2, cy2, &x2, &y2);
                compute_bezier_tangent(interpolation, px1, py1, px2, py2, cx1, cy1, cx2, cy2, &tx, &ty);

                length = sqrtf(tx * tx + ty * ty);
                nx = (length > 0.0f) ? (-ty / length) * half_width : 0.0f;
                ny = (length > 0.0f) ? ( tx / length) * half_width : 0.0f;

                const uint16_t left2  = add_geometry_vertex(geometry, x2 - nx, y2 - ny);
                const uint16_t right2 = add_geometry_vertex(geometry, x2 + nx, y2 + ny);

                geometry->indices[geometry->index_count++] = left1;
                geometry->indices[geometry->index_count++] = right1;
                geometry->indices[geometry->index_count++] = left2;

                geometry->indices[geometry->index_count++] = left2;
                geometry->indices[geometry->index_count++] = right1;
                geometry->indices[geometry->index_count++] = right2;

                left1  = left2;
                right1 = right2;
        }
}

void write_rounded_triangle_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float x3, const float y3,
        const float rounded_radius
) {
        if (rounded_radius <= 0.0f) {
                write_triangle_geometry(geometry, x1, y1, x2, y2, x3, y3);
                return;
        }

        const float vx[3] = {x1, x2, x3};
        const float vy[3] = {y1, y2, y3};

        const float double_signed_area = (vx[1] - vx[0]) * (vy[2] - vy[0]) - (vy[1] - vy[0]) * (vx[2] - vx[0]);
        const bool counterclockwise = double_signed_area > 0.0f;

        float maximum_radius = FLT_MAX;
        for (uint8_t index = 0; index < 3; ++index) {
                float edge1_x = vx[(index + 1) % 3] - vx[index];
                float edge1_y = vy[(index + 1) % 3] - vy[index];
                float edge2_x = vx[(index + 2) % 3] - vx[index];
                float edge2_y = vy[(index + 2) % 3] - vy[index];

                const float length1 = sqrtf(edge1_x * edge1_x + edge1_y * edge1_y);
                const float length2 = sqrtf(edge2_x * edge2_x + edge2_y * edge2_y);
                if (length1 == 0.0f || length2 == 0.0f) {
                        continue;
                }

                edge1_x /= length1;
                edge1_y /= length1;
                edge2_x /= length2;
                edge2_y /= length2;

                const float dot12 = fmaxf(-1.0f, fminf(1.0f, edge1_x * edge2_x + edge1_y * edge2_y));
                const float maximum_side_radius = fminf(length1, length2) * tanf(acosf(dot12) / 2.0f);
                if (maximum_side_radius < maximum_radius) {
                        maximum_radius = maximum_side_radius;
                }
        }

        const float clamped_rounded_radius = fminf(rounded_radius, maximum_radius);

        float   center_x[3],   center_y[3];
        float tangent1_x[3], tangent1_y[3];
        float tangent2_x[3], tangent2_y[3];

        for (uint8_t index = 0; index < 3; ++index) {
                float edge1_x = vx[(index + 1) % 3] - vx[index];
                float edge1_y = vy[(index + 1) % 3] - vy[index];
                float edge2_x = vx[(index + 2) % 3] - vx[index];
                float edge2_y = vy[(index + 2) % 3] - vy[index];

                const float length1 = sqrtf(edge1_x * edge1_x + edge1_y * edge1_y);
                const float length2 = sqrtf(edge2_x * edge2_x + edge2_y * edge2_y);
                if (length1 == 0.0f || length2 == 0.0f) {
                        continue;
                }

                edge1_x /= length1;
                edge1_y /= length1;
                edge2_x /= length2;
                edge2_y /= length2;

                const float theta = acosf(fmaxf(-1.0f, fminf(1.0f, edge1_x * edge2_x + edge1_y * edge2_y)));
                const float tangent = tanf(theta / 2.0f);
                if (tangent == 0.0f) {
                        continue;
                }

                float distance = clamped_rounded_radius / tangent;
                if (distance > length1) {
                        distance = length1;
                }

                if (distance > length2) {
                        distance = length2;
                }

                tangent1_x[index] = vx[index] + edge1_x * distance;
                tangent1_y[index] = vy[index] + edge1_y * distance;
                tangent2_x[index] = vx[index] + edge2_x * distance;
                tangent2_y[index] = vy[index] + edge2_y * distance;

                const float bisector_x = edge1_x + edge2_x;
                const float bisector_y = edge1_y + edge2_y;
                const float bisector_length = sqrtf(bisector_x * bisector_x + bisector_y * bisector_y);
                if (bisector_length == 0.0f) {
                        continue;
                }

                const float sine = sinf(theta / 2.0f);
                if (sine == 0.0f) {
                        continue;
                }

                center_x[index] = vx[index] + (bisector_x / bisector_length) * clamped_rounded_radius / sine;
                center_y[index] = vy[index] + (bisector_y / bisector_length) * clamped_rounded_radius / sine;

                const float angle1 = atan2f(tangent1_y[index] - center_y[index], tangent1_x[index] - center_x[index]);
                const float angle2 = atan2f(tangent2_y[index] - center_y[index], tangent2_x[index] - center_x[index]);
                float delta = angle2 - angle1;

                while (delta <= -M_PI) {
                        delta += 2.0f * (float)M_PI;
                }

                while (delta > M_PI) {
                        delta -= 2.0f * (float)M_PI;
                }

                write_circular_arc_geometry(
                        geometry,
                        center_x[index],
                        center_y[index],
                        clamped_rounded_radius,
                        angle1, angle2,
                        delta < 0.0f
                );
        }

        for (uint8_t index = 0; index < 3; ++index) {
                const float x1 = tangent1_x[index];
                const float y1 = tangent1_y[index];
                const float x2 = tangent2_x[(index + 1) % 3];
                const float y2 = tangent2_y[(index + 1) % 3];

                const float distance_x = x2 - x1;
                const float distance_y = y2 - y1;
                const float length = sqrtf(distance_x * distance_x + distance_y * distance_y);
                if (length == 0.0f) {
                        continue;
                }

                const float offset_x = (clamped_rounded_radius / 2.0f) * (counterclockwise ? -distance_y : distance_y) / length;
                const float offset_y = (clamped_rounded_radius / 2.0f) * (counterclockwise ? distance_x : -distance_x) / length;

                write_line_geometry(
                        geometry,
                        x1 + offset_x, y1 + offset_y,
                        x2 + offset_x, y2 + offset_y,
                        clamped_rounded_radius,
                        LINE_CAP_NONE
                );
        }

        write_triangle_geometry(
                geometry,
                center_x[0], center_y[0],
                center_x[1], center_y[1],
                center_x[2], center_y[2]
        );
}

void write_rounded_rectangle_geometry(
        struct Geometry *const geometry,
        const float x, const float y,
        const float w, const float h,
        const float rounded_radius,
        const float rotation
) {
        if (rounded_radius <= 0.0f) {
                write_rectangle_geometry(geometry, x, y, w, h, rotation);
                return;
        }

        const float half_w = w / 2.0f;
        const float half_h = h / 2.0f;
        float radius = rounded_radius;

        if (radius > half_w) {
                radius = half_w;
        }

        if (radius > half_h) {
                radius = half_h;
        }

        float x1 = x + half_w - radius, y1 = y - half_h + radius; // Top Right
        float x2 = x + half_w - radius, y2 = y + half_h - radius; // Bottom Right
        float x3 = x - half_w + radius, y3 = y + half_h - radius; // Bottom Left
        float x4 = x - half_w + radius, y4 = y - half_h + radius; // Top Left

        float x5 = x1 + radius / 2.0f, y5 = y1;
        float x6 = x2 + radius / 2.0f, y6 = y2;
        float x7 = x3 - radius / 2.0f, y7 = y3;
        float x8 = x4 - radius / 2.0f, y8 = y4;

        rotate_point(&x1, &y1, x, y, rotation);
        rotate_point(&x2, &y2, x, y, rotation);
        rotate_point(&x3, &y3, x, y, rotation);
        rotate_point(&x4, &y4, x, y, rotation);
        rotate_point(&x5, &y5, x, y, rotation);
        rotate_point(&x6, &y6, x, y, rotation);
        rotate_point(&x7, &y7, x, y, rotation);
        rotate_point(&x8, &y8, x, y, rotation);

        write_circular_arc_geometry(geometry, x1, y1, radius, rotation - M_PI_2, rotation,                      false);
        write_circular_arc_geometry(geometry, x2, y2, radius, rotation,          rotation + M_PI_2,             false);
        write_circular_arc_geometry(geometry, x3, y3, radius, rotation + M_PI_2, rotation + M_PI,               false);
        write_circular_arc_geometry(geometry, x4, y4, radius, rotation + M_PI,   rotation + 1.5f * (float)M_PI, false);
        write_rectangle_geometry(geometry, x, y, w - radius * 2.0f, h, rotation);
        write_line_geometry(geometry, x5, y5, x6, y6, radius, LINE_CAP_NONE);
        write_line_geometry(geometry, x7, y7, x8, y8, radius, LINE_CAP_NONE);
}

void write_rounded_quadrilateral_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float x3, const float y3,
        const float x4, const float y4,
        const float rounded_radius
) {
        if (rounded_radius <= 0.0f) {
                write_quadrilateral_geometry(geometry, x1, y1, x2, y2, x3, y3, x4, y4);
                return;
        }

        const float vx[4] = {x1, x2, x3, x4};
        const float vy[4] = {y1, y2, y3, y4};

        const float double_signed_area = (vx[1] - vx[0]) * (vy[2] - vy[0]) - (vy[1] - vy[0]) * (vx[2] - vx[0]);
        const bool counterclockwise = double_signed_area > 0.0f;

        float maximum_radius = FLT_MAX;
        for (uint8_t index = 0; index < 4; ++index) {
                float edge1_x = vx[(index + 1) % 4] - vx[index];
                float edge1_y = vy[(index + 1) % 4] - vy[index];
                float edge2_x = vx[(index + 3) % 4] - vx[index];
                float edge2_y = vy[(index + 3) % 4] - vy[index];

                float length1 = sqrtf(edge1_x * edge1_x + edge1_y * edge1_y);
                float length2 = sqrtf(edge2_x * edge2_x + edge2_y * edge2_y);
                if (length1 == 0.0f || length2 == 0.0f) {
                        continue;
                }

                edge1_x /= length1;
                edge1_y /= length1;
                edge2_x /= length2;
                edge2_y /= length2;

                const float dot12 = fmaxf(-1.0f, fminf(1.0f, edge1_x * edge2_x + edge1_y * edge2_y));
                const float maximum_side_radius = fminf(length1, length2) * tanf(acosf(dot12) / 2.0f);
                if (maximum_side_radius < maximum_radius) {
                        maximum_radius = maximum_side_radius;
                }
        }

        const float clamped_radius = fminf(rounded_radius, maximum_radius);

        float   center_x[4],   center_y[4];
        float tangent1_x[4], tangent1_y[4];
        float tangent2_x[4], tangent2_y[4];

        for (uint8_t index = 0; index < 4; ++index) {
                float edge1_x = vx[(index + 1) % 4] - vx[index];
                float edge1_y = vy[(index + 1) % 4] - vy[index];
                float edge2_x = vx[(index + 3) % 4] - vx[index];
                float edge2_y = vy[(index + 3) % 4] - vy[index];

                const float length1 = sqrtf(edge1_x * edge1_x + edge1_y * edge1_y);
                const float length2 = sqrtf(edge2_x * edge2_x + edge2_y * edge2_y);
                if (length1 == 0.0f || length2 == 0.0f) {
                        continue;
                }

                edge1_x /= length1;
                edge1_y /= length1;
                edge2_x /= length2;
                edge2_y /= length2;

                const float theta = acosf(fmaxf(-1.0f, fminf(1.0f, edge1_x * edge2_x + edge1_y * edge2_y)));
                const float tangent = tanf(theta / 2.0f);
                if (tangent == 0.0f) {
                        continue;
                }

                float distance = clamped_radius / tangent;

                if (distance > length1) {
                        distance = length1;
                }

                if (distance > length2) {
                        distance = length2;
                }

                tangent1_x[index] = vx[index] + edge1_x * distance;
                tangent1_y[index] = vy[index] + edge1_y * distance;
                tangent2_x[index] = vx[index] + edge2_x * distance;
                tangent2_y[index] = vy[index] + edge2_y * distance;

                const float bisector_x = edge1_x + edge2_x;
                const float bisector_y = edge1_y + edge2_y;
                const float bisector_length = sqrtf(bisector_x * bisector_x + bisector_y * bisector_y);
                if (bisector_length == 0.0f) {
                        continue;
                }

                const float sine = sinf(theta / 2.0f);
                if (sine == 0.0f) {
                        continue;
                }

                center_x[index] = vx[index] + (bisector_x / bisector_length) * clamped_radius / sine;
                center_y[index] = vy[index] + (bisector_y / bisector_length) * clamped_radius / sine;

                float angle1 = atan2f(tangent1_y[index] - center_y[index], tangent1_x[index] - center_x[index]);
                float angle2 = atan2f(tangent2_y[index] - center_y[index], tangent2_x[index] - center_x[index]);
                float delta = angle2 - angle1;

                while (delta <= -M_PI) {
                        delta += 2.0f * (float)M_PI;
                }

                while (delta > M_PI) {
                        delta -= 2.0f * (float)M_PI;
                }

                write_circular_arc_geometry(
                        geometry,
                        center_x[index],
                        center_y[index],
                        clamped_radius,
                        angle1, angle2,
                        delta < 0.0f
                );
        }

        for (uint8_t index = 0; index < 4; ++index) {
                const float x1 = tangent1_x[index];
                const float y1 = tangent1_y[index];
                const float x2 = tangent2_x[(index + 1) % 4];
                const float y2 = tangent2_y[(index + 1) % 4];

                const float distance_x = x2 - x1;
                const float distance_y = y2 - y1;
                const float length = sqrtf(distance_x * distance_x + distance_y * distance_y);
                if (length == 0.0f) {
                        continue;
                }

                const float offset_x = (clamped_radius / 2.0f) * (counterclockwise ? -distance_y : distance_y) / length;
                const float offset_y = (clamped_radius / 2.0f) * (counterclockwise ? distance_x : -distance_x) / length;

                write_line_geometry(
                        geometry,
                        x1 + offset_x, y1 + offset_y,
                        x2 + offset_x, y2 + offset_y,
                        clamped_radius,
                        LINE_CAP_NONE
                );
        }

        // NOTE: Using two lines (quads) and one large quad in the center instead of one line (quad) for each edge with one quad in the center
        // would be more efficient as it uses two less quads, but I couldn't make that work when it comes to quads that aren't rectangular.
        write_quadrilateral_geometry(
                geometry,
                center_x[0], center_y[0],
                center_x[1], center_y[1],
                center_x[2], center_y[2],
                center_x[3], center_y[3]
        );
}