#include "http_server_app.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include <stdlib.h>
#include "cJSON.h"

#define FRAME_SIZE_BYTES (16 * 16 * 3)
#define TEXT_JSON_MAX_LEN 256

static const char *TAG = "HTTP_SERVER_APP";
static httpd_handle_t s_server = NULL;
static http_frame_handler_t s_frame_handler = NULL;
static http_demo_handler_t s_demo_handler = NULL;
static http_brightness_handler_t s_brightness_handler = NULL;
static http_text_handler_t s_text_handler = NULL;

static void set_cors_headers(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
}

static esp_err_t options_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
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

    // The largest valid brightness text is "100", plus one byte for the string terminator.
    char body[4] = {0};

    size_t received_total = 0;

    while (received_total < req->content_len) {
        int received = httpd_req_recv(
            req,
            body + received_total,
            req->content_len - received_total
        );

        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "Request timeout");
            return ESP_FAIL;
        }

        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive body");
            return ESP_FAIL;
        }

        received_total += received;
    }

    // httpd_req_recv() reads raw bytes, so terminate the buffer before using string functions.
    body[received_total] = '\0';

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

static esp_err_t text_handler(httpd_req_t *req) {
    set_cors_headers(req);

    if (req->content_len == 0 || req->content_len > TEXT_JSON_MAX_LEN) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON length");
        return ESP_FAIL;
    }

    char body[TEXT_JSON_MAX_LEN + 1] = {0};

    size_t received_total = 0;

    while (received_total < req->content_len) {
        int received = httpd_req_recv(
            req,
            body + received_total,
            req->content_len - received_total
        );

        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "Request timeout");
            return ESP_FAIL;
        }

        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive body");
            return ESP_FAIL;
        }

        received_total += received;
    }

    // Null-terminate the string
    body[received_total] = '\0';

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *text_json = cJSON_GetObjectItemCaseSensitive(root, "text");
    if (!cJSON_IsString(text_json) || text_json->valuestring == NULL) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing text");
        return ESP_FAIL;
    }

    const char *text = text_json->valuestring;

    uint32_t speed_ms = 100;

    cJSON *speed_json = cJSON_GetObjectItemCaseSensitive(root, "speedMs");
    if (speed_json != NULL) {
        if (!cJSON_IsNumber(speed_json) || speed_json->valueint < 10 || speed_json->valueint > 5000) {
            cJSON_Delete(root);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid speedMs");
            return ESP_FAIL;
        }

        speed_ms = (uint32_t)speed_json->valueint;
    }

    rgb_t color = {255, 0, 0};

    cJSON *color_json = cJSON_GetObjectItemCaseSensitive(root, "color");
    if (color_json != NULL) {
        cJSON *r_json = cJSON_GetObjectItemCaseSensitive(color_json, "r");
        cJSON *g_json = cJSON_GetObjectItemCaseSensitive(color_json, "g");
        cJSON *b_json = cJSON_GetObjectItemCaseSensitive(color_json, "b");

        if (!cJSON_IsNumber(r_json) || !cJSON_IsNumber(g_json) || !cJSON_IsNumber(b_json) ||
            r_json->valueint < 0 || r_json->valueint > 255 ||
            g_json->valueint < 0 || g_json->valueint > 255 ||
            b_json->valueint < 0 || b_json->valueint > 255) {
            cJSON_Delete(root);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid color");
            return ESP_FAIL;
        }

        color = (rgb_t){
            .r = (uint8_t)r_json->valueint,
            .g = (uint8_t)g_json->valueint,
            .b = (uint8_t)b_json->valueint,
        };
    }

    text_display_config_t config = {
        .text = text,
        .color = color,
        .speed_ms = speed_ms,
    };

    esp_err_t err = s_text_handler(&config);
    cJSON_Delete(root);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to display text");
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

static const httpd_uri_t text_uri = {
    .uri = "/text",
    .method = HTTP_POST,
    .handler = text_handler,
    .user_ctx = NULL
};

static const httpd_uri_t text_options_uri = {
    .uri = "/text",
    .method = HTTP_OPTIONS,
    .handler = options_handler,
    .user_ctx = NULL
};

esp_err_t http_server_app_start(
    http_frame_handler_t frame_handler,
    http_demo_handler_t demo_handler,
    http_brightness_handler_t brightness_handler,
    http_text_handler_t text_handler
) {
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
    if (text_handler == NULL) {
        ESP_LOGE(TAG, "Text handler cannot be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    s_frame_handler = frame_handler;
    s_demo_handler = demo_handler;
    s_brightness_handler = brightness_handler;
    s_text_handler = text_handler;
    if (s_server != NULL) {
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    ESP_RETURN_ON_ERROR(httpd_start(&s_server, &config), TAG, "Failed to start HTTP server");
    esp_err_t err = httpd_register_uri_handler(s_server, &health_check_uri);
    esp_err_t err2 = httpd_register_uri_handler(s_server, &frame_draw_uri);
    esp_err_t err3 = httpd_register_uri_handler(s_server, &demo_enable_uri);
    esp_err_t err4 = httpd_register_uri_handler(s_server, &brightness_uri);
    esp_err_t err5 = httpd_register_uri_handler(s_server, &text_uri);
    esp_err_t err6 = httpd_register_uri_handler(s_server, &text_options_uri);

    if (err != ESP_OK || err2 != ESP_OK || err3 != ESP_OK || err4 != ESP_OK || err5 != ESP_OK || err6 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register URI handler");
        httpd_stop(s_server);
        s_server = NULL;
        return err != ESP_OK ? err : (err2 != ESP_OK ? err2 : (err3 != ESP_OK ? err3 : (err4 != ESP_OK ? err4 : (err5 != ESP_OK ? err5 : err6))));
    }
    return ESP_OK;
}
