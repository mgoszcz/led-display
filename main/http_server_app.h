#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include "display_types.h"

typedef esp_err_t (*http_frame_handler_t)(const uint8_t *data, size_t len);
typedef esp_err_t (*http_demo_handler_t)(void);
typedef esp_err_t (*http_brightness_handler_t)(int brightness);
typedef esp_err_t (*http_text_handler_t)(const text_display_config_t *config);
typedef esp_err_t (*http_animation_handler_t)(
    const uint8_t *data,
    size_t len,
    uint16_t width,
    uint16_t height,
    uint16_t frame_count,
    uint32_t frame_duration_ms,
    bool loop
);
typedef esp_err_t (*http_demo_animation_handler_t)(void);
typedef esp_err_t (*http_wifi_connect_handler_t)(const char *ssid, const char *password);


esp_err_t http_server_app_start(
    uint16_t display_width,
    uint16_t display_height,
    http_frame_handler_t frame_handler,
    http_demo_handler_t demo_handler,
    http_brightness_handler_t brightness_handler,
    http_text_handler_t text_handler,
    http_animation_handler_t animation_handler,
    http_demo_animation_handler_t demo_animation_handler,
    http_wifi_connect_handler_t wifi_connect_handler
);
