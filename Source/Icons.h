#pragma once

#include <SDL.h>

#define ICON_COUNT 5ULL
enum IconType {
        ICON_PLAY,
        ICON_UNDO,
        ICON_REDO,
        ICON_EXIT,
        ICON_RESTART
};

struct Icon;

struct Icon *create_icon(const enum IconType type, const float size, const float x, const float y, const SDL_Color color);
void destroy_icon(struct Icon *const icon);

void update_icon(struct Icon *const icon);
void set_icon_type(struct Icon *const icon, const enum IconType type);
void set_icon_size(struct Icon *const icon, const float size);
void set_icon_position(struct Icon *const icon, const float x, const float y);
void set_icon_color(struct Icon *const icon, const SDL_Color color);
void set_icon_rotation(struct Icon *const icon, const float rotation);

/*
TODO:
Exit / Close ✕ (X)
Settings / Gear ⚙ (maybe more complex, could approximate with a hexagon + cuts)
Volume on/off 🔊 / 🔇 (speaker + wave or slash)
Music toggle 🎵 (maybe just a note shape)
Fullscreen toggle ⛶ (rectangle with arrows)
Info / About ℹ (circle with an ‘i’—can be a filled circle + small line)
Question mark ? (maybe simplified)
Menu (three vertical lines or 3x3 grid)
*/