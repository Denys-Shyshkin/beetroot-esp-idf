#include "helpers.h"
#include "ldr.h"
#include "servo.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define ADC_CHANNEL ADC_CHANNEL_8 // Pin 9
#define ADC_UNIT ADC_UNIT_1

#define SERVO_PIN 14

#define SUPERLOOP_DELAY 100

static const char *TAG = "Mini Project";

void app_main(void) {
    ldr_init(ADC_UNIT, ADC_CHANNEL);
    servo_init(SERVO_PIN);

    int raw;
    int filtered_raw;
    int mv;
    int val_deg;
    int duty;

    while (1) {
        ldr_read_raw(ADC_CHANNEL, &raw);
        filtered_raw = moving_average(raw);
        ldr_read_voltage(&filtered_raw, &mv);

        ESP_LOGI(TAG, "Raw: %d | Filtered: %d | Voltage: %d mV", raw, filtered_raw, mv);

        servo_set_angle(mv, &val_deg, &duty);

        ESP_LOGI(TAG, "Angle: %d | Duty: %ld", val_deg, duty);

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY));
    }
}