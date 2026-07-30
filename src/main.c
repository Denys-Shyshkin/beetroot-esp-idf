#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SUPERLOOP_DELAY 100

static const char *TAG = "TAG";

void app_main(void) {


    while (1) {


        ESP_LOGI(TAG, "");

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY));
    }
}