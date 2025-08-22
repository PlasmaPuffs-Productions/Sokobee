#include "Memory.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef NDEBUG

struct AllocationInformation {
        void *pointer;
        size_t size;
        const char *file;
        size_t line;
        struct AllocationInformation *next;
};

static struct AllocationInformation *allocation_informations = NULL;
static size_t active_allocations = 0ULL;
static size_t active_bytes = 0ULL;
static size_t peak_bytes = 0ULL;

void flush_memory_leaks(void) {
        if (allocation_informations == NULL) {
                fprintf(stdout, "flush_memory_leaks(): No leaked memory\n");
                fflush(stdout);
                return;
        }

        fprintf(stderr, "flush_memory_leaks(): %zu active allocations (totalling %zu bytes) leaked\n", active_allocations, active_bytes);
        for (struct AllocationInformation *current_allocation_information = allocation_informations; current_allocation_information != NULL; current_allocation_information = current_allocation_information->next) {
                fprintf(stderr, "flush_memory_leaks(): %p (%zu bytes) allocated at %s:%zu\n", current_allocation_information->pointer, current_allocation_information->size, current_allocation_information->file, current_allocation_information->line);
        }

        fflush(stderr);
}

static void track_allocation(void *const pointer, const size_t size, const char *const file, const size_t line) {
        struct AllocationInformation *const allocation_information = malloc(sizeof(struct AllocationInformation));
        if (allocation_information == NULL) {
                exit(EXIT_FAILURE);
        }

        allocation_information->pointer = pointer;
        allocation_information->size = size;
        allocation_information->file = file;
        allocation_information->line = line;
        allocation_information->next = allocation_informations;
        allocation_informations = allocation_information;

        ++active_allocations;
        active_bytes += size;

        if (active_bytes > peak_bytes) {
                peak_bytes = active_bytes;
                // fprintf(stdout, "WARNING: Peak memory usage (of %zu bytes) reached with %p (%zu bytes) from %s:%zu\n", peak_bytes, pointer, size, file, line);
                // fflush(stdout);
        }
}

static void remove_allocation(void *const pointer, const char *const file, const size_t line) {
        struct AllocationInformation **current_allocation_information = &allocation_informations;

        while (*current_allocation_information != NULL) {
                if ((*current_allocation_information)->pointer == pointer) {
                        struct AllocationInformation *removed_allocation_information = *current_allocation_information;

                        active_bytes -= removed_allocation_information->size;
                        --active_allocations;

                        *current_allocation_information = removed_allocation_information->next;
                        free(removed_allocation_information);
                        return;
                }

                current_allocation_information = &(*current_allocation_information)->next;
        }

        fprintf(stderr, "xfree(%p): Unrecognized pointer at %s:%zu\n", pointer, file, line);
        fflush(stderr);
}

void *_malloc(const size_t size, const char *const file, const size_t line) {
        void *const pointer = malloc(size);
        if (pointer == NULL) {
                fprintf(stderr, "xmalloc(%zu): Out of memory at %s:%zu\n", size, file, line);
                fflush(stderr);

                exit(EXIT_FAILURE);
        }

        track_allocation(pointer, size, file, line);
        return pointer;
}

void *_calloc(const size_t count, const size_t size, const char *const file, const size_t line) {
        void *const pointer = calloc(count, size);
        if (pointer == NULL) {
                fprintf(stderr, "xcalloc(%zu, %zu): Out of memory at %s:%zu\n", count, size, file, line);
                fflush(stderr);

                exit(EXIT_FAILURE);
        }

        track_allocation(pointer, size, file, line);
        return pointer;
}

void *_realloc(void *const pointer, const size_t size, const char *const file, const size_t line) {
        if (pointer != NULL) {
                remove_allocation(pointer, file, line);
        }

        void *const next_pointer = realloc(pointer, size);
        if (next_pointer == NULL) {
                fprintf(stderr, "xrealloc(%p, %zu): Out of memory at %s:%zu\n", pointer, size, file, line);
                fflush(stderr);

                exit(EXIT_FAILURE);
        }

        track_allocation(next_pointer, size, file, line);
        return next_pointer;
}

void _free(void *const pointer, const char *const file, const size_t line) {
        if (pointer == NULL) {
                return;
        }

        remove_allocation(pointer, file, line);
        free(pointer);
}

#endif