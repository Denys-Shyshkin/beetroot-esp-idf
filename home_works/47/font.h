#pragma once

#include <stdint.h>

#define FONT_WIDTH 5
#define CHAR_GAP 1
#define CHAR_STEP (FONT_WIDTH + CHAR_GAP)

const uint8_t *font_get_glyph(char c);