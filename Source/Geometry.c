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

#define DEFAULT_VERTEX_CAPACITY 32ULL
#define DEFAULT_INDEX_CAPACITY 64ULL
#define DEFAULT_TEXTURE_COORDINATES (SDL_FPoint){1.0f, 1.0f}

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
        SDL_Vertex *vertices;
        size_t vertex_count;
        size_t vertex_capacity;
        int *indices;
        size_t index_count;
        size_t index_capacity;
        SDL_Color color;
};

static const int hexagon_indices[HEXAGON_INDEX_COUNT] = {
        0, 1, 2,
	0, 2, 3,
	0, 3, 4,
	0, 4, 5
};

struct Geometry *_create_geometry(const size_t vertex_capacity, const size_t index_capacity) {
        struct Geometry *const geometry = (struct Geometry *)xcalloc(1ULL, sizeof(struct Geometry));
        geometry->vertex_capacity = vertex_capacity ? vertex_capacity : DEFAULT_VERTEX_CAPACITY;
        geometry->index_capacity = index_capacity ? index_capacity : DEFAULT_INDEX_CAPACITY;
        geometry->vertices = (SDL_Vertex *)xmalloc(sizeof(SDL_Vertex) * geometry->vertex_capacity);
        geometry->indices = (int *)xmalloc(sizeof(int) * geometry->index_capacity);
        return geometry;
}

void _destroy_geometry(struct Geometry *const geometry) {
        if (geometry == NULL) {
                send_message(WARNING, "Geometry given to destroy is NULL");
                return;
        }

        xfree(geometry->vertices);
        xfree(geometry->indices);
        xfree(geometry);
}

#ifndef NDEBUG

struct GeometryInformation {
        struct Geometry *geometry;
        const char *file;
        size_t line;
        struct GeometryInformation *next;
};

static struct GeometryInformation *geometry_information_head = NULL;
static size_t active_geometry_count = 0ULL;

void flush_geometry_leaks(void) {
        if (geometry_information_head == NULL) {
                fprintf(stdout, "flush_geometry_leaks(): No leaked geometry\n");
                fflush(stdout);
                return;
        }

        struct GeometryInformation *current_geometry_information = geometry_information_head;
        fprintf(stderr, "flush_geometry_leaks(): %zu geometry leaks found\n", active_geometry_count);
        while (current_geometry_information != NULL) {
                struct GeometryInformation *geometry_information = current_geometry_information;
                fprintf(stderr, "flush_geometry_leaks(): Leaked geometry %p created at %s:%zu\n", (void *)geometry_information->geometry, geometry_information->file, geometry_information->line);
                current_geometry_information = geometry_information->next;
                xfree(geometry_information);
        }

        fflush(stderr);
}

struct Geometry *_create_tracked_geometry(const size_t vertex_capacity, const size_t index_capacity, const char *const file, const size_t line) {
        struct Geometry *const geometry = _create_geometry(vertex_capacity, index_capacity);

        struct GeometryInformation *geometry_information = xmalloc(sizeof(struct GeometryInformation));
        geometry_information->geometry = geometry;
        geometry_information->file = file;
        geometry_information->line = line;
        geometry_information->next = geometry_information_head;
        geometry_information_head = geometry_information;

        ++active_geometry_count;
        return geometry;
}

void _destroy_tracked_geometry(struct Geometry *const geometry, const char *const file, const size_t line) {
        if (geometry == NULL) {
                return;
        }

        _destroy_geometry(geometry);

        struct GeometryInformation **current_geometry_information = &geometry_information_head;
        while (*current_geometry_information != NULL) {
                if ((*current_geometry_information)->geometry == geometry) {
                        struct GeometryInformation *geometry_information = *current_geometry_information;
                        *current_geometry_information = geometry_information->next;
                        xfree(geometry_information);
                        --active_geometry_count;
                        return;
                }

                current_geometry_information = &(*current_geometry_information)->next;
        }

        fprintf(stderr, "WARNING: Destroying untracked or freed geometry %p at %s:%zu\n", geometry, file, line);
        fflush(stderr);
}

#endif

void clear_geometry(struct Geometry *const geometry) {
        geometry->index_count = 0ULL;
        geometry->vertex_count = 0ULL;
}

void set_geometry_color(struct Geometry *const geometry, const SDL_Color color) {
        geometry->color = color;
}

void render_geometry(const struct Geometry *const geometry) {
        tracked_vertex_count += geometry->vertex_count;
        tracked_index_count += geometry->index_count;
        SDL_RenderGeometry(get_context_renderer(), NULL, geometry->vertices, (int)geometry->vertex_count, geometry->indices, (int)geometry->index_count);
}

static void secure_capacity(void **const buffer, size_t *const current_capacity, const size_t required_capacity, const size_t element_size) {
        if (required_capacity <= *current_capacity) {
                return;
        }

        size_t next_capacity = *current_capacity * 2ULL;
        while (next_capacity < required_capacity) {
                next_capacity *= 2ULL;
        }

        *buffer = xrealloc(*buffer, next_capacity * element_size);
        *current_capacity = next_capacity;
}

void write_triangle_geometry(struct Geometry *const geometry, const SDL_FPoint vertices[3]) {
        secure_capacity((void **)&geometry->vertices, &geometry->vertex_capacity, geometry->vertex_count + 3ULL, sizeof(SDL_Vertex));
        secure_capacity((void **)&geometry->indices, &geometry->index_capacity, geometry->index_count + 3ULL, sizeof(int));

        geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){.position = vertices[0], .color = geometry->color, .tex_coord = DEFAULT_TEXTURE_COORDINATES};
        geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){.position = vertices[1], .color = geometry->color, .tex_coord = DEFAULT_TEXTURE_COORDINATES};
        geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){.position = vertices[2], .color = geometry->color, .tex_coord = DEFAULT_TEXTURE_COORDINATES};

        geometry->indices[geometry->index_count++] = (int)geometry->vertex_count - 3;
        geometry->indices[geometry->index_count++] = (int)geometry->vertex_count - 2;
        geometry->indices[geometry->index_count++] = (int)geometry->vertex_count - 1;
}

void write_quad_geometry(struct Geometry *const geometry, const SDL_FPoint vertices[4]) {
        secure_capacity((void **)&geometry->vertices, &geometry->vertex_capacity, geometry->vertex_count + 4ULL, sizeof(SDL_Vertex));
        secure_capacity((void **)&geometry->indices, &geometry->index_capacity, geometry->index_count + 6ULL, sizeof(int));

        const int start_index = (int)geometry->vertex_count;

        geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){.position = vertices[0], .color = geometry->color, .tex_coord = DEFAULT_TEXTURE_COORDINATES};
        geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){.position = vertices[1], .color = geometry->color, .tex_coord = DEFAULT_TEXTURE_COORDINATES};
        geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){.position = vertices[2], .color = geometry->color, .tex_coord = DEFAULT_TEXTURE_COORDINATES};
        geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){.position = vertices[3], .color = geometry->color, .tex_coord = DEFAULT_TEXTURE_COORDINATES};

        geometry->indices[geometry->index_count++] = start_index + 0;
        geometry->indices[geometry->index_count++] = start_index + 1;
        geometry->indices[geometry->index_count++] = start_index + 2;

        geometry->indices[geometry->index_count++] = start_index + 0;
        geometry->indices[geometry->index_count++] = start_index + 2;
        geometry->indices[geometry->index_count++] = start_index + 3;
}

void write_rectangle_geometry(struct Geometry *const geometry, const SDL_FRect rectangle, const float rotation) {
        SDL_FPoint vertices[4] = {
                (SDL_FPoint){rectangle.x - rectangle.w / 2.0f, rectangle.y - rectangle.h / 2.0f}, // Top Left
                (SDL_FPoint){rectangle.x + rectangle.w / 2.0f, rectangle.y - rectangle.h / 2.0f}, // Top Right
                (SDL_FPoint){rectangle.x + rectangle.w / 2.0f, rectangle.y + rectangle.h / 2.0f}, // Bottom Right
                (SDL_FPoint){rectangle.x - rectangle.w / 2.0f, rectangle.y + rectangle.h / 2.0f}, // Bottom Left
        };

        rotate_point(&vertices[0].x, &vertices[0].y, rectangle.x, rectangle.y, rotation);
        rotate_point(&vertices[1].x, &vertices[1].y, rectangle.x, rectangle.y, rotation);
        rotate_point(&vertices[2].x, &vertices[2].y, rectangle.x, rectangle.y, rotation);
        rotate_point(&vertices[3].x, &vertices[3].y, rectangle.x, rectangle.y, rotation);
        write_quad_geometry(geometry, vertices);
}

void write_circle_geometry(struct Geometry *const geometry, const SDL_FPoint position, const float radius, const size_t resolution) {
        if (!resolution) {
                send_message(WARNING, "Writing circlee geometry with resolution of zero");
                return;
        }

        write_ellipse_geometry(geometry, position, (SDL_FPoint){radius, radius}, 0.0f, resolution);
}

void write_ellipse_geometry(struct Geometry *const geometry, const SDL_FPoint position, const SDL_FPoint radii, const float rotation, const size_t resolution) {
        if (!resolution) {
                send_message(WARNING, "Writing ellipse geometry with resolution of zero");
                return;
        }

        secure_capacity((void **)&geometry->vertices, &geometry->vertex_capacity, geometry->vertex_count + resolution + 1ULL, sizeof(SDL_Vertex));
        secure_capacity((void **)&geometry->indices, &geometry->index_capacity, geometry->index_count + resolution * 3ULL, sizeof(int));

        const int start_index = (int)geometry->vertex_count;

        geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){
                .position = position,
                .color = geometry->color,
                .tex_coord = DEFAULT_TEXTURE_COORDINATES
        };

        for (size_t index = 0ULL; index < resolution; ++index) {
                const float angle = (float)index / (float)resolution * 2.0f * (float)M_PI;

                float x = radii.x * cosf(angle);
                float y = radii.y * sinf(angle);

                if (rotation != 0.0f) {
                        const float sine = sinf(rotation);
                        const float cosine = cosf(rotation);
                        const float v = x;
                        x = v * cosine - y * sine;
                        y = v * sine + y * cosine;
                }

                geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){
                        .color = geometry->color,
                        .tex_coord = DEFAULT_TEXTURE_COORDINATES,
                        .position = (SDL_FPoint){position.x + x, position.y + y}
                };
        }

        for (size_t index = 0; index < resolution; ++index) {
                geometry->indices[geometry->index_count++] = start_index;
                geometry->indices[geometry->index_count++] = start_index + 1 + (int)index;
                geometry->indices[geometry->index_count++] = start_index + 1 + (((int)index + 1) % resolution);
        }
}

void write_hexagon_geometry(struct Geometry *const geometry, const SDL_FPoint position, const float radius, const float rotation) {
        secure_capacity((void **)&geometry->vertices, &geometry->vertex_capacity, geometry->vertex_count + HEXAGON_VERTEX_COUNT, sizeof(SDL_Vertex));
        secure_capacity((void **)&geometry->indices, &geometry->index_capacity, geometry->index_count + HEXAGON_INDEX_COUNT, sizeof(int));

    	for (size_t index = 0ULL; index < HEXAGON_VERTEX_COUNT; ++index) {
        	const float angle = index * (float)M_PI / 3.0f + rotation;
		geometry->vertices[geometry->vertex_count + index] = (SDL_Vertex){
			.color = geometry->color,
			.tex_coord = DEFAULT_TEXTURE_COORDINATES,
			.position = (SDL_FPoint){
				.x = position.x + radius * cosf(angle),
				.y = position.y + radius * sinf(angle)
			}
		};
	}

	for (size_t index = 0ULL; index < HEXAGON_INDEX_COUNT; ++index) {
		geometry->indices[geometry->index_count + index] = (int)geometry->vertex_count + hexagon_indices[index];
	}

        geometry->vertex_count += HEXAGON_VERTEX_COUNT;
	geometry->index_count += HEXAGON_INDEX_COUNT;
}

static SDL_FPoint compute_bezier_point(const float interpolation, const SDL_FPoint endpoints[2], const SDL_FPoint control_points[2]);
void write_bezier_curve_geometry(struct Geometry *const geometry, const SDL_FPoint endpoints[2], const SDL_FPoint control_points[2], const float line_width, const size_t resolution) {
        if (!resolution) {
                send_message(WARNING, "Writing bezier curve geometry with resolution of zero");
                return;
        }

        const int start_index = (int)geometry->vertex_count;
        const float half_line_width = line_width * 0.5f;

        SDL_FPoint previous_point = compute_bezier_point(0.0f, endpoints, control_points);
        for (size_t point_index = 1ULL; point_index <= resolution; ++point_index) {
                const float interpolation = (float)point_index / (float)resolution;
                const SDL_FPoint next_point = compute_bezier_point(interpolation, endpoints, control_points);

                const float dx = next_point.x - previous_point.x;
                const float dy = next_point.y - previous_point.y;
                const float length = sqrtf(dx * dx + dy * dy);

                if (length == 0.0f) {
                        previous_point = next_point;
                        continue;
                }

                const SDL_FRect rectangle = (SDL_FRect){
                        .x = (previous_point.x + next_point.x) / 2.0f,
                        .y = (previous_point.y + next_point.y) / 2.0f,
                        .w = length + line_width / 10.0f,
                        .h = line_width
                };

                write_rectangle_geometry(geometry, rectangle, atan2f(dy, dx));
                previous_point = next_point;
        }
}

static SDL_FPoint compute_bezier_point(const float interpolation, const SDL_FPoint endpoints[2], const SDL_FPoint control_points[2]) {
        const float t = interpolation;
        const float u = 1.0f - t;
        const float tt = t * t;
        const float uu = u * u;
        const float uuu = uu * u;
        const float ttt = tt * t;

        return (SDL_FPoint){
                uuu * endpoints[0].x +
                3.0f * uu * t * control_points[0].x +
                3.0f * u * tt * control_points[1].x +
                ttt * endpoints[1].x,
                uuu * endpoints[0].y +
                3.0f * uu * t * control_points[0].y +
                3.0f * u * tt * control_points[1].y +
                ttt * endpoints[1].y
        };
}

static inline SDL_FPoint normalize(const SDL_FPoint point);
void write_rounded_triangle_geometry(struct Geometry *const geometry, const SDL_FPoint vertices[3], const float rounded_radius, const size_t arc_resolution) {
        if (!arc_resolution) {
                if (rounded_radius == 0.0f) {
                        send_message(WARNING, "Rounded triangle geometry has rounded radius of zero: Writing triangle geometry instead");
                        write_triangle_geometry(geometry, vertices);
                        return;
                }

                send_message(WARNING, "Writing rounded triangle geometry with arc resolution of zero");
                return;
        }

        SDL_FPoint counterclockwise_vertices[3];
        memcpy(counterclockwise_vertices, vertices, sizeof(counterclockwise_vertices));

        const float signed_area
                = (vertices[1].x - vertices[0].x) * (vertices[2].y - vertices[0].y)
                - (vertices[1].y - vertices[0].y) * (vertices[2].x - vertices[0].x);
        if (signed_area < 0.0f) {
                counterclockwise_vertices[1] = vertices[2];
                counterclockwise_vertices[2] = vertices[1];
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
                        const SDL_FPoint triangle[] = {arc_centers[corner_index], arc_points[point_index], arc_points[point_index + 1ULL]};
                        write_triangle_geometry(geometry, triangle);
                }

                arc_ends[corner_index][0] = arc_points[0];
                arc_ends[corner_index][1] = arc_points[resolution];
        }

        for (size_t edge_index = 0ULL; edge_index < 3ULL; ++edge_index) {
                const SDL_FPoint a = arc_ends[edge_index][1];
                const SDL_FPoint b = arc_ends[(edge_index + 1ULL) % 3ULL][0];

                SDL_FPoint midpoint = {
                        .x = (a.x + b.x) / 2.0f,
                        .y = (a.y + b.y) / 2.0f
                };

                const float dx = b.x - a.x;
                const float dy = b.y - a.y;
                const float length = sqrtf(dx * dx + dy * dy);
                const float angle = atan2f(dy, dx);

                const float nx = -dy / length;
                const float ny = dx / length;
                midpoint.x += nx * rounded_radius / 2.0f;
                midpoint.y += ny * rounded_radius / 2.0f;

                const SDL_FRect rect = {
                        .x = midpoint.x,
                        .y = midpoint.y,
                        .w = length,
                        .h = rounded_radius
                };

                write_rectangle_geometry(geometry, rect, angle);
        }

        write_triangle_geometry(geometry, arc_centers);
}

static inline SDL_FPoint normalize(const SDL_FPoint vector) {
        const float length = sqrtf(vector.x * vector.x + vector.y * vector.y);
        if (length == 0.0f) {
                return (SDL_FPoint){0.0f, 0.0f};
        }

        return (SDL_FPoint){vector.x / length, vector.y / length};
}

void write_arc_outline_geometry(struct Geometry *const geometry, const SDL_FPoint position, const float radius, const float line_width, const float start_angle, const float end_angle, const bool clockwise, const size_t resolution) {
        if (!resolution) {
                send_message(WARNING, "Writing arc outline geometry with resolution of zero");
                return;
        }

        write_elliptical_arc_outline_geometry(geometry, position, (SDL_FPoint){radius, radius}, line_width, start_angle, end_angle, clockwise, 0.0f, resolution);
}

void write_elliptical_arc_outline_geometry(struct Geometry *const geometry, const SDL_FPoint position, const SDL_FPoint radii, const float line_width, const float start_angle, const float end_angle, const bool clockwise, const float rotation, const size_t resolution) {
        if (!resolution) {
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
                radii.x - line_width * 0.5f,
                radii.y - line_width * 0.5f
        };
        SDL_FPoint outer_radii = {
                radii.x + line_width * 0.5f,
                radii.y + line_width * 0.5f
        };

        if (inner_radii.x < 0.0f) {
                inner_radii.x = 0.0f;
        }

        if (inner_radii.y < 0.0f) {
                inner_radii.y = 0.0f;
        }

        secure_capacity((void **)&geometry->vertices, &geometry->vertex_capacity, geometry->vertex_count + (resolution + 1) * 2ULL, sizeof(SDL_Vertex));
        secure_capacity((void **)&geometry->indices, &geometry->index_capacity, geometry->index_count + resolution * 6ULL, sizeof(int));

        const int start_index = (int)geometry->vertex_count;

        const float rotation_cos = cosf(rotation);
        const float rotation_sin = sinf(rotation);

        for (size_t index = 0ULL; index <= resolution; ++index) {
                const float angle = angle_1 + angle_span * (float)index / (float)resolution;

                const float x_outer = outer_radii.x * cosf(angle);
                const float y_outer = outer_radii.y * sinf(angle);

                const float x_inner = inner_radii.x * cosf(angle);
                const float y_inner = inner_radii.y * sinf(angle);

                const float rx_outer = x_outer * rotation_cos - y_outer * rotation_sin;
                const float ry_outer = x_outer * rotation_sin + y_outer * rotation_cos;

                const float rx_inner = x_inner * rotation_cos - y_inner * rotation_sin;
                const float ry_inner = x_inner * rotation_sin + y_inner * rotation_cos;

                geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){
                        .tex_coord = DEFAULT_TEXTURE_COORDINATES,
                        .color = geometry->color,
                        .position = {
                                .x = position.x + rx_outer,
                                .y = position.y + ry_outer
                        }
                };

                geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){
                        .tex_coord = DEFAULT_TEXTURE_COORDINATES,
                        .color = geometry->color,
                        .position = {
                                .x = position.x + rx_inner,
                                .y = position.y + ry_inner
                        }
                };

                if (index < resolution) {
                        const int base = start_index + (int)index * 2;
                        geometry->indices[geometry->index_count++] = base;
                        geometry->indices[geometry->index_count++] = base + 1;
                        geometry->indices[geometry->index_count++] = base + 2;
                        geometry->indices[geometry->index_count++] = base + 1;
                        geometry->indices[geometry->index_count++] = base + 3;
                        geometry->indices[geometry->index_count++] = base + 2;
                }
        }
}

static inline void add_arc(struct Geometry *const geometry, SDL_FPoint center, float start_angle, float end_angle, size_t arc_points, float radius) {
        for (size_t i = 0; i < arc_points; ++i) {
                float t = (float)i / (float)(arc_points - 1);
                float a = start_angle + (end_angle - start_angle) * t;
                float px = center.x + radius * cosf(a);
                float py = center.y + radius * sinf(a);
                geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){
                        .position = { px, py },
                        .color = geometry->color,
                        .tex_coord = DEFAULT_TEXTURE_COORDINATES
                };
        }
};

void write_rounded_rectangle_geometry(struct Geometry *const geometry, const SDL_FRect rectangle, const float rounded_radius, const size_t corner_resolution) {
        if (rounded_radius <= 0.0f || corner_resolution == 0ULL) {
                write_rectangle_geometry(geometry, rectangle, 0.0f);
                return;
        }

        float half_w = rectangle.w * 0.5f;
        float half_h = rectangle.h * 0.5f;

        float radius = rounded_radius;
        if (radius > half_w) {
                radius = half_w;
        }

        if (radius > half_h) {
                radius = half_h;
        }

        SDL_FPoint center_tl = { rectangle.x - half_w + radius, rectangle.y - half_h + radius };
        SDL_FPoint center_tr = { rectangle.x + half_w - radius, rectangle.y - half_h + radius };
        SDL_FPoint center_br = { rectangle.x + half_w - radius, rectangle.y + half_h - radius };
        SDL_FPoint center_bl = { rectangle.x - half_w + radius, rectangle.y + half_h - radius };


        const size_t arc_points = corner_resolution + 1ULL;
        const size_t outline_count = arc_points * 4ULL;

        secure_capacity((void**)&geometry->vertices, &geometry->vertex_capacity, geometry->vertex_count + outline_count + 1ULL, sizeof(SDL_Vertex));
        secure_capacity((void**)&geometry->indices, &geometry->index_capacity, geometry->index_count + outline_count * 3ULL, sizeof(int));

        int start_index = (int)geometry->vertex_count;
        add_arc(geometry, center_tr, -M_PI_2, 0.0f, arc_points, radius);
        add_arc(geometry, center_br, 0.0f, M_PI_2, arc_points, radius);
        add_arc(geometry, center_bl, M_PI_2, M_PI, arc_points, radius);
        add_arc(geometry, center_tl, M_PI, 1.5f * (float)M_PI, arc_points, radius);

        SDL_FPoint center_rect = {rectangle.x, rectangle.y};
        geometry->vertices[geometry->vertex_count++] = (SDL_Vertex){
                .position = center_rect,
                .color = geometry->color,
                .tex_coord = DEFAULT_TEXTURE_COORDINATES
        };

        int center_idx = (int)geometry->vertex_count - 1;

        for (int i = 0; i < (int)outline_count; ++i) {
                int next = (i + 1) % (int)outline_count;
                geometry->indices[geometry->index_count++] = center_idx;
                geometry->indices[geometry->index_count++] = start_index + i;
                geometry->indices[geometry->index_count++] = start_index + next;
        }
}