#pragma once

#include <stdint.h>
#include "graphics_types.h"

typedef struct {
    const char *text;
    rgb_t color;
    uint32_t speed_ms;
} text_display_config_t;