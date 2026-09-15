#include "display_controller.h"
#include "framebuffer.h"
#include "led_matrix.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "text_display_engine.h"

#define DISPLAY_MAX_WIDTH 32
#define DISPLAY_MAX_HEIGHT 32
#define DISPLAY_MAX_PIXELS (DISPLAY_MAX_WIDTH * DISPLAY_MAX_HEIGHT)


static const char *TAG = "DISPLAY_CONTROLLER";
static framebuffer_t fb;
static display_mode_t s_display_mode = DISPLAY_MODE_DEMO;
static SemaphoreHandle_t s_display_mutex = NULL;
static rgb_t s_framebuffer_pixels[DISPLAY_MAX_PIXELS];
static uint16_t s_display_width;
static uint16_t s_display_height;

static void demo_delay(void)
{
    for (int i = 0; i < 50; i++) {
        if (s_display_mode != DISPLAY_MODE_DEMO) {
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

esp_err_t display_controller_init(uint16_t width, uint16_t height) {
    if (width == 0 || height == 0 || width > DISPLAY_MAX_WIDTH || height > DISPLAY_MAX_HEIGHT) {
        ESP_LOGE(TAG, "Invalid display dimensions: %dx%d", width, height);
        return ESP_ERR_INVALID_ARG;
    }
    s_display_width = width;
    s_display_height = height;
    esp_err_t framebuffer_err = framebuffer_init(&fb, width, height, s_framebuffer_pixels, width * height);
    if (framebuffer_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize framebuffer: %s", esp_err_to_name(framebuffer_err));
        return framebuffer_err;
    }
    s_display_mutex = xSemaphoreCreateMutex();
    if (s_display_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create display mutex");
        return ESP_ERR_NO_MEM;
    }
    led_matrix_config_t config = {
        .width = width,
        .height = height,
        .gpio = 4,
        .origin = LED_MATRIX_ORIGIN_BOTTOM_LEFT,
        .layout = LED_MATRIX_LAYOUT_SERPENTINE,
    };
    esp_err_t led_matrix_err = led_matrix_init(&config);
    if (led_matrix_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LED matrix: %s", esp_err_to_name(led_matrix_err));
        return led_matrix_err;
    }
    esp_err_t led_matrix_clear_err = led_matrix_clear();
    if (led_matrix_clear_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear LED matrix: %s", esp_err_to_name(led_matrix_clear_err));
        return led_matrix_clear_err;
    }
    return ESP_OK;
}

static esp_err_t display_controller_lock(void) {
    if (s_display_mutex == NULL) {
        ESP_LOGE(TAG, "Display mutex is not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_display_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take display mutex");
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static esp_err_t display_controller_unlock(void) {
    if (xSemaphoreGive(s_display_mutex) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to give display mutex");
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

static esp_err_t display_controller_set_mode(display_mode_t mode) {
    if (s_display_mode == mode) {
        return ESP_OK;
    }

    display_mode_t previous_mode = s_display_mode;

    if (previous_mode == DISPLAY_MODE_TEXT) {
        ESP_LOGI(TAG, "Stopping text display task");
        esp_err_t text_display_err = text_display_stop();
        if (text_display_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to stop text display task: %s", esp_err_to_name(text_display_err));
            return text_display_err;
        }
    }

    s_display_mode = mode;
    return ESP_OK;
}

static esp_err_t display_controller_render_image(const led_matrix_image_t *image) {
    esp_err_t ret = display_controller_lock();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to lock display controller: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = framebuffer_clear(&fb);
    if (ret == ESP_OK) {
        ret = framebuffer_draw_image(&fb, image, 0, 0);
    }
    if (ret == ESP_OK) {
        ret = led_matrix_render_framebuffer(&fb);
    }
    
    esp_err_t unlock_err = display_controller_unlock();
    return ret != ESP_OK ? ret : unlock_err;
}

esp_err_t display_controller_display_demo_image(const led_matrix_image_t *image) {
     if (s_display_mode != DISPLAY_MODE_DEMO) {
        return ESP_OK;
    }

    esp_err_t ret = display_controller_render_image(image);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to display demo image: %s", esp_err_to_name(ret));
        return ret;
    }
    demo_delay();
    return ESP_OK;
}

esp_err_t display_controller_display_frame_rgb888(const uint8_t *data, size_t len)
{
    esp_err_t display_mode_err = display_controller_set_mode(DISPLAY_MODE_FRAME);
    if (display_mode_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set display mode: %s", esp_err_to_name(display_mode_err));
        return display_mode_err;
    }
    esp_err_t ret = display_controller_lock();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to lock display controller: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = framebuffer_draw_rgb888(&fb, s_display_width, s_display_height, data, len);
    if (ret == ESP_OK) {
        ret = led_matrix_render_framebuffer(&fb);
    }
    esp_err_t unlock_err = display_controller_unlock();
    return ret != ESP_OK ? ret : unlock_err;
}

esp_err_t display_controller_start_demo(void) {
    esp_err_t display_mode_err = display_controller_set_mode(DISPLAY_MODE_DEMO);
    if (display_mode_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set display mode: %s", esp_err_to_name(display_mode_err));
        return display_mode_err;
    }
    return ESP_OK;
}

esp_err_t display_controller_set_brightness(int brightness) {
    if (brightness < 0 || brightness > 100) {
        ESP_LOGE(TAG, "Brightness value out of range: %d", brightness);
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = display_controller_lock();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to lock display controller: %s", esp_err_to_name(ret));
        return ret;
    }
    led_matrix_set_brightness((uint8_t)brightness);

    ret = led_matrix_render_framebuffer(&fb);
    esp_err_t unlock_err = display_controller_unlock();
    if (ret == ESP_OK && unlock_err == ESP_OK) {
        ESP_LOGI(TAG, "Brightness set to: %d", brightness);
    }
    return ret != ESP_OK ? ret : unlock_err;
}

esp_err_t display_controller_display_text(const text_display_config_t *config) {
    if (text_display_is_running()) {
        ESP_LOGW(TAG, "Text display task is already running. Stopping it first.");
        esp_err_t err = text_display_stop();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to stop existing text display task: %s", esp_err_to_name(err));
            return err;
        }
    }
    esp_err_t display_mode_err = display_controller_set_mode(DISPLAY_MODE_TEXT);
    if (display_mode_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set display mode: %s", esp_err_to_name(display_mode_err));
        return display_mode_err;
    }
    esp_err_t ret = text_display_start(config, display_controller_render_image);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start text display: %s", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}

display_mode_t display_controller_get_mode(void) {
    return s_display_mode;
}