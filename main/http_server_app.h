#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include "display_types.h"

typedef esp_err_t (*http_frame_handler_t)(const uint8_t *data, size_t len);
typedef esp_err_t (*http_demo_handler_t)(void);
typedef esp_err_t (*http_brightness_handler_t)(int brightness);
typedef esp_err_t (*http_text_handler_t)(const text_display_config_t *config);

esp_err_t http_server_app_start(
    http_frame_handler_t frame_handler,
    http_demo_handler_t demo_handler,
    http_brightness_handler_t brightness_handler,
    http_text_handler_t text_handler
);
