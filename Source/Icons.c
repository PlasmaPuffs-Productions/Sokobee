#include "Icons.h"

#include <SDL.h>

#include "Geometry.h"
#include "Utilities.h"

struct Icon {
        enum IconType type;
        float size;
        float x;
        float y;
        float rotation;
        struct Geometry *geometry;
        bool outdated_geometry;
};

static void write_play_icon_geometry(struct Icon *);
static void write_undo_icon_geometry(struct Icon *);
static void write_redo_icon_geometry(struct Icon *);
static void write_restart_icon_geometry(struct Icon *);
static void write_exit_icon_geometry(struct Icon *);

static void(*icon_geometry_writers[ICON_COUNT])(struct Icon *) = {
        [ICON_PLAY] = write_play_icon_geometry,
        [ICON_UNDO] = write_undo_icon_geometry,
        [ICON_REDO] = write_redo_icon_geometry,
        [ICON_RESTART] = write_restart_icon_geometry,
        [ICON_EXIT] = write_exit_icon_geometry
};

struct Icon *create_icon(const enum IconType type, const float size, const float x, const float y, const SDL_Color color) {
        struct Icon *const icon = (struct Icon *)xmalloc(sizeof(struct Icon));
        icon->type = type;
        icon->size = size;
        icon->x = x;
        icon->y = y;
        icon->rotation = 0.0f;
        icon->geometry = create_geometry(0ULL, 0ULL);
        set_geometry_color(icon->geometry, color);

        icon_geometry_writers[type](icon);
        icon->outdated_geometry = false;
        return icon;
}

void destroy_icon(struct Icon *const icon) {
        if (!icon) {
                send_message(WARNING, "Icon given to destroy is NULL");
                return;
        }

        destroy_geometry(icon->geometry);
        xfree(icon);
}

void update_icon(struct Icon *const icon) {
        if (icon->outdated_geometry) {
                icon_geometry_writers[icon->type](icon);
                icon->outdated_geometry = false;
        }

        render_geometry(icon->geometry);
}

void set_icon_type(struct Icon *const icon, const enum IconType type) {
        if (icon->type == type) {
                return;
        }

        icon->type = type;
        icon->outdated_geometry = true;
}

void set_icon_size(struct Icon *const icon, const float size) {
        if (icon->size == size) {
                return;
        }

        icon->size = size;
        icon->outdated_geometry = true;
}

void set_icon_position(struct Icon *const icon, const float x, const float y) {
        if (icon->x == x && icon->y == y) {
                return;
        }

        icon->x = x;
        icon->y = y;
        icon->outdated_geometry = true;
}

void set_icon_color(struct Icon *const icon, const SDL_Color color) {
        set_geometry_color(icon->geometry, color);
        icon->outdated_geometry = true;
}

void set_icon_rotation(struct Icon *const icon, const float rotation) {
        if (icon->rotation == rotation) {
                return;
        }

        icon->rotation = rotation;
        icon->outdated_geometry = true;
}

static inline SDL_FPoint transform_icon_point(struct Icon *const icon, const SDL_FPoint point) {
        float x = icon->x + icon->size * (point.x - 0.5f);
        float y = icon->y + icon->size * (point.y - 0.5f);
        rotate_point(&x, &y, icon->x, icon->y, icon->rotation);

        return (SDL_FPoint){.x = x, .y = y};
}

static void write_play_icon_geometry(struct Icon *const icon) {
        const SDL_FPoint triangle_vertices[] = {
                transform_icon_point(icon, (SDL_FPoint){0.75f, 0.0f}),
                transform_icon_point(icon, (SDL_FPoint){0.0f, 0.5f}),
                transform_icon_point(icon, (SDL_FPoint){0.75f, 1.0f})
        };

        clear_geometry(icon->geometry);
        write_rounded_triangle_geometry(icon->geometry, triangle_vertices, icon->size / 15.0f, 8ULL);
}

static void write_undo_icon_geometry(struct Icon *const icon) {
        const SDL_FPoint triangle_vertices[] = {
                transform_icon_point(icon, (SDL_FPoint){0.4f, 0.15f}),
                transform_icon_point(icon, (SDL_FPoint){0.0f, 0.4f}),
                transform_icon_point(icon, (SDL_FPoint){0.4f, 0.65f})
        };

        const SDL_FPoint curve_endpoints[] = {
                transform_icon_point(icon, (SDL_FPoint){0.4f, 0.4f}),
                transform_icon_point(icon, (SDL_FPoint){0.8f, 0.8f})
        };

        const SDL_FPoint curve_control_points[] = {
                transform_icon_point(icon, (SDL_FPoint){0.65f, 0.4f}),
                transform_icon_point(icon, (SDL_FPoint){0.8f, 0.55f})
        };

        clear_geometry(icon->geometry);
        write_rounded_triangle_geometry(icon->geometry, triangle_vertices, icon->size / 20.0f, 8ULL);
        write_bezier_curve_geometry(icon->geometry, curve_endpoints, curve_control_points, icon->size / 10.0f, 32ULL);
        write_circle_geometry(icon->geometry, curve_endpoints[1], icon->size / 20.0f, 8ULL);
}

static void write_redo_icon_geometry(struct Icon *const icon) {
        const SDL_FPoint triangle_vertices[] = {
                transform_icon_point(icon, (SDL_FPoint){1.0f - 0.4f, 0.15f}),
                transform_icon_point(icon, (SDL_FPoint){1.0f - 0.0f, 0.4f}),
                transform_icon_point(icon, (SDL_FPoint){1.0f - 0.4f, 0.65f})
        };

        const SDL_FPoint curve_endpoints[] = {
                transform_icon_point(icon, (SDL_FPoint){1.0f - 0.4f, 0.4f}),
                transform_icon_point(icon, (SDL_FPoint){1.0f - 0.8f, 0.8f})
        };

        const SDL_FPoint curve_control_points[] = {
                transform_icon_point(icon, (SDL_FPoint){1.0f - 0.65f, 0.4f}),
                transform_icon_point(icon, (SDL_FPoint){1.0f - 0.8f, 0.55f})
        };

        clear_geometry(icon->geometry);
        write_rounded_triangle_geometry(icon->geometry, triangle_vertices, icon->size / 20.0f, 8ULL);
        write_bezier_curve_geometry(icon->geometry, curve_endpoints, curve_control_points, icon->size / 10.0f, 32ULL);
        write_circle_geometry(icon->geometry, curve_endpoints[1], icon->size / 20.0f, 8ULL);
}

static void write_restart_icon_geometry(struct Icon *const icon) {
        const SDL_FPoint triangle_vertices[] = {
                transform_icon_point(icon, (SDL_FPoint){1.0f - 0.25f, 0.0f}),
                transform_icon_point(icon, (SDL_FPoint){1.0f - 0.0f, 0.4f}),
                transform_icon_point(icon, (SDL_FPoint){1.0f - 0.4f, 0.45f})
        };

        const SDL_FPoint arc_endpoint = {
                .x = icon->x + icon->size / 3.0f * cosf((float)M_PI / 8.0f),
                .y = icon->y + icon->size / 3.0f * sinf((float)M_PI / 8.0f)
        };

        clear_geometry(icon->geometry);
        write_rounded_triangle_geometry(icon->geometry, triangle_vertices, icon->size / 25.0f, 8ULL);
        write_arc_outline_geometry(icon->geometry, (SDL_FPoint){icon->x, icon->y}, icon->size / 3.0f, icon->size / 10.0f, (float)M_PI / -4.0f, (float)M_PI / 8.0f, true, 32ULL);
        write_circle_geometry(icon->geometry, arc_endpoint, icon->size / 20.0f, 8ULL);
}

static void write_exit_icon_geometry(struct Icon *const icon) {
        const float line_width = icon->size / 10.0f;

        const SDL_FPoint top_left = transform_icon_point(icon, (SDL_FPoint){0.15f, 0.15f});
        const SDL_FPoint top_right = transform_icon_point(icon, (SDL_FPoint){0.6f, 0.15f});
        const SDL_FPoint bottom_left = transform_icon_point(icon, (SDL_FPoint){0.15f, 0.85f});
        const SDL_FPoint bottom_right = transform_icon_point(icon, (SDL_FPoint){0.6f, 0.85f});
        const SDL_FPoint top_opening = transform_icon_point(icon, (SDL_FPoint){0.6f, 0.35f});
        const SDL_FPoint bottom_opening = transform_icon_point(icon, (SDL_FPoint){0.6f, 0.65f});
        const SDL_FPoint center = transform_icon_point(icon, (SDL_FPoint){0.15f + (0.6f - 0.15f) / 2.0f, 0.15f + (0.85f - 0.15f) / 2.0f});

        const SDL_FPoint triangle_vertices[] = {
                transform_icon_point(icon, (SDL_FPoint){0.75f, 0.25f}),
                transform_icon_point(icon, (SDL_FPoint){1.0f, 0.5f}),
                transform_icon_point(icon, (SDL_FPoint){0.75f, 0.75f})
        };

        const float width = top_right.x - top_left.x;
        const float height = bottom_left.y - top_left.y;
        const float lower_height = top_opening.y - top_right.y;

        const SDL_FRect top_line = (SDL_FRect){
                .x = top_left.x + width / 2.0f,
                .y = top_left.y,
                .w = width,
                .h = line_width
        };

        const SDL_FRect bottom_line = (SDL_FRect){
                .x = top_left.x + width / 2.0f,
                .y = bottom_left.y,
                .w = width,
                .h = line_width
        };

        const SDL_FRect left_line = (SDL_FRect){
                .x = top_left.x,
                .y = top_left.y + height / 2.0f,
                .w = line_width,
                .h = height
        };

        const SDL_FRect top_right_line = (SDL_FRect){
                .x = top_right.x,
                .y = top_right.y + lower_height / 2.0f,
                .w = line_width,
                .h = lower_height
        };

        const SDL_FRect bottom_right_line = (SDL_FRect){
                .x = bottom_right.x,
                .y = bottom_right.y - lower_height / 2.0f,
                .w = line_width,
                .h = lower_height
        };

        const SDL_FRect arrow_line = (SDL_FRect){
                .x = top_right.x,
                .y = center.y,
                .w = width,
                .h = line_width
        };

        clear_geometry(icon->geometry);
        write_rectangle_geometry(icon->geometry, top_line, 0.0f);
        write_rectangle_geometry(icon->geometry, bottom_line, 0.0f);
        write_rectangle_geometry(icon->geometry, left_line, 0.0f);
        write_rectangle_geometry(icon->geometry, top_right_line, 0.0f);
        write_rectangle_geometry(icon->geometry, bottom_right_line, 0.0f);
        write_rectangle_geometry(icon->geometry, arrow_line, 0.0f);
        write_circle_geometry(icon->geometry, center, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, top_left, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, top_right, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, bottom_left, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, bottom_right, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, top_opening, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, bottom_opening, line_width / 2.0f, 8ULL);
        write_rounded_triangle_geometry(icon->geometry, triangle_vertices, line_width / 2.0f, 8ULL);
}