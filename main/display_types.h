#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "graphics_types.h"

typedef struct {
    const char *text;
    rgb_t color;
    uint32_t speed_ms;
} text_display_config_t;

typedef esp_err_t (*animation_get_frame_t)(
    uint16_t frame_index,
    rgb_t *out_pixels,
    uint16_t width,
    uint16_t height
);

// frame_count must be greater than 0,
// frame_duration_ms == 0 means default, 
// get_frame must not be NULL and must fill the out_pixels buffer with width * height pixels for the given frame_index.
// out_pixels must be a buffer not a pointer to a pointer, and must be large enough to hold width * height pixels.
// animations_source_t must live until animation_display_stop() returns or the animation finishes.
typedef struct {
    uint16_t frame_count;
    uint32_t frame_duration_ms;
    bool loop;
    animation_get_frame_t get_frame;
} animation_source_t;