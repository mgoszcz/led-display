#pragma once

#include "esp_err.h"
#include "graphics_types.h"

esp_err_t image_store_get(const char *name, const led_matrix_image_t **output_image);