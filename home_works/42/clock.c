#include "clock.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "i2c.h"
#include "esp_check.h"

static const char *TAG = "CLOCK";

i2c_master_dev_handle_t clock_handle;

// converts Binary-Coded Decimal byte into standard decimals integer
static uint8_t bcd2dec(uint8_t v) {
    return (uint8_t)((v >> 4) * 10 + (v & 0x0F));
}

esp_err_t clock_get_time(clock_time_t *time) {
    uint8_t reg = 0x00; // seconds read register, autoincrement
    uint8_t data_buffer[7];

    esp_err_t time_error = i2c_transmit_receive_data(clock_handle, &reg, 1, data_buffer, sizeof(data_buffer));
    ESP_RETURN_ON_ERROR(time_error, TAG, "Get time failed");

    time->sec = bcd2dec(data_buffer[0] & 0x7F);
    time->min = bcd2dec(data_buffer[1] & 0x7F);
    time->hour = bcd2dec(data_buffer[2] & 0x3F);
    time->dow = bcd2dec(data_buffer[3] & 0x07);
    time->day = bcd2dec(data_buffer[4] & 0x3F);
    time->month = bcd2dec(data_buffer[5] & 0x1F);
    time->year = bcd2dec(data_buffer[6]) + 2000;

    return ESP_OK;
}