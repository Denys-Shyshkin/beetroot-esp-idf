#pragma once

#include "driver/i2c_master.h"
#include <stdbool.h>
#include <stdint.h>

#define OLED_W 128
#define OLED_H 64
#define OLED_PAGES (OLED_H / 8)

typedef struct {
    const char *const *rows;
    int w;
    int h;
} sprite_t;

void oled_init(uint8_t addr, i2c_master_bus_handle_t *bus_handle);
void oled_flush(void);
void oled_flush_pages(int first, int last);
void oled_contrast(uint8_t value);
void oled_invert(bool on);

void fb_clear(void);
void fb_pixel(int x, int y, bool on);
void fb_hline(int x, int y, int w, bool on);
void fb_vline(int x, int y, int h, bool on);
void fb_rect_fill(int x, int y, int w, int h, bool on);
void fb_rect(int x, int y, int w, int h, bool on);
void fb_sprite(const sprite_t *s, int x, int y);