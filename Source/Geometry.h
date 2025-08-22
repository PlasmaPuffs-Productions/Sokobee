#pragma once

#include <stdlib.h>
#include <stdbool.h>

#include <SDL.h>

#define HEXAGON_VERTEX_COUNT 6ULL
#define HEXAGON_INDEX_COUNT 12ULL

void track_geometry_data(void);
void get_tracked_geometry_data(size_t *const out_vertex_count, size_t *const out_index_count);

struct Geometry;

struct Geometry *_create_geometry(const size_t vertex_capacity, const size_t index_capacity);
void _destroy_geometry(struct Geometry *const geometry);

#ifdef NDEBUG

static inline void flush_geometry_leaks(void) {
        return;
}

#define create_geometry(vertex_capacity, index_capacity) _create_geometry((vertex_capacity), (index_capacity))
#define destroy_geometry(geometry)                       _destroy_geometry((geometry))

#else

void flush_geometry_leaks(void);

struct Geometry *_create_tracked_geometry(const size_t vertex_capacity, const size_t index_capacity, const char *const file, const size_t line);
void _destroy_tracked_geometry(struct Geometry *const geometry,                                      const char *const file, const size_t line);

#define create_geometry(vertex_capacity, index_capacity) _create_tracked_geometry((vertex_capacity), (index_capacity), __FILE__, (size_t)__LINE__)
#define destroy_geometry(geometry)                       _destroy_tracked_geometry((geometry),                         __FILE__, (size_t)__LINE__)

#endif

void clear_geometry(struct Geometry *const geometry);
void set_geometry_color(struct Geometry *const geometry, const SDL_Color color);
void render_geometry(const struct Geometry *const geometry);

void write_triangle_geometry(struct Geometry *const geometry, const SDL_FPoint vertices[3]);
void write_quad_geometry(struct Geometry *const geometry, const SDL_FPoint vertices[4]);
void write_rectangle_geometry(struct Geometry *const geometry, const SDL_FRect rectangle, const float rotation);
void write_circle_geometry(struct Geometry *const geometry, const SDL_FPoint position, const float radius, const size_t resolution);
void write_ellipse_geometry(struct Geometry *const geometry, const SDL_FPoint position, const SDL_FPoint radii, const float rotation, const size_t resolution);
void write_hexagon_geometry(struct Geometry *const geometry, const SDL_FPoint position, const float radius, const float rotation);
void write_bezier_curve_geometry(struct Geometry *const geometry, const SDL_FPoint endpoints[2], const SDL_FPoint control_points[2], const float line_width, const size_t resolution);
void write_rounded_triangle_geometry(struct Geometry *const geometry, const SDL_FPoint vertices[3], const float rounded_radius, const size_t arc_resolution);
void write_arc_outline_geometry(struct Geometry *const geometry, const SDL_FPoint position, const float radius, const float line_width, const float start_angle, const float end_angle, const bool clockwise, const size_t resolution);
void write_elliptical_arc_outline_geometry(struct Geometry *const geometry, const SDL_FPoint position, const SDL_FPoint radii, const float line_width, const float start_angle, const float end_angle, const bool clockwise, const float rotation, const size_t resolution);
void write_rounded_rectangle_geometry(struct Geometry *const geometry, const SDL_FRect rectangle, const float rounded_radius, const size_t corner_resolution);