#include "spi.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_attr.h"
#include <string.h>

#define SPI_HOST SPI2_HOST
#define SPI_DMA_CH SPI_DMA_CH_AUTO
#define SPI_MODE 0
#define SPI_HZ 1000000
#define SPI_SCLK_GPIO GPIO_NUM_6
#define SPI_MOSI_GPIO GPIO_NUM_7
#define SPI_MISO_GPIO GPIO_NUM_15
#define SPI_DMA_BUF_SIZE 12

static const char *TAG = "SPI";

DMA_ATTR static uint8_t dma_tx_buf[SPI_DMA_BUF_SIZE];
DMA_ATTR static uint8_t dma_rx_buf[SPI_DMA_BUF_SIZE];

static bool dma_busy = false;
static size_t dma_len = 0;

static spi_transaction_t dma_transaction;

void spi_bus_init(void) {
    spi_bus_config_t bus_config = {
        .sclk_io_num = SPI_SCLK_GPIO,
        .mosi_io_num = SPI_MOSI_GPIO,
        .miso_io_num = SPI_MISO_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &bus_config, SPI_DMA_CH));

    ESP_LOGI(TAG, "Bus init successful - SCLK GPIO: %d | MOSI GPIO: %d | MISO GPIO: %d", SPI_SCLK_GPIO, SPI_MOSI_GPIO, SPI_MISO_GPIO);
}

void spi_add_device(gpio_num_t device_spi_cs_gpio, spi_device_handle_t *device_handle) {
    spi_device_interface_config_t device_config = {
        .clock_speed_hz = SPI_HZ,
        .mode = SPI_MODE,
        .spics_io_num = device_spi_cs_gpio,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST, &device_config, device_handle));

    ESP_LOGI(TAG, "Device config successful - CLOCK SPEED: %d | MODE: %d | CS_GPIO: %d", SPI_HZ, SPI_MODE, device_spi_cs_gpio);
}

esp_err_t spi_read_regs(spi_device_handle_t device_handle, uint8_t start_reg, uint8_t *data, size_t len) {
    uint8_t tx[len + 1];
    uint8_t rx[len + 1];

    memset(tx, 0, sizeof(tx));
    memset(rx, 0, sizeof(rx));

    // Set bit 7 to indicate READ operation
    tx[0] = start_reg | 0x80;

    spi_transaction_t transaction = {
        .length = (len + 1) * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };

    esp_err_t error = spi_device_transmit(device_handle, &transaction);
    ESP_RETURN_ON_ERROR(error, TAG, "Read error: %s", esp_err_to_name(error));

    // Skip the first received byte (address phase)
    memcpy(data, &rx[1], len);

    return ESP_OK;
}

esp_err_t spi_write_reg(spi_device_handle_t device_handle, uint8_t reg, uint8_t value) {
    // Clear bit 7 to indicate WRITE operation
    uint8_t tx[2] = {reg & 0x7F, value};

    spi_transaction_t transaction = {
        .length = 16,
        .tx_buffer = tx,
    };

    esp_err_t error = spi_device_transmit(device_handle, &transaction);
    ESP_RETURN_ON_ERROR(error, TAG, "Write error: %s", esp_err_to_name(error));

    return ESP_OK;
}

void spi_check_device_id(spi_device_handle_t device_handle, uint8_t device_id_reg, uint8_t expected_device_id) {
    uint8_t device_id = 0;

    ESP_ERROR_CHECK(spi_read_regs(device_handle, device_id_reg, &device_id, 1));

    if (device_id != expected_device_id) {
        ESP_LOGE(TAG, "Device ID check failed - EXPECTED: 0x%02X | RECEIVED: 0x%02X", expected_device_id, device_id);
        abort();
    }

    ESP_LOGI(TAG, "Device ID check successful: 0x%02X", device_id);
}

esp_err_t spi_start_read_dma(spi_device_handle_t device_handle, uint8_t start_reg, size_t len) {
    // Only one transfer at a time
    ESP_RETURN_ON_FALSE(!dma_busy, ESP_ERR_INVALID_STATE, TAG, "DMA read already in flight");
    ESP_RETURN_ON_FALSE(len + 1 <= SPI_DMA_BUF_SIZE, ESP_ERR_INVALID_SIZE, TAG, "DMA read too long: %u bytes", (unsigned)len);

    // Prepare buffers
    memset(dma_tx_buf, 0, sizeof(dma_tx_buf));
    memset(dma_rx_buf, 0, sizeof(dma_rx_buf));
    dma_tx_buf[0] = start_reg | 0x80;

    // Describe the transfer
    memset(&dma_transaction, 0, sizeof(dma_transaction));
    dma_transaction.length = (len + 1) * 8;
    dma_transaction.tx_buffer = dma_tx_buf;
    dma_transaction.rx_buffer = dma_rx_buf;

    esp_err_t error = spi_device_queue_trans(device_handle, &dma_transaction, 0);
    ESP_RETURN_ON_ERROR(error, TAG, "DMA start error: %s", esp_err_to_name(error));

    dma_len = len;
    dma_busy = true;

    return ESP_OK;
}

esp_err_t spi_finish_read_dma(spi_device_handle_t device_handle, uint8_t *data, TickType_t timeout) {
    ESP_RETURN_ON_FALSE(dma_busy, ESP_ERR_INVALID_STATE, TAG, "No DMA read in flight");

    // Check the driver if the transfer is done
    spi_transaction_t *done_transaction = NULL;
    esp_err_t error = spi_device_get_trans_result(device_handle, &done_transaction, timeout);
    if (error == ESP_ERR_TIMEOUT) {
        return ESP_ERR_TIMEOUT;
    }

    if (error != ESP_OK) {
        dma_busy = false;
        ESP_LOGE(TAG, "DMA finish error: %s", esp_err_to_name(error));
        return error;
    }

    // Skip the first received byte and copy the data out
    memcpy(data, &dma_rx_buf[1], dma_len);
    dma_busy = false;

    return ESP_OK;
}