#include "http_server_app.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdlib.h>

#define FRAME_SIZE_BYTES (16 * 16 * 3)

static const char *TAG = "HTTP_SERVER_APP";
static httpd_handle_t s_server = NULL;
static http_frame_handler_t s_frame_handler = NULL;
static http_demo_handler_t s_demo_handler = NULL;
static http_brightness_handler_t s_brightness_handler = NULL;

static void set_cors_headers(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
}

static esp_err_t health_check_handler(httpd_req_t *req) {
    set_cors_headers(req);
    const char *response = "OK";
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t frame_draw_handler(httpd_req_t *req) {
    set_cors_headers(req);

    if (req->content_len != FRAME_SIZE_BYTES) { // 16x16 RGB image has 768 bytes
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid content length");
        return ESP_FAIL;
    }
    uint8_t frame_data[FRAME_SIZE_BYTES];

    size_t received_total = 0;

    while (received_total < FRAME_SIZE_BYTES) {
        int received = httpd_req_recv(
            req,
            (char *)frame_data + received_total,
            FRAME_SIZE_BYTES - received_total
        );

        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "Request timeout");
            return ESP_FAIL;
        }

        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive request body");
            return ESP_FAIL;
        }

        received_total += received;
    }

    esp_err_t err = s_frame_handler(frame_data, FRAME_SIZE_BYTES);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to handle frame");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static esp_err_t demo_enable_handler(httpd_req_t *req) {
    set_cors_headers(req);

    esp_err_t err = s_demo_handler();
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to enable demo");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static esp_err_t brightness_handler(httpd_req_t *req) {
    set_cors_headers(req);

    if (req->content_len == 0 || req->content_len > 3) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid brightness length");
        return ESP_FAIL;
    }

    // Tworzysz tablicę 4 znaków.
    // Dlaczego 4? Bo największa wartość jasności to "100", czyli 3 znaki, a string w C musi mieć jeszcze znak końca '\0'.
    char body[4] = {0};

    // Tutaj ESP-IDF czyta body requesta i wpisuje odebrane bajty do body.
    int received = httpd_req_recv(req, body, req->content_len);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive body");
        return ESP_FAIL;
    }


    // To jest ważny moment: ręcznie kończysz string.
    // httpd_req_recv() odbiera surowe bajty, a nie string C. Ono nie dopisuje '\0'.
    // po dopisaniu ręcznie możesz potem użyć funkcji stringowych, np.:
    body[received] = '\0';

    char *end = NULL;
    long value = strtol(body, &end, 10);

    if (*end != '\0' || value < 0 || value > 100) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid brightness value");
        return ESP_FAIL;
    }
    esp_err_t err = s_brightness_handler((uint8_t)value);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to set brightness");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static const httpd_uri_t health_check_uri = {
    .uri = "/health",
    .method = HTTP_GET,
    .handler = health_check_handler,
    .user_ctx = NULL
};

static const httpd_uri_t frame_draw_uri = {
    .uri = "/frame",
    .method = HTTP_POST,
    .handler = frame_draw_handler,
    .user_ctx = NULL
};

static const httpd_uri_t demo_enable_uri = {
    .uri = "/demo",
    .method = HTTP_POST,
    .handler = demo_enable_handler,
    .user_ctx = NULL
};

static const httpd_uri_t brightness_uri = {
    .uri = "/brightness",
    .method = HTTP_POST,
    .handler = brightness_handler,
    .user_ctx = NULL
};

esp_err_t http_server_app_start(http_frame_handler_t frame_handler, http_demo_handler_t demo_handler, http_brightness_handler_t brightness_handler) {
    if (frame_handler == NULL) {
        ESP_LOGE(TAG, "Frame handler cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (demo_handler == NULL) {
        ESP_LOGE(TAG, "Demo handler cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (brightness_handler == NULL) {
        ESP_LOGE(TAG, "Brightness handler cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    s_frame_handler = frame_handler;
    s_demo_handler = demo_handler;
    s_brightness_handler = brightness_handler;
    if (s_server != NULL) {
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    ESP_RETURN_ON_ERROR(httpd_start(&s_server, &config), TAG, "Failed to start HTTP server");
    esp_err_t err = httpd_register_uri_handler(s_server, &health_check_uri);
    esp_err_t err2 = httpd_register_uri_handler(s_server, &frame_draw_uri);
    esp_err_t err3 = httpd_register_uri_handler(s_server, &demo_enable_uri);
    esp_err_t err4 = httpd_register_uri_handler(s_server, &brightness_uri);
    if (err != ESP_OK || err2 != ESP_OK || err3 != ESP_OK || err4 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register URI handler");
        httpd_stop(s_server);
        s_server = NULL;
        return err != ESP_OK ? err : (err2 != ESP_OK ? err2 : (err3 != ESP_OK ? err3 : err4));
    }
    return ESP_OK;
}
