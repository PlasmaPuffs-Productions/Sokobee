#pragma once

#include <stdbool.h>

bool load_persistent_data(void);
bool save_persistent_data(void);

bool get_persistent_sound_enabled(void);
void set_persistent_sound_enabled(const bool sound_enabled);

bool get_persistent_music_enabled(void);
void set_persistent_music_enabled(const bool music_enabled);