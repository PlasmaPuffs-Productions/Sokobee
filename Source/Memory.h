#pragma once

#include <stdlib.h>

#ifndef NDEBUG

void flush_memory_leaks(void);

void *_malloc(const size_t size, const char *const file, const size_t line);
void *_calloc(const size_t count, const size_t size, const char *const file, const size_t line);
void *_realloc(void *const pointer, const size_t size, const char *const file, const size_t line);
void  _free(void *const pointer, const char *const file, const size_t line);

#define xmalloc(size)           _malloc((size),              __FILE__, (size_t)__LINE__)
#define xcalloc(count, size)    _calloc((count), (size),     __FILE__, (size_t)__LINE__)
#define xrealloc(pointer, size) _realloc((pointer), (size),  __FILE__, (size_t)__LINE__)
#define xfree(pointer)          _free((pointer),             __FILE__, (size_t)__LINE__)

#else

static inline void flush_memory_leaks(void) {
        return;
}

static inline void *xmalloc(const size_t size) {
        void *const allocated = malloc(size);
        if (allocated == NULL) {
                exit(EXIT_FAILURE);
        }

        return allocated;
}

static inline void *xcalloc(const size_t count, const size_t size) {
        void *const allocated = calloc(count, size);
        if (allocated == NULL) {
                exit(EXIT_FAILURE);
        }

        return allocated;
}

static inline void *xrealloc(void *const pointer, const size_t size) {
        void *const reallocated = realloc(pointer, size);
        if (reallocated == NULL) {
                exit(EXIT_FAILURE);
        }

        return reallocated;
}

static inline void xfree(void *const pointer) {
        free(pointer);
}

#endif