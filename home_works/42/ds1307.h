#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdint.h>

typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t dow;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} ds1307_time_t;

void clock_init(uint8_t addr, i2c_master_bus_handle_t *bus_handle);
esp_err_t ds1307_set_time(const ds1307_time_t *time);
esp_err_t ds1307_get_time(ds1307_time_t *time);