#include "Scenes.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <SDL.h>

#include "../Assets.h"
#include "../Button.h"
#include "../Animation.h"
#include "../Utilities.h"
#include "../Context.h"
#include "../Layers.h"
#include "../Level.h"
#include "../Icons.h"
#include "../Text.h"

#define MOVE_COUNT_LABEL_BUFFER_SIZE 16ULL
#define LEVEL_TITLE_LABEL_BUFFER_SIZE 64ULL

static struct Level level;
static size_t displayed_move_count = 0ULL;

static float move_count_scale = 1.0f;
static struct Animation move_count_pulse;
static struct Text level_number_label;
static struct Text move_count_label;
static struct Button undo_button;
static struct Button redo_button;
static struct Button restart_button;
static struct Button quit_button;

static bool initialize_playing_scene(void);
static void present_playing_scene(void);
static void playing_scene_receive_event(const SDL_Event *const event);
static void update_playing_scene(const double delta_time);
static void dismiss_playing_scene(void);
static void terminate_playing_scene(void);

static const struct SceneAPI playing_scene_API = (struct SceneAPI){
        .initialize = initialize_playing_scene,
        .present = present_playing_scene,
        .receive_event = playing_scene_receive_event,
        .update = update_playing_scene,
        .dismiss = dismiss_playing_scene,
        .terminate = terminate_playing_scene
};

const struct SceneAPI *get_playing_scene_API(void) {
        return &playing_scene_API;
}

static void simulate_key_press(void *const key) {
        SDL_Event event = {};
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = (SDL_KeyCode)(intptr_t)key;
        playing_scene_receive_event(&event);
}

static void back_to_main_menu(void *const data) {
        scene_manager_present_scene(SCENE_MAIN_MENU);
}

static void quit_callback(void *const data) {
        trigger_transition_layer(back_to_main_menu, NULL);
}

static bool initialize_playing_scene(void) {
        initialize_text(&level_number_label, "Level: 0", FONT_HEADER_2);
        set_text_color(&level_number_label, COLOR_YELLOW, 255);

        initialize_text(&move_count_label, "Moves: 0", FONT_HEADER_1);
        set_text_color(&move_count_label, COLOR_YELLOW, 255);

        initialize_button(&undo_button, true);
        undo_button.grid_anchor_x = 1.0f;
        undo_button.tile_offset_column = -3;
        undo_button.callback = simulate_key_press;
        undo_button.callback_data = (void *)(intptr_t)SDLK_z;
        set_button_tooltip_text(&undo_button, "Undo");
        set_button_surface_icon(&undo_button, ICON_UNDO);

        initialize_button(&redo_button, true);
        redo_button.grid_anchor_x = 1.0f;
        redo_button.tile_offset_column = -2;
        redo_button.callback = simulate_key_press;
        redo_button.callback_data = (void *)(intptr_t)SDLK_x;
        set_button_tooltip_text(&redo_button, "Redo");
        set_button_surface_icon(&redo_button, ICON_REDO);

        initialize_button(&restart_button, true);
        restart_button.grid_anchor_x = 1.0f;
        restart_button.tile_offset_column = -1;
        restart_button.callback = simulate_key_press;
        restart_button.callback_data = (void *)(intptr_t)SDLK_r;
        set_button_tooltip_text(&restart_button, "Restart Level");
        set_button_surface_icon(&restart_button, ICON_RESTART);

        initialize_button(&quit_button, true);
        quit_button.grid_anchor_x = 1.0f;
        quit_button.callback = quit_callback;
        set_button_tooltip_text(&quit_button, "Quit Level");
        set_button_surface_icon(&quit_button, ICON_EXIT);

        redo_button.thickness_mask &= ~HEXAGON_THICKNESS_MASK_LEFT;
        redo_button.thickness_mask &= ~HEXAGON_THICKNESS_MASK_RIGHT;
        quit_button.thickness_mask &= ~HEXAGON_THICKNESS_MASK_LEFT;

        initialize_animation(&move_count_pulse, 2ULL);

        struct Action *const grow = &move_count_pulse.actions[0];
        grow->target.float_pointer = &move_count_scale;
        grow->keyframes.floats[0] = 1.0f;
        grow->keyframes.floats[1] = 1.05f;
        grow->type = ACTION_FLOAT;
        grow->duration = 50.0f;
        grow->easing = CUBE_OUT;

        struct Action *const shrink = &move_count_pulse.actions[1];
        shrink->target.float_pointer = &move_count_scale;
        shrink->keyframes.floats[0] = 1.05f;
        shrink->keyframes.floats[1] = 1.0f; 
        shrink->type = ACTION_FLOAT;
        shrink->duration = 50.0f;
        shrink->easing = CUBE_IN;
        shrink->delay = 25.0f;

        return true;
}

static void transition_to_next_level(void *);

static void present_level(void *const data) {
        deinitialize_level(&level);

        const size_t number = (size_t)(uintptr_t)data;
        const size_t same_level = current_level_number == number;
        current_level_number = number;

        const struct LevelMetadata *const next_level_metadata = get_level_metadata(current_level_number);
        if (!next_level_metadata) {
                send_message(INFORMATION, "All levels complete: Returning to main menu");
                scene_manager_present_scene(SCENE_MAIN_MENU);
                return;
        }

        if (!initialize_level(&level, next_level_metadata)) {
                send_message(ERROR, "Failed to load next level: Returning to main menu");
                scene_manager_present_scene(SCENE_MAIN_MENU);
                return;
        }

        level.completion_callback = transition_to_next_level;

        if (same_level) {
                return;
        }

        char level_count_string[LEVEL_TITLE_LABEL_BUFFER_SIZE];
        snprintf(level_count_string, sizeof(level_count_string), "Level %zu: %s", current_level_number, level.title);
        set_text_string(&level_number_label, level_count_string);
}

static void transition_to_next_level(void *const data) {
        trigger_transition_layer(present_level, (void *)(uintptr_t)(current_level_number + 1ULL));
}

static void present_playing_scene(void) {
        reset_animation(&move_count_pulse);
        present_level((void *)(uintptr_t)current_level_number);
}

static void playing_scene_receive_event(const SDL_Event *const event) {
        if (is_transition_triggered()) {
                return;
        }

        if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_r) {
                trigger_transition_layer(present_level, (void *)(uintptr_t)current_level_number);
                return;
        }

        level_receive_event(&level, event);

        button_receive_event(&undo_button, event);
        button_receive_event(&redo_button, event);
        button_receive_event(&restart_button, event);
        button_receive_event(&quit_button, event);
}

static void update_playing_scene(const double delta_time) {
        update_level(&level, delta_time);

        if (displayed_move_count != level.move_count) {
                displayed_move_count = level.move_count;

                char move_count_string[MOVE_COUNT_LABEL_BUFFER_SIZE];
                snprintf(move_count_string, sizeof(move_count_string), "Moves: %zu", displayed_move_count);
                set_text_string(&move_count_label, move_count_string);
                start_animation(&move_count_pulse, 0ULL);
        }

        int drawable_width;
        int drawable_height;
        SDL_GetRendererOutputSize(get_context_renderer(), &drawable_width, &drawable_height);

        const float padding = CLAMP_VALUE(fmaxf((float)drawable_width, (float)drawable_height) * 0.02f, 20.0f, 50.0f);

        level_number_label.absolute_offset_x = padding;
        level_number_label.absolute_offset_y = padding;
        update_text(&level_number_label);

        move_count_label.scale_x = move_count_scale;
        move_count_label.scale_y = move_count_scale;
        update_animation(&move_count_pulse, delta_time);

        move_count_label.absolute_offset_x = padding;
        move_count_label.absolute_offset_y = 1.5f * padding + (float)get_text_height(&level_number_label);
        update_text(&move_count_label);

        update_button(&undo_button, delta_time);
        update_button(&redo_button, delta_time);
        update_button(&restart_button, delta_time);
        update_button(&quit_button, delta_time);
}

static void dismiss_playing_scene(void) {
        deinitialize_level(&level);
}

static void terminate_playing_scene(void) {
        deinitialize_animation(&move_count_pulse);
        deinitialize_text(&level_number_label);
        deinitialize_text(&move_count_label);
        deinitialize_button(&undo_button);
        deinitialize_button(&redo_button);
        deinitialize_button(&restart_button);
        deinitialize_button(&quit_button);
}