#pragma once

#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    const rgb_t *pixels;
} led_matrix_image_t;