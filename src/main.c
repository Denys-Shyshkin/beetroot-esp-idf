#include "clock.h"
#include "display.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "font.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c.h"

#define UPDATE_DISPLAY_TIME_INTERVAL_US 1000000
#define SUPERLOOP_DELAY_MS 10

static const char *TAG = "MAIN";

static int64_t last_clock_read_us = 0;

static const char *days[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};

static void update_display_clock(void) {
    clock_time_t current_time;

    esp_err_t get_time_error = clock_get_time(&current_time);
    if (get_time_error == ESP_OK) {
        char time_buff[20];
        snprintf(time_buff, sizeof(time_buff), "%02d:%02d:%02d", current_time.hour, current_time.min, current_time.sec);
        
        char date_buff[20];
        snprintf(date_buff, sizeof(date_buff), "%s %02d.%02d.%04d", days[current_time.dow - 1], current_time.day, current_time.month, current_time.year);
    
        esp_err_t time_error = display_show_text(2, 40, time_buff);
        if (time_error != ESP_OK) {
            ESP_LOGE(TAG, "Update display clock time failed");
        }

        esp_err_t date_error = display_show_text(4, 22, date_buff);
        if (date_error != ESP_OK) {
            ESP_LOGE(TAG, "Update display clock date failed");
        }
    } else {
        ESP_LOGE(TAG, "Update display clock failed");
    }

    last_clock_read_us = esp_timer_get_time();
}

void app_main(void) {
    i2c_bus_init();

    i2c_device_config(DISPLAY_ADDRESS, &display_handle);

    display_setup();

    i2c_device_config(CLOCK_ADDRESS, &clock_handle);

    update_display_clock();

    while (1) {
        if (esp_timer_get_time() - last_clock_read_us >= UPDATE_DISPLAY_TIME_INTERVAL_US) {
            update_display_clock();
        }

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY_MS));
    }
}