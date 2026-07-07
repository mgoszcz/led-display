#include <stdio.h>
#include "led_matrix.h"
#include "image_store.h"
#include "framebuffer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "MAIN";
static rgb_t framebuffer_pixels[16 * 16];
static framebuffer_t fb;


void app_main(void)
{
    led_matrix_config_t config = {
    .width = 16,
    .height = 16,
    .gpio = 4,
    .origin = LED_MATRIX_ORIGIN_BOTTOM_LEFT,
    .layout = LED_MATRIX_LAYOUT_SERPENTINE,
};

    ESP_ERROR_CHECK(framebuffer_init(&fb, 16, 16, framebuffer_pixels, 256));
    
    ESP_ERROR_CHECK(led_matrix_init(&config));
    ESP_ERROR_CHECK(led_matrix_clear());
    
    const led_matrix_image_t *smile_image = image_store_find("smile");
    ESP_ERROR_CHECK(smile_image == NULL ? ESP_ERR_NOT_FOUND : ESP_OK);
    const led_matrix_image_t *lightning_image = image_store_find("lightning");
    ESP_ERROR_CHECK(lightning_image == NULL ? ESP_ERR_NOT_FOUND : ESP_OK);
    const led_matrix_image_t *heart_image = image_store_find("heart");
    ESP_ERROR_CHECK(heart_image == NULL ? ESP_ERR_NOT_FOUND : ESP_OK);
    while (1) {
        ESP_ERROR_CHECK(framebuffer_draw_image(&fb, smile_image, 0, 0));
        ESP_ERROR_CHECK(led_matrix_render_framebuffer(&fb));
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5 seconds
        ESP_ERROR_CHECK(framebuffer_draw_image(&fb, lightning_image, 0, 0));
        ESP_ERROR_CHECK(led_matrix_render_framebuffer(&fb));
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5 seconds
        ESP_ERROR_CHECK(framebuffer_draw_image(&fb, heart_image, 0, 0));
        ESP_ERROR_CHECK(led_matrix_render_framebuffer(&fb));
        vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5 seconds
    }
}
