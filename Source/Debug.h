#pragma once

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

void start_debug_frame_profiling(void);
void finish_debug_frame_profiling(void);

void initialize_debug_panel(void);
void terminate_debug_panel(void);

typedef union SDL_Event SDL_Event;
bool debug_panel_receive_event(const SDL_Event *const event);
void update_debug_panel(const double delta_time);

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
        #include "Memory.h"

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