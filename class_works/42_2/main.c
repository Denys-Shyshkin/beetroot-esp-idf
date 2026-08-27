#include "bme280.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "lab4";

void app_main(void)
{
    ESP_ERROR_CHECK(bme_spi_init());
    if (bme_chip_id() != BME_CHIP_ID) return;

    bme_read_calib();
    bme_configure();

    while (1) {
        bme_start_measure();
        esp_rom_delay_us(10000);
        while (bme_busy()) esp_rom_delay_us(500);

        bme_data_t ok = {0}, bad = {0};
        bme_read_all(&ok);

        ESP_LOGI(TAG, "burst: %.2f C  %.1f hPa  %.1f %%",
                 ok.temp_c, ok.press_hpa, ok.hum_pct);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}