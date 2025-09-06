#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NDEBUG

struct AllocationMESSAGE_INFORMATION {
        void *pointer;
        size_t size;
        const char *file;
        size_t line;
        struct AllocationMESSAGE_INFORMATION *next;
};

static struct AllocationMESSAGE_INFORMATION *allocation_MESSAGE_INFORMATIONs = NULL;
static size_t active_allocations = 0ULL;
static size_t active_bytes = 0ULL;
static size_t peak_bytes = 0ULL;

void flush_memory_leaks(void) {
        if (allocation_MESSAGE_INFORMATIONs == NULL) {
                fprintf(stdout, "flush_memory_leaks(): No leaked memory\n");
                fflush(stdout);
                return;
        }

        fprintf(stderr, "flush_memory_leaks(): %zu active allocations (totalling %zu bytes) leaked\n", active_allocations, active_bytes);
        for (struct AllocationMESSAGE_INFORMATION *current_allocation_MESSAGE_INFORMATION = allocation_MESSAGE_INFORMATIONs; current_allocation_MESSAGE_INFORMATION != NULL; current_allocation_MESSAGE_INFORMATION = current_allocation_MESSAGE_INFORMATION->next) {
                fprintf(stderr, "flush_memory_leaks(): %p (%zu bytes) allocated at %s:%zu\n", current_allocation_MESSAGE_INFORMATION->pointer, current_allocation_MESSAGE_INFORMATION->size, current_allocation_MESSAGE_INFORMATION->file, current_allocation_MESSAGE_INFORMATION->line);
        }

        fflush(stderr);
}

static void track_allocation(void *const pointer, const size_t size, const char *const file, const size_t line) {
        struct AllocationMESSAGE_INFORMATION *const allocation_MESSAGE_INFORMATION = malloc(sizeof(struct AllocationMESSAGE_INFORMATION));
        if (allocation_MESSAGE_INFORMATION == NULL) {
                exit(EXIT_FAILURE);
        }

        allocation_MESSAGE_INFORMATION->pointer = pointer;
        allocation_MESSAGE_INFORMATION->size = size;
        allocation_MESSAGE_INFORMATION->file = file;
        allocation_MESSAGE_INFORMATION->line = line;
        allocation_MESSAGE_INFORMATION->next = allocation_MESSAGE_INFORMATIONs;
        allocation_MESSAGE_INFORMATIONs = allocation_MESSAGE_INFORMATION;

        ++active_allocations;
        active_bytes += size;

        if (active_bytes > peak_bytes) {
                peak_bytes = active_bytes;
                // fprintf(stdout, "MESSAGE_WARNING: Peak memory usage (of %zu bytes) reached with %p (%zu bytes) from %s:%zu\n", peak_bytes, pointer, size, file, line);
                // fflush(stdout);
        }
}

static void remove_allocation(void *const pointer, const char *const file, const size_t line) {
        struct AllocationMESSAGE_INFORMATION **current_allocation_MESSAGE_INFORMATION = &allocation_MESSAGE_INFORMATIONs;

        while (*current_allocation_MESSAGE_INFORMATION != NULL) {
                if ((*current_allocation_MESSAGE_INFORMATION)->pointer == pointer) {
                        struct AllocationMESSAGE_INFORMATION *removed_allocation_MESSAGE_INFORMATION = *current_allocation_MESSAGE_INFORMATION;

                        active_bytes -= removed_allocation_MESSAGE_INFORMATION->size;
                        --active_allocations;

                        *current_allocation_MESSAGE_INFORMATION = removed_allocation_MESSAGE_INFORMATION->next;
                        free(removed_allocation_MESSAGE_INFORMATION);
                        return;
                }

                current_allocation_MESSAGE_INFORMATION = &(*current_allocation_MESSAGE_INFORMATION)->next;
        }

        fprintf(stderr, "xfree(%p): Unrecognized pointer at %s:%zu\n", pointer, file, line);
        fflush(stderr);
}

void *_malloc(const size_t size, const char *const file, const size_t line) {
        void *const allocated = malloc(size);
        if (allocated == NULL) {
                fprintf(stderr, "xmalloc(%zu): Out of memory at %s:%zu\n", size, file, line);
                fflush(stderr);

                exit(EXIT_FAILURE);
        }

        track_allocation(allocated, size, file, line);
        return allocated;
}

void *_calloc(const size_t count, const size_t size, const char *const file, const size_t line) {
        void *const allocated = calloc(count, size);
        if (allocated == NULL) {
                fprintf(stderr, "xcalloc(%zu, %zu): Out of memory at %s:%zu\n", count, size, file, line);
                fflush(stderr);

                exit(EXIT_FAILURE);
        }

        track_allocation(allocated, size, file, line);
        return allocated;
}

void *_realloc(void *const pointer, const size_t size, const char *const file, const size_t line) {
        if (pointer != NULL) {
                remove_allocation(pointer, file, line);
        }

        void *const reallocated = realloc(pointer, size);
        if (reallocated == NULL) {
                fprintf(stderr, "xrealloc(%p, %zu): Out of memory at %s:%zu\n", pointer, size, file, line);
                fflush(stderr);

                exit(EXIT_FAILURE);
        }

        track_allocation(reallocated, size, file, line);
        return reallocated;
}

char *_strdup(const char *const string, const char *const file, const size_t line) {
        char *const duplicated = strdup(string);
        if (duplicated == NULL) {
                fprintf(stderr, "xstrdup(%s): Out of memory at %s:%zu\n", string, file, line);
                fflush(stderr);

                exit(EXIT_FAILURE);
        }

        // NOTE: The size here is just the size of the pointer, but it should be fine
        track_allocation(duplicated, sizeof(duplicated), file, line);
        return duplicated;
}

void _free(void *const pointer, const char *const file, const size_t line) {
        if (pointer == NULL) {
                return;
        }

        remove_allocation(pointer, file, line);
        free(pointer);
}

#endif