#include "i2c.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_check.h"

#define I2C_PORT I2C_NUM_0
#define I2C_SDA_GPIO GPIO_NUM_4
#define I2C_SCL_GPIO GPIO_NUM_5
#define I2C_FREQ_HZ 100000
#define I2C_TIMEOUT_MS 200

static const char *TAG = "I2C";

static i2c_master_bus_handle_t i2c_bus;

void i2c_bus_init(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    ESP_LOGI(TAG, "Bus init successful - PORT: %d | SDA GPIO: %d | SCL GPIO: %d", I2C_PORT, I2C_SDA_GPIO, I2C_SCL_GPIO);
}

static void i2c_add_device(uint16_t device_address, i2c_master_dev_handle_t *device_handle) {
    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_address,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &device_config, device_handle));

    ESP_LOGI(TAG, "Device config successful - FREQ: %d", I2C_FREQ_HZ);
}

void i2c_device_config(uint16_t device_address, i2c_master_dev_handle_t *device_handle) {
    ESP_ERROR_CHECK(i2c_master_probe(i2c_bus, device_address, I2C_TIMEOUT_MS));

    ESP_LOGI(TAG, "Device probe successful - ADDRESS: 0x%02X", device_address);

    i2c_add_device(device_address, device_handle);
}

esp_err_t i2c_transmit_data(i2c_master_dev_handle_t device_handle, const uint8_t *write_buffer, size_t write_size) {
    esp_err_t error = i2c_master_transmit(device_handle, write_buffer, write_size, I2C_TIMEOUT_MS);
    ESP_RETURN_ON_ERROR(error, TAG, "Transmit error: %s", esp_err_to_name(error));

    return error;
}

esp_err_t i2c_transmit_receive_data(i2c_master_dev_handle_t device_handle, const uint8_t *write_buffer, size_t write_size, uint8_t *read_buffer, size_t read_size) {
    esp_err_t error = i2c_master_transmit_receive(device_handle, write_buffer, write_size, read_buffer, read_size, I2C_TIMEOUT_MS);
    ESP_RETURN_ON_ERROR(error, TAG, "Transmit/Receive error: %s", esp_err_to_name(error));

    return error;
}