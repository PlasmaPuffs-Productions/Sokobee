#include "Context.h"

#include <stdbool.h>

#include <SDL.h>
#include <SDL_render.h>

#include "Utilities.h"

#define INITIAL_WINDOW_WIDTH 1280
#define INITIAL_WINDOW_HEIGHT 720
#define MINIMUM_WINDOW_WIDTH 800
#define MINIMUM_WINDOW_HEIGHT 600

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

bool initialize_context(void) {
        window = SDL_CreateWindow("Sokobee", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        if (window == NULL) {
                send_message(ERROR, "Failed to initialize context: Failed to create window: %s", SDL_GetError());
                terminate_context();
                return false;
        }

        SDL_SetWindowMinimumSize(window, MINIMUM_WINDOW_WIDTH, MINIMUM_WINDOW_HEIGHT);

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (renderer == NULL) {
                send_message(ERROR, "Failed to initialize context: Failed to create renderer: %s", SDL_GetError());
                terminate_context();
                return false;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        return true;
}

void terminate_context(void) {
        SDL_DestroyWindow(window);
        SDL_DestroyRenderer(renderer);
}

SDL_Window *get_context_window(void) {
        return window;
}

SDL_Renderer *get_context_renderer(void) {
        return renderer;
}