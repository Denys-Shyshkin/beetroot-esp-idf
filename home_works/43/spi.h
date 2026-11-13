#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"

void spi_bus_init(void);
void spi_add_device(gpio_num_t device_spi_cs_gpio, spi_device_handle_t *device_handle);
void spi_check_device_id(spi_device_handle_t device_handle, uint8_t device_id_reg, uint8_t expected_device_id);
esp_err_t spi_read_regs(spi_device_handle_t device_handle, uint8_t start_reg, uint8_t *data, size_t len);
esp_err_t spi_write_reg(spi_device_handle_t device_handle, uint8_t reg, uint8_t value);