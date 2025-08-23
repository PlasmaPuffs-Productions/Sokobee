#include "Icons.h"

#include <SDL.h>

#include "Geometry.h"
#include "Utilities.h"

struct Icon {
        enum IconType type;
        float rotation;
        float size;
        float x;
        float y;
        struct Geometry *geometry;
        bool outdated_geometry;
};

static void write_play_icon_geometry(struct Icon *);
static void write_undo_icon_geometry(struct Icon *);
static void write_redo_icon_geometry(struct Icon *);
static void write_restart_icon_geometry(struct Icon *);
static void write_exit_icon_geometry(struct Icon *);

static void(*icon_geometry_writers[ICON_COUNT])(struct Icon *) = {
        [ICON_PLAY]    = write_play_icon_geometry,
        [ICON_UNDO]    = write_undo_icon_geometry,
        [ICON_REDO]    = write_redo_icon_geometry,
        [ICON_RESTART] = write_restart_icon_geometry,
        [ICON_EXIT]    = write_exit_icon_geometry
};

struct Icon *create_icon(const enum IconType type) {
        struct Icon *const icon = (struct Icon *)xmalloc(sizeof(struct Icon));
        icon->type = type;

        icon->rotation = 0.0f;
        icon->size = 0.0f;
        icon->x = 0.0f;
        icon->y = 0.0f;

        icon->geometry = create_geometry();
        set_geometry_color(icon->geometry, COLOR_BROWN, 255);

        icon->outdated_geometry = true;
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

void set_icon_rotation(struct Icon *const icon, const float rotation) {
        if (icon->rotation == rotation) {
                return;
        }

        icon->rotation = rotation;
        icon->outdated_geometry = true;
}

static inline void transform_icon_point(struct Icon *const icon, float *const x, float *const y) {
        *x = icon->x + icon->size * (*x - 0.5f);
        *y = icon->y + icon->size * (*y - 0.5f);
        rotate_point(x, y, icon->x, icon->y, icon->rotation);
}

static void write_play_icon_geometry(struct Icon *const icon) {
        float x1 = 0.75f, y1 = 0.0f, x2 = 0.0f, y2 = 0.5f, x3 = 0.75f, y3 = 1.0f;
        transform_icon_point(icon, &x1, &y1);
        transform_icon_point(icon, &x2, &y2);
        transform_icon_point(icon, &x3, &y3);

        clear_geometry(icon->geometry);
        write_rounded_triangle_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, icon->size / 15.0f, 8ULL);
}

static void write_undo_icon_geometry(struct Icon *const icon) {
        // Tip Triangle Vertices
        float x1 = 0.4f, y1 = 0.15f, x2 = 0.0f, y2 = 0.4f, x3 = 0.4f, y3 = 0.65f;
        transform_icon_point(icon, &x1, &y1);
        transform_icon_point(icon, &x2, &y2);
        transform_icon_point(icon, &x3, &y3);

        // Bezier Curve Endpoints
        float px1 = 0.4f, py1 = 0.4f, px2 = 0.8f, py2 = 0.8f;
        transform_icon_point(icon, &px1, &py1);
        transform_icon_point(icon, &px2, &py2);

        // Bezier Curve Control Points
        float cx1 = 0.65f, cy1 = 0.4f, cx2 = 0.8f, cy2 = 0.55f;
        transform_icon_point(icon, &cx1, &cy1);
        transform_icon_point(icon, &cx2, &cy2);

        clear_geometry(icon->geometry);
        write_rounded_triangle_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, icon->size / 20.0f, 8ULL);
        write_bezier_curve_geometry(icon->geometry, px1, py1, px2, py2, cx1, cy1, cx2, cy2, icon->size / 10.0f, 32ULL);

        // Give the endpoint that isn't touching the triangle a round line cap
        write_circle_geometry(icon->geometry, px2, py2, icon->size / 20.0f, 8ULL);
}

static void write_redo_icon_geometry(struct Icon *const icon) {

#define FLIP 1.0f -

        // Tip Triangle Vertices
        float x1 = FLIP 0.4f, y1 = 0.15f, x2 = FLIP 0.0f, y2 = 0.4f, x3 = FLIP 0.4f, y3 = 0.65f;
        transform_icon_point(icon, &x1, &y1);
        transform_icon_point(icon, &x2, &y2);
        transform_icon_point(icon, &x3, &y3);

        // Bezier Curve Endpoints
        float px1 = FLIP 0.4f, py1 = 0.4f, px2 = FLIP 0.8f, py2 = 0.8f;
        transform_icon_point(icon, &px1, &py1);
        transform_icon_point(icon, &px2, &py2);

        // Bezier Curve Control Points
        float cx1 = FLIP 0.65f, cy1 = 0.4f, cx2 = FLIP 0.8f, cy2 = 0.55f;
        transform_icon_point(icon, &cx1, &cy1);
        transform_icon_point(icon, &cx2, &cy2);

        clear_geometry(icon->geometry);
        write_rounded_triangle_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, icon->size / 20.0f, 8ULL);
        write_bezier_curve_geometry(icon->geometry, px1, py1, px2, py2, cx1, cy1, cx2, cy2, icon->size / 10.0f, 32ULL);

        // Give the endpoint that isn't touching the triangle a round line cap
        write_circle_geometry(icon->geometry, px2, py2, icon->size / 20.0f, 8ULL);

#undef FLIP

}

static void write_restart_icon_geometry(struct Icon *const icon) {

#define FLIP 1.0f -

        // Tip Triangle Vertices
        float x1 = FLIP 0.25f, y1 = 0.0f, x2 = FLIP 0.0f, y2 = 0.4f, x3 = FLIP 0.4f, y3 = 0.45f;
        transform_icon_point(icon, &x1, &y1);
        transform_icon_point(icon, &x2, &y2);
        transform_icon_point(icon, &x3, &y3);

        // Arc Endpoint
        const float px = icon->x + icon->size / 3.0f * cosf((float)M_PI / 8.0f);
        const float py = icon->y + icon->size / 3.0f * sinf((float)M_PI / 8.0f);

        clear_geometry(icon->geometry);
        write_rounded_triangle_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, icon->size / 25.0f, 8ULL);
        write_arc_outline_geometry(icon->geometry, icon->x, icon->y, icon->size / 3.0f, icon->size / 10.0f, (float)M_PI / -4.0f, (float)M_PI / 8.0f, true, 32ULL);
        write_circle_geometry(icon->geometry, px, py, icon->size / 20.0f, 8ULL);

#undef FLIP

}

static void write_exit_icon_geometry(struct Icon *const icon) {
        const float line_width = icon->size / 10.0f;

        // tl - Top Left
        // tr - Top Right
        // bl - Bottom Left
        // br - Bottom Right
        // to - Top Opening
        // bo - Bottom Opening

        float tlx = 0.15f, tly = 0.15f;
        float trx = 0.6f,  try = 0.15f;
        float blx = 0.15f, bly = 0.85f;
        float brx = 0.6f,  bry = 0.85f;
        float tox = 0.6f,  toy = 0.35f;
        float box = 0.6f,  boy = 0.65f;
        transform_icon_point(icon, &tlx, &tly);
        transform_icon_point(icon, &trx, &try);
        transform_icon_point(icon, &blx, &bly);
        transform_icon_point(icon, &brx, &bry);
        transform_icon_point(icon, &tox, &toy);
        transform_icon_point(icon, &box, &boy);

        // Center
        float cx = 0.15f + (0.6f - 0.15f) / 2.0f, cy = 0.15f + (0.85f - 0.15f) / 2.0f;
        transform_icon_point(icon, &cx, &cy);

        // Tip Triangle Vertices
        float x1 = 0.75f, y1 = 0.25f, x2 = 1.0f, y2 = 0.5f, x3 = 0.75f, y3 = 0.75f;
        transform_icon_point(icon, &x1, &y1);
        transform_icon_point(icon, &x2, &y2);
        transform_icon_point(icon, &x3, &y3);

        const float width        = trx - tlx;
        const float height       = bly - tly;
        const float lower_height = toy - try;

        const SDL_FRect top_line = (SDL_FRect){
                .x = tlx + width / 2.0f,
                .y = tly,
                .w = width,
                .h = line_width
        };

        const SDL_FRect bottom_line = (SDL_FRect){
                .x = tlx + width / 2.0f,
                .y = bly,
                .w = width,
                .h = line_width
        };

        const SDL_FRect left_line = (SDL_FRect){
                .x = tlx,
                .y = tly + height / 2.0f,
                .w = line_width,
                .h = height
        };

        const SDL_FRect top_right_line = (SDL_FRect){
                .x = trx,
                .y = try + lower_height / 2.0f,
                .w = line_width,
                .h = lower_height
        };

        const SDL_FRect bottom_right_line = (SDL_FRect){
                .x = brx,
                .y = bry - lower_height / 2.0f,
                .w = line_width,
                .h = lower_height
        };

        const SDL_FRect arrow_line = (SDL_FRect){
                .x = trx,
                .y = cy,
                .w = width,
                .h = line_width
        };

        clear_geometry(icon->geometry);
        write_rectangle_geometry(icon->geometry, top_line.x, top_line.y, top_line.w, top_line.h, 0.0f);
        write_rectangle_geometry(icon->geometry, bottom_line.x, bottom_line.y, bottom_line.w, bottom_line.h, 0.0f);
        write_rectangle_geometry(icon->geometry, left_line.x, left_line.y, left_line.w, left_line.h, 0.0f);
        write_rectangle_geometry(icon->geometry, top_right_line.x, top_right_line.y, top_right_line.w, top_right_line.h, 0.0f);
        write_rectangle_geometry(icon->geometry, bottom_right_line.x, bottom_right_line.y, bottom_right_line.w, bottom_right_line.h, 0.0f);
        write_rectangle_geometry(icon->geometry, arrow_line.x, arrow_line.y, arrow_line.w, arrow_line.h, 0.0f);
        write_circle_geometry(icon->geometry, cx,cy, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, tlx, tly, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, trx, try, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, blx, bly, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, brx, bry, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, tox, toy, line_width / 2.0f, 8ULL);
        write_circle_geometry(icon->geometry, box, boy, line_width / 2.0f, 8ULL);
        write_rounded_triangle_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, line_width / 2.0f, 8ULL);
}