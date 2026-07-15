#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

const gpio_num_t POT_CHANNEL = ADC_CHANNEL_3;
#define WINDOW 16

int buffer[WINDOW] = {0};
int running_sum = 0;
int idx = 0;
int count = 0;

int moving_average(int raw) {
    running_sum -= buffer[idx];
    buffer[idx] = raw;
    running_sum += raw;
    idx = (idx+1) % WINDOW;

    if (count < WINDOW) {
        count++;
    }

    return (int)(running_sum / count);
}

void app_main(void) {
    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1
    };

    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    
    adc_oneshot_config_channel(adc_handle, POT_CHANNEL, &chan_cfg);

    while (1) {
        int raw = 0;
        adc_oneshot_read(adc_handle, POT_CHANNEL, &raw);
        int filtered = moving_average(raw);

        printf("raw: %d | filtered: %d \n", raw, filtered);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
