#pragma once

#include <stdbool.h>

bool initialize_context(void);
void terminate_context(void);

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;

SDL_Window *get_context_window(void);
SDL_Renderer *get_context_renderer(void);