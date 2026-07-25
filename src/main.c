#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define ENC_CLK GPIO_NUM_9
#define ENC_DT GPIO_NUM_10
#define ENC_SW GPIO_NUM_11

static void delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void app_main(void) {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << ENC_CLK) | (1ULL << ENC_DT) | (1ULL << ENC_SW),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    while (1) {
        printf("CLK=%d  DT=%d  SW=%d\n", gpio_get_level(ENC_CLK), gpio_get_level(ENC_DT), gpio_get_level(ENC_SW));

        delay_ms(200);
    }
}