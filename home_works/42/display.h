#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "clock.h"

#define DISPLAY_ADDRESS 0x3C
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

extern i2c_master_dev_handle_t display_handle;

void display_setup(void);
esp_err_t display_show_text(uint8_t page, uint8_t col, const char *text);