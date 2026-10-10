#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "graphics_types.h"
#include "display_types.h"

esp_err_t uploaded_animation_store_save_rgb888(
    const uint8_t *data,
    size_t data_len,
    uint16_t width,
    uint16_t height,
    uint16_t frame_count,
    uint32_t frame_duration_ms,
    bool loop
);
const animation_source_t *uploaded_animation_store_get_source(void);