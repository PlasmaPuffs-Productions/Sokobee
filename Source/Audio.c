#include "Audio.h"

#include <stdbool.h>

#include <SDL_mixer.h>

#include "Debug.h"
#include "Persistent.h"

#define SOUND_CHANNEL_COUNT 4
#define SOUND_GROUP         1

static Mix_Chunk *sound_chunks[SOUND_COUNT];
static Mix_Music *music_tracks[MUSIC_COUNT];

static const char *sound_paths[SOUND_COUNT] = {
        "Assets/Audio/Click.wav",
        "Assets/Audio/Hit.wav",
        "Assets/Audio/Move.wav",
        "Assets/Audio/Placed.wav",
        "Assets/Audio/Push.wav",
        "Assets/Audio/Turn.wav",
        "Assets/Audio/Undo.wav",
        "Assets/Audio/Win.wav"
};

static const char *music_paths[MUSIC_COUNT] = {
        "Assets/Audio/BGM.wav"
};

bool initialize_audio(void) {
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
                send_message(ERROR, "Failed to initialize audio: %s", Mix_GetError());
                return false;
        }

        Mix_AllocateChannels(SOUND_CHANNEL_COUNT);
        Mix_GroupChannels(0, SOUND_CHANNEL_COUNT - 1, SOUND_GROUP);

        if (!get_persistent_sound_enabled()) {
                toggle_sound(false);
        }

        if (!get_persistent_music_enabled()) {
                toggle_music(false);
        }

        for (size_t index = 0ULL; index < MUSIC_COUNT; index++) {
                if (!(music_tracks[index] = Mix_LoadMUS(music_paths[index]))) {
                        send_message(ERROR, "Failed to initialize audio: Failed to load music %zu from path \"%s\": %s", index, music_paths[index], Mix_GetError());
                        terminate_audio();
                        return false;
                }
        }

        for (size_t index = 0ULL; index < SOUND_COUNT; index++) {
                if (!(sound_chunks[index] = Mix_LoadWAV(sound_paths[index]))) {
                        send_message(ERROR, "Failed to initialize audio: Failed to load sound %zu from path \"%s\": %s", index, sound_paths[index], Mix_GetError());
                        terminate_audio();
                        return false;
                }
        }

        return true;
}

void terminate_audio(void) {
        for (size_t index = 0; index < MUSIC_COUNT; index++) {
                if (music_tracks[index]) {
                        Mix_FreeMusic(music_tracks[index]);
                        music_tracks[index] = NULL;
                }
        }

        for (size_t index = 0; index < SOUND_COUNT; index++) {
                if (sound_chunks[index]) {
                        Mix_FreeChunk(sound_chunks[index]);
                        sound_chunks[index] = NULL;
                }
        }

        Mix_Quit();
}

void play_sound(const enum Sound sound) {
        if (!sound_chunks[sound]) {
                send_message(ERROR, "Failed to play sound %d: Sound is unavailable", (int)sound);
                return;
        }

        const int free_channel = Mix_GroupAvailable(SOUND_GROUP);
        if (Mix_PlayChannel(free_channel < 0 ? Mix_GroupOldest(SOUND_GROUP) : free_channel, sound_chunks[sound], 0) < 0) {
                send_message(ERROR, "Failed to play sound %d: %s", (int)sound, Mix_GetError());
        }
}

void toggle_sound(const bool enabled) {
        for (int channel = 0; channel < SOUND_CHANNEL_COUNT; ++channel) {
                Mix_Volume(channel, enabled ? MIX_MAX_VOLUME : 0);
        }
}

void play_music(const enum Music music) {
        if (!music_tracks[music]) {
                send_message(ERROR, "Failed to play music %d: Music is unavailable", (int)music);
                return;
        }

        if (Mix_PlayMusic(music_tracks[music], -1) < 0) {
                send_message(ERROR, "Failed to play music %d: %s", (int)music, Mix_GetError());
        }
}

void toggle_music(const bool enabled) {
        Mix_VolumeMusic(enabled ? MIX_MAX_VOLUME : 0);
}