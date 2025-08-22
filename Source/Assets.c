#include "Assets.h"

#include <stdlib.h>
#include <stdbool.h>

#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include "Context.h"
#include "Utilities.h"
#include "cJSON.h"

static bool load_fonts(const cJSON *const json);
static void unload_fonts(void);

static bool load_levels(const cJSON *const json);
static void unload_levels(void);

static bool load_sounds(const cJSON *const json);
static void unload_sounds(void);

bool load_assets(const char *const path) {
        send_message(INFORMATION, "Assets data file to load: \"%s\"", path);
        char *const json_string = load_text_file(path);
        if (!json_string) {
                send_message(ERROR, "Failed to load assets: Failed to load data file");
                return false;
        }

        cJSON *const json = cJSON_Parse(json_string);
        xfree(json_string);
        if (!json) {
                send_message(ERROR, "Failed to load assets: Failed to parse \"%s\" as JSON data: %s", path, cJSON_GetErrorPtr());
                return false;
        }

        if (!load_fonts((const void *)cJSON_GetObjectItemCaseSensitive(json, "fonts"))) {
                send_message(ERROR, "Failed to load assets: Failed to load fonts");
                cJSON_Delete(json);
                unload_assets();
                return false;
        }

        if (!load_levels((const void *)cJSON_GetObjectItemCaseSensitive(json, "levels"))) {
                send_message(ERROR, "Failed to load assets: Failed to load levels");
                cJSON_Delete(json);
                unload_assets();
                return false;
        }

        if (!load_sounds((const void *)cJSON_GetObjectItemCaseSensitive(json, "sounds"))) {
                send_message(ERROR, "Failed to load assets: Failed to load sounds");
                cJSON_Delete(json);
                unload_assets();
                return false;
        }

        cJSON_Delete(json);
        return true;
}

void unload_assets(void) {
        unload_fonts();
        unload_levels();
        unload_sounds();
}

// ================================================================================================
// Fonts
// ================================================================================================

static TTF_Font *fonts[FONT_COUNT];
static size_t font_sizes[FONT_COUNT] = {
        [FONT_TITLE]    = 48ULL,
        [FONT_HEADER_1] = 36ULL,
        [FONT_HEADER_2] = 24ULL,
        [FONT_HEADER_3] = 16ULL,
        [FONT_BODY]     = 16ULL,
        [FONT_CAPTION]  = 12ULL,
        [FONT_DEBUG]    = 16ULL,
};

static bool font_kerning_allowed[FONT_COUNT] = {
        [FONT_TITLE]    = true,
        [FONT_HEADER_1] = true,
        [FONT_HEADER_2] = true,
        [FONT_HEADER_3] = true,
        [FONT_BODY]     = true,
        [FONT_CAPTION]  = true,
        [FONT_DEBUG]    = true,
};

TTF_Font *get_font(const enum Font font) {
        return fonts[font];
}

static bool load_fonts(const cJSON *const json) {
        if (!cJSON_IsObject(json)) {
                send_message(ERROR, "Failed to load fonts: JSON data is invalid");
                return false;
        }

        const cJSON *const display_font_json = cJSON_GetObjectItemCaseSensitive(json, "display");
        const cJSON *const debug_font_json = cJSON_GetObjectItemCaseSensitive(json, "debug");
        const cJSON *const body_font_json = cJSON_GetObjectItemCaseSensitive(json, "body");

        if (!cJSON_IsString(display_font_json) || !cJSON_IsString(debug_font_json) || !cJSON_IsString(body_font_json)) {
                send_message(ERROR, "Failed to load fonts: JSON data is invalid");
                return false;
        }

        const char *font_paths[FONT_COUNT] = {
                [FONT_TITLE]    = display_font_json->valuestring,
                [FONT_HEADER_1] = display_font_json->valuestring,
                [FONT_HEADER_2] = display_font_json->valuestring,
                [FONT_HEADER_3] = display_font_json->valuestring,
                [FONT_BODY]     = body_font_json->valuestring,
                [FONT_CAPTION]  = body_font_json->valuestring,
                [FONT_DEBUG]    = debug_font_json->valuestring,
        };

        int window_height;
        int drawable_height;
        SDL_GetWindowSize(get_context_window(), NULL, &window_height);
        SDL_GetRendererOutputSize(get_context_renderer(), NULL, &drawable_height);

        const float scale = (float)drawable_height / (float)window_height;
        for (size_t font_index = 0ULL; font_index < FONT_COUNT; ++font_index) {
                if (!(fonts[font_index] = TTF_OpenFont(font_paths[font_index], (int)(font_sizes[font_index] * scale)))) {
                        send_message(ERROR, "Failed to load fonts: Failed to open font %zu: %s", font_index, TTF_GetError());
                        return false;
                }

                TTF_SetFontKerning(fonts[font_index], (int)font_kerning_allowed[font_index]);
        }

        return true;
}

static void unload_fonts(void) {
        for (size_t font_index = 0ULL; font_index < FONT_COUNT; ++font_index) {
                TTF_CloseFont(fonts[font_index]);
                fonts[font_index] = NULL;
        }
}

// ================================================================================================
// Levels
// ================================================================================================

static struct LevelMetadata *level_metadatas = NULL;
static size_t level_metadata_count = 0ULL;

size_t get_level_count(void) {
        return 100; // level_metadata_count;
}

const struct LevelMetadata *get_level_metadata(const size_t level) {
        if (!level || level > level_metadata_count) {
                return NULL;
        }

        return &level_metadatas[level - 1ULL];
}

static bool load_levels(const cJSON *const json) {
        if (!cJSON_IsArray(json)) {
                send_message(ERROR, "Failed to load levels: JSON data is invalid");
                return false;
        }

        level_metadata_count = (size_t)cJSON_GetArraySize(json);
        level_metadatas = (struct LevelMetadata *)xmalloc(level_metadata_count * sizeof(struct LevelMetadata));

        size_t level_metadata_index = 0ULL;
        const cJSON *level_metadata_json = NULL;
        cJSON_ArrayForEach(level_metadata_json, json) {
                if (!cJSON_IsObject(level_metadata_json)) {
                        send_message(ERROR, "Failed to load levels: JSON data is invalid");
                        return false;
                }

                const cJSON *const title = cJSON_GetObjectItemCaseSensitive(level_metadata_json, "title");
                const cJSON *const path = cJSON_GetObjectItemCaseSensitive(level_metadata_json, "path");

                if (!cJSON_IsString(title) || !cJSON_IsString(path)) {
                        send_message(ERROR, "Failed to load levels: JSON data is invalid");
                        return false;
                }

                struct LevelMetadata *const level_metadata = &level_metadatas[level_metadata_index++];
                level_metadata->title = strdup(title->valuestring);
                level_metadata->path = strdup(path->valuestring);
        }

        return true;
}

static void unload_levels(void) {
        if (!level_metadata_count) {
                return;
        }

        for (size_t level_metadata_index = 0ULL; level_metadata_index < level_metadata_count; ++level_metadata_index) {
                free(level_metadatas[level_metadata_index].title);
                free(level_metadatas[level_metadata_index].path);
        }

        xfree(level_metadatas);
}

// ================================================================================================
// Sounds
// ================================================================================================

static Mix_Chunk *sounds[SOUND_COUNT];

void play_sound(const enum Sound sound) {
        if (!sounds[sound]) {
                send_message(ERROR, "Failed to play sound %d: Sound is unavailable", (int)sound);
                return;
        }

        if (Mix_PlayChannel(-1, sounds[sound], 0) < 0) {
                send_message(ERROR, "Failed to play sound %d: %s", (int)sound, Mix_GetError());
        }
}

static bool load_sounds(const cJSON *const json) {
        if (!cJSON_IsObject(json)) {
                send_message(ERROR, "Failed to parse sounds: JSON data is invalid");
                return false;
        }

        for (const cJSON *child = json->child; child != NULL; child = child->next) {
                if (!cJSON_IsString(child)) {
                        send_message(ERROR, "Failed to parse sounds: JSON data is invalid");
                        return false;
                }

                const char *const key = child->string;
                bool sound_found = false;
                enum Sound sound;

                if (!strcmp(key, "Click")) {
                        sound_found = true;
                        sound = SOUND_CLICK;
                } else if (!strcmp(key, "Hit")) {
                        sound_found = true;
                        sound = SOUND_HIT;
                } else if (!strcmp(key, "Move")) {
                        sound_found = true;
                        sound = SOUND_MOVE;
                } else if (!strcmp(key, "Placed")) {
                        sound_found = true;
                        sound = SOUND_PLACED;
                } else if (!strcmp(key, "Push")) {
                        sound_found = true;
                        sound = SOUND_PUSH;
                } else if (!strcmp(key, "Turn")) {
                        sound_found = true;
                        sound = SOUND_TURN;
                } else if (!strcmp(key, "Undo")) {
                        sound_found = true;
                        sound = SOUND_UNDO;
                } else if (!strcmp(key, "Win")) {
                        sound_found = true;
                        sound = SOUND_WIN;
                }

                if (!sound_found) {
                        send_message(ERROR, "Failed to parse sounds: JSON data is invalid");
                        return false;
                }

                if (!(sounds[sound] = Mix_LoadWAV(child->valuestring))) {
                        send_message(ERROR, "Failed to parse sounds: Failed to load sound \"%s\": %s", key, Mix_GetError());
                        return false;
                }
        }

        return true;
}

static void unload_sounds(void) {
        for (size_t sound_index = 0ULL; sound_index < SOUND_COUNT; ++sound_index) {
                Mix_FreeChunk(sounds[sound_index]);
                sounds[sound_index] = NULL;
        }
}