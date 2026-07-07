// images.h
#pragma once

#include "graphics_types.h"

typedef struct {
    const char *name;
    const led_matrix_image_t *image;
} image_entry_t;

const led_matrix_image_t *images_find(const char *name);