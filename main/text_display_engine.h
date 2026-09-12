#pragma once

#include "framebuffer.h"
#include "esp_err.h"
#include "display_types.h"
#include <stdbool.h>
#include <stdint.h>

esp_err_t text_display_start(framebuffer_t *fb, const text_display_config_t *config);
esp_err_t text_display_stop(void);
bool text_display_is_running(void);