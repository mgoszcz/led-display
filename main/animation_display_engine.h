#pragma once
#include "esp_err.h"
#include "display_types.h"
#include <stdbool.h>

typedef esp_err_t (*animation_frame_renderer_t)(const led_matrix_image_t *frame);

// animation_source must remain valid until animation_display_stop() returns
// or the animation finishes.
esp_err_t animation_display_start_borrowed(const animation_source_t *animation_source, animation_frame_renderer_t render_frame);

esp_err_t animation_display_stop(void);

esp_err_t animation_display_init(uint16_t display_width, uint16_t display_height);

bool animation_is_running(void);