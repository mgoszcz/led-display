#pragma once

#include "esp_err.h"
#include "wifi_types.h"

esp_err_t wifi_app_start(void);
esp_err_t wifi_app_connect(const char *ssid, const char *password);
esp_err_t wifi_app_get_status(wifi_app_status_t *out_status);