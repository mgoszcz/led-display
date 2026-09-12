idf_component_register(SRCS "font_5x7.c" "text_display_engine.c" "wifi_app.c" "framebuffer.c" "image_store.c" "images.c" "led_matrix.c" "main.c" "http_server_app.c"
                    INCLUDE_DIRS "."
                    REQUIRES led_strip esp_wifi nvs_flash esp_netif esp_event esp_http_server esp_timer json)
