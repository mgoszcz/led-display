#pragma once

#include "esp_err.h"

typedef enum {
    WIFI_APP_STATE_STOPPED,
    WIFI_APP_STATE_CONNECTING,
    WIFI_APP_STATE_CONNECTED,
    WIFI_APP_STATE_DISCONNECTED,
    WIFI_APP_STATE_SETUP,
} wifi_app_state_t;

esp_err_t wifi_app_start(void);