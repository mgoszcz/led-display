I'm building my first standalone ESP-IDF project to learn embedded development properly (without Arduino).

Your role in this project is a PM and senior developer that has review role ONLY, you are not responsible for any coding here. The only you can do is to advise some solutions, review existing code, provide some code snippets as an example but DO NOT change any code in files unless directly asked.

Project:

- ESP32-S3
- ESP-IDF v5.5
- One WS2812B RGB LED matrix 16x16 (later two matrices combined into 32x16)
- Using Espressif's official led_strip component (RMT backend), not FastLED.

Current status:

- GPIO4 is used as the data pin (GPIO18 did not work on my board).
- The matrix works correctly.
- The physical layout is serpentine.
- LED #0 is located in the bottom-left corner.
- The panel has 256 LEDs.
- A custom led_matrix module has already been created.
- The module initializes the strip and provides basic drawing functions.

The long-term roadmap is:

1. Draw individual pixels.
2. Display predefined images stored in flash.
3. Support brightness and color manipulation.
4. Display predefined animations.
5. Receive and display arbitrary images over WiFi.
6. Receive and display animations over WiFi.

I want this project to be educational, not just functional.

Important:

- I want to follow good ESP-IDF architecture.
- I want to learn FreeRTOS, modules, event-driven programming and clean separation of responsibilities.
- Please avoid Arduino-style code or putting everything into app_main().

# LED Matrix ESP32 --- Project Roadmap

## 0. Hardware & Library Setup ✅ DONE

- ESP32-S3 + ESP-IDF
- Official `led_strip` component (RMT backend)
- GPIO4 used as DATA
- One 16x16 WS2812B matrix
- Physical layout discovered:
  - LED #0: bottom-left corner
  - Layout: serpentine
- Working features:
  - Matrix initialization
  - `set_pixel()`
  - Full panel rendering
  - First static image

---

## 1. Improve the `led_matrix` Module

**Goal:** Build a solid foundation.

Tasks:

- Introduce `led_matrix_config_t`
  - width
  - height
  - gpio
  - origin
  - layout
- Validate all public arguments
- Return `esp_err_t` from every public API
- Add global brightness support
- Finalize the public API:
  - `led_matrix_init()`
  - `led_matrix_clear()`
  - `led_matrix_set_pixel()`
  - `led_matrix_show()`
  - `led_matrix_display_image()`

---

## 2. Image Format

**Goal:** Define how images are stored.

Current recommendation:

```c
typedef struct {
    uint16_t width;
    uint16_t height;
    const rgb_t *pixels;
} led_matrix_image_t;
```

Tasks:

- Create `images.h` / `images.c`
- Store predefined images:
  - smiley
  - gradient
  - checkerboard
  - icons
- Implement:

```c
esp_err_t led_matrix_display_image(const led_matrix_image_t *image);
```

---

## 3. Application Architecture

**Goal:** Keep things simple at first.

Start with:

```c
while (1) {
    ...
    vTaskDelay(...);
}
```

Later introduce:

- `render_task`
- `command_task`
- `app_state`

Avoid overengineering until new requirements appear.

---

## 4. Framebuffer

**Goal:** Separate drawing from rendering.

```c
rgb_t framebuffer[WIDTH * HEIGHT];
```

Functions:

- framebuffer_clear()
- framebuffer_set_pixel()
- framebuffer_draw_image()
- led_matrix_render_framebuffer()

Flow:

    UART / Animation / Image
              ↓
         Framebuffer
              ↓
         Render Task
              ↓
          LED Strip

---

## 5. UART Control

Commands:

    PX x y r g b
    CLEAR
    SHOW
    BR value
    IMG name

Examples:

    PX 3 5 255 0 0
    BR 20
    IMG smile
    CLEAR

Tasks:

- Command parser
- Argument validation
- Status responses

---

## 6. Image Registry

```c
typedef struct {
    const char *name;
    const led_matrix_image_t *image;
} image_entry_t;
```

Commands:

    IMG smile
    IMG gradient
    IMG checker

---

## 7. Image Creation Workflow

Preferred pipeline:

    16x16 PNG
         ↓
    Python converter
         ↓
    C source/header
         ↓
    ESP-IDF

Implement a small `png_to_c.py` utility.

---

## 8. Animations

```c
typedef struct {
    uint16_t frame_count;
    uint16_t frame_delay_ms;
    const led_matrix_image_t *frames;
} led_matrix_animation_t;
```

Initial animations:

- blinking
- moving pixel
- rainbow
- fade
- animated smiley

---

## 9. Event-Driven Architecture

Events:

```c
EVENT_SET_PIXEL
EVENT_CLEAR
EVENT_SET_BRIGHTNESS
EVENT_SHOW_IMAGE
EVENT_START_ANIMATION
EVENT_STOP_ANIMATION
```

Flow:

    UART Task
        ↓
    Event Queue
        ↓
    Application Logic
        ↓
    Framebuffer
        ↓
    Render Task

---

## 10. Dual Matrix (32x16)

Hardware:

    ESP32 → Panel 1 DIN
    Panel 1 DOUT → Panel 2 DIN

Configuration:

```c
.width = 32;
.height = 16;
.max_leds = 512;
```

Tasks:

- Extend `xy_to_index()`
- Handle two chained panels
- Verify orientation
- Test all four corners

---

## 11. Wi-Fi Image Upload

Start with a simple HTTP server.

    POST /image

Payload:

    width
    height
    RGBRGBRGB...

---

## 12. Wi-Fi Animations

Payload:

    frame_count
    frame_delay
    frame1
    frame2
    ...

Future ideas:

- Save animations to flash
- Web UI
- Animation library

---

# Next Milestone

1.  Finalize `led_matrix_config_t`
2.  Finalize `led_matrix_image_t`
3.  Move images into `images.c/.h`
4.  Finish `display_image()`
5.  Create several predefined images
6.  Add brightness control
7.  Implement UART control

# Stretch goals fro future

- GIF → C converter
- Text rendering (5×7 font)
- Scrolling text
- Brightness auto-adjustment
- SD card support
- Web UI (upload images/animations)
- Snake / Tetris / Conway's Game of Life
- MQTT/Home Assistant integration
