#pragma once

#include <stdbool.h>

bool initialize_cursor(void);
void terminate_cursor(void);

#define CURSOR_COUNT 2ULL
enum CursorType {
        CURSOR_ARROW,
        CURSOR_POINTER
};

void request_cursor(const enum CursorType type);
void request_tooltip(const bool active);
void set_tooltip_text(char *const text);
void update_cursor(const double delta_time);