#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

void i2c_bus_init(void);
void i2c_device_config(uint16_t device_address, i2c_master_dev_handle_t *device_handle);
esp_err_t i2c_transmit_data(i2c_master_dev_handle_t device_handle, const uint8_t *write_buffer, size_t write_size);
esp_err_t i2c_transmit_receive_data(i2c_master_dev_handle_t device_handle, const uint8_t *write_buffer, size_t write_size, uint8_t *read_buffer, size_t read_size);
