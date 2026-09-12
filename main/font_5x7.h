#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FONT_5X7_WIDTH 5
#define FONT_5X7_HEIGHT 7

typedef uint8_t font_5x7_glyph_t[FONT_5X7_HEIGHT];

const font_5x7_glyph_t *font_5x7_get_glyph(char c);
bool font_5x7_get_pixel(const font_5x7_glyph_t *glyph, uint8_t x, uint8_t y);
