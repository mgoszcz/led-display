#include "images.h"
#include "led_matrix.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

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

const rgb_t letter[7][5] = {
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

void timer_init() {
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "text_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_timer, 100000));
}

static void text_task(void *arg) {
    framebuffer_t *fb = (framebuffer_t *)arg;
    while (1) {
        uint16_t length = TEXT_IMAGE_WIDTH;
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        build_text_image(s_text_image);

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

        esp_err_t err = framebuffer_draw_viewport(fb, &image_to_display);
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
}

esp_err_t display_text(framebuffer_t *fb) {
    xTaskCreate(text_task, "text_task", 4096, fb, 5, &s_task_handle);
    timer_init();
    
    

    return ESP_OK;
}