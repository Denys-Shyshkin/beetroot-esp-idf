#pragma once

#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"

#define SENSOR_SPI_CS_GPIO GPIO_NUM_16
#define SENSOR_ID_REG 0xD0
#define SENSOR_ID 0x60

typedef struct {
    int16_t temp_c;
    uint16_t pressure_hpa;
    uint8_t humidity_pct;
} sensor_measurements_t;

typedef enum {
    SENSOR_IDLE,
    SENSOR_WAITING,
} sensor_state_t;

extern spi_device_handle_t sensor_handle;
extern sensor_state_t sensor_state;
extern int64_t last_sensor_start_us;

void sensor_setup(void);
void sensor_init_read(sensor_measurements_t *data);
esp_err_t sensor_read_measurements(sensor_measurements_t *data);
esp_err_t sensor_start_measurement(void);
esp_err_t sensor_finish_measurement(sensor_measurements_t *data, TickType_t timeout);