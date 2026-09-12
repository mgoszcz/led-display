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

- `display_types.h`
  - Shared display-level request/config types.
  - Keeps cross-module payload structs outside individual infrastructure modules.
  - Current use:
    - text display configuration for HTTP `/text` and text rendering.

- `font_5x7.h` / `font_5x7.c`
  - 5x7 bitmap font module.
  - Stores glyphs as compact bitmasks.
  - Provides glyph lookup and per-pixel access helpers.
  - Should not know about framebuffer, LED matrix, scrolling, HTTP, or application modes.

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
  - Should not know image names, image registry, Wi-Fi, HTTP, UART commands, or application state.

- `framebuffer.h` / `framebuffer.c`
  - RAM drawing surface.
  - Stores mutable pixels before they are sent to the LED strip.
  - Handles:
    - init
    - clear
    - fill
    - set pixel
    - draw image at destination coordinates
    - draw raw RGB888 frame data
  - Should not know about GPIO, RMT, serpentine layout, physical LED ordering, Wi-Fi, or HTTP.

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
  - Current API returns `esp_err_t` and writes the found image through an output pointer.
  - In the future, it should hide whether an image comes from firmware, flash filesystem, Wi-Fi upload, or another backend.

- `wifi_app.h` / `wifi_app.c`
  - Wi-Fi infrastructure module.
  - Starts Wi-Fi in station mode.
  - Uses project configuration from Kconfig / `sdkconfig`.
  - Initializes NVS, netif, event loop, default STA interface, Wi-Fi driver, and connection.
  - Handles Wi-Fi disconnect reconnect attempts and logs IP on `IP_EVENT_STA_GOT_IP`.
  - Should not know about framebuffer, LED matrix, images, or HTTP endpoints.

- `http_server_app.h` / `http_server_app.c`
  - HTTP infrastructure module.
  - Starts ESP-IDF HTTP server.
  - Currently exposes:
    - `GET /health` -> `OK`
    - `POST /frame` -> accepts one full raw RGB888 frame
    - `POST /demo` -> enables demo mode again
    - `POST /brightness` -> sets brightness percent
    - `POST /text` -> starts scrolling text from JSON config
  - Keeps server handle as static module state.
  - Uses callbacks passed from `main.c` so it does not directly know framebuffer or LED matrix internals.

- `text_display_engine.h` / `text_display_engine.c`
  - Text rendering / scrolling module.
  - Builds a wider virtual text image from the 5x7 font.
  - Uses an ESP timer and FreeRTOS task to scroll a 16x16 viewport.
  - Accepts text config:
    - text
    - color
    - speed
  - Owns its task/timer lifecycle through start/stop functions.
  - Currently renders directly through framebuffer and LED matrix; this is acceptable for MVP but should later move behind a display controller.

- `tools/pixel-editor.html`
  - Local browser-based 16x16 pixel editor.
  - Supports:
    - ESP32 IP input
    - `GET /health` check
    - drawing pixels with selected color
    - right-click erase
    - clear grid
    - save/load in browser local storage
    - export/import `led-image` JSON file
    - import PNG/JPEG/WebP/GIF and convert to 16x16 pixels
    - brightness slider using `POST /brightness`
    - `POST /frame`
    - `POST /demo`
    - text display using `POST /text` JSON
  - Verified with the ESP32 device in the current local workflow.

- `main/Kconfig.projbuild`
  - Project menuconfig options:
    - Wi-Fi SSID
    - Wi-Fi password
    - Wi-Fi maximum retry count

- `main.c`
  - Current application entry point.
  - Initializes framebuffer, LED matrix, Wi-Fi, and HTTP server.
  - Looks up predefined images.
  - Runs a simple built-in image demo on startup.
  - Provides HTTP callbacks:
    - frame upload disables demo mode and renders uploaded frame
    - demo endpoint enables demo mode again
    - brightness endpoint updates brightness and re-renders current framebuffer
    - text endpoint starts/restarts scrolling text
  - Currently owns simple display mode switching:
    - demo
    - frame
    - text

# Current Render Flow

Current built-in image flow:

```text
image_store_get("name", &image)
        |
        v
framebuffer_clear()
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

Raw HTTP frame flow:

```text
POST /frame
body: RGBRGBRGB...
        |
        v
http_server_app callback
        |
        v
framebuffer_draw_rgb888()
        |
        v
led_matrix_render_framebuffer()
        |
        v
WS2812B matrix
```

This is the preferred direction. The application should talk to `image_store` for image lookup and to `framebuffer` for drawing. The `led_matrix` module should remain the low-level hardware renderer.

Current local web editor flow:

```text
tools/pixel-editor.html
        |
        v
POST /frame raw RGB888, 768 bytes
        |
        v
ESP32 HTTP server
        |
        v
framebuffer
        |
        v
LED matrix
```

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

## 3. Image Store - DONE FOR CURRENT NEEDS

Current API:

```c
esp_err_t image_store_get(const char *name, const led_matrix_image_t **out_image);
```

Current behavior:

- Looks up images by name.
- Returns `ESP_OK` and writes the image pointer when found.
- Returns `ESP_ERR_NOT_FOUND` when not found.
- Returns `ESP_ERR_INVALID_ARG` for invalid arguments.

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
- `framebuffer_fill()`
- `framebuffer_clear()`
- `framebuffer_set_pixel()`
- `framebuffer_draw_image()`
- `framebuffer_draw_rgb888()`

Current behavior:

- Framebuffer is a mutable RAM drawing surface.
- Built-in images are copied into the framebuffer before rendering.
- Raw RGB888 data can be copied into the framebuffer.
- `framebuffer_draw_rgb888()` currently expects a full-frame payload matching framebuffer width and height.
- `led_matrix_render_framebuffer()` sends the framebuffer to the physical LED matrix.

## 5. Wi-Fi Infrastructure - INITIAL VERSION DONE

Current behavior:

- Uses STA mode.
- Wi-Fi settings are configured through Kconfig / `sdkconfig`.
- Initializes NVS with recovery for `ESP_ERR_NVS_NO_FREE_PAGES` and `ESP_ERR_NVS_NEW_VERSION_FOUND`.
- Registers Wi-Fi and IP event handlers.
- Retries Wi-Fi connection up to configured retry count.
- Logs the assigned IP address.

Current config options:

```text
CONFIG_WIFI_SSID
CONFIG_WIFI_PASSWORD
CONFIG_WIFI_MAX_RETRY
```

## 6. HTTP Server - INITIAL VERSION DONE

Current behavior:

- Starts ESP-IDF HTTP server.
- Keeps `httpd_handle_t` as static module state.
- `http_server_app_start()` is idempotent.
- Uses callbacks for application-specific behavior.
- Adds CORS response header for local browser tooling:

```text
Access-Control-Allow-Origin: *
```

Current endpoints:

```text
GET /health -> OK
POST /frame -> raw RGB888 frame, exactly 768 bytes
POST /demo -> enables startup demo again
POST /brightness -> brightness percent
POST /text -> JSON text config
```

## 7. First Interactive Pixel Editor - INITIAL VERSION DONE

Current behavior:

- Local standalone HTML page in `tools/pixel-editor.html`.
- User can draw a 16x16 frame in the browser.
- The page sends raw RGB888 bytes to `POST /frame`.
- The editor supports save/load in browser local storage.
- The editor supports export/import as `led-image` JSON files.
- The editor imports common image files and converts them to the current 16x16 grid.
- A brightness slider sends the selected value to `POST /brightness`.
- Health check, frame upload, demo trigger, and brightness control have been verified on device.
- Text display control sends JSON to `POST /text`.
- The editor separates controls into connection, text display, pixel frame, and device sections.
- The editor is local only; it is not hosted by ESP32 yet.

## 8. Scrolling Text - INITIAL VERSION DONE

Current behavior:

- Device has a 5x7 bitmap font.
- `/text` accepts JSON:

```json
{
  "text": "Maria",
  "color": { "r": 255, "g": 0, "b": 0 },
  "speedMs": 100
}
```

- Text engine copies request text into its own static buffer.
- Text engine builds a wider virtual image with left/right padding.
- A timer notifies a FreeRTOS task.
- The task copies a moving 16x16 viewport into the framebuffer and renders it.
- New `/text` requests restart the previous text display.
- Leaving text mode stops the text task/timer.

Current limitations:

- ASCII-oriented 5x7 font only.
- Text length is capped.
- Color and speed are per text run, not dynamic during a run.
- Text engine currently renders directly. Later this should go through a display controller or render owner.

# Current Near-Term Architecture

The application is transitioning from a simple loop to mode-based rendering:

```text
HTTP /frame       -> DISPLAY_MODE_FRAME -> framebuffer + render
HTTP /demo        -> DISPLAY_MODE_DEMO  -> main demo loop renders images
HTTP /text        -> DISPLAY_MODE_TEXT  -> text task/timer renders scrolling text
HTTP /brightness  -> update brightness  -> re-render current framebuffer
```

For MVP, mode switching remains in `main.c`.
The next architectural cleanup should be a small display controller or render owner so only one task owns framebuffer/render operations.

# Next Milestone

Prepare the project for larger rectangular displays and stabilize rendering ownership before adding more animation features:

```text
Browser pixel editor / text controls
        |
        v
ESP32 HTTP endpoints
        |
        v
framebuffer / text engine
        |
        v
LED matrix render
```

Suggested next steps:

1. Introduce display size constants/configuration instead of scattered hardcoded `16x16` assumptions.
2. Prepare firmware and tooling for rectangular or square displays made from 16x16 panels:
   - 16x16
   - 32x16
   - 16x32
   - 32x32
   - future rectangular combinations if they remain regular panel grids.
3. Add/verify CORS `OPTIONS` handling for JSON endpoints such as `/text`.
4. Test `/text`, `/frame`, `/demo`, and `/brightness` mode switching repeatedly on hardware.
5. Add a minimal synchronization strategy around framebuffer/render operations, or start extracting a display controller.
6. Consider extracting demo/application mode handling out of `main.c`.
7. Decide the next product direction:
   - keep using local HTML during development
   - or host the editor directly from ESP32

# Known Issues For Future

These are known improvement areas. They are not all blockers for the next small milestone.

- `wifi_app_start()` starts Wi-Fi and begins connecting, but it does not wait until `IP_EVENT_STA_GOT_IP`. For later reliability, add `wifi_app_wait_connected(timeout_ms)` or an event group.
- HTTP server currently starts immediately after `wifi_app_start()`. This is acceptable for `/health`, but future user-facing endpoints may want to wait for Wi-Fi connection/IP.
- Kconfig symbols are currently generic: `CONFIG_WIFI_SSID`, `CONFIG_WIFI_PASSWORD`, `CONFIG_WIFI_MAX_RETRY`. Later consider prefixing them, for example `CONFIG_LED_DISPLAY_WIFI_SSID`.
- Wi-Fi retry handling logs failure after max retries, but does not expose a connection status or failure state to the application.
- `wifi_app` has no stop/deinit function yet. Add only when lifecycle requires it.
- `http_server_app` has no stop function yet. Add `http_server_app_stop()` when needed.
- HTTP server CORS support is minimal. Current endpoints work with simple browser requests, but future custom headers may require `OPTIONS` handling.
- JSON endpoints such as `/text` can trigger browser preflight requests because of `Content-Type: application/json`. Add proper `OPTIONS` handling and CORS headers for JSON endpoints.
- `led_matrix` should eventually validate that it has been initialized before public operations.
- Consider returning `esp_err_t` from `led_matrix_set_brightness()`.
- Keep public API argument validation consistent across all modules.
- Decide later whether off-screen drawing should fail or clip. Current framebuffer image drawing requires the image to fit.
- `framebuffer_draw_rgb888()` currently supports only full-frame payloads. That is intentional for the first HTTP MVP.
- Display dimensions are still partially hardcoded as 16x16 in firmware and tooling. This should be addressed before going deeper into animations or multi-panel support.
- Demo/application mode is currently simple shared state in `main.c`. This is acceptable for MVP, but should become a small app-state module, display controller, or event-driven flow later.
- Rendering can currently be initiated by more than one context:
  - main demo loop
  - HTTP callbacks
  - text display task
  - brightness callback re-render
  This can potentially race around shared framebuffer/LED rendering. A rare ESP hang/reset was observed once while changing brightness during scrolling text. It did not reproduce immediately, but the likely future fix is a mutex around framebuffer/render operations or, better, a single display/render owner task.
- Decide whether `led_matrix_set_brightness()` should store brightness as percent or raw 0-255 internally. Current API direction is percent from HTTP/UI.
- Automatic brightness based on ambient light is a future hardware/software feature, likely using a photoresistor or light sensor.
- `tools/pixel-editor.html` is not hosted by ESP32 yet.
- Local editor save/load currently uses one browser local storage slot only. Multiple named drawings are future work.
- Current `/frame` endpoint accepts only full 16x16 frames. Partial updates or single-pixel control are future work.
- `led-image` is currently an editor/document format, not a device render format. Keep `/frame` raw RGB888 for live rendering.
- Photo/image conversion currently supports browser-side fit/crop into the 16x16 grid. More advanced controls such as gamma correction, contrast, dithering, and palette reduction are future work.
- Text rendering exists as an MVP, but it currently owns its own timer/task and renders directly. Later it should feed frames/events into a display controller.
- `sdkconfig` may contain Wi-Fi credentials. It is ignored by git now; keep it that way unless credentials are removed.

# Future Roadmap

## Display Size And Multi-Panel Layout

Near-term architectural priority:

- Remove scattered hardcoded 16x16 assumptions.
- Support displays as regular rectangular/square grids made from 16x16 panels.
- Initial target configurations:
  - 16x16: one panel
  - 32x16: two panels side by side
  - 16x32: two panels stacked vertically
  - 32x32: four panels

Important distinction:

- Logical display size:
  - what the application, framebuffer, text engine, HTTP endpoints, and editor see
  - examples: 16x16, 32x16, 16x32, 32x32
- Physical LED mapping:
  - how logical `(x, y)` maps to the actual LED index in chained WS2812B strips
  - depends on panel order, panel orientation, origin, and serpentine layout

Suggested configuration direction:

```c
#define DISPLAY_WIDTH 16
#define DISPLAY_HEIGHT 16
#define DISPLAY_PIXEL_COUNT (DISPLAY_WIDTH * DISPLAY_HEIGHT)
#define DISPLAY_FRAME_BYTES (DISPLAY_PIXEL_COUNT * 3)
```

Later, move from compile-time constants to a display config struct if needed:

```c
typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t panel_width;
    uint16_t panel_height;
    uint8_t panels_x;
    uint8_t panels_y;
} display_config_t;
```

Expected affected areas:

- `main.c`
  - framebuffer pixel buffer size
  - `led_matrix_config_t`
- `http_server_app`
  - `/frame` byte length
  - request validation
- `framebuffer`
  - should mostly already be dimension-aware
- `text_display_engine`
  - viewport width/height should follow display size
  - text padding should adapt to display width
- `tools/pixel-editor.html`
  - grid dimensions
  - raw frame byte count
  - image import conversion size
  - save/export metadata
- built-in images
  - currently 16x16; larger displays may need scaling, centering, or separate assets

Preferred order:

1. Centralize display width/height for current 16x16.
2. Replace hardcoded firmware constants with centralized size constants.
3. Update local editor to use configurable width/height constants.
4. Verify current 16x16 still works.
5. Add/verify physical mapping for two-panel configurations.
6. Test 32x16 and/or 16x32 on real hardware.
7. Only then build more advanced animation features.

## Web Pixel Editor

Current short-term product goal:

```text
Browser 16x16 grid editor
        |
        v
POST /frame raw RGB888
        |
        v
ESP32 framebuffer
        |
        v
LED matrix
```

Preferred order:

1. Implement `POST /frame`. DONE
2. Test with generated raw RGB888 data. DONE
3. Build a local browser-based 16x16 editor. DONE
4. Send frames from browser to ESP32. DONE
5. Add simple save/load/export for drawings. DONE
6. Add device-side brightness control. DONE
7. Later host the web UI directly from ESP32.

Potential next editor features:

- Export/import `led-image` JSON. DONE
- Import PNG/JPEG/WebP/GIF and convert to grid pixels. DONE
- Send scrolling text with color and speed. DONE
- Export C array for built-in firmware images.
- Save recent IP address in browser local storage.
- Preview sent payload brightness separately from editing colors.
- Add predefined palette.
- Add fill bucket or line tool.
- Add grid coordinate display.

## Brightness Control

Current behavior:

- The local editor sends brightness changes to `POST /brightness`.
- `/frame` sends raw RGB888 data without client-side brightness scaling.
- Device brightness is applied by `led_matrix` while rendering.
- Changing brightness re-renders the current framebuffer immediately.

Current implementation:

```text
POST /brightness
body: brightness percent, 0-100
        |
        v
app callback
        |
        v
led_matrix_set_brightness()
        |
        v
led_matrix_render_framebuffer(current framebuffer)
```

Important behavior:

- Changing brightness should update the LEDs immediately.
- `led_matrix_set_brightness()` only changes the brightness state.
- The application callback calls `led_matrix_render_framebuffer()` after changing brightness to push recalculated pixel values to the LED strip.

Future automatic brightness:

- Add photoresistor or light sensor input.
- Read ambient light with ADC or sensor driver.
- Map ambient light to LED brightness.
- Smooth changes over time to avoid visible jumping.

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
- current input source, for example demo loop, HTTP frame, UART command, animation

## Event-Driven Architecture

Events:

```c
EVENT_SET_PIXEL
EVENT_CLEAR
EVENT_SET_BRIGHTNESS
EVENT_SHOW_IMAGE
EVENT_SHOW_RAW_FRAME
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
- scrolling text

## Text Rendering

Text should be treated as a drawing/animation feature, not as a low-level LED matrix feature.

Current building blocks:

- 5x7 bitmap font.
- JSON `/text` endpoint.
- `text_display_config_t` style config:
  - text
  - color
  - speed
- Text engine builds a virtual image and scrolls a 16x16 viewport over it.

Current endpoint:

```text
POST /text
body: text, color, speed, mode
```

Current short-term JSON shape:

```json
{
  "text": "Hello",
  "color": { "r": 255, "g": 0, "b": 0 },
  "speedMs": 100
}
```

Later, the endpoint should create an animation/state command rather than directly owning render timing.

Future rich text direction:

- Support multiple text segments with separate colors.
- Useful for status displays such as stock/crypto quotes, where symbols and values may need different colors.
- Prefer segment-level coloring over per-character coloring for the first richer format.

Possible future payload:

```json
{
  "segments": [
    {
      "text": "AAPL ",
      "color": { "r": 255, "g": 255, "b": 255 }
    },
    {
      "text": "+1.24%",
      "color": { "r": 0, "g": 255, "b": 0 }
    }
  ],
  "speedMs": 100
}
```

Possible internal direction:

```c
typedef struct {
    char text[TEXT_SEGMENT_MAX_CHARS + 1];
    rgb_t color;
} text_segment_t;
```

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

## Image Upload And Conversion

Raw RGB888 works well for the first live frame endpoint, but it is not necessarily the best long-term format for user-uploaded images.

Current format split:

- Render format:
  - `POST /frame`
  - raw RGB888 bytes
  - length = `width * height * 3`
  - optimized for direct framebuffer rendering
- Document format:
  - `led-image` JSON
  - used by the local editor for save/load/export/import
  - carries metadata and editable pixel data

Current `led-image` v1 document:

```json
{
  "format": "led-image",
  "version": 1,
  "type": "image",
  "width": 16,
  "height": 16,
  "brightness": 35,
  "pixels": [
    [0, 0, 0],
    [255, 0, 0]
  ]
}
```

Possible input formats:

- Raw RGB888 frame:
  - simplest for ESP32
  - no metadata
  - good for live preview
- JSON with metadata and pixel data:
  - easy to inspect/debug
  - larger payload
  - reasonable for small 16x16 images
- PNG/JPEG upload:
  - familiar to users
  - likely better converted in browser or desktop tooling first
  - decoding on ESP32 is possible but more complexity than needed now

Current short-term direction:

```text
PNG/JPEG in browser or local tool
        |
        v
resize/crop to 16x16 or 32x16
        |
        v
convert to RGB888
        |
        v
POST /frame or save/export
```

Current local editor behavior:

- Imports PNG/JPEG/WebP/GIF through a browser file input.
- Uses an offscreen canvas to convert the image to 16x16.
- Supports:
  - crop mode, good for filling the whole matrix
  - fit mode, good for preserving the full image with black margins
- Writes converted pixels into the editable grid.
- Does not automatically send the converted image; the user still clicks `SEND`.

Useful conversion options:

- fit vs crop
- nearest-neighbor vs smooth resize
- brightness/gamma correction
- palette reduction
- preview before send

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

## Persistent Wi-Fi Image Upload

Later endpoints may look like:

```text
POST /images/{name}
GET /images
DELETE /images/{name}
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
