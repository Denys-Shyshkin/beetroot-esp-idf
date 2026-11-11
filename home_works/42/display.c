#include "display.h"
#include "clock.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "font.h"
#include "i2c.h"
#include "esp_check.h"

#define PIXELS_PER_BYTE 8
#define DISPLAY_PAGES 8

static const char *TAG = "DISPLAY";

static uint8_t clear_buffer[1 + (DISPLAY_WIDTH * DISPLAY_HEIGHT) / PIXELS_PER_BYTE] = {0x40}; // control byte, followed by zeros

i2c_master_dev_handle_t display_handle;

static esp_err_t display_clear(void) {
    const uint8_t cmd_buffer[] = {
        0x00,             // control byte: commands follow
        0x21, 0x00, 0x7F, // column range 0..127
        0x22, 0x00, 0x07, // page range 0..7
    };

    esp_err_t cmd_error = i2c_transmit_data(display_handle, cmd_buffer, sizeof(cmd_buffer));
    ESP_RETURN_ON_ERROR(cmd_error, TAG, "Clear failed");

    esp_err_t clear_error = i2c_transmit_data(display_handle, clear_buffer, sizeof(clear_buffer));
    ESP_RETURN_ON_ERROR(clear_error, TAG, "Clear failed");

    return ESP_OK;
}

void display_setup(void) {
    uint8_t setup_buffer[] = {
        0x00,       // control byte: commands follow
        0xAE,       // display off while configuring
        0xD5, 0x80, // clock divide ratio / oscillator frequency (default)
        0xA8, 0x3F, // multiplex ratio: 64 rows
        0xD3, 0x00, // display offset: 0
        0x40,       // display start line: 0
        0x8D, 0x14, // charge pump on
        0xA1,       // mirror columns
        0xC8,       // flip rows
        0xDA, 0x12, // COM pins configuration for 128x64
        0x81, 0x7F, // contrast (0x00..0xFF)
        0x20, 0x00, // memory addressing mode: horizontal
        0x2E,       // stop scrolling
        0xA4,       // display shows RAM content
        0xA6,       // normal display (not inverted)
    };

    ESP_ERROR_CHECK(i2c_transmit_data(display_handle, setup_buffer, sizeof(setup_buffer)));

    ESP_ERROR_CHECK(display_clear());

    const uint8_t on_buffer[] = {
        0x00, // control byte: commands follow
        0xAF  // display on
    };

    ESP_ERROR_CHECK(i2c_transmit_data(display_handle, on_buffer, sizeof(on_buffer)));

    ESP_LOGI(TAG, "Setup successful");
}

static esp_err_t display_show_char(uint8_t page, uint8_t col, char c) {
    ESP_RETURN_ON_FALSE(page < DISPLAY_PAGES, ESP_ERR_INVALID_ARG, TAG, "Show char failed: page out of range %d (%d max)", page, DISPLAY_PAGES - 1);
    ESP_RETURN_ON_FALSE(col + CHAR_STEP <= DISPLAY_WIDTH, ESP_ERR_INVALID_ARG, TAG, "Show char failed: col out of range %d (%d max)", col, DISPLAY_WIDTH - CHAR_STEP);

    const uint8_t cmd_buffer[] = {
        0x00,                           // control byte: commands follow
        0x21, col, col + CHAR_STEP - 1, // column range: 6 columns of this char
        0x22, page, page,               // one page
    };

    esp_err_t cmd_error = i2c_transmit_data(display_handle, cmd_buffer, sizeof(cmd_buffer));
    ESP_RETURN_ON_ERROR(cmd_error, TAG, "Show char failed: %c", c);

    const uint8_t *glyph = font_get_glyph(c);
    if (glyph == NULL) {
        ESP_LOGW(TAG, "Show char failed: glyph not found %c", c);
    }

    uint8_t data_buffer[1 + CHAR_STEP] = {0x40}; // control byte: data follow
    for (uint8_t i = 0; i < FONT_WIDTH; i++) {
        data_buffer[1 + i] = glyph == NULL ? 0x00 : glyph[i];
    }

    esp_err_t data_error = i2c_transmit_data(display_handle, data_buffer, sizeof(data_buffer));
    ESP_RETURN_ON_ERROR(data_error, TAG, "Show char failed: %c", c);

    return ESP_OK;
}

esp_err_t display_show_text(uint8_t page, uint8_t col, const char *text) {
    for (const char *c = text; *c != '\0'; c++) {
        esp_err_t error = display_show_char(page, col, *c);
        ESP_RETURN_ON_ERROR(error, TAG, "Show text failed, %s", text);

        col += CHAR_STEP;
    }

    return ESP_OK;
}