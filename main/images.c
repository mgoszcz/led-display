#include "images.h"
#include <string.h>

#define Y ((rgb_t){255, 255, 0})
#define R ((rgb_t){255, 20, 0})
#define OFF ((rgb_t){0, 0, 0})

static const rgb_t smile_pixels[16 * 16] = {
       Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
        Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
        Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
        Y,Y,Y,Y,R,R,Y,Y,Y,Y,R,R,Y,Y,Y,Y,
        Y,Y,Y,Y,R,R,Y,Y,Y,Y,R,R,Y,Y,Y,Y,
        Y,Y,Y,Y,R,R,Y,Y,Y,Y,R,R,Y,Y,Y,Y,
        Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
        Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
        Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
        Y,Y,Y,R,Y,Y,Y,Y,Y,Y,Y,Y,R,Y,Y,Y,
        Y,Y,Y,Y,R,Y,Y,Y,Y,Y,Y,R,Y,Y,Y,Y,
        Y,Y,Y,Y,Y,R,R,R,R,R,R,Y,Y,Y,Y,Y,
        Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
        Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
        Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
        Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,
};

static const rgb_t lightning_pixels[16 * 16] = {
    OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,Y,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,Y,Y,Y,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,Y,Y,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,
    OFF,OFF,OFF,OFF,Y,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,OFF,
};

static const rgb_t heart_pixels[16 * 16] = {
    OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF,
    OFF, OFF, OFF, R,   R,   OFF, OFF, OFF, OFF, OFF, OFF, R,   R,   OFF, OFF, OFF,
    OFF, OFF, R,   R,   R,   R,   OFF, OFF, OFF, OFF, R,   R,   R,   R,   OFF, OFF,
    OFF, R,   R,   R,   R,   R,   R,   OFF, OFF, R,   R,   R,   R,   R,   R,   OFF,
    OFF, R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   OFF,
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,
    OFF, R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   OFF,
    OFF, OFF, R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   OFF, OFF,
    OFF, OFF, OFF, R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   OFF, OFF, OFF,
    OFF, OFF, OFF, OFF, R,   R,   R,   R,   R,   R,   R,   R,   OFF, OFF, OFF, OFF,
    OFF, OFF, OFF, OFF, OFF, R,   R,   R,   R,   R,   R,   OFF, OFF, OFF, OFF, OFF,
    OFF, OFF, OFF, OFF, OFF, OFF, R,   R,   R,   R,   OFF, OFF, OFF, OFF, OFF, OFF,
    OFF, OFF, OFF, OFF, OFF, OFF, OFF, R,   R,   OFF, OFF, OFF, OFF, OFF, OFF, OFF,
    OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF,
    OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF,
};

static const led_matrix_image_t smile_image = {
    .width = 16,
    .height = 16,
    .pixels = smile_pixels,
};

static const led_matrix_image_t lightning_image = {
    .width = 16,
    .height = 16,
    .pixels = lightning_pixels,
};

static const led_matrix_image_t heart_image = {
    .width = 16,
    .height = 16,
    .pixels = heart_pixels,
};

static const image_entry_t image_registry[] = {
    {"smile", &smile_image},
    {"lightning", &lightning_image},
    {"heart", &heart_image},
};

const led_matrix_image_t *images_find(const char *name) {
    if (name == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < sizeof(image_registry) / sizeof(image_registry[0]); i++) {
        if (strcmp(image_registry[i].name, name) == 0) {
            return image_registry[i].image;
        }
    }
    return NULL; // Image not found
}