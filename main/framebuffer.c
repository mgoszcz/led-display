#include "framebuffer.h"

esp_err_t framebuffer_init(framebuffer_t *fb, uint16_t width, uint16_t height, rgb_t *pixels, size_t pixel_count) {
    if (fb == NULL || width == 0 || height == 0 || pixels == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (pixel_count != width * height) {
        return ESP_ERR_INVALID_ARG;
    }

    fb->width = width;
    fb->height = height;
    fb->pixels = pixels;

    return ESP_OK;
}

esp_err_t framebuffer_fill(framebuffer_t *fb, rgb_t color) {
    if (fb == NULL || fb->pixels == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < fb->width * fb->height; i++) {
        fb->pixels[i] = color;
    }
    return ESP_OK;
}

esp_err_t framebuffer_clear(framebuffer_t *fb)
{
    return framebuffer_fill(fb, (rgb_t){0, 0, 0});
}

esp_err_t framebuffer_set_pixel(framebuffer_t *fb, int x, int y, rgb_t color) {
    if (fb == NULL || fb->pixels == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int index = y * fb->width + x;
    fb->pixels[index] = color;
    return ESP_OK;
}

esp_err_t framebuffer_draw_image(framebuffer_t *fb, const led_matrix_image_t *image, int dst_x, int dst_y) {
    if (fb == NULL || fb->pixels == NULL || image == NULL || image->pixels == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (image->width > fb->width || image->height > fb->height) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (dst_x < 0 || dst_y < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if ((uint16_t)dst_x > fb->width - image->width) {
        return ESP_ERR_INVALID_SIZE;
    }

    if ((uint16_t)dst_y > fb->height - image->height) {
        return ESP_ERR_INVALID_SIZE;
    }

    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            int fb_x = dst_x + x;
            int fb_y = dst_y + y;

            rgb_t pixel_color = image->pixels[y * image->width + x];
            esp_err_t err = framebuffer_set_pixel(fb, fb_x, fb_y, pixel_color);
            if (err != ESP_OK) {
                return err;
            }
        }
    }
    return ESP_OK;
}

esp_err_t framebuffer_draw_rgb888(framebuffer_t *fb, uint16_t width, uint16_t height, const uint8_t *data, size_t data_len) {
    if (fb == NULL || data == NULL || fb->pixels == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (width == 0 || height == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (data_len != width * height * 3) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (width != fb->width || height != fb->height) {
        return ESP_ERR_INVALID_SIZE;
    }

    size_t pixel_count = width * height;

    for (size_t i = 0; i < pixel_count; i++) {
        size_t data_index = i * 3;
        rgb_t color = {
            .r = data[data_index + 0],
            .g = data[data_index + 1],
            .b = data[data_index + 2],
        };
        fb->pixels[i] = color;
    }
    return ESP_OK;
}
