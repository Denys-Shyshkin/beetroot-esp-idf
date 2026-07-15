/**
 *  Урок 32
 *  Модуль 3.3.
 *  Домашнє завдання
 *  Широтно Імпульсна Модуляція (PWM)
 */
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
#define LEDC_RESOLUTION LEDC_TIMER_10_BIT
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_FREQUENCY 1000
#define DUTY_CYCLE_MAX 1023
#define DUTY_CYCLE_STEP 10
#define DUTY_CYCLE_INIT 0
#define LED_GPIO 5

#define POT ADC_CHANNEL_3 // GPIO 4 is ADC1_3

// ESP LOG name tag
const char *TAG = "ADC_PWM";

void app_main(void) {
    // ADC setup functions
    // ADC OneShot Unit init
    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t init_cfg = {.unit_id = ADC_UNIT_1};

    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    // ADC Channel config
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, POT, &chan_cfg));

    // PWM setup functions
    // Timer setup - diff timer is used for diff frequency
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_SPEED_MODE,      // Low speed timer is enough for most use cases
        .duty_resolution = LEDC_RESOLUTION, // Resolution 10 bit (0 - 1023)
        .timer_num = LEDC_TIMER,            // Timer number (for low speed 0-7)
        .freq_hz = LEDC_FREQUENCY,          // For led 1-5 kHz, for motors 10-20 kHz
        .clk_cfg = LEDC_AUTO_CLK            // Source clock config, in auto source clock will be automatically selected based on the giving resolution and duty parameter
    };
    ledc_timer_config(&timer_config);

    // Channel setup - diff channel is used for GPIO and duty cycle
    ledc_channel_config_t channel_config = {
        .gpio_num = LED_GPIO,          // GPIO number
        .speed_mode = LEDC_SPEED_MODE, // Low speed timer, same as for timer
        .channel = LEDC_CHANNEL,       // Channel number
        .timer_sel = LEDC_TIMER,       // Timer number, same as for timer
        .duty = DUTY_CYCLE_INIT,       // Duty cycle initial value
        .hpoint = 0                    // Comparison point, 0 in most use cases
    };
    ledc_channel_config(&channel_config);

    while (1) {
        int pot_reading = 0;
        adc_oneshot_read(adc_handle, POT, &pot_reading);

        // Dividing reading by 4 to convert 12 bit value to 10 bit resolution value
        int brightness = pot_reading >> 2;

        ledc_set_duty(LEDC_SPEED_MODE, LEDC_CHANNEL, brightness);
        ledc_update_duty(LEDC_SPEED_MODE, LEDC_CHANNEL);

        ESP_LOGI(TAG, "Potentiometer reading: %d \n", brightness);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
