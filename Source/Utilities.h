#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>

#include "Memory.h"
#include "Debug.h"

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

#define COLOR_BLACK          0,   0,   0

#define COLOR_WHITE        255, 255, 255

#define COLOR_YELLOW       240, 170,  35

#define COLOR_LIGHT_YELLOW 255, 220, 120

#define COLOR_GOLD         190, 140,  35

#define COLOR_BROWN         50,  35,  15

#define COLOR_DARK_BROWN    35,  20,   0

#define COLOR_OPAQUE        255

#define COLOR_TRANSPARENT   0

// ================================================================================================
// File IO
// ================================================================================================

static inline char *load_text_file(const char *const path) {
        FILE *const file = fopen(path, "rb");
        if (file == NULL) {
                send_message(ERROR, "Failed to load text file \"%s\": %s", path, strerror(errno));
                return NULL;
        }

        fseek(file, 0L, SEEK_END);
        const size_t size = (size_t)ftell(file);
        rewind(file);

        char *const buffer = (char *)xmalloc(size + 1ULL);
        if (fread(buffer, 1ULL, size, file) != size) {
                send_message(ERROR, "Failed to read text file \"%s\": %s", path, strerror(errno));
                xfree(buffer);
                fclose(file);
                return NULL;
        }

        buffer[size] = '\0';
        fclose(file);

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

static inline void rotate_point(float *const px, float *const py, const float ox, const float oy, const float rotation) {
        if (rotation == 0.0f || px == NULL || py == NULL) {
                return;
        }

        const float sin = sinf(rotation);
        const float cos = cosf(rotation);
        const float dx = *px - ox;
        const float dy = *py - oy;
        *px = ox + dx * cos - dy * sin;
        *py = oy + dx * sin + dy * cos;
}