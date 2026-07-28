#include "servo.h"
#include <driver/ledc.h>
#include <esp_log.h>

#define SERVO_FREQ 50
#define SERVO_PERIOD_MS (1000 / SERVO_FREQ)
#define SERVO_RESOLUTION LEDC_TIMER_12_BIT
#define SERVO_MAX_DUTY ((1 << SERVO_RESOLUTION) - 1)
#define SERVO_UNIT LEDC_TIMER_0
#define SERVO_CHANNEL LEDC_CHANNEL_0

// Calibration variables
static int SERVO_MIN_DEG = 10;
static int SERVO_MAX_DEG = 170;

static int SERVO_MIN_US = 500;
static int SERVO_MAX_US = 2500;

static int LIGHT_MIN_MV = 50;
static int LIGHT_MAX_MV = 3000;

void servo_init(int gpio_num) {
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = SERVO_UNIT,
        .duty_resolution = SERVO_RESOLUTION,
        .freq_hz = SERVO_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

    ledc_channel_config_t channel_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = SERVO_CHANNEL,
        .timer_sel = SERVO_UNIT,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = gpio_num,
        .duty = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_config));
}

static int val_clamp(int value, int val_min, int val_max) {
    if (value < val_min)
        return val_min;
    if (value > val_max)
        return val_max;
    return value;
}

static int mad_map(int value, int in_min, int in_max, int out_min, int out_max) {
    if (in_min == in_max)
        return out_max;

    value = val_clamp(value, in_min, in_max);

    return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min);
}

void servo_set_angle(int mv, int *val_deg, int *duty) {
    mv = val_clamp(mv, LIGHT_MIN_MV, LIGHT_MAX_MV);

    *val_deg = mad_map(mv, LIGHT_MIN_MV, LIGHT_MAX_MV, SERVO_MIN_DEG, SERVO_MAX_DEG);
    int val_us = mad_map(*val_deg, SERVO_MIN_DEG, SERVO_MAX_DEG, SERVO_MIN_US, SERVO_MAX_US);
    int val_ms = val_us / 1000;

    *duty = (int)(SERVO_MAX_DUTY / SERVO_PERIOD_MS) * val_ms;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, *duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL));
}