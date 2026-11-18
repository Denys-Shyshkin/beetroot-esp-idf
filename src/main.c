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
#include "uart.h"
#include <string.h>

#define UPDATE_DISPLAY_TIME_INTERVAL_US 1000000
#define READ_SENSOR_INTERVAL_US 1000000
#define SUPERLOOP_DELAY_MS 10

static const char *TAG = "MAIN";

static int64_t last_clock_read_us = 0;

static const char *days[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};

static char current_cmd[UART_BUFFER_SIZE] = "TEMP";
static char read_cmd[UART_BUFFER_SIZE];

static sensor_measurements_t latest;

static void update_display_clock(void) {
    clock_time_t current_time;

    esp_err_t get_time_error = clock_get_time(&current_time);
    if (get_time_error == ESP_OK) {
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
    char data_buff[10];
    if (strcmp(current_cmd, uart_valid_cmds[CMD_PRES]) == 0) {
        snprintf(data_buff, sizeof(data_buff), "%4dHPA", latest.pressure_hpa);
    } else if (strcmp(current_cmd, uart_valid_cmds[CMD_TEMP]) == 0) {
        snprintf(data_buff, sizeof(data_buff), "%6dC", latest.temp_c);
    } else if (strcmp(current_cmd, uart_valid_cmds[CMD_HUM]) == 0) {
        snprintf(data_buff, sizeof(data_buff), "%6d%%", latest.humidity_pct);
    }

    esp_err_t show_error = display_show_text(5, 40, data_buff);
    if (show_error != ESP_OK) {
        ESP_LOGE(TAG, "Update display sensor data failed");
    }
}

static void poll_sensor_data(void) {
    if (sensor_state == SENSOR_IDLE) {
        if (esp_timer_get_time() - last_sensor_start_us >= READ_SENSOR_INTERVAL_US) {
            if (sensor_start_measurement() == ESP_OK) {
                sensor_state = SENSOR_WAITING;
            }
            last_sensor_start_us = esp_timer_get_time();
        }
    } else {
        esp_err_t error = sensor_finish_measurement(&latest, 0);
        if (error == ESP_OK) {
            sensor_state = SENSOR_IDLE;
            update_display_sensor_data();
        } else if (error != ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "Sensor measurement failed");
            sensor_state = SENSOR_IDLE;
        }
    }
}

static bool update_current_cmd(void) {
    if (read_cmd[0] != '\0') {
        strncpy(current_cmd, read_cmd, sizeof(read_cmd) - 1);
        read_cmd[0] = '\0';

        return true;  
    }

    return false;
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
    
    // UART init
    uart_init();
    
    // Init values read and start up update
    sensor_init_read(&latest);
    update_display_clock();
    update_display_sensor_data();

    uart_print_cmds();

     while (1) {
        uart_read_cmd(read_cmd, current_cmd);

        poll_sensor_data();

        if ((esp_timer_get_time() - last_clock_read_us >= UPDATE_DISPLAY_TIME_INTERVAL_US) || update_current_cmd()) {
            update_display_clock();
            update_display_sensor_data();

            last_clock_read_us = esp_timer_get_time();
        }

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY_MS));
    }
}