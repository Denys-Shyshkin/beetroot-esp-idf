#pragma once

#include "driver/i2c_master.h"

#define CLOCK_ADDRESS 0x68

extern i2c_master_dev_handle_t clock_handle;

typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t dow;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} clock_time_t;

esp_err_t clock_get_time(clock_time_t *time);