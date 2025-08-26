#pragma once

void start_debug_frame_profiling(void);
void finish_debug_frame_profiling(void);

void initialize_debug_panel(void);
void terminate_debug_panel(void);

typedef union SDL_Event SDL_Event;
void debug_panel_receive_event(const SDL_Event *const event);
void update_debug_panel(const double delta_time);