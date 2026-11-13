#include "clock.h"
#include "display.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "font.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c.h"
#include "sensor.h"
#include "spi.h"

#define UPDATE_DISPLAY_TIME_INTERVAL_US 1000000
#define SUPERLOOP_DELAY_MS 10

static const char *TAG = "MAIN";

static int64_t last_clock_read_us = 0;

static const char *days[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};

static void update_display_clock(void) {
    clock_time_t current_time;

    esp_err_t get_time_error = clock_get_time(&current_time);
    if (get_time_error == ESP_OK) {
        ESP_LOGI(TAG, "Time and date: %02d:%02d:%02d | %s %02d.%02d.%04d", current_time.hour, current_time.min, current_time.sec, days[current_time.dow - 1], current_time.day, current_time.month,
                 current_time.year);

        char time_buff[20];
        snprintf(time_buff, sizeof(time_buff), "%02d:%02d:%02d", current_time.hour, current_time.min, current_time.sec);

        char date_buff[20];
        snprintf(date_buff, sizeof(date_buff), "%s %02d.%02d.%04d", days[current_time.dow - 1], current_time.day, current_time.month, current_time.year);

        esp_err_t time_error = display_show_text(1, 40, time_buff);
        if (time_error != ESP_OK) {
            ESP_LOGE(TAG, "Update display clock time failed");
        }

        esp_err_t date_error = display_show_text(3, 22, date_buff);
        if (date_error != ESP_OK) {
            ESP_LOGE(TAG, "Update display clock date failed");
        }
    } else {
        ESP_LOGE(TAG, "Update display clock failed");
    }
}

static void update_display_sensor_data(void) {
    sensor_measurements_t sensor_measurements;

    esp_err_t data_error = sensor_read_measurements(&sensor_measurements);
    if (data_error == ESP_OK) {
        ESP_LOGI(TAG, "Measured data: P=%d T=%d H=%d", sensor_measurements.pressure_hpa, sensor_measurements.temp_c, sensor_measurements.humidity_pct);

        char data_buff[30];
        snprintf(data_buff, sizeof(data_buff), "%dHPA  %dC  %d%%", sensor_measurements.pressure_hpa, sensor_measurements.temp_c, sensor_measurements.humidity_pct);

        esp_err_t show_error = display_show_text(5, 16, data_buff);
        if (show_error != ESP_OK) {
            ESP_LOGE(TAG, "Update display sensor data failed");
        }
    } else {
        ESP_LOGE(TAG, "Update display sensor data failed");
    }
}

void app_main(void) {
    // I2C bus init and devices add, config & check
    i2c_bus_init();
    i2c_device_config(DISPLAY_ADDRESS, &display_handle);
    i2c_device_config(CLOCK_ADDRESS, &clock_handle);

    // I2C device setup
    display_setup();

    // SPI bus init and devices add, config & check
    spi_bus_init();
    spi_add_device(SENSOR_SPI_CS_GPIO, &sensor_handle);
    spi_check_device_id(sensor_handle, SENSOR_ID_REG, SENSOR_ID);

    // SPI device setup
    sensor_setup();

    // Init values read / update
    update_display_clock();
    update_display_sensor_data();

    while (1) {
        if (esp_timer_get_time() - last_clock_read_us >= UPDATE_DISPLAY_TIME_INTERVAL_US) {
            update_display_clock();
            update_display_sensor_data();

            last_clock_read_us = esp_timer_get_time();
        }

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY_MS));
    }
}