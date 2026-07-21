#pragma once

#include "graphics_types.h"
#include "esp_err.h"
#include <stddef.h>

typedef struct {
    uint16_t width;
    uint16_t height;
    rgb_t *pixels;
} framebuffer_t;

esp_err_t framebuffer_init(framebuffer_t *fb,
                           uint16_t width,
                           uint16_t height,
                           rgb_t *pixels,
                           size_t pixel_count);

esp_err_t framebuffer_clear(framebuffer_t *fb);

esp_err_t framebuffer_set_pixel(framebuffer_t *fb,
                                int x,
                                int y,
                                rgb_t color);

esp_err_t framebuffer_draw_image(framebuffer_t *fb,
                                 const led_matrix_image_t *image,
                                 int dst_x,
                                 int dst_y);

esp_err_t framebuffer_fill(framebuffer_t *fb,
                                rgb_t color);

esp_err_t framebuffer_draw_rgb888(framebuffer_t *fb,
                                    uint16_t width,
                                    uint16_t height,
                                    const uint8_t *data,
                                    size_t data_len
);