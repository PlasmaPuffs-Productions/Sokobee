#pragma once

#include <stdbool.h>

bool initialize_audio(void);
void terminate_audio(void);

enum Sound {
        SOUND_CLICK,
        SOUND_HIT,
        SOUND_MOVE,
        SOUND_PLACED,
        SOUND_PUSH,
        SOUND_TURN,
        SOUND_UNDO,
        SOUND_WIN,
        SOUND_COUNT
};

void play_sound(const enum Sound sound);

void toggle_sound(const bool enabled);

enum Music {
        MUSIC_BGM,
        MUSIC_COUNT
};

void play_music(const enum Music music);

void toggle_music(const bool enabled);