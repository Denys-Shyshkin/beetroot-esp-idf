#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#define PIN_SDA 10
#define PIN_SCL 9

static const char *TAG = "scan";

void app_main(void)
{
    esp_log_level_set("i2c.master", ESP_LOG_NONE);

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port                     = I2C_NUM_0,
        .sda_io_num                   = PIN_SDA,
        .scl_io_num                   = PIN_SCL,
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    while (1) {
        ESP_LOGI(TAG, "--- scan 0x08..0x77 ---");
        int found = 0;
        for (uint8_t a = 0x08; a <= 0x77; a++) {
            if (i2c_master_probe(bus, a, 50) == ESP_OK) {
                ESP_LOGI(TAG, "  ACK -> 0x%02X (8-bit: W=0x%02X R=0x%02X)", a, a << 1, (a << 1) | 1);
                found++;
            }
        }
        ESP_LOGI(TAG, "Total found: %d", found);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}