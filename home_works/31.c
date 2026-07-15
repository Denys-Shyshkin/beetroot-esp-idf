#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define LDR ADC_CHANNEL_3 // GPIO 4 is ADC1_3
#define LED_GPIO GPIO_NUM_5
#define WINDOW 16
#define LED_ON_LEVEL 1000
#define LED_OFF_LEVEL 1500

// ESP LOG name tag
const char *TAG = "ADC_SMA";

// Use teleplot graphs otherwise output to monitor
const bool plot_vars = false;

int buffer[WINDOW] = {0};
int running_sum = 0;
int idx = 0;
int count = 0;

int moving_average(int raw);
void log_values(int raw, int filtered);

void app_main(void) {
    // LED init
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    // ADC OneShot Unit init
    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t init_cfg = {.unit_id = ADC_UNIT_1};

    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    // ADC Channel config
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    adc_oneshot_config_channel(adc_handle, LDR, &chan_cfg);

    while (1) {
        int raw = 0;
        adc_oneshot_read(adc_handle, LDR, &raw);

        int filtered = moving_average(raw);

        if (filtered <= LED_ON_LEVEL) {
            gpio_set_level(LED_GPIO, 1);
        } else if (filtered >= LED_OFF_LEVEL) {
            gpio_set_level(LED_GPIO, 0);
        }

        log_values(raw, filtered);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

int moving_average(int raw) {
    // Subtract the oldest reading from the running sum
    running_sum -= buffer[idx];

    // Put the newest reading to the buffer
    buffer[idx] = raw;

    // Add the newest reading to the running sum
    running_sum += raw;

    // Update current index
    idx = (idx + 1) % WINDOW;

    // Increase the buffer count as it grows up to max value
    if (count < WINDOW) {
        count++;
    }

    return (int)(running_sum / count);
}

void log_values(int raw, int filtered) {
    if (plot_vars) {
        printf(">raw: %d \n", raw);
        printf(">filtered: %d \n", filtered);
    } else {
        ESP_LOGI(TAG, "RAW: %d \n", raw);
    }
}
