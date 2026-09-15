#include "image_store.h"
#include "wifi_app.h"
#include "http_server_app.h"
#include "display_controller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"


static esp_err_t handle_frame_upload(const uint8_t *data, size_t len)
{
    return display_controller_display_frame_rgb888(data, len);
}

static esp_err_t handle_demo_enable(void)
{
   return display_controller_start_demo();
}

static esp_err_t handle_brightness(int brightness)
{
    return display_controller_set_brightness(brightness);
}

static esp_err_t handle_text(const text_display_config_t *config)
{
    return display_controller_display_text(config);
}



void app_main(void)
{
    const led_matrix_image_t *heart_image = NULL;
    const led_matrix_image_t *smile_image = NULL;
    const led_matrix_image_t *lightning_image = NULL;

    ESP_ERROR_CHECK(display_controller_init(16, 16));
    
    ESP_ERROR_CHECK(wifi_app_start());
    ESP_ERROR_CHECK(http_server_app_start(handle_frame_upload, handle_demo_enable, handle_brightness, handle_text));
    
    ESP_ERROR_CHECK(image_store_get("smile", &smile_image));
    ESP_ERROR_CHECK(image_store_get("lightning", &lightning_image));
    ESP_ERROR_CHECK(image_store_get("heart", &heart_image));

    while (1) {
        ESP_ERROR_CHECK(display_controller_display_demo_image(smile_image));
        ESP_ERROR_CHECK(display_controller_display_demo_image(lightning_image));
        ESP_ERROR_CHECK(display_controller_display_demo_image(heart_image));

        if (display_controller_get_mode() != DISPLAY_MODE_DEMO) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
