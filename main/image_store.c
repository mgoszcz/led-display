#include "image_store.h"
#include "images.h"

esp_err_t image_store_get (const char *name, const led_matrix_image_t **output_image) {
    if (name == NULL || output_image == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    const led_matrix_image_t *image = images_find(name);

    if (image == NULL) {
        *output_image = NULL;
        return ESP_ERR_NOT_FOUND;
    }
    *output_image = image;
    return ESP_OK;

}