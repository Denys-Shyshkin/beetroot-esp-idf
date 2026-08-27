#include "driver/spi_master.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#define PIN_SCLK 15
#define PIN_MOSI 16
#define PIN_MISO 17
#define PIN_CS 10

static const char *TAG = "bme_id";
static spi_device_handle_t s_bme;

static void bme_spi_init(void) {
    spi_bus_config_t bus = {
        .sclk_io_num = PIN_SCLK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 4 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &s_bme));
}

static uint8_t read_id(void) {
    uint8_t tx[2] = {0xD0 | 0x80, 0x00};
    uint8_t rx[2] = {0};

    spi_transaction_t t = {.length = 16, .tx_buffer = tx, .rx_buffer = rx};
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_bme, &t));
    return rx[1];
}

void app_main(void) {
    bme_spi_init();
    while (1) {
        ESP_LOGI(TAG, "0x%02X (очікуємо 0x60)", read_id());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}