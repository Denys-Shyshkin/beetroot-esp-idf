#include "servo.h"
#include "driver/ledc.h"

#define SERVO_GPIO 18
#define SERVO_TIMER LEDC_TIMER_0
#define SERVO_CHANNEL LEDC_CHANNEL_0
#define SERVO_MODE LEDC_LOW_SPEED_MODE
#define SERVO_RES LEDC_TIMER_14_BIT
#define SERVO_FREQ_HZ 50
#define SERVO_PERIOD_US 20000

#define SERVO_MIN_US 500
#define SERVO_MAX_US 2400

static float s_cur_angle = 90.0f;

void servo_init(void) {
    ledc_timer_config_t timer_cfg = {
        .speed_mode = SERVO_MODE,
        .duty_resolution = SERVO_RES,
        .timer_num = SERVO_TIMER,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t ch_cfg = {
        .gpio_num = SERVO_GPIO,
        .speed_mode = SERVO_MODE,
        .channel = SERVO_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = SERVO_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ch_cfg);

    servo_write_angle(90.0f);
}

void servo_write_us(uint32_t us) {
    if (us < SERVO_MIN_US)
        us = SERVO_MIN_US;
    if (us > SERVO_MAX_US)
        us = SERVO_MAX_US;

    uint32_t max_duty = (1u << SERVO_RES) - 1;
    uint32_t duty = (uint32_t)(((uint64_t)us * max_duty) / SERVO_PERIOD_US);

    ledc_set_duty(SERVO_MODE, SERVO_CHANNEL, duty);
    ledc_update_duty(SERVO_MODE, SERVO_CHANNEL);
}

void servo_write_angle(float deg) {
    if (deg < 0.0f)
        deg = 0.0f;
    if (deg > 180.0f)
        deg = 180.0f;

    s_cur_angle = deg;

    uint32_t us = SERVO_MIN_US + (uint32_t)((deg / 180.0f) * (SERVO_MAX_US - SERVO_MIN_US));
    servo_write_us(us);
}

float servo_get_angle(void) {
    return s_cur_angle;
}

void servo_stop(void) {
    ledc_stop(SERVO_MODE, SERVO_CHANNEL, 0);
}