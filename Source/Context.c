#include "Context.h"

#include <stdbool.h>

#include <SDL.h>
#include <SDL_render.h>

#include "Utilities.h"

#define INITIAL_WINDOW_WIDTH  1280
#define INITIAL_WINDOW_HEIGHT  720
#define MINIMUM_WINDOW_WIDTH   800
#define MINIMUM_WINDOW_HEIGHT  600

#define MISSING_TEXTURE_TILE_SIZE 16ULL

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *missing_texture = NULL;

SDL_Window *get_context_window(void) {
        return window;
}

SDL_Renderer *get_context_renderer(void) {
        return renderer;
}

SDL_Texture *get_mising_texture(void) {
        return missing_texture;
}

bool initialize_context(void) {
        if (!(window = SDL_CreateWindow("Sokobee", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI))) {
                send_message(MESSAGE_ERROR, "Failed to initialize context: Failed to create window: %s", SDL_GetMESSAGE_ERROR());
                terminate_context();
                return false;
        }

        SDL_SetWindowMinimumSize(window, MINIMUM_WINDOW_WIDTH, MINIMUM_WINDOW_HEIGHT);

        if (!(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED))) {
                send_message(MESSAGE_ERROR, "Failed to initialize context: Failed to create renderer: %s", SDL_GetMESSAGE_ERROR());
                terminate_context();
                return false;
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        if (!(missing_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, MISSING_TEXTURE_WIDTH, MISSING_TEXTURE_HEIGHT))) {
                send_message(MESSAGE_ERROR, "Failed to initialize context: Failed to create missing texture: %s", SDL_GetMESSAGE_ERROR());
                terminate_context();
                return false;
        }

        if (!apply_missing_texture(missing_texture)) {
                send_message(MESSAGE_ERROR, "Failed to initialize context: Failed to apply missing texture pattern to missing texture");
                terminate_context();
                return false;
        }

        return true;
}

void terminate_context(void) {
        if (missing_texture) {
                SDL_DestroyTexture(missing_texture);
                missing_texture = NULL;
        }

        if (renderer) {
                SDL_DestroyRenderer(renderer);
                renderer = NULL;
        }

        if (window) {
                SDL_DestroyWindow(window);
                window = NULL;
        }
}

bool apply_missing_texture(SDL_Texture *const texture) {
        static const Uint32 color1 = (255 << 24) | (  0 << 16) | (255 << 8) | 255; // Magenta
        static const Uint32 color2 = (  0 << 24) | (  0 << 16) | (  0 << 8) | 255; // Black

        int texture_width, texture_height;
        SDL_QueryTexture(texture, NULL, NULL, &texture_width, &texture_height);

        void *pixels;
        int pitch;
        if (SDL_LockTexture(texture, NULL, &pixels, &pitch) != 0) {
                SDL_Log("Failed to apply missing texture: Failed to lock texture: %s", SDL_GetMESSAGE_ERROR());
                return false;
        }

        Uint32 *const destination = (Uint32 *)pixels;
        for (size_t column = 0ULL; column < (size_t)texture_width; ++column) {
                for (size_t row = 0ULL; row < (size_t)texture_height; ++row) {
                        const size_t tile_x = column / MISSING_TEXTURE_TILE_SIZE;
                        const size_t tile_y = row / MISSING_TEXTURE_TILE_SIZE;

                        destination[row * ((size_t)pitch / 4ULL) + column] = ((tile_x + tile_y) % 2ULL == 0ULL) ? color1 : color2;
                }
        }

        SDL_UnlockTexture(texture);
        return true;
}