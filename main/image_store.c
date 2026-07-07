#include "image_store.h"
#include "images.h"

const led_matrix_image_t *image_store_find(const char *name) {
    return images_find(name);
}