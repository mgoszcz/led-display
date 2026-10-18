#pragma once

#include "esp_err.h"
#include <stddef.h>

esp_err_t wifi_config_store_init(void);
esp_err_t wifi_config_store_save(const char *ssid, const char *password);
esp_err_t wifi_config_store_load(char *ssid, size_t ssid_size, char *password, size_t password_size);
esp_err_t wifi_config_store_clear(void);