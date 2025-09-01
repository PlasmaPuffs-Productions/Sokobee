#pragma once

#include <stdbool.h>
#include <stdlib.h>

typedef union SDL_Event SDL_Event;

#define SCENE_COUNT 3ULL
enum Scene {
        SCENE_MAIN_MENU,
        SCENE_PLAYING,
        SCENE_NONE
};

struct SceneAPI {
        bool (*initialize)(void);
        void (*present)(void);
        bool (*receive_event)(const SDL_Event *);
        void (*update)(double delta_time);
        void (*dismiss)(void);
        void (*terminate)(void);
};

extern size_t current_level_number;

bool initialize_scene_manager(void);
void terminate_scene_manager(void);

void scene_manager_present_scene(const enum Scene next_scene);
bool scene_manager_receive_event(const SDL_Event *const event);
void update_scene_manager(const double delta_time);

const struct SceneAPI *get_main_menu_scene_API(void);
const struct SceneAPI *get_playing_scene_API(void);