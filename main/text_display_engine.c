#include "images.h"
#include "led_matrix.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "font_5x7.h"
#include "text_display_engine.h"
#include <string.h>

#define RED ((rgb_t){255, 0, 0})
#define OFF ((rgb_t){0, 0, 0})
#define TEXT_MAX_CHARS 64
#define TEXT_IMAGE_WIDTH_MAX 512
#define TEXT_IMAGE_HEIGHT_MAX 32
#define FONT_WIDTH 5
#define FONT_HEIGHT 7
#define FONT_SPACING 1
#define TEXT_VIEWPORT_PIXELS_MAX (32 * 32)

static const char *TAG = "TEXT_DISPLAY_ENGINE";

static uint16_t s_viewport_width;
static uint16_t s_viewport_height;
static uint16_t s_text_padding_x;
static rgb_t s_text_image[TEXT_IMAGE_HEIGHT_MAX][TEXT_IMAGE_WIDTH_MAX];
static uint16_t s_text_image_width;
static char s_text[TEXT_MAX_CHARS + 1];
static rgb_t s_text_color;
static uint32_t s_frame_duration_ms;
static rgb_t s_viewport_pixels[TEXT_VIEWPORT_PIXELS_MAX];

static TaskHandle_t s_task_handle = NULL;
static esp_timer_handle_t s_timer;
static uint16_t s_scroll_x = 0;
static bool s_stop_requested = false;
static text_frame_renderer_t s_render_frame = NULL;

static esp_err_t build_text_image(rgb_t image[TEXT_IMAGE_HEIGHT_MAX][TEXT_IMAGE_WIDTH_MAX]) {
    size_t length = strlen(s_text);
    if (length > TEXT_MAX_CHARS) {
        length = TEXT_MAX_CHARS;
    }
    s_text_image_width = s_text_padding_x + length * (FONT_5X7_WIDTH + FONT_SPACING) + s_text_padding_x;
    if (s_text_image_width > TEXT_IMAGE_WIDTH_MAX) {
        ESP_LOGW(TAG, "Text image width exceeds maximum size");
        return ESP_ERR_INVALID_SIZE;
    }
    for (int y = 0; y < s_viewport_height; y++) {
        for (int x = 0; x < s_text_image_width; x++) {
            image[y][x] = OFF;
        }
    }
    for (size_t i = 0; i < length; i++) {
        char c = s_text[i];
        const font_5x7_glyph_t *glyph = font_5x7_get_glyph(c);
        for (int y = 0; y < FONT_HEIGHT; y++) {
            for (int x = 0; x < FONT_5X7_WIDTH; x++) {
                int img_x = s_text_padding_x + i * (FONT_5X7_WIDTH + FONT_SPACING) + x;
                int img_y = ((s_viewport_height - FONT_5X7_HEIGHT) / 2) + y;
                if (font_5x7_get_pixel(glyph, x, y)) {
                    image[img_y][img_x] = s_text_color;
                }
            }
        }
    }
    return ESP_OK;
}

static void timer_callback(void *arg)
{
    if (s_task_handle != NULL) {
        xTaskNotifyGive(s_task_handle);
    }

}

static esp_err_t timer_init() {
    if (s_timer != NULL) {
        return ESP_OK;
    }
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "text_timer"
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &s_timer), TAG, "Failed to create timer");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(s_timer, s_frame_duration_ms * 1000), TAG, "Failed to start timer");
    return ESP_OK;
}

static void text_task(void *arg) {
    esp_err_t err = build_text_image(s_text_image);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to build text image: %s", esp_err_to_name(err));
        s_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }
    while (1) {
        uint16_t length = s_text_image_width; // Exclude padding from length for scrolling
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (s_stop_requested) {
            break;
        }

        for (int y = 0; y < s_viewport_height; y++) {
            for (int x = 0; x < s_viewport_width; x++) {
                s_viewport_pixels[y * s_viewport_width + x] = s_text_image[y][x + s_scroll_x];
            }
        }

        s_scroll_x++;
        if (s_scroll_x + s_viewport_width > length) {
            s_scroll_x = 0;
        }

        led_matrix_image_t image_to_display = {
            .width = s_viewport_width,
            .height = s_viewport_height,
            .pixels = s_viewport_pixels
        };

        err = s_render_frame(&image_to_display);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to render text frame: %s", esp_err_to_name(err));
            break;
        }
    }

    s_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t text_display_start(const text_display_config_t *config, text_frame_renderer_t render_frame) {
    if (render_frame == NULL) {
        ESP_LOGE(TAG, "Render frame function cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (s_task_handle != NULL) {
        ESP_LOGW(TAG, "Text display task is already running");
        return ESP_ERR_INVALID_STATE;
    }
    if (config == NULL) {
        ESP_LOGE(TAG, "Text display config is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (config->text == NULL) {
        ESP_LOGE(TAG, "Text is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    s_text_color = config->color;
    if (config->speed_ms != 0) {
        s_frame_duration_ms = config->speed_ms;
    } else {
        s_frame_duration_ms = 100; // Default speed
    }
    strlcpy(s_text, config->text, sizeof(s_text));
    s_scroll_x = 0;
    s_render_frame = render_frame;
    BaseType_t task_created = xTaskCreate(text_task, "text_task", 4096, NULL, 5, &s_task_handle);
    if (task_created != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    ESP_RETURN_ON_ERROR(timer_init(), TAG, "Failed to initialize timer");
    return ESP_OK;
}

esp_err_t text_display_stop() {
    if (s_task_handle == NULL) {
        ESP_LOGW(TAG, "Text display task is not running");
        return ESP_ERR_INVALID_STATE;
    }
    s_stop_requested = true;
    xTaskNotifyGive(s_task_handle);
    while (s_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    s_stop_requested = false;
    esp_timer_stop(s_timer);
    esp_timer_delete(s_timer);
    s_timer = NULL;
    return ESP_OK;
}

bool text_display_is_running() {
    return s_task_handle != NULL;
}

esp_err_t text_display_init(uint16_t display_width, uint16_t display_height) {
    if (display_height > TEXT_IMAGE_HEIGHT_MAX) {
        ESP_LOGE(TAG, "Display height exceeds maximum");
        return ESP_ERR_INVALID_ARG;
    }
    if (display_width > TEXT_IMAGE_WIDTH_MAX) {
        ESP_LOGE(TAG, "Display width exceeds maximum");
        return ESP_ERR_INVALID_ARG;
    }
    if (display_width * display_height > TEXT_VIEWPORT_PIXELS_MAX) {
        ESP_LOGE(TAG, "Viewport size exceeds maximum");
        return ESP_ERR_INVALID_SIZE;
    }
    s_viewport_width = display_width;
    s_viewport_height = display_height;
    s_text_padding_x = display_width;
    return ESP_OK;
}