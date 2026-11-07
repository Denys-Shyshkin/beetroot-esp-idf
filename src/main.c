#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SUPERLOOP_DELAY_MS 10

// static const char *TAG = "MAIN";

void app_main(void) {


    while (1) {

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY_MS));
    }
}