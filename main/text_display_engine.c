#include "images.h"
#include "led_matrix.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include <stdbool.h>

#define RED ((rgb_t){255, 0, 0})
#define OFF ((rgb_t){0, 0, 0})
#define TEXT_IMAGE_HEIGHT 16
#define TEXT_IMAGE_WIDTH (16 + 5 + 16)
#define TEXT_VIEWPORT_WIDTH 16
#define TEXT_VIEWPORT_HEIGHT 16
#define FONT_WIDTH 5
#define FONT_HEIGHT 7
#define FONT_SPACING 1
#define TEXT_PADDING_X 16
#define TEXT_PADDING_Y 6

static const char *TAG = "TEXT_DISPLAY_ENGINE";

static rgb_t s_text_image[TEXT_IMAGE_HEIGHT][TEXT_IMAGE_WIDTH];

static TaskHandle_t s_task_handle = NULL;
static esp_timer_handle_t s_timer;
static uint16_t s_scroll_x = 0;
static bool s_stop_requested = false;

static const rgb_t letter[7][5] = {
    {OFF, RED, RED, RED, OFF},
    {RED, OFF, OFF, OFF, RED},
    {RED, OFF, OFF, OFF, OFF},
    {RED, OFF, OFF, OFF, OFF},
    {RED, OFF, OFF, OFF, OFF},
    {RED, OFF, OFF, OFF, RED},
    {OFF, RED, RED, RED, OFF},
};

static void build_text_image(rgb_t image[TEXT_IMAGE_HEIGHT][TEXT_IMAGE_WIDTH]) {
    for (int y = 0; y < TEXT_IMAGE_HEIGHT; y++) {
        for (int x = 0; x < TEXT_IMAGE_WIDTH; x++)  {
            if (y < TEXT_PADDING_Y || y >= TEXT_PADDING_Y + FONT_HEIGHT) {
                image[y][x] = OFF;
            } else {
                if (x < TEXT_PADDING_X || x >= TEXT_IMAGE_WIDTH - TEXT_PADDING_X) {
                    image[y][x] = OFF;
                } else {
                    image[y][x] = letter[y - TEXT_PADDING_Y][x - TEXT_PADDING_X];
                }
            }
        }
    }
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
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(s_timer, 100000), TAG, "Failed to start timer");
    return ESP_OK;
}

static void text_task(void *arg) {
    framebuffer_t *fb = (framebuffer_t *)arg;
    build_text_image(s_text_image);
    while (1) {
        uint16_t length = TEXT_IMAGE_WIDTH;
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

        esp_err_t err = framebuffer_draw_image(fb, &image_to_display, 0, 0);
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

esp_err_t text_display_start(framebuffer_t *fb) {
    if (s_task_handle != NULL) {
        ESP_LOGW(TAG, "Text display task is already running");
        return ESP_ERR_INVALID_STATE;
    }
    if (fb == NULL) {
        ESP_LOGE(TAG, "Framebuffer is NULL");
        return ESP_ERR_INVALID_ARG;
    }
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