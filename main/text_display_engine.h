#pragma once

#include "esp_err.h"
#include "display_types.h"
#include <stdbool.h>
#include <stdint.h>

esp_err_t text_display_stop(void);
bool text_display_is_running(void);

typedef esp_err_t (*text_frame_renderer_t)(const led_matrix_image_t *frame);

esp_err_t text_display_start(
    const text_display_config_t *config,
    text_frame_renderer_t render_frame
);