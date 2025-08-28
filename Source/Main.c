#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include <SDL.h>
#include <SDL_render.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include "Audio.h"
#include "Debug.h"
#include "Cursor.h"
#include "Assets.h"
#include "Layers.h"
#include "Context.h"
#include "Geometry.h"
#include "Persistent.h"
#include "Scenes/Scenes.h"

#define WINDOW_MINIMIZED_THROTTLE 100ULL

static void initialize(void);
static void update(const double delta_time);
static void terminate(const int exit_code);

int main(void) {
        srand((unsigned int)time(NULL));
        initialize();

        scene_manager_present_scene(SCENE_MAIN_MENU);

        Uint64 previous_time = SDL_GetPerformanceCounter();
        while (true) {
                const Uint64 current_time = SDL_GetPerformanceCounter();
                const double delta_time = 1000.0 * (double)(current_time - previous_time) / (double)SDL_GetPerformanceFrequency();
                previous_time = current_time;
                update(delta_time);
        }

        return EXIT_FAILURE;
}

static void initialize(void) {
        send_message(INFORMATION, "Initializing program...");

        if (SDL_Init(SDL_INIT_EVERYTHING) < 0 || TTF_Init() < 0) {
                send_message(FATAL, "Failed to initialize program: Failed to initialize SDL: %s", SDL_GetError());
                terminate(EXIT_FAILURE);
        }

        if (!load_persistent_data()) {
                send_message(FATAL, "Failed to initialize program: Failed to load persistent data");
                terminate(EXIT_FAILURE);
        }

        if (!initialize_audio()) {
                send_message(FATAL, "Failed to initialize program: Failed to initialize audio");
                terminate(EXIT_FAILURE);
        }

        if (!initialize_context()) {
                send_message(FATAL, "Failed to initialize program: Failed to initialize context");
                terminate(EXIT_FAILURE);
        }

        if (!load_assets("Assets/Assets.json")) {
                send_message(FATAL, "Failed to initialize program: Failed to load assets");
                terminate(EXIT_FAILURE);
        }

        if (!initialize_cursor()) {
                send_message(FATAL, "Failed to initialize program: Failed to initialize cursor");
                terminate(EXIT_FAILURE);
        }

        if (!initialize_scene_manager()) {
                send_message(FATAL, "Failed to initialize program: Failed to intialize scene manager");
                terminate(EXIT_FAILURE);
        }

        initialize_layers();
        initialize_debug_panel();

        play_music(MUSIC_BGM);

        send_message(INFORMATION, "Program initialized successfully");
}

static void update(const double delta_time) {
        // I need to start profiling when the frame starts because the true FPS gets capped on some environments (like mine)
        start_debug_frame_profiling();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                        terminate(EXIT_SUCCESS);
                }

                if (event.type == SDL_WINDOW_MINIMIZED) {
                        SDL_Delay((Uint32)WINDOW_MINIMIZED_THROTTLE);
                        return;
                }

                layers_receive_event(&event);
                scene_manager_receive_event(&event);
                debug_panel_receive_event(&event);
        }

        SDL_Renderer *const renderer = get_context_renderer();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        update_layers(delta_time);
        render_background_layer();
        update_scene_manager(delta_time);
        render_transition_layer();

        update_debug_panel(delta_time);

        update_cursor(delta_time);
        request_cursor(CURSOR_ARROW);
        request_tooltip(false);

        SDL_RenderPresent(renderer);

        finish_debug_frame_profiling();
}

static void terminate(const int exit_code) {
        send_message(INFORMATION, "Terminating program...");

        terminate_scene_manager();
        terminate_debug_panel();
        terminate_layers();
        terminate_cursor();

        unload_assets();
        terminate_context();
        terminate_audio();

        TTF_Quit();
        SDL_Quit();

        send_message(INFORMATION, "Exiting program with code \"EXIT_%s\"...", exit_code == EXIT_SUCCESS ? "SUCCESS" : "FAILURE");
        flush_memory_leaks();
        exit(exit_code);
}