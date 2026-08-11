#include "led_matrix.h"
#include "image_store.h"
#include "framebuffer.h"
#include "wifi_app.h"
#include "http_server_app.h"
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"

static const char *TAG = "MAIN";
static rgb_t framebuffer_pixels[16 * 16];
static framebuffer_t fb;
static bool s_demo_enabled = true;

static esp_err_t display_image(const led_matrix_image_t *image) {
    ESP_RETURN_ON_ERROR(framebuffer_clear(&fb), TAG, "Failed to clear framebuffer");
    ESP_RETURN_ON_ERROR(framebuffer_draw_image(&fb, image, 0, 0), TAG, "Failed to draw image");
    ESP_RETURN_ON_ERROR(led_matrix_render_framebuffer(&fb), TAG, "Failed to render framebuffer");
    return ESP_OK;
}

static esp_err_t handle_frame_upload(const uint8_t *data, size_t len)
{
    s_demo_enabled = false; // Disable demo mode when a frame is uploaded
    ESP_RETURN_ON_ERROR(framebuffer_draw_rgb888(&fb, 16, 16, data, len), TAG, "Failed to draw RGB888 frame");
    ESP_RETURN_ON_ERROR(led_matrix_render_framebuffer(&fb), TAG, "Failed to render framebuffer");

    return ESP_OK;
}

static esp_err_t handle_demo_enable(void)
{
    s_demo_enabled = true; // Enable demo mode
    return ESP_OK;
}

static void demo_delay(void)
{
    for (int i = 0; i < 50; i++) {
        if (!s_demo_enabled) {
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    led_matrix_config_t config = {
    .width = 16,
    .height = 16,
    .gpio = 4,
    .origin = LED_MATRIX_ORIGIN_BOTTOM_LEFT,
    .layout = LED_MATRIX_LAYOUT_SERPENTINE,
};

    const led_matrix_image_t *heart_image = NULL;
    const led_matrix_image_t *smile_image = NULL;
    const led_matrix_image_t *lightning_image = NULL;

    ESP_ERROR_CHECK(framebuffer_init(&fb, 16, 16, framebuffer_pixels, 256));
    
    ESP_ERROR_CHECK(led_matrix_init(&config));
    ESP_ERROR_CHECK(wifi_app_start());
    ESP_ERROR_CHECK(http_server_app_start(handle_frame_upload, handle_demo_enable));

    ESP_ERROR_CHECK(led_matrix_clear());
    
    ESP_ERROR_CHECK(image_store_get("smile", &smile_image));
    ESP_ERROR_CHECK(image_store_get("lightning", &lightning_image));
    ESP_ERROR_CHECK(image_store_get("heart", &heart_image));

    while (1) {
        if (s_demo_enabled) {
            ESP_ERROR_CHECK(display_image(smile_image));
            demo_delay();
        }

        if (s_demo_enabled) {
            ESP_ERROR_CHECK(display_image(lightning_image));
            demo_delay();
        }

        if (s_demo_enabled) {
            ESP_ERROR_CHECK(display_image(heart_image));
            demo_delay();
        }

        if (!s_demo_enabled) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
//192.168.1.28