#include "uploaded_animation_store.h"
#include "display_config.h"
#include "esp_log.h"

static const char *TAG = "UPLOADED_ANIMATION_STORE";

static rgb_t s_frames[UPLOADED_ANIMATION_MAX_PIXELS];
static animation_source_t s_source;
static uint16_t s_frame_count;
static uint16_t s_width;
static uint16_t s_height;
static uint32_t s_frame_duration;
static bool s_loop;

esp_err_t uploaded_animation_store_save_rgb888(
    const uint8_t *data,
    size_t data_len,
    uint16_t width,
    uint16_t height,
    uint16_t frame_count,
    uint32_t frame_duration_ms,
    bool loop) {
        if (data == NULL) {
        ESP_LOGE(TAG, "Data cannot be NULL");
        return ESP_ERR_INVALID_ARG;
        }
        if (width == 0 || height == 0) {
            ESP_LOGE(TAG, "Width and height must be greater than 0");
            return ESP_ERR_INVALID_ARG;
        }
        if (frame_count == 0) {
            ESP_LOGE(TAG, "Frame count must be greater than 0");
            return ESP_ERR_INVALID_ARG;
        }
        if (width * height > DISPLAY_MAX_PIXELS) {
            ESP_LOGE(TAG, "Width and height exceeds max dimensions");
            return ESP_ERR_INVALID_ARG;
        }
        if (frame_count > UPLOADED_ANIMATION_MAX_FRAMES) {
            ESP_LOGE(TAG, "Frame count exceeds limit");
            return ESP_ERR_INVALID_ARG;
        }
        size_t pixel_count = (size_t)frame_count * width * height;
        size_t expected_len = pixel_count * 3;
        if (data_len != expected_len) {
            ESP_LOGE(TAG, "Invalid data length");
            return ESP_ERR_INVALID_ARG;
        }
        if (pixel_count > UPLOADED_ANIMATION_MAX_PIXELS) {
            ESP_LOGE(TAG, "Pixels count exceeds limit");
            return ESP_ERR_INVALID_ARG;
        }
        s_frame_count = frame_count;
        s_width = width;
        s_height = height;
        s_frame_duration = frame_duration_ms;
        s_loop = loop;
        for (size_t i = 0; i < pixel_count; i++) {
            size_t data_index = i * 3;
            rgb_t color = {
                .r = data[data_index + 0],
                .g = data[data_index + 1],
                .b = data[data_index + 2],
            };
            s_frames[i] = color;
        }
        return ESP_OK;
}

static esp_err_t uploaded_animation_get_frame(
    uint16_t frame_index,
    rgb_t *out_pixels,
    uint16_t width,
    uint16_t height
) {
    if (width == 0 || height == 0 || width * height > DISPLAY_MAX_PIXELS) {
        return ESP_ERR_INVALID_ARG;
    }

    if (out_pixels == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (frame_index >= s_frame_count) {
        return ESP_ERR_INVALID_ARG;
    }
    if (width != s_width || height != s_height) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t pixels_per_frame = (size_t)width * height;
    size_t start_index = (size_t)frame_index * pixels_per_frame;
    for (size_t i = 0; i < pixels_per_frame; i++) {
        out_pixels[i] = s_frames[start_index + i];
    }
    return ESP_OK;
}

const animation_source_t *uploaded_animation_store_get_source(void) {
    if (s_frame_count == 0 || s_width == 0 || s_height == 0) {
        return NULL;
    }
    s_source = (animation_source_t){
        .frame_count = s_frame_count,
        .frame_duration_ms = s_frame_duration,
        .loop = s_loop,
        .get_frame = uploaded_animation_get_frame
    };
    return &s_source;
}