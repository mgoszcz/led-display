idf_component_register(SRCS "framebuffer.c" "image_store.c" "images.c" "led_matrix.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES led_strip)
