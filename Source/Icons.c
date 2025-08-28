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
static void write_sounds_on_icon_geometry(struct Icon *);
static void write_sounds_off_icon_geometry(struct Icon *);
static void write_music_on_icon_geometry(struct Icon *);
static void write_music_off_icon_geometry(struct Icon *);
static void write_exit_icon_geometry(struct Icon *);

static void(*icon_geometry_writers[ICON_COUNT])(struct Icon *) = {
        [ICON_PLAY]       = write_play_icon_geometry,
        [ICON_UNDO]       = write_undo_icon_geometry,
        [ICON_REDO]       = write_redo_icon_geometry,
        [ICON_RESTART]    = write_restart_icon_geometry,
        [ICON_SOUNDS_ON]  = write_sounds_on_icon_geometry,
        [ICON_SOUNDS_OFF] = write_sounds_off_icon_geometry,
        [ICON_MUSIC_ON]   = write_music_on_icon_geometry,
        [ICON_MUSIC_OFF]  = write_music_off_icon_geometry,
        [ICON_EXIT]       = write_exit_icon_geometry
};

struct Icon *create_icon(const enum IconType type) {
        struct Icon *const icon = (struct Icon *)xmalloc(sizeof(struct Icon));
        icon->type = type;

        icon->rotation = 0.0f;
        icon->size = 0.0f;
        icon->x = 0.0f;
        icon->y = 0.0f;

        icon->geometry = create_geometry();
        set_geometry_color(icon->geometry, COLOR_BROWN, COLOR_OPAQUE);

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
        write_rounded_triangle_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, icon->size / 15.0f);
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
        write_rounded_triangle_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, icon->size / 20.0f);
        write_bezier_curve_geometry(icon->geometry, px1, py1, px2, py2, cx1, cy1, cx2, cy2, icon->size / 10.0f);

        // Give the endpoint that isn't touching the triangle a round line cap
        write_circle_geometry(icon->geometry, px2, py2, icon->size / 20.0f);
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
        write_rounded_triangle_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, icon->size / 20.0f);
        write_bezier_curve_geometry(icon->geometry, px1, py1, px2, py2, cx1, cy1, cx2, cy2, icon->size / 10.0f);

        // Give the endpoint that isn't touching the triangle a round line cap
        write_circle_geometry(icon->geometry, px2, py2, icon->size / 20.0f);

#undef FLIP

}

static void write_restart_icon_geometry(struct Icon *const icon) {

#define FLIP 1.0f -

        // Tip Triangle Vertices
        float x1 = FLIP 0.25f, y1 = 0.0f, x2 = FLIP 0.0f, y2 = 0.4f, x3 = FLIP 0.4f, y3 = 0.45f;
        transform_icon_point(icon, &x1, &y1);
        transform_icon_point(icon, &x2, &y2);
        transform_icon_point(icon, &x3, &y3);

        clear_geometry(icon->geometry);
        write_rounded_triangle_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, icon->size / 25.0f);
        write_circular_arc_outline_geometry(
                icon->geometry,
                icon->x, icon->y,
                icon->size / 3.0f,
                icon->size / 10.0f,
                (float)M_PI / -4.0f,
                (float)M_PI / 8.0f,
                true,
                LINE_CAP_END
        );

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

        write_line_geometry(icon->geometry, tlx, tly + line_width / 2.0f, blx, bly - line_width / 2.0f, line_width, LINE_CAP_NONE);
        write_line_geometry(icon->geometry, tlx + line_width / 2.0f, tly, trx - line_width / 2.0f, try, line_width, LINE_CAP_NONE);
        write_line_geometry(icon->geometry, blx + line_width / 2.0f, bly, brx - line_width / 2.0f, bry, line_width, LINE_CAP_NONE);

        write_line_geometry(icon->geometry, trx, try + line_width / 2.0f, tox, toy - line_width / 2.0f, line_width, LINE_CAP_END);
        write_line_geometry(icon->geometry, brx, bry - line_width / 2.0f, box, boy + line_width / 2.0f, line_width, LINE_CAP_END);

        write_line_geometry(icon->geometry, cx, cy, cx - line_width / 2.0f + (trx - tlx), cy, line_width, LINE_CAP_START);

        write_circular_arc_geometry(icon->geometry, tlx + line_width / 2.0f, tly + line_width / 2.0f, line_width, -M_PI_2, M_PI, true);
        write_circular_arc_geometry(icon->geometry, blx + line_width / 2.0f, bly - line_width / 2.0f, line_width, -M_PI, M_PI_2, true);
        write_circular_arc_geometry(icon->geometry, trx - line_width / 2.0f, try + line_width / 2.0f, line_width, 0.0f, M_PI_2 * 3.0f, true);
        write_circular_arc_geometry(icon->geometry, brx - line_width / 2.0f, bry - line_width / 2.0f, line_width, M_PI_2, M_PI * 2.0f, true);

        write_rounded_triangle_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, line_width / 2.0f);
}

static inline void write_speaker_geometry(struct Icon *const icon) {
        float x1 = 0.10, y1 = 0.35;
        float x2 = 0.35, y2 = 0.35;
        float x3 = 0.35, y3 = 0.65;
        float x4 = 0.10, y4 = 0.65;
        transform_icon_point(icon, &x1, &y1);
        transform_icon_point(icon, &x2, &y2);
        transform_icon_point(icon, &x3, &y3);
        transform_icon_point(icon, &x4, &y4);

        float x5 = 0.15, y5 = 0.5;
        float x6 = 0.5,  y6 = 0.1;
        float x7 = 0.5,  y7 = 0.9;
        transform_icon_point(icon, &x5, &y5);
        transform_icon_point(icon, &x6, &y6);
        transform_icon_point(icon, &x7, &y7);

        const float rounded_radius = icon->size / 20.0f;

        write_rounded_quadrilateral_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, x4, y4, rounded_radius);
        write_rounded_triangle_geometry(     icon->geometry, x5, y5, x6, y6, x7, y7,         rounded_radius);
}

static void write_sounds_on_icon_geometry(struct Icon *const icon) {
        float x = 0.5, y = 0.5;
        transform_icon_point(icon, &x, &y);

        clear_geometry(icon->geometry);
        write_speaker_geometry(icon);

        static const float start_angle = M_PI_2 - M_PI_4;
        static const float end_angle   = M_PI + M_PI_2 + M_PI_4;

        const float radius_x   = icon->size / 10.0f;
        const float radius_y   = icon->size /  9.0f;
        const float line_width = icon->size / 10.0f;

        write_elliptical_arc_outline_geometry(icon->geometry, x, y, radius_x * 1.0f, radius_y * 1.0f, 0.0f, line_width, start_angle, end_angle, true, LINE_CAP_BOTH);
        write_elliptical_arc_outline_geometry(icon->geometry, x, y, radius_x * 2.5f, radius_y * 2.5f, 0.0f, line_width, start_angle, end_angle, true, LINE_CAP_BOTH);
        write_elliptical_arc_outline_geometry(icon->geometry, x, y, radius_x * 4.0f, radius_y * 4.0f, 0.0f, line_width, start_angle, end_angle, true, LINE_CAP_BOTH);
}

static void write_sounds_off_icon_geometry(struct Icon *const icon) {
        float x1 = 0.6, y1 = 0.35;
        float x2 = 0.9, y2 = 0.65;
        float x3 = 0.6, y3 = 0.65;
        float x4 = 0.9, y4 = 0.35;
        transform_icon_point(icon, &x1, &y1);
        transform_icon_point(icon, &x2, &y2);
        transform_icon_point(icon, &x3, &y3);
        transform_icon_point(icon, &x4, &y4);

        clear_geometry(icon->geometry);
        write_speaker_geometry(icon);

        const float line_width = icon->size / 10.0f;

        write_line_geometry(icon->geometry, x1, y1, x2, y2, line_width, LINE_CAP_BOTH);
        write_line_geometry(icon->geometry, x3, y3, x4, y4, line_width, LINE_CAP_BOTH);
}

static void write_music_note_geometry(struct Icon *const icon) {
        float x1 = 0.25, y1 = 0.2;
        float x2 = 0.25, y2 = 0.4;
        float x3 = 0.85, y3 = 0.3;
        float x4 = 0.85, y4 = 0.1;
        transform_icon_point(icon, &x1, &y1);
        transform_icon_point(icon, &x2, &y2);
        transform_icon_point(icon, &x3, &y3);
        transform_icon_point(icon, &x4, &y4);

        float x5 = 0.3, y5 = 0.25;
        float x6 = 0.3, y6 = 0.8;
        transform_icon_point(icon, &x5, &y5);
        transform_icon_point(icon, &x6, &y6);

        float x7 = 0.8, y7 = 0.15;
        float x8 = 0.8, y8 = 0.7;
        transform_icon_point(icon, &x7, &y7);
        transform_icon_point(icon, &x8, &y8);

        float x9  = 0.725, y9  = 0.7;
        float x10 = 0.225, y10 = 0.8;
        transform_icon_point(icon,  &x9,  &y9);
        transform_icon_point(icon, &x10, &y10);

        const float line_width     = icon->size / 10.0f;
        const float radius_x       = icon->size /  7.5f;
        const float radius_y       = icon->size / 10.0f;
        const float rounded_radius = icon->size / 20.0f;

        write_rounded_quadrilateral_geometry(icon->geometry, x1, y1, x2, y2, x3, y3, x4, y4, rounded_radius);

        write_line_geometry(icon->geometry, x5, y5, x6, y6, line_width, LINE_CAP_NONE);
        write_line_geometry(icon->geometry, x7, y7, x8, y8, line_width, LINE_CAP_NONE);

        write_ellipse_geometry(icon->geometry,  x9,  y9, radius_x, radius_y, M_PI_4 / -2.0f);
        write_ellipse_geometry(icon->geometry, x10, y10, radius_x, radius_y, M_PI_4 / -2.0f);
}

static void write_music_on_icon_geometry(struct Icon *const icon) {
        clear_geometry(icon->geometry);
        write_music_note_geometry(icon);
}

static void write_music_off_icon_geometry(struct Icon *const icon) {
        float x1 = 0.15, y1 = 0.15;
        float x2 = 0.85, y2 = 0.85;
        transform_icon_point(icon, &x1, &y1);
        transform_icon_point(icon, &x2, &y2);

        clear_geometry(icon->geometry);
        write_music_note_geometry(icon);

        const float line_width = icon->size / 10.0f;

        write_line_geometry(icon->geometry, x1, y1, x2, y2, line_width, LINE_CAP_BOTH);
}