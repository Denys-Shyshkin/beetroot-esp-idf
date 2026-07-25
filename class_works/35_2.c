#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define ENC_CLK GPIO_NUM_9
#define ENC_DT  GPIO_NUM_10


#define DEBOUNCE_US 200

static volatile int     raw_edges     = 0;   
static volatile int     debounced     = 0;   
static volatile int64_t last_edge_us  = 0;

static void IRAM_ATTR clk_isr(void *arg)
{
    raw_edges++;

    int64_t now = esp_timer_get_time();
    if (now - last_edge_us > DEBOUNCE_US) {
        debounced++;
        last_edge_us = now;
    }
}


void app_main(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << ENC_CLK) | (1ULL << ENC_DT),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(ENC_CLK, clk_isr, NULL);


    while (1) {
        int r = raw_edges;
        int d = debounced;

        printf("RAW: %4d  |  Debounce: %4d  |  Diff: %4d \n",
               r, d, r - d);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}