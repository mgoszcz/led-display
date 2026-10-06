#include "animations.h"
#include <stdio.h>

#define Y ((rgb_t){255, 255, 0})
#define R ((rgb_t){255, 20, 0})
#define OFF ((rgb_t){0, 0, 0})

#define FRONT ((rgb_t){0, 180, 255})
#define TRAIL1 ((rgb_t){0, 60, 120})
#define TRAIL2 ((rgb_t){0, 20, 50})

#define SCANNER_FRAME_COUNT_MAX 32
#define DISPLAY_MAX_PIXELS 1024

static rgb_t s_pixels[SCANNER_FRAME_COUNT_MAX][DISPLAY_MAX_PIXELS];
static led_matrix_image_t s_frames[SCANNER_FRAME_COUNT_MAX];
static animation_t s_animation;

const animation_t *demo_animation(uint16_t width, uint16_t height) {
    if (width == 0 || height == 0 || width * height > DISPLAY_MAX_PIXELS) {
        return NULL;
    }
    uint16_t frame_count = width;
    if (frame_count > SCANNER_FRAME_COUNT_MAX) {
        frame_count = SCANNER_FRAME_COUNT_MAX;
    }
    for (int i = 0; i < frame_count; i++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint32_t index = y * width + x;
                if (x == i) {
                    s_pixels[i][index] = FRONT;
                } else if (i > 0 && x == i - 1) {
                    s_pixels[i][index] = TRAIL1;
                } else if (i > 1 && x == i - 2) {
                    s_pixels[i][index] = TRAIL2;
                } else {
                    s_pixels[i][index] = OFF;
                }
            }
        }
        s_frames[i] = (led_matrix_image_t){
            .width = width,
            .height = height,
            .pixels = s_pixels[i],
        };
    }
    s_animation = (animation_t){
        .frame_count = frame_count,
        .frame_duration_ms = 100,
        .frames = s_frames,
        .loop = true,
    };
    return &s_animation;
}