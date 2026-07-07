#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "framebuffer.h"
#include "graphics_types.h"


#define BLINK_GPIO 4
#define MAX_Y 16
#define MAX_X 16

typedef enum {
    LED_MATRIX_ORIGIN_TOP_LEFT,
    LED_MATRIX_ORIGIN_BOTTOM_LEFT,
} led_matrix_origin_t;

typedef enum {
    LED_MATRIX_LAYOUT_PROGRESSIVE,
    LED_MATRIX_LAYOUT_SERPENTINE,
} led_matrix_layout_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t gpio;

    led_matrix_origin_t origin;
    led_matrix_layout_t layout;
} led_matrix_config_t;


esp_err_t led_matrix_init(const led_matrix_config_t *config);
esp_err_t led_matrix_clear(void);
esp_err_t led_matrix_set_pixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue);
void led_matrix_set_brightness(uint8_t new_brightness);
esp_err_t led_matrix_show(void);
esp_err_t led_matrix_display_image(const led_matrix_image_t *image);
esp_err_t led_matrix_render_framebuffer(const framebuffer_t *fb);