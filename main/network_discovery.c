#include "network_discovery.h"
#include "mdns.h"
#include "esp_log.h"

static const char *TAG = "network_discovery";

esp_err_t esp_mdns_discovery_start(void) {
    const char *host_name = "led-display";
    const char *instance_name = "ESP32_LED_Display";

    esp_err_t init_err = mdns_init();
    if (init_err != ESP_OK) {
      ESP_LOGE(TAG, "mdns_init fail %s", esp_err_to_name(init_err));
      mdns_free();
      return ESP_FAIL;
    }

    esp_err_t hostname_err = mdns_hostname_set(host_name);
    if (hostname_err != ESP_OK) {
      ESP_LOGE(TAG, "mdns_hostname_set fail %s", esp_err_to_name(hostname_err));
      mdns_free();
      return ESP_FAIL;
    }

    esp_err_t instance_name_err = mdns_instance_name_set(instance_name);
    if (instance_name_err != ESP_OK) {
      ESP_LOGE(TAG, "mdns_instance_name_set fail %s", esp_err_to_name(instance_name_err));
      mdns_free();
      return ESP_FAIL;
    }

    esp_err_t service_add_err = mdns_service_add(instance_name, "_http", "_tcp", 80, NULL, 0);
    if (service_add_err != ESP_OK) {
      ESP_LOGE(TAG, "mdns_service_add fail %s", esp_err_to_name(service_add_err));
      mdns_free();
      return ESP_FAIL;
    }

    ESP_LOGI(TAG, "mDNS discovery started with hostname: %s", host_name);

    return ESP_OK;
}