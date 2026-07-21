
#include "http_server_app.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "HTTP_SERVER_APP";
static httpd_handle_t s_server = NULL;

static esp_err_t health_check_handler(httpd_req_t *req) {
    const char *response = "OK";
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t health_check_uri = {
    .uri = "/health",
    .method = HTTP_GET,
    .handler = health_check_handler,
    .user_ctx = NULL
};

esp_err_t http_server_app_start(void) {
    if (s_server != NULL) {
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    ESP_RETURN_ON_ERROR(httpd_start(&s_server, &config), TAG, "Failed to start HTTP server");
    esp_err_t err = httpd_register_uri_handler(s_server, &health_check_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register URI handler");
        httpd_stop(s_server);
        s_server = NULL;
        return err;
    }
    return ESP_OK;
}