#include "buzzer.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LEDC_TIMER LEDC_TIMER_1
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_1
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY 2700
#define DUTY 512
#define DURATION_TO_GAP_RATIO 1.3

void buzzer_init(uint8_t gpio_num) {
    ledc_timer_config_t t = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t c = {
        .gpio_num = gpio_num,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}

static void tone_on(uint16_t freq) {
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, DUTY);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

static void tone_off() {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

static void play_note(uint16_t freq, uint16_t duration) {
    uint16_t gap = duration * DURATION_TO_GAP_RATIO;

    if (freq == REST) {
        tone_off();
        vTaskDelay(pdMS_TO_TICKS(duration));
    } else {
        tone_on(freq);
        vTaskDelay(pdMS_TO_TICKS(duration));

        tone_off();
        vTaskDelay(pdMS_TO_TICKS(gap));
    }
}

void play_melody(const note_t melody[], uint8_t length) {
    for (int i = 0; i < length; i++) {
        play_note(melody[i].freq, melody[i].duration);
    }
}