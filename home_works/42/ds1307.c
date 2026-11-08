#include "ds1307.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "ds1307";

static i2c_master_dev_handle_t rtc;

static inline uint8_t bcd2dec(uint8_t v) {
    return (v >> 4) * 10 + (v & 0x0F);
}
static inline uint8_t dec2bcd(uint8_t v) {
    return ((v / 10) << 4) | (v % 10);
}

void clock_init(uint8_t addr, i2c_master_bus_handle_t *bus_handle) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_cfg, &rtc));

    ESP_ERROR_CHECK(i2c_master_probe(*bus_handle, addr, 100));
    ESP_LOGI(TAG, "годинник знайдено на 0x%02X", addr);
}

esp_err_t ds1307_set_time(const ds1307_time_t *time) {
    uint8_t data[8];

    data[0] = 0x00;

    data[1] = dec2bcd(time->sec) & 0x7F;
    data[2] = dec2bcd(time->min);

    data[3] = dec2bcd(time->hour) & 0x3F;

    data[4] = dec2bcd(time->dow);
    data[5] = dec2bcd(time->day);
    data[6] = dec2bcd(time->month);

    data[7] = dec2bcd(time->year - 2000);

    return i2c_master_transmit(rtc, data, sizeof(data), 1000);
}

esp_err_t ds1307_get_time(ds1307_time_t *time) {
    uint8_t reg = 0x00;
    uint8_t data[7];

    esp_err_t err = i2c_master_transmit_receive(rtc, &reg, 1, data, sizeof(data), 1000);
    if (err != ESP_OK)
        return err;

    time->sec = bcd2dec(data[0] & 0x7F);
    time->min = bcd2dec(data[1] & 0x7F);
    time->hour = bcd2dec(data[2] & 0x3F);
    time->dow = bcd2dec(data[3] & 0x07);
    time->day = bcd2dec(data[4] & 0x3F);
    time->month = bcd2dec(data[5] & 0x1F);
    time->year = bcd2dec(data[6]) + 2000;

    return ESP_OK;
}