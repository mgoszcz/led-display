#pragma once

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