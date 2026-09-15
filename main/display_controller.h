#pragma once
#include "esp_err.h"
#include "graphics_types.h"
#include "display_types.h"

typedef enum {
    DISPLAY_MODE_IDLE,
    DISPLAY_MODE_DEMO,
    DISPLAY_MODE_FRAME,
    DISPLAY_MODE_TEXT,
} display_mode_t;

esp_err_t display_controller_init(uint16_t width, uint16_t height);
esp_err_t display_controller_display_frame_rgb888(const uint8_t *data, size_t len);
esp_err_t display_controller_start_demo(void);
esp_err_t display_controller_set_brightness(int brightness);
esp_err_t display_controller_display_text(const text_display_config_t *config);
esp_err_t display_controller_display_demo_image(const led_matrix_image_t *image);
display_mode_t display_controller_get_mode(void);