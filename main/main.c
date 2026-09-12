#include "led_matrix.h"
#include "image_store.h"
#include "framebuffer.h"
#include "wifi_app.h"
#include "http_server_app.h"
#include "text_display_engine.h"
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"

typedef enum {
    DISPLAY_MODE_IDLE,
    DISPLAY_MODE_DEMO,
    DISPLAY_MODE_FRAME,
    DISPLAY_MODE_TEXT,
} display_mode_t;


static const char *TAG = "MAIN";
static rgb_t framebuffer_pixels[16 * 16];
static framebuffer_t fb;
static display_mode_t s_display_mode = DISPLAY_MODE_DEMO;

static esp_err_t set_display_mode(display_mode_t mode) {
    if (s_display_mode == mode) {
        return ESP_OK;
    }

    display_mode_t previous_mode = s_display_mode;

    if (previous_mode == DISPLAY_MODE_TEXT) {
        ESP_LOGI(TAG, "Stopping text display task");
        ESP_RETURN_ON_ERROR(text_display_stop(), TAG, "Failed to stop text display task");
    }

    s_display_mode = mode;
    return ESP_OK;
}

static esp_err_t display_image(const led_matrix_image_t *image) {
    ESP_RETURN_ON_ERROR(framebuffer_clear(&fb), TAG, "Failed to clear framebuffer");
    ESP_RETURN_ON_ERROR(framebuffer_draw_image(&fb, image, 0, 0), TAG, "Failed to draw image");
    ESP_RETURN_ON_ERROR(led_matrix_render_framebuffer(&fb), TAG, "Failed to render framebuffer");
    return ESP_OK;
}

static esp_err_t handle_frame_upload(const uint8_t *data, size_t len)
{
    ESP_RETURN_ON_ERROR(set_display_mode(DISPLAY_MODE_FRAME), TAG, "Failed to set display mode");
    ESP_RETURN_ON_ERROR(framebuffer_draw_rgb888(&fb, 16, 16, data, len), TAG, "Failed to draw RGB888 frame");
    ESP_RETURN_ON_ERROR(led_matrix_render_framebuffer(&fb), TAG, "Failed to render framebuffer");

    return ESP_OK;
}

static esp_err_t handle_demo_enable(void)
{
    ESP_RETURN_ON_ERROR(set_display_mode(DISPLAY_MODE_DEMO), TAG, "Failed to set display mode");
    return ESP_OK;
}

static esp_err_t handle_brightness(int brightness)
{
    if (brightness < 0 || brightness > 100) {
        ESP_LOGE(TAG, "Brightness value out of range: %d", brightness);
        return ESP_ERR_INVALID_ARG;
    }
    led_matrix_set_brightness((uint8_t)brightness);

    ESP_RETURN_ON_ERROR(
        led_matrix_render_framebuffer(&fb),
        TAG,
        "Failed to render framebuffer after brightness change"
    );
    ESP_LOGI(TAG, "Brightness set to: %d", brightness);
    return ESP_OK;
}

static esp_err_t handle_text(const text_display_config_t *config)
{
    if (text_display_is_running()) {
        ESP_LOGW(TAG, "Text display task is already running. Stopping it first.");
        ESP_RETURN_ON_ERROR(text_display_stop(), TAG, "Failed to stop existing text display task");
    }
    ESP_RETURN_ON_ERROR(set_display_mode(DISPLAY_MODE_TEXT), TAG, "Failed to set display mode");
    ESP_RETURN_ON_ERROR(text_display_start(&fb, config), TAG, "Failed to start text display");
    return ESP_OK;
}

static void demo_delay(void)
{
    for (int i = 0; i < 50; i++) {
        if (s_display_mode != DISPLAY_MODE_DEMO) {
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
    ESP_ERROR_CHECK(http_server_app_start(handle_frame_upload, handle_demo_enable, handle_brightness, handle_text));

    ESP_ERROR_CHECK(led_matrix_clear());
    
    ESP_ERROR_CHECK(image_store_get("smile", &smile_image));
    ESP_ERROR_CHECK(image_store_get("lightning", &lightning_image));
    ESP_ERROR_CHECK(image_store_get("heart", &heart_image));

    while (1) {
        if (s_display_mode == DISPLAY_MODE_DEMO) {
            ESP_ERROR_CHECK(display_image(smile_image));
            demo_delay();
        }

        if (s_display_mode == DISPLAY_MODE_DEMO) {
            ESP_ERROR_CHECK(display_image(lightning_image));
            demo_delay();
        }

        if (s_display_mode == DISPLAY_MODE_DEMO) {
            ESP_ERROR_CHECK(display_image(heart_image));
            demo_delay();
        }

        if (s_display_mode != DISPLAY_MODE_DEMO) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
