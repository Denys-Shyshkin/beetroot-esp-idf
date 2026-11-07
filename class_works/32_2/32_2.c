#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define LED_GPIO_1 GPIO_NUM_4
#define LED_GPIO_2 GPIO_NUM_5

#define FREQ 25000
#define DUTY_MAX 1023

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_RESOLUTION LEDC_TIMER_10_BIT

#define LEDC_CHANNEL_1 LEDC_CHANNEL_0
#define LEDC_CHANNEL_2 LEDC_CHANNEL_1

static const char *TAG = "ESP_LOG";

void app_main(void) {
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_RESOLUTION,
        .timer_num = LEDC_TIMER,
        .freq_hz = FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config_1 = {
        .gpio_num = LED_GPIO_1,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&channel_config_1);

    ledc_channel_config_t channel_config_2 = {
        .gpio_num = LED_GPIO_2,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_2,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&channel_config_2);

    while (1) {
        for (int d = 0; d <= DUTY_MAX; d += 10) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, d);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

            vTaskDelay(pdMS_TO_TICKS(100));
        }

        
        for (int d = DUTY_MAX; d >= 0; d -= 10) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, d);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
            
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        // ===============

        for (int d = 0; d <= DUTY_MAX; d += 50) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, d);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);

            vTaskDelay(pdMS_TO_TICKS(100));
        }

        
        for (int d = DUTY_MAX; d >= 0; d -= 50) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, d);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
            
            vTaskDelay(pdMS_TO_TICKS(100));
        }



        // printf("State: %d \n", button_read);
        // ESP_LOGI(TAG, "Button State: %d \n", button_read);
    }
}
