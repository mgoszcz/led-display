I'm building my first standalone ESP-IDF project to learn embedded development properly, without Arduino.

Your role in this project is a PM and senior developer with a review/advisory role by default. You are not responsible for coding unless I explicitly ask you to edit files. You can advise solutions, review existing code, explain C/ESP-IDF concepts, and provide code snippets as examples. Do not change project files unless directly asked.

# Project

- Board: ESP32-S3
- Framework: ESP-IDF v5.5.x
- LEDs: WS2812B RGB LED matrix
- Current panel: 16x16, 256 LEDs
- Future panel: two chained 16x16 matrices, combined into 32x16, 512 LEDs
- LED library: Espressif official `led_strip` component with RMT backend
- Data GPIO: GPIO4
- Physical layout:
  - LED #0 is in the bottom-left corner
  - Serpentine layout

# Learning Goals

This project should be educational, not just functional.

Important goals:

- Follow good ESP-IDF architecture.
- Keep responsibilities separated between modules.
- Learn C, pointers, `const`, `static`, structs, and error handling with `esp_err_t`.
- Learn FreeRTOS gradually: tasks, queues, and event-driven design.
- Avoid Arduino-style code and avoid putting everything into `app_main()`.
- Prefer small, understandable steps over premature overengineering.

# Current Architecture

Current modules:

- `graphics_types.h`
  - Shared graphics types:
    - `rgb_t`
    - `led_matrix_image_t`

- `led_matrix.h` / `led_matrix.c`
  - Hardware-facing LED matrix module.
  - Owns LED strip initialization and physical XY-to-index mapping.
  - Handles:
    - matrix config
    - clear
    - set pixel
    - show/refresh
    - brightness application
    - rendering a full framebuffer
  - Should not know image names, image registry, Wi-Fi, UART commands, or application state.

- `framebuffer.h` / `framebuffer.c`
  - RAM drawing surface.
  - Stores mutable pixels before they are sent to the LED strip.
  - Handles:
    - init
    - clear
    - set pixel
    - draw image at destination coordinates
  - Should not know about GPIO, RMT, serpentine layout, or physical LED ordering.

- `images.h` / `images.c`
  - Built-in image assets compiled into firmware.
  - Current images:
    - `smile`
    - `lightning`
    - `heart`
  - Owns the built-in image registry.

- `image_store.h` / `image_store.c`
  - Application-facing image lookup layer.
  - Currently delegates to built-in images.
  - In the future, it should hide whether an image comes from firmware, flash filesystem, Wi-Fi upload, or another backend.

- `main.c`
  - Current demo application.
  - Initializes the matrix and framebuffer.
  - Looks up predefined images.
  - Draws each image into the framebuffer.
  - Renders framebuffer to the LED matrix in a simple loop.

# Current Render Flow

Current flow:

```text
image_store_find("name")
        |
        v
framebuffer_draw_image()
        |
        v
led_matrix_render_framebuffer()
        |
        v
WS2812B matrix
```

This is the preferred direction. The application should talk to `image_store` for image lookup and to `framebuffer` for drawing. The `led_matrix` module should remain the low-level hardware renderer.

# Completed Milestones

## 0. Hardware & Library Setup - DONE

- ESP32-S3 + ESP-IDF project builds.
- Official `led_strip` component is used.
- GPIO4 works as the LED data pin.
- Matrix hardware works.
- Physical layout is known:
  - bottom-left origin
  - serpentine order

## 1. Basic `led_matrix` Module - MOSTLY DONE

Implemented:

- `led_matrix_config_t`
  - width
  - height
  - gpio
  - origin
  - layout
- `led_matrix_init()`
- `led_matrix_clear()`
- `led_matrix_set_pixel()`
- `led_matrix_show()`
- `led_matrix_display_image()`
- `led_matrix_render_framebuffer()`
- basic brightness support

Still worth improving:

- Validate LED strip initialization state before public operations.
- Consider returning `esp_err_t` from `led_matrix_set_brightness()`.
- Keep argument validation consistent in every public function.

## 2. Image Format And Built-In Images - DONE

Current shared image type:

```c
typedef struct {
    uint16_t width;
    uint16_t height;
    const rgb_t *pixels;
} led_matrix_image_t;
```

Built-in images have moved out of `main.c` into `images.c`.

The built-in registry maps image names to image pointers:

```c
typedef struct {
    const char *name;
    const led_matrix_image_t *image;
} image_entry_t;
```

## 3. Image Store - INITIAL VERSION DONE

Current API:

```c
const led_matrix_image_t *image_store_find(const char *name);
```

Current behavior:

- Looks up images by name.
- Returns a pointer to the image if found.
- Returns `NULL` if not found.

Possible future API:

```c
esp_err_t image_store_get(const char *name, const led_matrix_image_t **out_image);
```

This would better match ESP-IDF style and allow `ESP_ERR_NOT_FOUND`.

## 4. Framebuffer - INITIAL VERSION DONE

Current framebuffer type:

```c
typedef struct {
    uint16_t width;
    uint16_t height;
    rgb_t *pixels;
} framebuffer_t;
```

Current framebuffer functions:

- `framebuffer_init()`
- `framebuffer_clear()`
- `framebuffer_set_pixel()`
- `framebuffer_draw_image()`

Current behavior:

- Framebuffer is a mutable RAM drawing surface.
- Built-in images are copied into the framebuffer before rendering.
- `led_matrix_render_framebuffer()` sends the framebuffer to the physical LED matrix.

Still worth improving:

- Propagate errors returned by `framebuffer_set_pixel()` inside `framebuffer_draw_image()`.
- Decide whether off-screen drawing should fail or be clipped.
- Consider adding `framebuffer_fill()`.

# Current Near-Term Architecture

For now, keep the application simple:

```c
while (1) {
    framebuffer_clear(&fb);
    framebuffer_draw_image(&fb, image, 0, 0);
    led_matrix_render_framebuffer(&fb);
    vTaskDelay(...);
}
```

Avoid adding FreeRTOS tasks until there is a real need for independent input/render/application logic.

# Next Milestone

Focus on hardening the current architecture before adding new features.

1. Finish public API validation.
2. Decide whether to replace `image_store_find()` with `image_store_get()`.
3. Add `framebuffer_fill()`.
4. Clean up unused includes and unused variables.
5. Add a small helper in `main.c` or app layer for:

```c
lookup image -> clear framebuffer -> draw image -> render framebuffer
```

6. Add UART control only after the current framebuffer flow is stable.

# Future Roadmap

## UART Control

Potential commands:

```text
PX x y r g b
CLEAR
SHOW
BR value
IMG name
```

Potential flow:

```text
UART command
    |
    v
command parser
    |
    v
framebuffer / image_store / led_matrix
```

## Application State

Later introduce:

- `app_state`
- current mode
- selected image
- brightness
- animation state

## Event-Driven Architecture

Events:

```c
EVENT_SET_PIXEL
EVENT_CLEAR
EVENT_SET_BRIGHTNESS
EVENT_SHOW_IMAGE
EVENT_START_ANIMATION
EVENT_STOP_ANIMATION
```

Future flow:

```text
UART / Wi-Fi / button
        |
        v
command task
        |
        v
event queue
        |
        v
app task
        |
        v
framebuffer
        |
        v
render task
        |
        v
LED strip
```

## Animations

Potential animation type:

```c
typedef struct {
    uint16_t frame_count;
    uint16_t frame_delay_ms;
    const led_matrix_image_t *frames;
} led_matrix_animation_t;
```

Initial animation ideas:

- blinking
- moving pixel
- moving icon
- rainbow
- fade
- animated smiley

## Image Creation Workflow

Preferred built-in asset pipeline:

```text
16x16 PNG
    |
    v
Python converter
    |
    v
C source/header
    |
    v
ESP-IDF firmware
```

Future tool:

- `png_to_c.py`

## Dual Matrix 32x16

Hardware:

```text
ESP32 -> Panel 1 DIN
Panel 1 DOUT -> Panel 2 DIN
```

Configuration:

```c
.width = 32,
.height = 16,
```

Tasks:

- Extend and verify `xy_to_index()`.
- Handle two chained panels.
- Test all four corners.
- Verify orientation and serpentine behavior.

## Wi-Fi Image Upload

Start with a simple HTTP server.

Example endpoint:

```text
POST /image
```

Initial raw payload idea:

```text
width
height
RGBRGBRGB...
```

Future storage:

- built-in images: firmware / `images.c`
- uploaded images: flash filesystem, for example SPIFFS, LittleFS, or FATFS
- metadata: NVS or a small index file

`image_store` should hide those details from the application.

## Wi-Fi Animations

Payload idea:

```text
frame_count
frame_delay
frame1
frame2
...
```

Future ideas:

- Save animations to flash.
- Web UI.
- Animation library.

# Stretch Goals

- GIF to C converter
- Text rendering with a 5x7 font
- Scrolling text
- Brightness auto-adjustment
- SD card support
- Web UI for uploads
- Snake / Tetris / Conway's Game of Life
- MQTT / Home Assistant integration

