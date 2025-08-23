#include "Text.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <SDL.h>
#include <SDL_ttf.h>

#include "Assets.h"
#include "Context.h"
#include "Utilities.h"

struct TextImplementation {
        char *string;
        enum Font font;
        enum TextAlignment alignment;
        float maximum_width;
        float line_spacing;
        bool outdated_texture;
        SDL_Texture *texture;
        size_t texture_width;
        size_t texture_height;
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
};

static bool refresh_text(struct Text *const text);

struct Text *create_text(const char *const string, const enum Font font) {
        struct Text *const text = (struct Text *)xmalloc(sizeof(struct Text));
        initialize_text(text, string, font);
        return text;
}

void destroy_text(struct Text *const text) {
        if (!text) {
                send_message(WARNING, "Text given to destroy is NULL");
                return;
        }

        deinitialize_text(text);
        xfree(text);
}

void initialize_text(struct Text *const text, const char *const string, const enum Font font) {
        text->screen_position_x = 0.0f;
        text->screen_position_y = 0.0f;
        text->relative_offset_x = 0.0f;
        text->relative_offset_y = 0.0f;
        text->absolute_offset_x = 0.0f;
        text->absolute_offset_y = 0.0f;
        text->scale_x = 1.0f;
        text->scale_y = 1.0f;
        text->rotation = 0.0f;
        text->visible = true;

        text->implementation = (struct TextImplementation *)xmalloc(sizeof(struct TextImplementation));
        text->implementation->string = strdup(string);
        text->implementation->font = font;
        text->implementation->alignment = TEXT_ALIGNMENT_LEFT;
        text->implementation->maximum_width = 0.0f;
        text->implementation->line_spacing = 0.0f;
        text->implementation->outdated_texture = true;
        text->implementation->texture = NULL;
        text->implementation->texture_width = 0ULL;
        text->implementation->texture_height = 0ULL;
        text->implementation->r = 255;
        text->implementation->g = 255;
        text->implementation->b = 255;
        text->implementation->a = 255;
}

void deinitialize_text(struct Text *const text) {
        if (!text) {
                send_message(WARNING, "Text given to deinitialize is NULL");
                return;
        }

        if (text->implementation) {
                if (text->implementation->string) {
                        free(text->implementation->string);
                }

                if (text->implementation->texture) {
                        SDL_DestroyTexture(text->implementation->texture);
                }

                xfree(text->implementation);
                text->implementation = NULL;
        }
}

void update_text(struct Text *const text) {
        if (text->implementation->outdated_texture) {
                text->implementation->outdated_texture = false;
                if (!refresh_text(text)) {
                        send_message(ERROR, "Failed to refresh text!");
                        // TODO: Handle
                }
        }

        if (text->scale_x == 0.0f || text->scale_y == 0.0f) {
                return;
        }

        int drawable_width;
        int drawable_height;
        SDL_GetRendererOutputSize(get_context_renderer(), &drawable_width, &drawable_height);

        SDL_Rect destination = (SDL_Rect){
                .x = (int)(text->screen_position_x * (float)drawable_width  + text->relative_offset_x * text->implementation->texture_width  + text->absolute_offset_x),
                .y = (int)(text->screen_position_y * (float)drawable_height + text->relative_offset_y * text->implementation->texture_height + text->absolute_offset_y),
                .w = (int)(text->implementation->texture_width  * fabsf(text->scale_x)),
                .h = (int)(text->implementation->texture_height * fabsf(text->scale_y))
        };

        SDL_RendererFlip renderer_flip = SDL_FLIP_NONE;

        if (text->scale_x < 0.0f) {
                renderer_flip &= SDL_FLIP_HORIZONTAL;
        }

        if (text->scale_y < 0.0f) {
                renderer_flip &= SDL_FLIP_VERTICAL;
        }

        SDL_SetTextureAlphaMod(text->implementation->texture, (Uint8)text->implementation->a);
        SDL_RenderCopyEx(get_context_renderer(), text->implementation->texture, NULL, &destination, text->rotation * 180.0f / (float)M_PI, NULL, renderer_flip);
}

size_t get_text_width(struct Text *const text) {
        if (text->implementation->outdated_texture) {
                text->implementation->outdated_texture = false;
                if (!refresh_text(text)) {
                        send_message(ERROR, "Failed to refresh text to get text width: Keeping outdated texture for now");
                }
        }

        return text->implementation->texture_width;
}

size_t get_text_height(struct Text *const text) {
        if (text->implementation->outdated_texture) {
                text->implementation->outdated_texture = false;
                if (!refresh_text(text)) {
                        send_message(ERROR, "Failed to refresh text to get text height: Keeping outdated texture for now");
                }
        }

        return text->implementation->texture_height;
}

void set_text_string(struct Text *const text, const char *const string) {
        free(text->implementation->string);
        text->implementation->string = strdup(string);
        text->implementation->outdated_texture = true;
}

void set_text_format(struct Text *const text, const enum Font font) {
        text->implementation->font = font;
        text->implementation->outdated_texture = true;
}

void set_text_alignment(struct Text *const text, const enum TextAlignment alignment) {
        text->implementation->alignment = alignment;
        text->implementation->outdated_texture = true;
}

void set_text_maximum_width(struct Text *const text, const float maximum_width) {
        text->implementation->maximum_width = maximum_width;
        text->implementation->outdated_texture = true;
}

void set_text_line_spacing(struct Text *const text, const float line_spacing) {
        text->implementation->line_spacing = line_spacing;
        text->implementation->outdated_texture = true;
}

void set_text_color(struct Text *const text, const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) {
        if (text->implementation->r == r && text->implementation->g == g && text->implementation->b == b) {
                text->implementation->a = a;
                return;
        }

        text->implementation->r = r;
        text->implementation->g = g;
        text->implementation->b = b;
        text->implementation->a = a;

        text->implementation->outdated_texture = true;
}

#define MAXIMUM_WORD_SIZE 512ULL
#define MAXIMUM_LINE_SIZE 1024ULL
#define MAXIMUM_LINE_COUNT 128ULL

static bool refresh_text(struct Text *const text) {
        TTF_Font *const font = get_font(text->implementation->font);

        char word_buffer[MAXIMUM_WORD_SIZE];
        char line_buffer[MAXIMUM_LINE_SIZE] = "";
        char *lines[MAXIMUM_LINE_COUNT];
        size_t line_count = 0ULL;
        size_t current_width = 0ULL;
        size_t total_width = 0ULL;

        int space_width;
        TTF_SizeUTF8(font, " ", &space_width, NULL);

        int line_height;
        TTF_SizeUTF8(font, "Ay", NULL, &line_height);

        const size_t line_gap = (size_t)((float)line_height * text->implementation->line_spacing);
        const size_t maximum_line_width = text->implementation->maximum_width == 0.0f ? SIZE_MAX : (size_t)roundf(text->implementation->maximum_width);

        bool previous_space = false;
        const char *position = text->implementation->string;
        while (*position && line_count < MAXIMUM_LINE_COUNT) {
                if (*position == ' ' || *position == '\t') {
                        size_t space_count = 0;
                        while (*position == ' ' || *position == '\t') {
                                ++space_count;
                                ++position;
                        }

                        // build a string containing that many spaces
                        if (space_count >= sizeof(word_buffer)) {
                                send_message(ERROR, "Too many consecutive spaces");
                                return false;
                        }
                        memset(word_buffer, ' ', space_count);
                        word_buffer[space_count] = '\0';

                        int word_width;
                        TTF_SizeUTF8(font, word_buffer, &word_width, NULL);

                        // append directly to line buffer
                        strncat(line_buffer, word_buffer, sizeof(line_buffer) - strlen(line_buffer) - 1ULL);
                        current_width += (size_t)word_width;

                        previous_space = true;
                        continue; // skip to next iteration
                }

                const char *word_start = position;
                while (*position && *position != ' ' && *position != '\t' && *position != '\n') {
                        ++position;
                }

                const size_t word_length = position - word_start;
                if (word_length >= sizeof(word_buffer)) {
                        send_message(ERROR, "Failed to refresh text: Text \"%s\" has word that exceeds the maximum word length of %zu", text->implementation->string, MAXIMUM_WORD_SIZE);
                        for (size_t line_index = 0ULL; line_index < line_count; ++line_index) {
                                xfree(lines[line_index]);
                        }

                        return false;
                }

                strncpy(word_buffer, word_start, word_length);
                word_buffer[word_length] = '\0';

                int word_width;
                TTF_SizeUTF8(font, word_buffer, &word_width, NULL);

                const size_t spacing = (line_buffer[0] != '\0') ? (size_t)space_width : 0ULL;
                const size_t projected_width = current_width + spacing + (size_t)word_width;
                if (projected_width > maximum_line_width && line_buffer[0] != '\0') {
                        lines[line_count++] = strdup(line_buffer);
                        if (current_width > total_width) {
                                total_width = current_width;
                        }

                        line_buffer[0] = '\0';
                        current_width = 0ULL;
                }

                if (line_buffer[0] && !previous_space) {
                        strncat(line_buffer, " ", sizeof(line_buffer) - strlen(line_buffer) - 1ULL);
                        current_width += spacing;
                }

                previous_space = false;

                strncat(line_buffer, word_buffer, sizeof(line_buffer) - strlen(line_buffer) - 1ULL);
                current_width += (size_t)word_width;

                if (*position == '\n' || projected_width > maximum_line_width) {
                        lines[line_count++] = strdup(line_buffer[0] ? line_buffer : " ");
                        if (current_width > total_width) {
                                total_width = current_width;
                        }

                        line_buffer[0] = '\0';
                        current_width = 0ULL;
                }

                if (*position == '\n') {
                        ++position;
                }
        }

        if (line_buffer[0] != '\0' && line_count < MAXIMUM_LINE_COUNT) {
                int line_width;
                TTF_SizeUTF8(font, line_buffer, &line_width, NULL);
                lines[line_count++] = strdup(line_buffer);
                if ((size_t)line_width > total_width) {
                        total_width = (size_t)line_width;
                }
        }

        if (!line_count) {
                send_message(ERROR, "Failed to refresh text: Text contains no visible content");
                return false;
        }

        const size_t total_height = line_count * line_height + (line_count - 1ULL) * line_gap;
        SDL_Surface *new_surface = SDL_CreateRGBSurfaceWithFormat(0, total_width, total_height, 32, SDL_PIXELFORMAT_RGBA32);
        if (!new_surface) {
                send_message(ERROR, "Failed to refresh text: Failed to create surface: %s", SDL_GetError());
                for (size_t line_index = 0ULL; line_index < line_count; ++line_index) {
                        xfree(lines[line_index]);
                }

                return false;
        }

        SDL_SetSurfaceBlendMode(new_surface, SDL_BLENDMODE_BLEND);
        const SDL_Color baked_color = (SDL_Color){
                .r = text->implementation->r,
                .g = text->implementation->g,
                .b = text->implementation->b,
                .a = 255
        };

        for (size_t line_index = 0ULL; line_index < line_count; ++line_index) {
                SDL_Surface *const line_surface = TTF_RenderUTF8_Blended(font, lines[line_index], baked_color);
                if (!line_surface) {
                        send_message(ERROR, "Failed to refresh text: Failed to render line %zu: %s", line_index, TTF_GetError());
                        for (size_t line_index = 0ULL; line_index < line_count; ++line_index) {
                                free(lines[line_index]);
                        }

                        SDL_FreeSurface(new_surface);
                        return false;
                }

                size_t left_side;
                switch (text->implementation->alignment) {
                        case TEXT_ALIGNMENT_LEFT: {
                                left_side = 0ULL;
                                break;
                        }

                        case TEXT_ALIGNMENT_CENTER: {
                                left_side = (total_width - (size_t)line_surface->w) / 2ULL;
                                break;
                        }

                        case TEXT_ALIGNMENT_RIGHT: {
                                left_side = total_width - (size_t)line_surface->w;
                                break;
                        }
                }

                SDL_Rect destination = (SDL_Rect){
                        .x = (int)left_side,
                        .y = line_index * (line_height + line_gap),
                        .w = line_surface->w,
                        .h = line_surface->h
                };

                SDL_BlitSurface(line_surface, NULL, new_surface, &destination);
                SDL_FreeSurface(line_surface);
        }

        for (size_t line_index = 0ULL; line_index < line_count; ++line_index) {
                free(lines[line_index]);
        }

        SDL_Texture *const new_texture = SDL_CreateTextureFromSurface(get_context_renderer(), new_surface);
        SDL_FreeSurface(new_surface);

        if (new_texture == NULL) {
                send_message(ERROR, "Failed to refresh text: Failed to create texture from surface: %s", SDL_GetError());
                return false;
        }

        SDL_DestroyTexture(text->implementation->texture);
        text->implementation->texture = new_texture;
        text->implementation->texture_width = total_width;
        text->implementation->texture_height = total_height;
        return true;
}