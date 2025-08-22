#pragma once

#include <stdlib.h>
#include <stdbool.h>

bool load_assets(const char *const path);
void unload_assets(void);

// ================================================================================================
// Fonts
// ================================================================================================

#define FONT_COUNT 7ULL
enum Font {
        FONT_TITLE,
        FONT_HEADER_1,
        FONT_HEADER_2,
        FONT_HEADER_3,
        FONT_BODY,
        FONT_CAPTION,
        FONT_DEBUG,
};

typedef struct TTF_Font TTF_Font;
TTF_Font *get_font(const enum Font font);

// ================================================================================================
// Levels
// ================================================================================================

struct LevelMetadata {
        char *title;
        char *path;
};

size_t get_level_count(void);

const struct LevelMetadata *get_level_metadata(const size_t level);

// ================================================================================================
// Sounds
// ================================================================================================

#define SOUND_COUNT 8
enum Sound {
        SOUND_CLICK,
        SOUND_HIT,
        SOUND_MOVE,
        SOUND_PLACED,
        SOUND_PUSH,
        SOUND_TURN,
        SOUND_UNDO,
        SOUND_WIN
};

void play_sound(const enum Sound sound);