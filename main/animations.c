#include "animations.h"
#include "display_config.h"
#include <stddef.h>

#define OFF ((rgb_t){0, 0, 0})

#define FRONT ((rgb_t){0, 180, 255})
#define TRAIL1 ((rgb_t){0, 60, 120})
#define TRAIL2 ((rgb_t){0, 20, 50})

#define SCANNER_FRAME_COUNT_MAX 64

static animation_source_t s_scanner_animation;

static esp_err_t scanner_get_frame(
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

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t index = y * width + x;
            if (frame_index < width) {
                if (x == frame_index) {
                    out_pixels[index] = FRONT;
                } else if (frame_index > 0 && x == frame_index - 1) {
                    out_pixels[index] = TRAIL1;
                } else if (frame_index > 1 && x == frame_index - 2) {
                    out_pixels[index] = TRAIL2;
                } else {
                    out_pixels[index] = OFF;
                }
            } else {
                if (x == width - (frame_index + 1 - width)) {
                    out_pixels[index] = FRONT;
                } else if (frame_index > width && x == width - (frame_index - width)) {
                    out_pixels[index] = TRAIL1;
                } else if (frame_index > (width + 1) && x == width - (frame_index - 1 - width)) {
                    out_pixels[index] = TRAIL2;
                } else {
                    out_pixels[index] = OFF;
                }
            }
        }
    }
    return ESP_OK;
}

const animation_source_t *scanner_animation_create(uint16_t width, uint16_t height) {
    if (width < 2 || height == 0 || width * height > DISPLAY_MAX_PIXELS) {
        return NULL;
    }

    uint16_t frame_count = (width * 2) - 2;
    if (frame_count > SCANNER_FRAME_COUNT_MAX) {
        frame_count = SCANNER_FRAME_COUNT_MAX;
    }

    s_scanner_animation = (animation_source_t){
        .frame_count = frame_count,
        .frame_duration_ms = 100,
        .loop = true,
        .get_frame = scanner_get_frame,
    };

    return &s_scanner_animation;
}
