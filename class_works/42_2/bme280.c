#include "bme280.h"

#include <string.h>
#include "driver/spi_master.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

static const char *TAG = "bme280";
static spi_device_handle_t s_bme;

static uint16_t dig_T1, dig_P1;
static int16_t  dig_T2, dig_T3;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t  dig_H1, dig_H3;
static int16_t  dig_H2, dig_H4, dig_H5;
static int8_t   dig_H6;

static int32_t  t_fine;

#define U16LE(p)  ((uint16_t)((uint16_t)(p)[1] << 8 | (p)[0]))
#define S16LE(p)  ((int16_t) ((uint16_t)(p)[1] << 8 | (p)[0]))

esp_err_t bme_spi_init(void)
{
    spi_bus_config_t bus = {
        .sclk_io_num     = BME_PIN_SCLK,
        .mosi_io_num     = BME_PIN_MOSI,
        .miso_io_num     = BME_PIN_MISO,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 64,
    };
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_DISABLED);
    if (err != ESP_OK) return err;

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 4 * 1000 * 1000,
        .mode           = 0,
        .spics_io_num   = BME_PIN_CS,
        .queue_size     = 1,
    };
    err = spi_bus_add_device(SPI2_HOST, &dev, &s_bme);
    if (err != ESP_OK) return err;
    
    return ESP_OK;
}


esp_err_t bme_read(uint8_t reg, uint8_t *dst, size_t len)
{
    uint8_t tx[33] = {0};
    uint8_t rx[33] = {0};

    if (len == 0 || len > 32) return ESP_ERR_INVALID_SIZE;

    tx[0] = reg | 0x80;

    spi_transaction_t t = {
        .length    = (len + 1) * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    esp_err_t err = spi_device_polling_transmit(s_bme, &t);
    if (err == ESP_OK) {
        memcpy(dst, &rx[1], len);
    }
    return err;
}

esp_err_t bme_write(uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { (uint8_t)(reg & 0x7F), val };
    spi_transaction_t t = { .length = 16, .tx_buffer = tx };
    return spi_device_polling_transmit(s_bme, &t);
}


uint8_t bme_chip_id(void)
{
    uint8_t id = 0;
    bme_read(BME_REG_ID, &id, 1);
    return id;
}

bool bme_busy(void)
{
    uint8_t st = 0;
    bme_read(BME_REG_STATUS, &st, 1);
    return (st & 0x08) != 0; 
}

void bme_start_measure(void)
{
    bme_write(BME_REG_CTRL_MEAS, BME_FORCED_X1);
}

void bme_configure(void)
{
    bme_write(BME_REG_RESET, 0xB6);
    esp_rom_delay_us(5000);

    bme_write(BME_REG_CTRL_HUM,  0x01);
    bme_write(BME_REG_CONFIG,    0x00);
    bme_write(BME_REG_CTRL_MEAS, BME_FORCED_X1);
}



void bme_read_calib(void)
{
    uint8_t c[26], h[7];

    bme_read(0x88, c, 26);
    bme_read(0xE1, h, 7);

    dig_T1 = U16LE(&c[0]);
    dig_T2 = S16LE(&c[2]);
    dig_T3 = S16LE(&c[4]);

    dig_P1 = U16LE(&c[6]);
    dig_P2 = S16LE(&c[8]);
    dig_P3 = S16LE(&c[10]);
    dig_P4 = S16LE(&c[12]);
    dig_P5 = S16LE(&c[14]);
    dig_P6 = S16LE(&c[16]);
    dig_P7 = S16LE(&c[18]);
    dig_P8 = S16LE(&c[20]);
    dig_P9 = S16LE(&c[22]);
    dig_H1 = c[25];

    dig_H2 = S16LE(&h[0]);
    dig_H3 = h[2];
    dig_H4 = (int16_t)(((int8_t)h[3] << 4) | (h[4] & 0x0F));
    dig_H5 = (int16_t)(((int8_t)h[5] << 4) | (h[4] >> 4));
    dig_H6 = (int8_t)h[6];
}

int32_t bme_compensate_T(int32_t adc_T)
{
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * (int32_t)dig_T2) >> 11;

    int32_t d  = (adc_T >> 4) - (int32_t)dig_T1;
    int32_t v2 = (((d * d) >> 12) * (int32_t)dig_T3) >> 14;

    t_fine = v1 + v2;
    return (t_fine * 5 + 128) >> 8;
}

static uint32_t compensate_P(int32_t adc_P)
{
    int64_t v1 = (int64_t)t_fine - 128000;
    int64_t v2 = v1 * v1 * (int64_t)dig_P6;

    v2 += (v1 * (int64_t)dig_P5) << 17;
    v2 += ((int64_t)dig_P4) << 35;

    v1 = ((v1 * v1 * (int64_t)dig_P3) >> 8) + ((v1 * (int64_t)dig_P2) << 12);
    v1 = ((((int64_t)1 << 47) + v1) * (int64_t)dig_P1) >> 33;

    if (v1 == 0) return 0;

    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - v2) * 3125) / v1;

    v1 = ((int64_t)dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    v2 = ((int64_t)dig_P8 * p) >> 19;

    return (uint32_t)(((p + v1 + v2) >> 8) + ((int64_t)dig_P7 << 4));
}

static uint32_t compensate_H(int32_t adc_H)
{
    int32_t v = t_fine - 76800;

    v = (((((adc_H << 14) - ((int32_t)dig_H4 << 20)
            - ((int32_t)dig_H5 * v)) + 16384) >> 15)
         * (((((((v * (int32_t)dig_H6) >> 10)
                * (((v * (int32_t)dig_H3) >> 11) + 32768)) >> 10)
              + 2097152) * (int32_t)dig_H2 + 8192) >> 14));

    v -= (((((v >> 15) * (v >> 15)) >> 7) * (int32_t)dig_H1) >> 4);

    if (v < 0) v = 0;
    if (v > 419430400) v = 419430400;

    return (uint32_t)(v >> 12);
}

void bme_read_all(bme_data_t *out)
{
    uint8_t d[8];
    bme_read(BME_REG_DATA, d, 8);

    int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    int32_t adc_H = ((int32_t)d[6] <<  8) |  (int32_t)d[7];

    out->temp_c    = bme_compensate_T(adc_T) / 100.0f;
    out->press_hpa = compensate_P(adc_P)     / 25600.0f;
    out->hum_pct   = compensate_H(adc_H)     / 1024.0f;
}
