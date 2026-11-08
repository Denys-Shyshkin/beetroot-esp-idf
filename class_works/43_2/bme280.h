#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#define BME_PIN_SCLK   15
#define BME_PIN_MOSI   16
#define BME_PIN_MISO   17
#define BME_PIN_CS     10

/* ---- регістри BME280 ---- */
#define BME_REG_ID         0xD0
#define BME_REG_RESET      0xE0
#define BME_REG_CTRL_HUM   0xF2
#define BME_REG_STATUS     0xF3
#define BME_REG_CTRL_MEAS  0xF4
#define BME_REG_CONFIG     0xF5
#define BME_REG_DATA       0xF7

#define BME_CHIP_ID        0x60
#define BME_FORCED_X1      0x25   /* osrs_t=x1, osrs_p=x1, mode=forced */

typedef struct {
    float temp_c;
    float press_hpa;
    float hum_pct;
} bme_data_t;



esp_err_t bme_spi_init(void);
esp_err_t bme_read(uint8_t reg, uint8_t *dst, size_t len);
esp_err_t bme_write(uint8_t reg, uint8_t val);

uint8_t   bme_chip_id(void);
void      bme_configure(void);
bool      bme_busy(void);
void      bme_start_measure(void);
void    bme_read_calib(void);
int32_t bme_compensate_T(int32_t adc_T);

void bme_read_all(bme_data_t *out);      
void bme_read_all_wrong(bme_data_t *out);
