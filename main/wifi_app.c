#include "sdkconfig.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_check.h"
#include <stdbool.h>

#define WIFI_MAX_RETRY CONFIG_WIFI_MAX_RETRY

static const char *TAG = "WIFI_APP";
static int s_retry_count = 0;
static bool s_started = false;

static esp_err_t wifi_app_init_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "Failed to erase NVS flash");
        err = nvs_flash_init();
    }

    return err;
}

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_count < WIFI_MAX_RETRY) {
            s_retry_count++;
            ESP_LOGW(TAG, "WiFi disconnected, reconnecting... (%d/%d)", s_retry_count, WIFI_MAX_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "WiFi connection failed after %d retries", WIFI_MAX_RETRY);
        }
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        s_retry_count = 0;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t wifi_app_start(void) {
    if (s_started) {
        return ESP_OK;
    }

    wifi_config_t sta_config = {
    .sta = {
        .ssid = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    },
};
    
    ESP_RETURN_ON_ERROR(wifi_app_init_nvs(), TAG, "Failed to initialize NVS flash");
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Failed to initialize ESP-NETIF");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "Failed to create ESP event loop");
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        return ESP_FAIL;
    }

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    
    ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_config), TAG, "Failed to initialize WiFi");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed to set WiFi mode to STA");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &sta_config), TAG, "Failed to configure WiFi STA");

    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(WIFI_EVENT,
                                ESP_EVENT_ANY_ID,
                                &wifi_event_handler,
                                NULL),
        TAG,
        "Failed to register WiFi event handler"
    );

    ESP_RETURN_ON_ERROR(
        esp_event_handler_register(IP_EVENT,
                                IP_EVENT_STA_GOT_IP,
                                &wifi_event_handler,
                                NULL),
        TAG,
        "Failed to register IP event handler"
    );

    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start WiFi");
    ESP_LOGI(TAG, "WiFi initialization complete, connecting to SSID: %s", CONFIG_WIFI_SSID);
    ESP_RETURN_ON_ERROR(esp_wifi_connect(), TAG, "Failed to connect to WiFi");
    
    s_started = true;
    return ESP_OK;
}
