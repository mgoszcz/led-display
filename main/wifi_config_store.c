#include "wifi_config_store.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_check.h"
#include "esp_log.h"
#include <string.h>

static nvs_handle_t nvs_handle_cfg;
static char *TAG = "WIFI_CONFIG_STORE";

esp_err_t wifi_config_store_init(void) {
    ESP_RETURN_ON_ERROR(
        nvs_open("wifi_config", NVS_READWRITE, &nvs_handle_cfg),
        TAG,
        "Failed to open NVS partition"
    );
    return ESP_OK;
}

esp_err_t wifi_config_store_save(const char *ssid, const char *password) {
    if (ssid == NULL || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "Invalid ssid parameter, cannot be NULL or empty");
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(ssid) > 32) {
        ESP_LOGE(TAG, "Invalid ssid parameter, length exceeds 32 characters");
        return ESP_ERR_INVALID_ARG;
    }
    if (password == NULL) {
        ESP_LOGE(TAG, "Invalid password parameter, cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(password) > 63) {
        ESP_LOGE(TAG, "Invalid password parameter, length exceeds 63 characters");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(
        nvs_set_str(nvs_handle_cfg, "ssid", ssid),
        TAG,
        "Failed to set SSID in NVS"
    );
    ESP_RETURN_ON_ERROR(
        nvs_set_str(nvs_handle_cfg, "password", password),
        TAG,
        "Failed to set password in NVS"
    );
    ESP_RETURN_ON_ERROR(
        nvs_commit(nvs_handle_cfg),
        TAG,
        "Failed to commit changes to NVS"
    );
    return ESP_OK;
}

esp_err_t wifi_config_store_load(char *ssid, size_t ssid_size, char *password, size_t password_size) {
    if (ssid == NULL) {
        ESP_LOGE(TAG, "Invalid parameter, ssid cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (password == NULL) {
        ESP_LOGE(TAG, "Invalid parameter, password cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (ssid_size == 0) {
        ESP_LOGE(TAG, "Invalid parameter, ssid_size cannot be 0");
        return ESP_ERR_INVALID_ARG;
    }
    if (password_size == 0) {
        ESP_LOGE(TAG, "Invalid parameter, password_size cannot be 0");
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(
        nvs_get_str(nvs_handle_cfg, "ssid", ssid, &ssid_size),
        TAG,
        "Failed to get SSID from NVS"
    );
    ESP_RETURN_ON_ERROR(
        nvs_get_str(nvs_handle_cfg, "password", password, &password_size),
        TAG,
        "Failed to get password from NVS"
    );
    return ESP_OK;
}

esp_err_t wifi_config_store_clear(void) {
    esp_err_t erase_ssid = nvs_erase_key(nvs_handle_cfg, "ssid");
    if (erase_ssid != ESP_OK && erase_ssid != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to erase SSID %s", esp_err_to_name(erase_ssid));
        return erase_ssid;
    }
    esp_err_t erase_pass = nvs_erase_key(nvs_handle_cfg, "password");
    if (erase_pass != ESP_OK && erase_pass != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to erase Password %s", esp_err_to_name(erase_pass));
        return erase_pass;
    }
    ESP_RETURN_ON_ERROR(
        nvs_commit(nvs_handle_cfg),
        TAG,
        "Failed to commit changes to NVS"
    );
    return ESP_OK;
}