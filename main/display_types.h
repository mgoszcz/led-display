#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "graphics_types.h"

typedef struct {
    const char *text;
    rgb_t color;
    uint32_t speed_ms;
} text_display_config_t;

typedef struct {
    const led_matrix_image_t *frames;
    uint16_t frame_count;
    uint32_t frame_duration_ms;
    bool loop;
} animation_t;