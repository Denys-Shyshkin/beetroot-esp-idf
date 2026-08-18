#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#define PIN_SDA 10
#define PIN_SCL 9
static const char *TAG = "reg0";

void app_main(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0, .sda_io_num = PIN_SDA, .scl_io_num = PIN_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = 0x68,
        .scl_speed_hz    = 100000,
    };
    i2c_master_dev_handle_t rtc;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &rtc));

    while (1) {
        uint8_t reg = 0x00, raw = 0;
        esp_err_t err = i2c_master_transmit_receive(rtc, &reg, 1, &raw, 1, 1000);
        ESP_LOGI(TAG, "err=%s  reg0=0x%02X  як dec: %u",
                 esp_err_to_name(err), raw, raw);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}