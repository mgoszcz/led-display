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
#include <stddef.h>

#define RED ((rgb_t){255, 0, 0})
#define OFF ((rgb_t){0, 0, 0})
#define TEXT_MAX_CHARS 64
#define TEXT_IMAGE_WIDTH_MAX 512
#define TEXT_IMAGE_HEIGHT_MAX 32
#define FONT_WIDTH 5
#define FONT_HEIGHT 7
#define FONT_SPACING 1
#define TEXT_VIEWPORT_PIXELS_MAX (32 * 32)
#define TEXT_MAX_SEGMENTS 16

static const char *TAG = "TEXT_DISPLAY_ENGINE";

static uint16_t s_viewport_width;
static uint16_t s_viewport_height;
static uint16_t s_text_padding_x;
static uint16_t s_text_image_width;
static char s_text[TEXT_MAX_CHARS + 1];
static rgb_t s_text_color;
static uint32_t s_frame_duration_ms;
static rgb_t s_viewport_pixels[TEXT_VIEWPORT_PIXELS_MAX];
static text_segment_t s_segments[TEXT_MAX_SEGMENTS];
static size_t s_segment_count = 0;
static bool s_continuous;

static TaskHandle_t s_task_handle = NULL;
static esp_timer_handle_t s_timer;
static uint16_t s_scroll_x = 0;
static bool s_stop_requested = false;
static text_frame_renderer_t s_render_frame = NULL;

static esp_err_t get_segment(size_t text_index, text_segment_t *out_segment) {
    if (out_segment == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < s_segment_count; i++) {
        if (text_index >= s_segments[i].offset && text_index < (s_segments[i].offset + s_segments[i].length)) {
            *out_segment = s_segments[i];
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t build_viewport_pixels() {
    memset(s_viewport_pixels, 0, sizeof(s_viewport_pixels));
    size_t length = strlen(s_text);
    size_t text_width = length * (FONT_5X7_WIDTH + FONT_SPACING);
    for (int viewport_x = 0; viewport_x < s_viewport_width; viewport_x++) {
        int world_x = s_scroll_x + viewport_x;
        int text_x = world_x - s_text_padding_x;
        if (text_x < 0) {
            continue;
        }
        if (text_x >= text_width) {
            if (!s_continuous) {
                continue;
            }
            text_x = text_x % text_width;
        }
        int char_index = text_x / (FONT_5X7_WIDTH + FONT_SPACING);
        int char_position = text_x % (FONT_5X7_WIDTH + FONT_SPACING);
        if (char_position >= FONT_5X7_WIDTH) {
            continue;
        }
        rgb_t color;
        if (s_segment_count > 0) {
            text_segment_t segment = {0};
            esp_err_t err = get_segment(char_index, &segment);
            if (err == ESP_OK) {
                color = segment.color;
            } else {
                return err;
            }
        } else {
            color = s_text_color;
        }
        char character = s_text[char_index];
        const font_5x7_glyph_t *glyph = font_5x7_get_glyph(character);
        for (int y = 0; y < FONT_5X7_HEIGHT; y++) {
            int img_y = ((s_viewport_height - FONT_5X7_HEIGHT) / 2) + y;
            if (font_5x7_get_pixel(glyph, char_position, y)) {
                s_viewport_pixels[img_y * s_viewport_width + viewport_x] = color;
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
    while (1) {
        uint16_t length = s_text_image_width; // Exclude padding from length for scrolling
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (s_stop_requested) {
            break;
        }

        esp_err_t err = build_viewport_pixels();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to build viewport: %s", esp_err_to_name(err));
            s_task_handle = NULL;
            vTaskDelete(NULL);
            return;
        }

        s_scroll_x++;
        if (!s_continuous && (s_scroll_x + s_viewport_width > length)) {
            s_scroll_x = 0;
        } else if (s_continuous && s_scroll_x >= s_text_image_width) {
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
    if (config->segment_count > TEXT_MAX_SEGMENTS ||
        (config->segment_count > 0 && config->segments == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    s_continuous = config->continuous;
    if (s_continuous) {
        s_text_padding_x = 0;
    } else {
        s_text_padding_x = s_viewport_width;
    }
    s_segment_count = config->segment_count;
    memcpy(s_segments, config->segments,
       config->segment_count * sizeof(s_segments[0]));
    s_text_color = config->color;
    if (config->speed_ms != 0) {
        s_frame_duration_ms = config->speed_ms;
    } else {
        s_frame_duration_ms = 100; // Default speed
    }
    strlcpy(s_text, config->text, sizeof(s_text));
    s_scroll_x = 0;
    s_render_frame = render_frame;
    s_text_image_width = s_text_padding_x + strlen(s_text) * (FONT_5X7_WIDTH + FONT_SPACING) + s_text_padding_x;
    if (s_text_image_width > TEXT_IMAGE_WIDTH_MAX) {
        ESP_LOGW(TAG, "Text image width exceeds maximum size");
        return ESP_ERR_INVALID_SIZE;
    }
    BaseType_t task_created = xTaskCreate(text_task, "text_task", 4096, NULL, 5, &s_task_handle);
    if (task_created != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t timer_err = timer_init();
    if (timer_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize timer: %s", esp_err_to_name(timer_err));
        s_stop_requested = true;
        xTaskNotifyGive(s_task_handle);
        while (s_task_handle != NULL) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        s_stop_requested = false;
        return timer_err;
    }
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