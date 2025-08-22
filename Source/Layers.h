#pragma once

#include <stdbool.h>

void initialize_layers(void);
void terminate_layers(void);

typedef union SDL_Event SDL_Event;
void layers_receive_event(const SDL_Event *const event);
void update_layers(const double delta_time);

void render_background_layer(void);
void render_transition_layer(void);
bool is_transition_triggered(void);
bool trigger_transition_layer(void (*const callback)(void *), void *const callback_data);