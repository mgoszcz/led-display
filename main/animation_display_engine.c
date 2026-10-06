#include "animation_display_engine.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "ANIMATION_DISPLAY_ENGINE";

static uint16_t s_display_width;
static uint16_t s_display_height;
static TaskHandle_t s_task_handle = NULL;
static esp_timer_handle_t s_timer;
static bool s_stop_requested = false;
static uint32_t s_frame_duration_ms;
static animation_frame_renderer_t s_render_frame;
static const animation_t *s_current_animation = NULL;

static void timer_callback(void *arg)
{
    if (s_task_handle != NULL) {
        xTaskNotifyGive(s_task_handle);
    }

}

static void animation_cleanup_timer(void)
{
    if (s_timer != NULL) {
        esp_timer_stop(s_timer);
        esp_timer_delete(s_timer);
        s_timer = NULL;
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
        .name = "animation_timer"
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_args, &s_timer), TAG, "Failed to create timer");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(s_timer, s_frame_duration_ms * 1000), TAG, "Failed to start timer");
    return ESP_OK;
}

static void animation_task(void *arg) {
    const animation_t *animation = s_current_animation;
    if (animation == NULL || animation->frames == NULL || animation->frame_count == 0) {
        ESP_LOGE(TAG, "Invalid animation data");
        s_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    size_t current_frame_index = 0;

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (s_stop_requested) {
            break;
        }

        const led_matrix_image_t *current_frame = &animation->frames[current_frame_index];

        if (current_frame->height > s_display_height || current_frame->width > s_display_width) {
            ESP_LOGE(TAG, "Frame size exceeds display size");
            break;
        }

        esp_err_t render_err = s_render_frame(current_frame);
        if (render_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to render frame %zu: %s", current_frame_index, esp_err_to_name(render_err));
            break;
        }

        current_frame_index = (current_frame_index + 1) % animation->frame_count;
        if (current_frame_index == 0 && !animation->loop) {
            break;
        }
            
    }

    animation_cleanup_timer();
    s_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t animation_display_start_borrowed(const animation_t *animation, animation_frame_renderer_t render_frame) {
    if (render_frame == NULL) {
        ESP_LOGE(TAG, "Render frame function cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (s_task_handle != NULL) {
        ESP_LOGW(TAG, "Animation display task is already running");
        return ESP_ERR_INVALID_STATE;
    }
    if (animation == NULL) {
        ESP_LOGE(TAG, "animation is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (animation->frames == NULL || animation->frame_count == 0) {
        ESP_LOGE(TAG, "Animation frames are NULL or frame count is zero");
        return ESP_ERR_INVALID_ARG;
    }
    if (animation->frame_duration_ms != 0) {
        s_frame_duration_ms = animation->frame_duration_ms;
    } else {
        s_frame_duration_ms = 100; // Default speed
    }
    s_render_frame = render_frame;
    s_current_animation = animation;
    BaseType_t task_created = xTaskCreate(animation_task, "animation_task", 4096, NULL, 5, &s_task_handle);
    if (task_created != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    ESP_RETURN_ON_ERROR(timer_init(), TAG, "Failed to initialize timer");
    return ESP_OK;
}

esp_err_t animation_display_stop(void) {
    if (s_task_handle == NULL) {
        ESP_LOGW(TAG, "Animation display task is not running");
        return ESP_OK;
    }
    s_stop_requested = true;
    xTaskNotifyGive(s_task_handle);
    while (s_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    s_stop_requested = false;
    animation_cleanup_timer();
    return ESP_OK;
}

esp_err_t animation_display_init(uint16_t display_width, uint16_t display_height) {
    s_display_width = display_width;
    s_display_height = display_height;
    return ESP_OK;
}

bool animation_is_running(void) {
    return s_task_handle != NULL;
}