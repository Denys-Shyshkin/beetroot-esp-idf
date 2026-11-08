#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#define PIN_SDA 10
#define PIN_SCL 9
static const char *TAG = "ds1307_time";

typedef struct
{
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t dow;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} ds1307_time_t;

static inline uint8_t bcd2dec(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
static inline uint8_t dec2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

ds1307_time_t new_time = {
    .sec = 0,
    .min = 30,
    .hour = 12,
    .dow = 1,
    .day = 17,
    .month = 8,
    .year = 2026};

esp_err_t ds1307_set_time(i2c_master_dev_handle_t rtc, const ds1307_time_t *time)
{

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

esp_err_t ds1307_get_time(i2c_master_dev_handle_t rtc, ds1307_time_t *time)
{
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

void app_main(void)
{

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_SDA,
        .scl_io_num = PIN_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x68,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t rtc;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &rtc));

    ds1307_time_t current_time;

    ESP_ERROR_CHECK(ds1307_set_time(rtc, &new_time));
    ESP_LOGI(TAG, "Час успішно встановлено!");

    while (1)
    {
        if (ds1307_get_time(rtc, &current_time) == ESP_OK)
        {
            ESP_LOGI(TAG, "Поточний час: %02d %02d.%02d.%04d %02d:%02d:%02d",
                     current_time.dow, current_time.day, current_time.month, current_time.year,
                     current_time.hour, current_time.min, current_time.sec);
        }
        else
        {
            ESP_LOGE(TAG, "Помилка читання годинника");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}