#include "Scenes.h"

#include <stdbool.h>

#include <SDL.h>

#include "Utilities.h"

size_t current_level_number = 0ULL;

static const struct SceneAPI *current_scene = NULL;
static const struct SceneAPI *(*scene_API_getters[SCENE_COUNT - 1ULL])(void) = {
        [SCENE_MAIN_MENU] = get_main_menu_scene_API,
        [SCENE_PLAYING] = get_playing_scene_API
};

bool initialize_scene_manager(void) {
        for (size_t scene_API_index = 0ULL; scene_API_index < SCENE_COUNT - 1ULL; ++scene_API_index) {
                const struct SceneAPI *const scene_API = scene_API_getters[(enum Scene)scene_API_index]();
                if (scene_API->initialize) {
                        if (!scene_API->initialize()) {
                                send_message(ERROR, "Failed to initialize scene manager: Failed to initialize scene %zu", scene_API_index);
                                terminate_scene_manager();
                                return false;
                        }
                }
        }

        return true;
}

void terminate_scene_manager(void) {
        if (current_scene) {
                if (current_scene->dismiss) {
                        current_scene->dismiss();
                }

                current_scene = NULL;
        }

        for (size_t scene_API_index = 0ULL; scene_API_index < SCENE_COUNT - 1ULL; ++scene_API_index) {
                const struct SceneAPI *const scene_API = scene_API_getters[(enum Scene)scene_API_index]();
                if (scene_API->terminate) {
                        scene_API->terminate();
                }
        }
}

void scene_manager_present_scene(const enum Scene next_scene) {
        if (current_scene) {
                if (current_scene->dismiss) {
                        current_scene->dismiss();
                }

                current_scene = NULL;
        }

        if (next_scene == SCENE_NONE) {
                return;
        }

        current_scene = scene_API_getters[next_scene]();
        if (current_scene->present) {
                current_scene->present();
        }
}

bool scene_manager_receive_event(const SDL_Event *const event) {
        if (current_scene && current_scene->receive_event) {
                return current_scene->receive_event(event);
        }

        return false;
}

void update_scene_manager(const double delta_time) {
        if (current_scene && current_scene->update) {
                current_scene->update(delta_time);
        }
}