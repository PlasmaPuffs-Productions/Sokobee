#pragma once

#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "Memory.h"

// ================================================================================================
// Platform Detection
// ================================================================================================

#undef PLATFORM_APPLE
#undef PLATFORM_APPLE_MACOS
#undef PLATFORM_APPLE_IOS
#undef PLATFORM_WINDOWS
#undef PLATFORM_WINDOWS_32
#undef PLATFORM_WINDOWS_64
#undef PLATFORM_LINUX
#undef PLATFORM_ANDROID
#undef PLATFORM_EMSCRIPTEN
#undef PLATFORM_SWITCH
#undef PLATFORM_UNKNOWN

#if defined(__APPLE__)
        #define PLATFORM_APPLE

        #include <TargetConditionals.h>
        #if TARGET_OS_MAC && !TARGET_OS_IPHONE
                #define PLATFORM_APPLE_MACOS
        #elif TARGET_OS_IOS
                #define PLATFORM_APPLE_IOS
        #endif
#elif defined(_WIN32)
        #define PLATFORM_WINDOWS
        #if defined(_WIN64)
                #define PLATFORM_WINDOWS_64
        #else
                #define PLATFORM_WINDOWS_32
        #endif
#elif defined(__linux__) || defined(__unix__)
        #define PLATFORM_LINUX
#elif defined(__ANDROID__)
        #define PLATFORM_ANDROID
#elif defined(__EMSCRIPTEN__)
        #define PLATFORM_EMSCRIPTEN
#elif defined(__SWITCH__)
        #define PLATFORM_SWITCH
#else
        #define PLATFORM_UNKNOWN
#endif

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE_MACOS) || defined(PLATFORM_EMSCRIPTEN)
    #define PLATFORM_HAS_MOUSE 1
#else
    #define PLATFORM_HAS_MOUSE 0
#endif

// ================================================================================================
// Color Palette
// ================================================================================================

#define COLOR_TRANSPARENT  (SDL_Color){0,   0,   0,   0}

#define COLOR_YELLOW       (SDL_Color){240, 170, 35,  255}

#define COLOR_LIGHT_YELLOW (SDL_Color){255, 220, 120, 255}

#define COLOR_GOLD         (SDL_Color){190, 140, 35,  255}

#define COLOR_BROWN        (SDL_Color){50,  35,  15,  255}

#define COLOR_DARK_BROWN   (SDL_Color){35,  20,  0,   255}

// ================================================================================================
// Logging
// ================================================================================================

enum MessageSeverity {
        FATAL,
        ERROR,
        WARNING,
        INFORMATION,
        DEBUG,
        VERBOSE,
        COUNT
};

#ifdef NDEBUG
        #define send_message(...) ((void)0)
        #define set_minimum_message_severity(...) ((void)0)
#else
        #define TIME_STRING_SIZE 64

        static const char *const message_severity_strings[COUNT] = {
                [FATAL]       = "      \033[37;41mFATAL\033[m",
                [ERROR]       = "      \033[31mERROR\033[m",
                [WARNING]     = "    \033[33mWARNING\033[m",
                [INFORMATION] = "\033[32mINFORMATION\033[m",
                [DEBUG]       = "      \033[36mDEBUG\033[m",
                [VERBOSE]     = "    \033[34mVERBOSE\033[m"
        };

        static inline void send_message(const enum MessageSeverity message_severity, const char *message, ...) {
                struct timespec time_specification;
                struct tm local_time;
                char time_string[TIME_STRING_SIZE];

                timespec_get(&time_specification, TIME_UTC);
                localtime_r(&time_specification.tv_sec, &local_time);
                strftime(time_string, sizeof(time_string), "%Y-%m-%d - %I:%M:%S", &local_time);

                va_list arguments;
                va_start(arguments, message);

                const size_t size = (size_t)vsnprintf(NULL, 0ULL, message, arguments) + 1ULL;
                char *const formatted = (char *)xmalloc(size);
                vsnprintf(formatted, size, message, arguments);

                FILE *const stream = message_severity <= ERROR ? stderr : stdout;
                fprintf(stream, "%s(%s.%09ld %s): %s\n", message_severity_strings[message_severity], time_string, time_specification.tv_nsec, local_time.tm_hour >= 12 ? "PM" : "AM", formatted);
                fflush(stream);

                xfree(formatted);
                va_end(arguments);
        }
#endif

// ================================================================================================
// File IO
// ================================================================================================

static inline char *load_text_file(const char *const path) {
        send_message(VERBOSE, "Loading text file \"%s\"...", path);

        FILE *const file = fopen(path, "rb");
        if (!file) {
                send_message(ERROR, "Failed to load text file \"%s\": %s", path, strerror(errno));
                return NULL;
        }

        fseek(file, 0, SEEK_END);
        const size_t size = (size_t)ftell(file);
        rewind(file);

        send_message(VERBOSE, "Text file \"%s\" has buffer size of %lld bytes", path, size);

        char *const buffer = (char *)xmalloc(size + 1ULL);
        if (fread(buffer, 1ULL, size, file) != size) {
                send_message(ERROR, "Failed to load text file \"%s\": %s", path, strerror(errno));
                xfree(buffer);
                fclose(file);
                return NULL;
        }

        buffer[size] = '\0';
        fclose(file);

        send_message(VERBOSE, "Loaded text file \"%s\" successfully (don't forget to free the returned buffer)", path);
        return buffer;
}

// ================================================================================================
// Math Helpers
// ================================================================================================

#define MINIMUM_VALUE(a, b)                  (a < b ? a : b)

#define MAXIMUM_VALUE(a, b)                  (a > b ? a : b)

#define CLAMP_VALUE(value, minimum, maximum) (value < minimum ? minimum : (value > maximum ? maximum : value))

#define RANDOM_INTEGER(minimum, maximum)     ((size_t)minimum + (size_t)rand() % ((size_t)maximum - (size_t)minimum + 1ULL))

#define RANDOM_NUMBER(minimum, maximum)      ((size_t)minimum + ((float)rand() / (float)RAND_MAX) * ((size_t)maximum - (size_t)minimum))

static inline void rotate_point(float *const out_x, float *const out_y, const float pivot_x, const float pivot_y, const float rotation) {
        if (rotation == 0.0f || out_x == NULL || out_y == NULL) {
                return;
        }

        const float s = sinf(rotation);
        const float c = cosf(rotation);
        const float dx = *out_x - pivot_x;
        const float dy = *out_y - pivot_y;
        *out_x = pivot_x + dx * c - dy * s;
        *out_y = pivot_y + dx * s + dy * c;
}