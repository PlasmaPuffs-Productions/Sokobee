#pragma once

#include "SDL.h"

enum IconType {
        ICON_PLAY,
        ICON_UNDO,
        ICON_REDO,
        ICON_EXIT,
        ICON_SOUNDS_ON,
        ICON_SOUNDS_OFF,
        ICON_MUSIC_ON,
        ICON_MUSIC_OFF,
        ICON_RESTART,
        ICON_COUNT
};

struct Icon;

struct Icon *create_icon(const enum IconType type);

void destroy_icon(struct Icon *const icon);

void update_icon(struct Icon *const icon);

void set_icon_type(struct Icon *const icon, const enum IconType type);

void set_icon_size(struct Icon *const icon, const float size);

void set_icon_position(struct Icon *const icon, const float x, const float y);

void set_icon_rotation(struct Icon *const icon, const float rotation);