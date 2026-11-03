#include "sdkconfig.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_check.h"
#include "wifi_app.h"
#include <stdbool.h>
#include <string.h>
#include "wifi_config_store.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define WIFI_MAX_RETRY CONFIG_WIFI_MAX_RETRY

static const char *TAG = "WIFI_APP";
static int s_retry_count = 0;
static bool s_started = false;
static wifi_app_state_t s_wifi_state = WIFI_APP_STATE_STOPPED;
static wifi_config_t s_wifi_config = {0};
static char s_ssid[33] = {0};
static char s_password[65] = {0};
static bool s_credentials_pending = false;
static esp_netif_t *s_ap_netif = NULL;
static char s_ip_address[16] = {0};
static bool s_credentials_saved = false;
static SemaphoreHandle_t s_wifi_app_mutex = NULL;

static esp_err_t wifi_app_init_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "Failed to erase NVS flash");
        err = nvs_flash_init();
    }

    return err;
}

static esp_err_t wifi_app_lock(void) {
    if (s_wifi_app_mutex == NULL) {
        ESP_LOGE(TAG, "Wifi app mutex is not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_wifi_app_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take wifi app mutex");
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static esp_err_t wifi_app_unlock(void) {
    if (xSemaphoreGive(s_wifi_app_mutex) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to give wifi app mutex");
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

static esp_err_t wifi_app_start_setup(void) {
    if (s_ap_netif == NULL) {
        s_ap_netif = esp_netif_create_default_wifi_ap();
        if (s_ap_netif == NULL) {
            return ESP_FAIL;
        }
    }
    
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "LED-display-Setup",
            .max_connection = 1,
            .password = "LEDsetupPass",
            .authmode = WIFI_AUTH_WPA2_PSK
        }
    };
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "Failed to set WiFi mode to AP");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &ap_config), TAG, "Failed to configure WiFi AP");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start WiFi");
    esp_err_t ret = wifi_app_lock();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(ret));
        return ret;
    }
    s_wifi_state = WIFI_APP_STATE_SETUP;
    esp_err_t unlock = wifi_app_unlock();
    return unlock;
}

static esp_err_t restart_wifi_to_ap(void) {
     esp_err_t wifi_stop = esp_wifi_stop();
    if (wifi_stop != ESP_OK) {
        ESP_LOGE(TAG, "Stopping wifi failed: %s", esp_err_to_name(wifi_stop));
        return wifi_stop;
    }
    esp_err_t ap_start = wifi_app_start_setup();
    if (ap_start != ESP_OK) {
        ESP_LOGE(TAG, "Starting WiFi in AP mode failed: %s", esp_err_to_name(ap_start));
        return ap_start;
    }
    return ESP_OK;
}

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_err_t ret = wifi_app_lock();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(ret));
            return;
        }
        if (s_wifi_state != WIFI_APP_STATE_CONNECTING &&
            s_wifi_state != WIFI_APP_STATE_CONNECTED) {
                esp_err_t unlock = wifi_app_unlock();
                if (unlock != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
                }
                return;
        }
        s_wifi_state = WIFI_APP_STATE_CONNECTING;
        s_ip_address[0] = '\0';
        esp_err_t unlock = wifi_app_unlock();
        if (unlock != ESP_OK) {
            ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
        }
        if (s_retry_count < WIFI_MAX_RETRY) {
            s_retry_count++;
            ESP_LOGW(TAG, "WiFi disconnected, reconnecting... (%d/%d)", s_retry_count, WIFI_MAX_RETRY);
            esp_err_t err = esp_wifi_connect();
            if (err != ESP_OK) {
                esp_err_t ret = wifi_app_lock();
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(ret));
                    return;
                }
                s_wifi_state = WIFI_APP_STATE_DISCONNECTED;
                s_credentials_pending = false;
                esp_err_t unlock = wifi_app_unlock();
                if (unlock != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
                }
                ESP_LOGE(TAG, "Reconnect failed: %s", esp_err_to_name(err));
                esp_err_t ap_err = restart_wifi_to_ap();
                if (ap_err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to restart WiFi in AP mode: %s", esp_err_to_name(ap_err));
                }
            }
        } else {
            ESP_LOGE(TAG, "WiFi connection failed after %d retries", WIFI_MAX_RETRY);
            esp_err_t ret = wifi_app_lock();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(ret));
            }
            s_wifi_state = WIFI_APP_STATE_DISCONNECTED;
            s_credentials_pending = false;
            esp_err_t unlock = wifi_app_unlock();
            if (unlock != ESP_OK) {
                ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
            }
            esp_err_t ap_err = restart_wifi_to_ap();
            if (ap_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to restart WiFi in AP mode: %s", esp_err_to_name(ap_err));
            }
        }
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        s_retry_count = 0;
        esp_err_t ret = wifi_app_lock();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(ret));
            return;
        }
        s_wifi_state = WIFI_APP_STATE_CONNECTED;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        snprintf(s_ip_address, 16, IPSTR, IP2STR(&event->ip_info.ip));
        if (s_credentials_pending) {
            esp_err_t unlock = wifi_app_unlock();
            if (unlock != ESP_OK) {
                ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
            }
            esp_err_t err = wifi_config_store_save(s_ssid, s_password);
            esp_err_t ret = wifi_app_lock();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(ret));
                return;
            }
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to save WiFi credentials: %s", esp_err_to_name(err));
                s_credentials_pending = false;
                s_credentials_saved = false;
            } else {
                ESP_LOGI(TAG, "WiFi credentials saved successfully.");
                s_credentials_pending = false;
                s_credentials_saved = true;
            }
        }
        esp_err_t unlock = wifi_app_unlock();
        if (unlock != ESP_OK) {
            ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
        }
    }
}

static esp_err_t wifi_app_init(void)
{
    s_wifi_app_mutex = xSemaphoreCreateMutex();
    if (s_wifi_app_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create wifi app mutex");
        return ESP_ERR_NO_MEM;
    }
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(wifi_app_init_nvs(), TAG, "Failed to initialize NVS flash");
    ESP_RETURN_ON_ERROR(wifi_config_store_init(), TAG, "Failed to initialize WiFi config store");
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "Failed to initialize ESP-NETIF");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "Failed to create ESP event loop");
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        return ESP_FAIL;
    }
    ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_config), TAG, "Failed to initialize WiFi");
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

    return ESP_OK;
}

static esp_err_t wifi_read_config(void)
{
    esp_err_t err = wifi_config_store_load(s_ssid, sizeof(s_ssid), s_password, sizeof(s_password));
    esp_err_t ret = wifi_app_lock();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(ret));
        return ret;
    }
    if (err == ESP_OK) {
        if (s_ssid[0] == '\0') {
            s_credentials_saved = false;
            esp_err_t unlock = wifi_app_unlock();
            if (unlock != ESP_OK) {
                ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
            }
            return ESP_ERR_NVS_NOT_FOUND;
        } else {
            s_credentials_saved = true;
        }
    } else {
        s_credentials_saved = false;
    }
    esp_err_t unlock = wifi_app_unlock();
    if (unlock != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
    }
    return err;
}

static esp_err_t sta_connect(void)
{
    s_retry_count = 0;
    
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed to set WiFi mode to STA");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &s_wifi_config), TAG, "Failed to configure WiFi STA");
    
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start WiFi");
    esp_err_t ret = wifi_app_lock();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(ret));
        return ret;
    }
    s_wifi_state = WIFI_APP_STATE_CONNECTING;
    esp_err_t unlock = wifi_app_unlock();
    if (unlock != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
        return unlock;
    }
    ESP_LOGI(TAG, "WiFi initialization complete, connecting to SSID: %s", s_ssid);
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        esp_err_t ret = wifi_app_lock();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(ret));
            return ret;
        }
        s_wifi_state = WIFI_APP_STATE_DISCONNECTED;
        esp_err_t unlock = wifi_app_unlock();
        if (unlock != ESP_OK) {
            ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
            return unlock;
        }
        ESP_LOGE(TAG, "Reconnect failed: %s", esp_err_to_name(err));
        esp_err_t ap_err = restart_wifi_to_ap();
        if (ap_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to restart WiFi in AP mode: %s", esp_err_to_name(ap_err));
        }
        return ap_err;
    }

    return err;
}


esp_err_t wifi_app_start(void) {
    if (s_started) {
        return ESP_OK;
    }
    
    ESP_RETURN_ON_ERROR(wifi_app_init(), TAG, "Failed to initialize WiFi app");

    esp_err_t config_read = wifi_read_config();
    if (config_read == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "WiFi configuration is not set. Setting up WiFi in AP mode for configuration.");
        ESP_RETURN_ON_ERROR(wifi_app_start_setup(), TAG, "Failed to initialize AP");
        s_ip_address[0] = '\0';
    } else if (config_read != ESP_OK) {
        return config_read;
    } else {
        ESP_LOGI(TAG, "WiFi configuration read successfully. Attempting to connect to SSID: %s", s_ssid);
        memset(&s_wifi_config, 0, sizeof(s_wifi_config));

        memcpy(s_wifi_config.sta.ssid, s_ssid, strlen(s_ssid));
        memcpy(s_wifi_config.sta.password, s_password, strlen(s_password));
        ESP_RETURN_ON_ERROR(sta_connect(), TAG, "Failed to connect to WiFi");
    }
    
    s_started = true;
    return ESP_OK;
}

esp_err_t wifi_app_connect(const char *ssid, const char *password) {
    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(ssid) > sizeof(s_wifi_config.sta.ssid) || strlen(password) > sizeof(s_wifi_config.sta.password) - 1) {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(ssid) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t lock = wifi_app_lock();
    if (lock != ESP_OK) {
        ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(lock));
        return lock;
    }
    if (s_wifi_state != WIFI_APP_STATE_SETUP) {
        ESP_LOGE(TAG, "WiFi is not in setup mode.");
        esp_err_t unlock = wifi_app_unlock();
        if (unlock != ESP_OK) {
            ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
        }
        return ESP_ERR_INVALID_STATE;
    }
    s_credentials_saved = false;
    memcpy(s_ssid, ssid, strlen(ssid) + 1);
    memcpy(s_password, password, strlen(password) + 1);
    memset(&s_wifi_config, 0, sizeof(s_wifi_config));

    memcpy(s_wifi_config.sta.ssid, s_ssid, strlen(s_ssid));
    memcpy(s_wifi_config.sta.password, s_password, strlen(s_password));
    s_retry_count = 0;
    s_wifi_state = WIFI_APP_STATE_CONNECTING;
    s_credentials_pending = true;
    esp_err_t unlock = wifi_app_unlock();
    if (unlock != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
        return unlock;
    }

    esp_err_t set_mode_err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (set_mode_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode to APSTA: %s", esp_err_to_name(set_mode_err));
        esp_err_t lock = wifi_app_lock();
        if (lock != ESP_OK) {
            ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(lock));
            return lock;
        }
        s_wifi_state = WIFI_APP_STATE_DISCONNECTED;
        s_credentials_pending = false;
        return set_mode_err;
    }
    esp_err_t set_config_err = esp_wifi_set_config(WIFI_IF_STA, &s_wifi_config);
    if (set_config_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure WiFi STA: %s", esp_err_to_name(set_config_err));
        esp_err_t lock = wifi_app_lock();
        if (lock != ESP_OK) {
            ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(lock));
            return lock;
        }
        s_wifi_state = WIFI_APP_STATE_DISCONNECTED;
        s_credentials_pending = false;
        return set_config_err;
    }
    
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect to WiFi: %s", esp_err_to_name(err));
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "Failed to set WiFi mode to AP");
        esp_err_t lock = wifi_app_lock();
        if (lock != ESP_OK) {
            ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(lock));
            return lock;
        }
        s_credentials_pending = false;
        s_wifi_state = WIFI_APP_STATE_SETUP;
        esp_err_t unlock = wifi_app_unlock();
        if (unlock != ESP_OK) {
            ESP_LOGE(TAG, "Failed to unlock wifi app controller: %s", esp_err_to_name(unlock));
        }
        return err;
    }
    return ESP_OK;
}

esp_err_t wifi_app_get_status(wifi_app_status_t *out_status) {
    if (out_status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = wifi_app_lock();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to lock wifi app controller: %s", esp_err_to_name(ret));
        return ret;
    }
    out_status->credentials_saved = s_credentials_saved;
    out_status->state = s_wifi_state;
    memcpy(out_status->ssid, s_ssid, sizeof(out_status->ssid));
    memcpy(out_status->ip_address, s_ip_address,
        sizeof(out_status->ip_address));
    esp_err_t unlock = wifi_app_unlock();
    return unlock;
}