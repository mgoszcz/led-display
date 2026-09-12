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
#define TEXT_IMAGE_HEIGHT 16
#define TEXT_IMAGE_WIDTH_MAX (16 + (TEXT_MAX_CHARS * (FONT_5X7_WIDTH + 1)) + 16)
#define TEXT_VIEWPORT_WIDTH 16
#define TEXT_VIEWPORT_HEIGHT 16
#define FONT_WIDTH 5
#define FONT_HEIGHT 7
#define FONT_SPACING 1
#define TEXT_PADDING_X 16
#define TEXT_PADDING_Y 6

static const char *TAG = "TEXT_DISPLAY_ENGINE";

static rgb_t s_text_image[TEXT_IMAGE_HEIGHT][TEXT_IMAGE_WIDTH_MAX];
static uint16_t s_text_image_width;
static char s_text[TEXT_MAX_CHARS + 1];
static rgb_t s_text_color;
static uint32_t s_speed_ms;

static TaskHandle_t s_task_handle = NULL;
static esp_timer_handle_t s_timer;
static uint16_t s_scroll_x = 0;
static bool s_stop_requested = false;

static esp_err_t build_text_image(rgb_t image[TEXT_IMAGE_HEIGHT][TEXT_IMAGE_WIDTH_MAX]) {
    size_t length = strlen(s_text);
    if (length > TEXT_MAX_CHARS) {
        length = TEXT_MAX_CHARS;
    }
    s_text_image_width = 16 + length * (FONT_5X7_WIDTH + FONT_SPACING) + 16;
    for (int y = 0; y < TEXT_IMAGE_HEIGHT; y++) {
        for (int x = 0; x < s_text_image_width; x++) {
            image[y][x] = OFF;
        }
    }
    for (size_t i = 0; i < length; i++) {
        char c = s_text[i];
        const font_5x7_glyph_t *glyph = font_5x7_get_glyph(c);
        for (int y = 0; y < FONT_HEIGHT; y++) {
            for (int x = 0; x < FONT_5X7_WIDTH; x++) {
                int img_x = TEXT_PADDING_X + i * (FONT_5X7_WIDTH + FONT_SPACING) + x;
                int img_y = TEXT_PADDING_Y + y;
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
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(s_timer, s_speed_ms * 1000), TAG, "Failed to start timer");
    return ESP_OK;
}

static void text_task(void *arg) {
    framebuffer_t *fb = (framebuffer_t *)arg;
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

        rgb_t pixels[TEXT_VIEWPORT_HEIGHT * TEXT_VIEWPORT_WIDTH];

        for (int y = 0; y < TEXT_VIEWPORT_HEIGHT; y++) {
            for (int x = 0; x < TEXT_VIEWPORT_WIDTH; x++) {
                pixels[y * TEXT_VIEWPORT_WIDTH + x] = s_text_image[y][x + s_scroll_x];
            }
        }

        s_scroll_x++;
        if (s_scroll_x + TEXT_VIEWPORT_WIDTH > length) {
            s_scroll_x = 0;
        }

        led_matrix_image_t image_to_display = {
            .width = TEXT_VIEWPORT_WIDTH,
            .height = TEXT_VIEWPORT_HEIGHT,
            .pixels = pixels
        };

        err = framebuffer_draw_image(fb, &image_to_display, 0, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to draw text viewport: %s", esp_err_to_name(err));
            vTaskDelete(NULL);
            return;
        }

        err = led_matrix_render_framebuffer(fb);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to render text frame: %s", esp_err_to_name(err));
            vTaskDelete(NULL);
            return;
        }

    }

    s_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t text_display_start(framebuffer_t *fb, const text_display_config_t *config) {
    if (s_task_handle != NULL) {
        ESP_LOGW(TAG, "Text display task is already running");
        return ESP_ERR_INVALID_STATE;
    }
    if (fb == NULL) {
        ESP_LOGE(TAG, "Framebuffer is NULL");
        return ESP_ERR_INVALID_ARG;
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
        s_speed_ms = config->speed_ms;
    } else {
        s_speed_ms = 100; // Default speed
    }
    strlcpy(s_text, config->text, sizeof(s_text));
    s_scroll_x = 0;
    BaseType_t task_created = xTaskCreate(text_task, "text_task", 4096, fb, 5, &s_task_handle);
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