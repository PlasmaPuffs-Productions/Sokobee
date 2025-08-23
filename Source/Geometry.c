#include "Geometry.h"

#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <SDL.h>
#include <SDL_render.h>

#include "Utilities.h"
#include "Context.h"

#define INITIAL_VERTEX_CAPACITY 64ULL
#define INITIAL_INDEX_CAPACITY  64ULL

#define INDEX_STRIDE           (sizeof(uint16_t))
#define VERTEX_POSITION_STRIDE (sizeof(float) * 2ULL)
#define VERTEX_COLOR_STRIDE    (sizeof(uint8_t) * 4ULL)

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
                send_message(WARNING, "Geometry given to destroy is NULL");
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
                geometry->index_count,         // Index Count
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

void write_line_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float line_width
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
}

void write_ellipse_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float rx, const float ry,
        const float rotation,
        const size_t resolution
) {
        if (resolution == 0ULL) {
                send_message(WARNING, "Writing ellipse geometry with resolution of zero");
                return;
        }

        if (rx <= 0.0f || ry <= 0.0f) {
                send_message(WARNING, "Writing ellipse geometry with invalid radii");
                return;
        }

        secure_geometry_vertex_capacity(geometry, geometry->vertex_count + resolution + 1ULL);
        secure_geometry_index_capacity(geometry, geometry->index_count + resolution * 3ULL);

        const uint16_t center_index = add_geometry_vertex(geometry, cx, cy);

        for (size_t index = 0ULL; index < resolution; ++index) {
                const float angle = (float)index / (float)resolution * 2.0f * (float)M_PI;

                float x = rx * cosf(angle);
                float y = ry * sinf(angle);

                if (rotation != 0.0f) {
                        const float sin = sinf(rotation);
                        const float cos = cosf(rotation);
                        const float v = x;
                        x = v * cos - y * sin;
                        y = v * sin + y * cos;
                }

                add_geometry_vertex(geometry, cx + x, cy + y);
        }

        for (size_t index = 0ULL; index < resolution; ++index) {
                geometry->indices[geometry->index_count++] = center_index;
                geometry->indices[geometry->index_count++] = center_index + 1 + (uint16_t)index;
                geometry->indices[geometry->index_count++] = center_index + 1 + ((uint16_t)(index + 1ULL) % resolution);
        }
}

void write_circle_geometry(
        struct Geometry *const geometry,
        const float x, const float y,
        const float radius,
        const size_t resolution
) {
        if (resolution == 0ULL) {
                send_message(WARNING, "Writing circle geometry with resolution of zero");
                return;
        }

        if (radius <= 0.0f) {
                send_message(WARNING, "Writing circle geometry with invalid radius");
                return;
        }

        write_ellipse_geometry(geometry, x, y, radius, radius, 0.0f, resolution);
}

void write_hexagon_geometry(struct Geometry *const geometry, const float x, const float y, const float radius, const float rotation) {
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
        const float line_width,
        const size_t resolution
) {
        if (resolution == 0ULL) {
                send_message(WARNING, "Writing bezier curve strip with resolution of zero");
                return;
        }

        secure_geometry_vertex_capacity(geometry, geometry->vertex_count + (resolution + 1ULL) * 2ULL);
        secure_geometry_index_capacity(geometry, geometry->index_count + resolution * 6ULL);

        float x1, y1;
        compute_bezier_point(0.0f, px1, py1, px2, py2, cx1, cy1, cx2, cy2, &x1, &y1);

        float tx, ty;
        compute_bezier_tangent(0.0f, px1, py1, px2, py2, cx1, cy1, cx2, cy2, &tx, &ty);

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

static inline SDL_FPoint normalize(const SDL_FPoint vector) {
        const float length = sqrtf(vector.x * vector.x + vector.y * vector.y);
        if (length == 0.0f) {
                return (SDL_FPoint){0.0f, 0.0f};
        }

        return (SDL_FPoint){vector.x / length, vector.y / length};
}

void write_rounded_triangle_geometry(
        struct Geometry *const geometry,
        const float x1, const float y1,
        const float x2, const float y2,
        const float x3, const float y3,
        const float rounded_radius,
        const size_t arc_resolution
) {
        if (arc_resolution == 0.0f) {
                if (rounded_radius == 0.0f) {
                        send_message(WARNING, "Rounded triangle geometry has rounded radius of zero: Writing triangle geometry instead");
                        write_triangle_geometry(geometry, x1, y1, x2, y2, x3, y3);
                        return;
                }

                send_message(WARNING, "Writing rounded triangle geometry with arc resolution of zero");
                return;
        }

        SDL_FPoint counterclockwise_vertices[] = {{x1, y1}, {x2, y2}, {x3, y3}};
        const float signed_area = (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);
        if (signed_area < 0.0f) {
                counterclockwise_vertices[1] = (SDL_FPoint){x3, y3};
                counterclockwise_vertices[2] = (SDL_FPoint){x2, y2};
        }

        SDL_FPoint edges[3];
        for (size_t edge_index = 0ULL; edge_index < 3ULL; ++edge_index) {
                const size_t next_edge_index = (edge_index + 1ULL) % 3ULL;
                edges[edge_index] = (SDL_FPoint){
                        counterclockwise_vertices[next_edge_index].x - counterclockwise_vertices[edge_index].x,
                        counterclockwise_vertices[next_edge_index].y - counterclockwise_vertices[edge_index].y
                };
        }

        SDL_FPoint arc_ends[3][2];
        SDL_FPoint arc_centers[3];
        const size_t resolution = arc_resolution <= 64ULL ? arc_resolution : 64ULL;
        for (size_t corner_index = 0ULL; corner_index < 3ULL; ++corner_index) {
                const size_t previous_corner_index = (corner_index + 2ULL) % 3ULL;
                const SDL_FPoint previous_vertex = normalize((SDL_FPoint){-edges[previous_corner_index].x, -edges[previous_corner_index].y});
                const SDL_FPoint current_vertex = normalize(edges[corner_index]);

                const float dot = CLAMP_VALUE(previous_vertex.x * current_vertex.x + previous_vertex.y * current_vertex.y, -1.0f, 1.0f);
                const SDL_FPoint bisector = normalize((SDL_FPoint){previous_vertex.x + current_vertex.x, previous_vertex.y + current_vertex.y});
                const float offset = rounded_radius / sinf(acosf(dot) / 2.0f);
                arc_centers[corner_index] = (SDL_FPoint){
                        .x = counterclockwise_vertices[corner_index].x + bisector.x * offset,
                        .y = counterclockwise_vertices[corner_index].y + bisector.y * offset
                };

                const float arc_span = (float)M_PI - acosf(dot);
                const float start_angle = atan2f(previous_vertex.y, previous_vertex.x) + (float)M_PI_2;
                const float end_angle = start_angle;

                SDL_FPoint arc_points[resolution + 1ULL];
                for (size_t point_index = 0ULL; point_index <= arc_resolution; ++point_index) {
                        const float theta = start_angle + arc_span * (float)point_index / (float)arc_resolution;
                        arc_points[point_index].x = arc_centers[corner_index].x + rounded_radius * cosf(theta);
                        arc_points[point_index].y = arc_centers[corner_index].y + rounded_radius * sinf(theta);
                }

                for (size_t point_index = 0ULL; point_index < arc_resolution; ++point_index) {
                        write_triangle_geometry(
                                geometry,
                                arc_centers [corner_index      ].x, arc_centers [corner_index      ].y,
                                arc_points  [point_index       ].x, arc_points  [point_index       ].y,
                                arc_points  [point_index + 1ULL].x, arc_points  [point_index + 1ULL].y
                        );
                }

                arc_ends[corner_index][0] = arc_points[0];
                arc_ends[corner_index][1] = arc_points[resolution];
        }

        for (size_t edge_index = 0ULL; edge_index < 3ULL; ++edge_index) {
                const SDL_FPoint a = arc_centers[edge_index];
                const SDL_FPoint b = arc_centers[(edge_index + 1ULL) % 3ULL];
                write_line_geometry(geometry, a.x, a.y, b.x, b.y, rounded_radius * 2.0f);
        }

        write_triangle_geometry(
                geometry,
                arc_centers[0].x, arc_centers[0].y,
                arc_centers[1].x, arc_centers[1].y,
                arc_centers[2].x, arc_centers[2].y
        );
}

void write_arc_outline_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float radius,
        const float line_width,
        const float start_angle,
        const float end_angle,
        const bool clockwise,
        const size_t resolution
) {
        if (resolution == 0ULL) {
                send_message(WARNING, "Writing arc outline geometry with resolution of zero");
                return;
        }

        write_elliptical_arc_outline_geometry(
                geometry,
                cx, cy,
                radius, radius,
                line_width,
                start_angle,
                end_angle,
                clockwise,
                0.0f,
                resolution
        );
}

void write_elliptical_arc_outline_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float rx, const float ry,
        const float line_width,
        const float start_angle,
        const float end_angle,
        const bool clockwise,
        const float rotation,
        const size_t resolution
) {
        if (resolution == 0ULL) {
                send_message(WARNING, "Writing elliptical arc outline geometry resolution of zero");
                return;
        }

        float angle_1 = start_angle;
        float angle_2 = end_angle;
        float angle_span = angle_2 - angle_1;
        if (clockwise) {
                angle_1 = end_angle;
                angle_2 = start_angle;
                angle_span = angle_2 - angle_1;
        }

        if (angle_span <= 0.0f) {
                angle_span += 2.0f * (float)M_PI;
        }

        SDL_FPoint inner_radii = {
                rx - line_width / 2.0f,
                ry - line_width / 2.0f
        };

        SDL_FPoint outer_radii = {
                rx + line_width / 2.0f,
                ry + line_width / 2.0f
        };

        if (inner_radii.x < 0.0f) {
                inner_radii.x = 0.0f;
        }

        if (inner_radii.y < 0.0f) {
                inner_radii.y = 0.0f;
        }

        secure_geometry_vertex_capacity(geometry, geometry->vertex_count + (resolution + 1ULL) * 2ULL);
        secure_geometry_index_capacity(geometry, geometry->index_count + resolution * 6ULL);

        const uint16_t start_index = (uint16_t)geometry->vertex_count;

        const float cos = cosf(rotation);
        const float sin = sinf(rotation);

        for (size_t index = 0ULL; index <= resolution; ++index) {
                const float angle = angle_1 + angle_span * (float)index / (float)resolution;

                const float x_outer = outer_radii.x * cosf(angle);
                const float y_outer = outer_radii.y * sinf(angle);

                const float x_inner = inner_radii.x * cosf(angle);
                const float y_inner = inner_radii.y * sinf(angle);

                const float rx_outer = x_outer * cos - y_outer * sin;
                const float ry_outer = x_outer * sin + y_outer * cos;

                const float rx_inner = x_inner * cos - y_inner * sin;
                const float ry_inner = x_inner * sin + y_inner * cos;

                add_geometry_vertex(geometry, cx + rx_outer, cy + ry_outer);
                add_geometry_vertex(geometry, cx + rx_inner, cy + ry_inner);

                if (index < resolution) {
                        const uint16_t base = start_index + (uint16_t)index * 2;

                        geometry->indices[geometry->index_count++] = base;
                        geometry->indices[geometry->index_count++] = base + 1;
                        geometry->indices[geometry->index_count++] = base + 2;
                        geometry->indices[geometry->index_count++] = base + 1;
                        geometry->indices[geometry->index_count++] = base + 3;
                        geometry->indices[geometry->index_count++] = base + 2;
                }
        }
}

static inline void write_arc_geometry(
        struct Geometry *const geometry,
        const float cx, const float cy,
        const float start_angle,
        const float end_angle,
        const size_t arc_points,
        const float radius
) {
        // TODO: Use 'write_elliptical_arc' or 'write_arc' instead of this
        for (size_t index = 0ULL; index < arc_points; ++index) {
                const float interpolation = (float)index / (float)(arc_points - 1ULL);
                const float angle = start_angle + (end_angle - start_angle) * interpolation;
                add_geometry_vertex(geometry, cx + radius * cosf(angle), cy + radius * sinf(angle));
        }
};

void write_rounded_rectangle_geometry(
        struct Geometry *const geometry,
        const float x, const float y,
        const float w, const float h,
        const float rounded_radius,
        const size_t corner_resolution
) {
        if (rounded_radius <= 0.0f || corner_resolution == 0ULL) {
                // TODO: Fallback to writing rectangle geometry
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

        const size_t arc_points = corner_resolution + 1ULL;
        const size_t outline_count = arc_points * 4ULL;

        secure_geometry_vertex_capacity(geometry, geometry->vertex_count + outline_count + 1ULL);
        secure_geometry_index_capacity(geometry, geometry->index_count + outline_count * 3ULL);

        const uint16_t start_index = (uint16_t)geometry->vertex_count;
        write_arc_geometry(geometry, x + half_w - radius, y - half_h + radius, -M_PI_2, 0.0f,            arc_points, radius); // Center Top Right
        write_arc_geometry(geometry, x + half_w - radius, y + half_h - radius, 0.0f, M_PI_2,             arc_points, radius); // Center Bottom Right
        write_arc_geometry(geometry, x - half_w + radius, y + half_h - radius, M_PI_2, M_PI,             arc_points, radius); // Center Bottom Left
        write_arc_geometry(geometry, x - half_w + radius, y - half_h + radius, M_PI, 1.5f * (float)M_PI, arc_points, radius); // Center Top Left

        const uint16_t center_index = add_geometry_vertex(geometry, x, y);
        for (size_t index = 0ULL; index < (size_t)outline_count; ++index) {
                const uint16_t next_index = (uint16_t)(index + 1ULL) % (uint16_t)outline_count;

                geometry->indices[geometry->index_count++] = center_index;
                geometry->indices[geometry->index_count++] = start_index + index;
                geometry->indices[geometry->index_count++] = start_index + next_index;
        }
}