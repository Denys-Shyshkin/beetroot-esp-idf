#include "driver/i2c_master.h"
#include "ds1307.h"
#include "esp_timer.h"
#include "font5x7.h"
#include "ssd1306.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>

#define PIN_SDA 10
#define PIN_SCL 9

#define OLED_ADDR 0x3C
#define RTC_ADDR 0x68

#define TIME_UPDATE_DELAY_US 1 * 1000 * 1000
#define SUPERLOOP_DELAY 100

static const char *TAG = "MAIN";

static i2c_master_bus_handle_t bus;

ds1307_time_t new_time = {
    .sec = 0,
    .min = 0,
    .hour = 22,
    .dow = 4,
    .day = 27,
    .month = 8,
    .year = 2026,
};

ds1307_time_t current_time;

static uint32_t last_time_update = 0;

static void i2c_bus_init() {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_SDA,
        .scl_io_num = PIN_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));
}

static const char *day_to_str(int day_num) {
    static const char *days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

    return days[day_num - 1];
}

static void show_date_time() {
    uint32_t now = esp_timer_get_time();

    if (now - last_time_update >= TIME_UPDATE_DELAY_US) {
        last_time_update = now;
        esp_err_t get_time_error = ds1307_get_time(&current_time);
        if (get_time_error != ESP_OK) {
            ESP_LOGI(TAG, "Не вдалося отримати час %s", esp_err_to_name(get_time_error));
            return;
        }

        char time_buff[20];
        snprintf(time_buff, sizeof(time_buff), "%02d:%02d:%02d", current_time.hour, current_time.min, current_time.sec);
        ESP_LOGI(TAG, "Поточний час: %s", time_buff);

        char date_buff[20];
        snprintf(date_buff, sizeof(date_buff), "%s %02d.%02d.%02d", day_to_str(current_time.dow), current_time.day, current_time.month, current_time.year);
        ESP_LOGI(TAG, "Поточна дата: %s", date_buff);

        fb_clear();
        fb_text(44, 20, time_buff);
        fb_text(29, 30, date_buff);
        oled_flush();
    }
}

void app_main(void) {
    i2c_bus_init();

    oled_init(OLED_ADDR, &bus);
    clock_init(RTC_ADDR, &bus);

    ds1307_set_time(&new_time);

    while (1) {
        show_date_time();

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY));
    }
}