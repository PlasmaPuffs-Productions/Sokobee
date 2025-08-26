#include "Debug.h"

#include <SDL.h>

#ifdef NDEBUG

void initialize_debug_panel(void) {
        return;
}

void terminate_debug_panel(void) {
        return;
}

void debug_panel_receive_event(const SDL_Event *const event) {
        return;
}

void update_debug_panel(const double delta_time) {
        return;
}

#else

#include <stdlib.h>
#include <stdbool.h>

#include "Context.h"
#include "Geometry.h"
#include "Utilities.h"
#include "Text.h"

#if defined(PLATFORM_WINDOWS)

#include <windows.h>
#include <psapi.h>

bool get_process_memory_usage_bytes(size_t *const out_memory_usage_bytes) {
        if (out_memory_usage_bytes == NULL) {
                return false;
        }

        PROCESS_MEMORY_COUNTERS process_memory_counters;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &process_memory_counters, sizeof(process_memory_counters))) {
                *out_memory_usage_bytes = (size_t)process_memory_counters.WorkingSetSize;
                return true;
        }

        return false;
}

#elif defined(PLATFORM_APPLE)

#include <mach/mach.h>

bool get_process_memory_usage_bytes(size_t *const out_memory_usage_bytes) {
        if (out_memory_usage_bytes == NULL) {
                return false;
        }

        struct task_basic_info task_information;
        mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;

        if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&task_information, &count) == KERN_SUCCESS) {
                *out_memory_usage_bytes = (size_t)task_information.resident_size;
                return true;
        }

        return false;
}

#elif defined(PLATFORM_LINUX)

#include <stdio.h>
#include <unistd.h>

bool get_process_memory_usage_bytes(size_t *const out_memory_usage_bytes) {
        if (out_memory_usage_bytes == NULL) {
                return false;
        }

        FILE *file = fopen("/proc/self/statm", "r");
        if (file == NULL) {
                return false;
        }

        long pages = 0;
        if (fscanf(file, "%ld", &pages) != 1L) {
                fclose(file);
                return false;
        }

        fclose(file);

        *out_memory_usage_bytes (size_t)pages * (size_t)sysconf(_SC_PAGESIZE);
        return true;
}

#else

#warning "Unsupported platform for getting process memory usage"

bool get_process_memory_usage_bytes(size_t *const out_memory_usage_bytes) {
        return false;
}

#endif

#define REFRESH_MILLISECONDS 500
static double actual_time_elapsed = 0.0;
static double actual_time_accumulator = 0.0;

static Uint64 frame_start = 0;
static double time_accumulator = 0.0;
static double previous_frame_duration = 0.0;
static size_t frame_accumulator = 0ULL;
static size_t displayed_viewport_width = 0ULL;
static size_t displayed_viewport_height = 0ULL;

static struct Geometry *debug_panel_geometry;

static struct Text debug_FPS_text;
static struct Text debug_current_frame_time_text;
static struct Text debug_average_frame_time_text;
static struct Text debug_memory_usage_text;
static struct Text debug_vertex_count_text;
static struct Text debug_index_count_text;
static struct Text debug_viewport_width_text;
static struct Text debug_viewport_height_text;
static struct Text debug_time_elapsed_text;

#define DEBUG_TEXT_BUFFER_SIZE 32
#define DEBUG_TEXT_COLOR (SDL_Color){255, 255, 255, 255}
static struct Text *const debug_texts[] = {
        &debug_FPS_text,
        &debug_current_frame_time_text,
        &debug_average_frame_time_text,
        &debug_memory_usage_text,
        &debug_vertex_count_text,
        &debug_index_count_text,
        &debug_viewport_width_text,
        &debug_viewport_height_text,
        &debug_time_elapsed_text,
};

static void resize_debug_panel(void);

void start_debug_frame_profiling(void) {
        frame_start = SDL_GetPerformanceCounter();
        track_geometry_data();
}

void finish_debug_frame_profiling(void) {
        const Uint64 frame_finish = SDL_GetPerformanceCounter();
        previous_frame_duration = (double)(frame_finish - frame_start) / (double)SDL_GetPerformanceFrequency();

        ++frame_accumulator;
        time_accumulator += previous_frame_duration;
}

void initialize_debug_panel(void) {
        const uint8_t debug_text_count = (uint8_t)(sizeof(debug_texts) / sizeof(debug_texts[0]));
        for (uint8_t debug_text_index = 0; debug_text_index < debug_text_count; ++debug_text_index) {
                struct Text *const debug_text = debug_texts[debug_text_index];
                initialize_text(debug_text, "[Debug Text]", FONT_DEBUG);
                set_text_color(debug_text, COLOR_WHITE, 255);
                debug_text->relative_offset_y = -1.0f;
        }

        debug_panel_geometry = create_geometry();
        set_geometry_color(debug_panel_geometry, COLOR_BLACK, COLOR_OPAQUE / 2);
        resize_debug_panel();
}

void terminate_debug_panel(void) {
        destroy_geometry(debug_panel_geometry);
        debug_panel_geometry = NULL;

        const uint8_t debug_text_count = (uint8_t)(sizeof(debug_texts) / sizeof(debug_texts[0]));
        for (uint8_t debug_text_index = 0; debug_text_index < debug_text_count; ++debug_text_index) {
                deinitialize_text(debug_texts[debug_text_index]);
        }
}

void debug_panel_receive_event(const SDL_Event *const event) {
        if (event->type == SDL_WINDOWEVENT) {
                const Uint8 window_event = event->window.event;
                if (window_event == SDL_WINDOWEVENT_RESIZED || window_event == SDL_WINDOWEVENT_MAXIMIZED || window_event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        resize_debug_panel();
                }
        }
}

static void refresh_debug_panel(void);

void update_debug_panel(const double delta_time) {
        actual_time_elapsed += delta_time;
        actual_time_accumulator += delta_time;
        if (actual_time_accumulator >= REFRESH_MILLISECONDS) {
                while (actual_time_accumulator >= REFRESH_MILLISECONDS) {
                        actual_time_accumulator -= REFRESH_MILLISECONDS;
                }

                refresh_debug_panel();
        }

        render_geometry(debug_panel_geometry);

        const uint8_t debug_text_count = (uint8_t)(sizeof(debug_texts) / sizeof(debug_texts[0]));
        for (uint8_t debug_text_index = 0; debug_text_index < debug_text_count; ++debug_text_index) {
                update_text(debug_texts[debug_text_index]);
        }
}

static void resize_debug_panel(void) {
        const uint8_t debug_text_count = (uint8_t)(sizeof(debug_texts) / sizeof(debug_texts[0]));

        size_t debug_text_width = 0.0f, debug_text_height;
        get_text_dimensions(debug_texts[0], NULL, &debug_text_height);

        for (uint8_t debug_text_index = 0; debug_text_index < debug_text_count; ++debug_text_index) {
                size_t width;
                get_text_dimensions(debug_texts[debug_text_index], &width, NULL);

                if (width > debug_text_width) {
                        debug_text_width = width;
                }
        }

        int drawable_width;
        int drawable_height;
        SDL_GetRendererOutputSize(get_context_renderer(), &drawable_width, &drawable_height);
        const float padding = CLAMP_VALUE(MAXIMUM_VALUE((float)drawable_width, (float)drawable_height) / 100.0f, 10.0f, 20.0f);

        const float debug_panel_width = (float)debug_text_width + padding * 2.0f;
        const float debug_panel_height = (float)debug_text_height * (float)debug_text_count + padding * 2.0f;

        const float debug_panel_x =                           padding + debug_panel_width  / 2.0f;
        const float debug_panel_y = (float)drawable_height - (padding + debug_panel_height / 2.0f);

        clear_geometry(debug_panel_geometry);
        write_rounded_rectangle_geometry(
                debug_panel_geometry,
                debug_panel_x, debug_panel_y,
                debug_panel_width, debug_panel_height,
                (float)debug_text_height / 5.0f,
                0.0f
        );

        for (uint8_t debug_text_index = 0; debug_text_index < debug_text_count; ++debug_text_index) {
                const uint8_t reversed_index = debug_text_count - debug_text_index - 1;
                debug_texts[debug_text_index]->absolute_offset_x = padding * 2.0f;
                debug_texts[debug_text_index]->absolute_offset_y = (float)drawable_height - padding * 2.0f - (float)debug_text_height * (float)reversed_index;
        }
}

static void refresh_debug_panel(void) {
        char debug_text_buffer[DEBUG_TEXT_BUFFER_SIZE];
        const unsigned long debug_text_buffer_size = sizeof(debug_text_buffer);

        snprintf(debug_text_buffer, debug_text_buffer_size, "FPS:      %.3lf", (double)frame_accumulator / time_accumulator);
        set_text_string(&debug_FPS_text, debug_text_buffer);

        snprintf(debug_text_buffer, debug_text_buffer_size, "Current:  %.3lfms", previous_frame_duration * 1000.0);
        set_text_string(&debug_current_frame_time_text, debug_text_buffer);

        snprintf(debug_text_buffer, debug_text_buffer_size, "Average:  %.3lfms", time_accumulator * 1000.0 / (double)frame_accumulator);
        set_text_string(&debug_average_frame_time_text, debug_text_buffer);

        size_t memory_usage_bytes;
        if (get_process_memory_usage_bytes(&memory_usage_bytes)) {
                snprintf(debug_text_buffer, debug_text_buffer_size, "Memory:   %.1lfMB", (double)memory_usage_bytes / (1024.0 * 1024.0));
                set_text_string(&debug_memory_usage_text, debug_text_buffer);
        } else {
                set_text_string(&debug_memory_usage_text, "Memory:   Unknown");
        }

        size_t vertex_count;
        size_t index_count;
        get_tracked_geometry_data(&vertex_count, &index_count);

        snprintf(debug_text_buffer, debug_text_buffer_size, "Vertices: %zu", vertex_count);
        set_text_string(&debug_vertex_count_text, debug_text_buffer);

        snprintf(debug_text_buffer, debug_text_buffer_size, "Indices:  %zu", index_count);
        set_text_string(&debug_index_count_text, debug_text_buffer);

        int window_width;
        int window_height;
        SDL_GetWindowSizeInPixels(get_context_window(), &window_width, &window_height);

        if (displayed_viewport_width != (size_t)window_width || displayed_viewport_height != (size_t)window_height) {
                displayed_viewport_width  = (size_t)window_width;
                snprintf(debug_text_buffer, debug_text_buffer_size, "Width:    %zupx", displayed_viewport_width);
                set_text_string(&debug_viewport_width_text, debug_text_buffer);
        }

        if (displayed_viewport_height != (size_t)window_height) {
                displayed_viewport_height = (size_t)window_height;
                snprintf(debug_text_buffer, debug_text_buffer_size, "Height:   %zupx", displayed_viewport_height);
                set_text_string(&debug_viewport_height_text, debug_text_buffer);
        }

        snprintf(debug_text_buffer, debug_text_buffer_size, "Time:     %.3lfsec", actual_time_elapsed / 1000.0);
        set_text_string(&debug_time_elapsed_text, debug_text_buffer);

        time_accumulator = 0.0f;
        frame_accumulator = 0ULL;
        resize_debug_panel();
}

#endif