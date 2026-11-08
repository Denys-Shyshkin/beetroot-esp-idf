#include "ssd1306.h"

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ssd1306";

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_dev;

/* Фреймбуфер: сторінкова організація, як у самого контролера.
 * Байт s_fb[page * OLED_W + x] тримає 8 пікселів по вертикалі:
 * біт 0 — верхній (y = page*8), біт 7 — нижній (y = page*8 + 7).
 */
static uint8_t s_fb[OLED_W * OLED_PAGES];
static uint8_t s_tx[1 + sizeof(s_fb)]; /* 0x40 + дані */

/* ---------------- низький рівень ---------------- */

static void oled_cmd(uint8_t c) {
    uint8_t buf[2] = {0x00, c};
    ESP_ERROR_CHECK(i2c_master_transmit(s_dev, buf, sizeof(buf), 100));
}

void oled_init(int sda_pin, int scl_pin, uint8_t addr) {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &s_bus));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev));

    ESP_ERROR_CHECK(i2c_master_probe(s_bus, addr, 100));
    ESP_LOGI(TAG, "дисплей знайдено на 0x%02X", addr);

    static const uint8_t init_seq[] = {
        0xAE,       /* display off                */
        0xD5, 0x80, /* clock divide / osc freq    */
        0xA8, 0x3F, /* multiplex ratio = 64 рядки */
        0xD3, 0x00, /* display offset = 0         */
        0x40,       /* start line = 0             */
        0x8D, 0x14, /* charge pump ON             */
        0x20, 0x00, /* horizontal addressing mode */
        0xA1,       /* segment remap (дзеркало X) */
        0xC8,       /* COM scan direction (Y)     */
        0xDA, 0x12, /* COM pins config            */
        0x81, 0x9F, /* contrast                   */
        0xD9, 0xF1, /* pre-charge                 */
        0xDB, 0x30, /* VCOMH deselect             */
        0xA4,       /* виводити вміст RAM         */
        0xA6,       /* нормальний (не інверсний)  */
        0xAF,       /* display on                 */
    };
    for (size_t i = 0; i < sizeof(init_seq); i++) {
        oled_cmd(init_seq[i]);
    }

    fb_clear();
    oled_flush();
}

void oled_contrast(uint8_t value) {
    oled_cmd(0x81);
    oled_cmd(value);
}

void oled_invert(bool on) {
    oled_cmd(on ? 0xA7 : 0xA6);
}

void oled_flush_pages(int first, int last) {
    if (first < 0)
        first = 0;
    if (last > OLED_PAGES - 1)
        last = OLED_PAGES - 1;
    if (first > last)
        return;

    size_t n = (size_t)(last - first + 1) * OLED_W;

    oled_cmd(0x21);
    oled_cmd(0);
    oled_cmd(OLED_W - 1);
    oled_cmd(0x22);
    oled_cmd(first);
    oled_cmd(last);

    s_tx[0] = 0x40;
    memcpy(&s_tx[1], &s_fb[(size_t)first * OLED_W], n);
    ESP_ERROR_CHECK(i2c_master_transmit(s_dev, s_tx, n + 1, 200));
}

void oled_flush(void) {
    oled_flush_pages(0, OLED_PAGES - 1);
}

void fb_clear(void) {
    memset(s_fb, 0, sizeof(s_fb));
}

void fb_pixel(int x, int y, bool on) {
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H)
        return;

    uint8_t *byte = &s_fb[(y >> 3) * OLED_W + x];
    uint8_t mask = 1u << (y & 7);

    if (on)
        *byte |= mask;
    else
        *byte &= ~mask;
}

void fb_hline(int x, int y, int w, bool on) {
    for (int i = 0; i < w; i++)
        fb_pixel(x + i, y, on);
}

void fb_vline(int x, int y, int h, bool on) {
    for (int i = 0; i < h; i++)
        fb_pixel(x, y + i, on);
}

void fb_rect_fill(int x, int y, int w, int h, bool on) {
    for (int j = 0; j < h; j++)
        fb_hline(x, y + j, w, on);
}

void fb_rect(int x, int y, int w, int h, bool on) {
    fb_hline(x, y, w, on);
    fb_hline(x, y + h - 1, w, on);
    fb_vline(x, y, h, on);
    fb_vline(x + w - 1, y, h, on);
}

void fb_sprite(const sprite_t *s, int x, int y) {
    for (int row = 0; row < s->h; row++) {
        const char *line = s->rows[row];
        for (int col = 0; col < s->w; col++) {
            char c = line[col];
            if (c == '\0')
                break;
            if (c == '.' || c == ' ')
                continue;
            fb_pixel(x + col, y + row, true);
        }
    }
}