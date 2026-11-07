#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define LED_GPIO GPIO_NUM_4
#define BUTTON_GPIO GPIO_NUM_5

static const char *TAG = "ESP_LOG";

void app_main(void) {
    // Simple gpio set
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    // Setting gpio via config
    gpio_config_t io_config = {.pin_bit_mask = 1ULL << BUTTON_GPIO,
                               .mode = GPIO_MODE_INPUT,
                               .pull_up_en = GPIO_PULLUP_ENABLE,
                               .pull_down_en = GPIO_PULLDOWN_DISABLE,
                               .intr_type = GPIO_INTR_DISABLE};

    gpio_config(&io_config);

    while (1) {
        bool button_read = gpio_get_level(BUTTON_GPIO);

        if (button_read == 0) {
            gpio_set_level(LED_GPIO, 1);
        } else {
            gpio_set_level(LED_GPIO, 0);
        }

        // printf("State: %d \n", button_read);
        ESP_LOGI(TAG, "Button State: %d \n", button_read);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
