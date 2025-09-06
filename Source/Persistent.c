#include "Persistent.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <SDL.h>

#include "cJSON.h"
#include "Debug.h"
#include "Memory.h"

#define LOAD_BOOLEAN(json, name)                                                    \
        do {                                                                        \
                cJSON *const value = cJSON_GetObjectItemCaseSensitive(json, #name); \
                if (cJSON_IsBool(value)) {                                          \
                        name = cJSON_IsTrue(value);                                 \
                }                                                                   \
        } while(0)

#define SAVE_BOOLEAN(json, name) cJSON_AddBoolToObject(json, #name, name)

static char persistent_data_file_path[1024];

static bool persistent_sound_enabled = true;
static bool persistent_music_enabled = true;

bool get_persistent_sound_enabled(void) {
        return persistent_sound_enabled;
}

void set_persistent_sound_enabled(const bool sound_enabled) {
        persistent_sound_enabled = sound_enabled;
        save_persistent_data();
}

bool get_persistent_music_enabled(void) {
        return persistent_music_enabled;
}

void set_persistent_music_enabled(const bool music_enabled) {
        persistent_music_enabled = music_enabled;
        save_persistent_data();
}

bool load_persistent_data(void) {
        char *const writable_directory_path = SDL_GetPrefPath("PlasmaPuffsProductions", "Sokobee");
        if (!writable_directory_path) {
                send_message(MESSAGE_ERROR, "Failed to load persistent data: Failed to query writable directory path: %s", SDL_GetMESSAGE_ERROR());
                return false;
        }

        snprintf(persistent_data_file_path, sizeof(persistent_data_file_path), "%s%s", writable_directory_path, "save.json");
        SDL_free(writable_directory_path);

        send_message(MESSAGE_INFORMATION, "Persistent data save file: \"%s\"", persistent_data_file_path);

        FILE *file = fopen(persistent_data_file_path, "rb");
        if (file == NULL) {
                file = fopen(persistent_data_file_path, "w+");
                if (file == NULL) {
                        send_message(MESSAGE_ERROR, "Failed to load persistent data: Failed to create save file at \"%s\": %s", persistent_data_file_path, strMESSAGE_ERROR(errno));
                        return false;
                }
        }

        fseek(file, 0L, SEEK_END);
        const size_t size = (size_t)ftell(file);
        rewind(file);

        char *const data = (char *)xmalloc(size + 1ULL);
        if (fread(data, 1ULL, size, file) != size) {
                send_message(MESSAGE_ERROR, "Failed to load persistent data; Failed to read save file at \"%s\": %s", persistent_data_file_path, strMESSAGE_ERROR(errno));
                xfree(data);
                fclose(file);
                return false;
        }

        data[size] = '\0';
        fclose(file);

        cJSON *const json = cJSON_Parse(data);
        xfree(data);

        if (json == NULL) {
                send_message(MESSAGE_ERROR, "Failed to load persistent data: Failed to parse save file into JSON: %s", cJSON_GetMESSAGE_ERRORPtr());
                return false;
        }

        LOAD_BOOLEAN(json, persistent_sound_enabled);
        LOAD_BOOLEAN(json, persistent_music_enabled);

        cJSON_Delete(json);
        return true;
}

bool save_persistent_data(void) {
        cJSON *const json = cJSON_CreateObject();

        SAVE_BOOLEAN(json, persistent_sound_enabled);
        SAVE_BOOLEAN(json, persistent_music_enabled);

        char *const json_string = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);

        FILE *const file = fopen(persistent_data_file_path, "w");
        if (file == NULL) {
                send_message(MESSAGE_ERROR, "Failed to save persistent data: Failed to open save file in write mode: %s", strMESSAGE_ERROR(errno));
                free(json_string);
                return false;
        }

        fputs(json_string, file);
        fclose(file);

        free(json_string);
        return true;
}