#pragma once

#include "esp_err.h"
#include <stdbool.h>

typedef enum {
    WIFI_APP_STATE_STOPPED,
    WIFI_APP_STATE_CONNECTING,
    WIFI_APP_STATE_CONNECTED,
    WIFI_APP_STATE_DISCONNECTED,
    WIFI_APP_STATE_SETUP,
} wifi_app_state_t;

typedef struct {
    wifi_app_state_t state;
    char ssid[33];
    bool credentials_saved;
    char ip_address[16];
} wifi_app_status_t;

esp_err_t wifi_app_start(void);
esp_err_t wifi_app_connect(const char *ssid, const char *password);
esp_err_t wifi_app_get_status(wifi_app_status_t *out_status);