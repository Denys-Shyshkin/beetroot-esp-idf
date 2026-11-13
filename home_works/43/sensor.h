#pragma once

#include "driver/spi_master.h"

#define SENSOR_SPI_CS_GPIO GPIO_NUM_16
#define SENSOR_ID_REG 0xD0
#define SENSOR_ID 0x60

extern spi_device_handle_t sensor_handle;

typedef struct {
    int16_t temp_c;
    uint16_t pressure_hpa;
    uint8_t humidity_pct;
} sensor_measurements_t;

void sensor_setup(void);
esp_err_t sensor_read_measurements(sensor_measurements_t *data);