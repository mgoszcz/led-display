#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

typedef esp_err_t (*http_frame_handler_t)(const uint8_t *data, size_t len);
typedef esp_err_t (*http_demo_handler_t)(void);

esp_err_t http_server_app_start(http_frame_handler_t frame_handler, http_demo_handler_t demo_handler);
