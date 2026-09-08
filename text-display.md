# Implementation plan

Codex proposal was to start with font 5x7 - it is good to start but need to keep in mind that minimal screen size will be 16x16 so we should be able to scale up)

## Required components to be created

- Characters library (each character has it's own representation as an array with pixels)
- endpoints to display text (POST accepting body with text. Raw string or chars?)
- text display engine
  - based on text build full image to display (size will be n x 7 depending on string length), so just simply combine all chars together with 1 pixel space between
  - Create frame per each display iteration (text is moving from right to left, starts with empty screen and ends with empty screen)
  - Handle delay between frames (to be set experimentally but around 500 ms or even less) to ensure text is moving smoothly. Delay will be implemented by using timer

## Text display engine

This is first implementation, later we will work on animations which will work in very similar way, they will just change frames after specific delay. The difference is that with text display, we are providing text, and animation must be done by implemented logic. Animations will be already delivered in frames format and engine will only need to display them in a proper speed

## Starting point

This is good oportunity to get familiar with things that were not used so far in this project, I recommend to start with some experiments

- implement animation engine that will just move a simple shape like small square moving frmo right to left in a loop, here you can work on two things:
  - preparing frames to display, each next frame should be moved left by 1 pixel (in x axis)
  - handling delay between frames displayed via timers
- work with endpoints to figure out proper way of sending text

## Endpoints

For now we might need two endpoints

- endpoint to send text
- endpoint to set text color
  In future we might set font size and background color unless we decide that we will send all in one request

## Implementations

- AI proposed to create framebuffer_draw_char() and framebuffer_draw_text(). My proposal was to handle it externally and just use framebuffer_draw_image. To be decided on experimentation phase
- we can start with hardcoding display mode in main.c in experimentation pahse but it should be moved to endpoints quickly

## HL Flow

User send text to endpoint
|
v
it invokes method to display text
|
v
method combines all characters into one image and creates frames to display
|
v
method calls framebuffer_draw_image with first frame
|
v
method waits specific amount of time
|
v
method calls framebuffer_draw_image with next frame
|
v
...
|
v
when all frames are displayed it starts again from first frame
