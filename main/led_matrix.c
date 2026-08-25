#include "led_matrix.h"
#include "led_strip.h"

static led_strip_handle_t led_strip = NULL;
static uint8_t s_brightness_raw = 20; // 0-255
static led_matrix_config_t s_config;

static uint8_t apply_brightness(uint8_t value)
{
    return (value * s_brightness_raw) / 255;
}

static int xy_to_index(int x, int y)
{
    int physical_y = y;

    if (s_config.origin == LED_MATRIX_ORIGIN_BOTTOM_LEFT) {
        physical_y = s_config.height - 1 - y;
    }

    if (s_config.layout == LED_MATRIX_LAYOUT_SERPENTINE) {
        if (physical_y % 2 == 0) {
            return physical_y * s_config.width + x;
        } else {
            return physical_y * s_config.width + (s_config.width - 1 - x);
        }
    }

    return physical_y * s_config.width + x;
}

esp_err_t led_matrix_init(const led_matrix_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->width <= 0 || config->height <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    s_config = *config;

    led_strip_config_t strip_config = {
        .strip_gpio_num = s_config.gpio,
        .max_leds = s_config.width * s_config.height,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        }
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false,
        }
    };
  
    return led_strip_new_rmt_device(
        &strip_config,
        &rmt_config,
        &led_strip
    );
}

esp_err_t led_matrix_clear(void) {
    esp_err_t err_clear = led_strip_clear(led_strip);
    if (err_clear != ESP_OK) {
        return err_clear;
    }
    return led_strip_refresh(led_strip);
}

esp_err_t led_matrix_set_pixel(int x, int y, uint8_t red, uint8_t green, uint8_t blue) {
    if (x < 0 || x >= s_config.width) {
        return ESP_ERR_INVALID_ARG;
    }

    if (y < 0 || y >= s_config.height) {
        return ESP_ERR_INVALID_ARG;
    }
    int index = xy_to_index(x, y);
    return led_strip_set_pixel(
        led_strip, 
        index, 
        apply_brightness(red), 
        apply_brightness(green), 
        apply_brightness(blue)
    );
}

esp_err_t led_matrix_show(void) {
    return led_strip_refresh(led_strip);
}

void led_matrix_set_brightness(uint8_t new_brightness) {
    s_brightness_raw = (255 * new_brightness) / 100; // Convert 0-100 to 0-255
}

esp_err_t led_matrix_display_image(const led_matrix_image_t *image) {
    if (image == NULL || image->pixels == NULL)
        return ESP_ERR_INVALID_ARG;
    
    if (image->width != s_config.width)
        return ESP_ERR_INVALID_SIZE;

    if (image->height != s_config.height)
        return ESP_ERR_INVALID_SIZE;

    for (int y = 0; y < s_config.height; y++) {
        for (int x = 0; x < s_config.width; x++) {
            int image_index = y * s_config.width + x;
            const rgb_t *pixel = &image->pixels[image_index];
            esp_err_t err_set_pixel = led_matrix_set_pixel(x, y, pixel->r, pixel->g, pixel->b);
            if (err_set_pixel != ESP_OK) {
                return err_set_pixel;
            }
        }
    }

    return led_matrix_show();
}

esp_err_t led_matrix_render_framebuffer(const framebuffer_t *fb) {
    if (fb == NULL || fb->pixels == NULL) return ESP_ERR_INVALID_ARG;
    if (fb->width != s_config.width || fb->height != s_config.height) return ESP_ERR_INVALID_SIZE;
    
    for (int y = 0; y < fb->height; y++) {
        for (int x = 0; x < fb->width; x++) {
            int image_index = y * fb->width + x;
            const rgb_t *pixel = &fb->pixels[image_index];
            esp_err_t err_set_pixel = led_matrix_set_pixel(x, y, pixel->r, pixel->g, pixel->b);
            if (err_set_pixel != ESP_OK) {
                return err_set_pixel;
            }
        }
    }
    return led_matrix_show();
}