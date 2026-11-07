/**
 *  Урок 33
 *  Модуль 3.4.
 *  Домашнє завдання
 *  Програвання звуку на зумері (PWM)
 */
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define BUZZER_GPIO 4
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY 2700
#define DUTY 512
#define DURATION_TO_GAP_RATIO 1.3

// Notes frequencies list
#define REST 0
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523

// Notes duration list
#define WHOLE 800
#define HALF 400
#define QUARTER 200
#define EIGHTH 100

typedef struct {
    uint16_t freq;
    uint16_t duration;
} note_t;

const note_t baby_shark[] = {
    {NOTE_C4, QUARTER}, {NOTE_D4, QUARTER}, {NOTE_E4, HALF},    
    {NOTE_C4, QUARTER}, {NOTE_D4, QUARTER}, {NOTE_E4, HALF},
    {NOTE_C4, QUARTER}, {NOTE_D4, QUARTER}, {NOTE_E4, HALF},
};

const note_t jingle_bells[] = {
    {NOTE_E4, QUARTER}, {NOTE_E4, QUARTER}, {NOTE_E4, HALF},    
    {NOTE_E4, QUARTER}, {NOTE_E4, QUARTER}, {NOTE_E4, HALF},
    {NOTE_E4, QUARTER}, {NOTE_G4, QUARTER}, {NOTE_C4, QUARTER}, {NOTE_D4, QUARTER}, {NOTE_E4, WHOLE}
};

const note_t we_will_rock_you[] = {
    {NOTE_C4, EIGHTH}, {NOTE_C4, EIGHTH}, {NOTE_D4, QUARTER}, {REST, QUARTER},
    {NOTE_C4, EIGHTH}, {NOTE_C4, EIGHTH}, {NOTE_D4, QUARTER}, {REST, QUARTER},
    {NOTE_C4, EIGHTH}, {NOTE_C4, EIGHTH}, {NOTE_D4, QUARTER}, {NOTE_E4, HALF}
};

const note_t twinkle_little_star[] = {
    {NOTE_C4, QUARTER}, {NOTE_C4, QUARTER}, {NOTE_G4, QUARTER}, {NOTE_G4, QUARTER},
    {NOTE_A4, QUARTER}, {NOTE_A4, QUARTER}, {NOTE_G4, HALF},
};

#define NOTE_LEN sizeof(note_t)

enum MELODIES {
    BABY_SHARK,
    JINGLE_BELLS,
    WE_WILL_ROCK_YOU,
    TWINKLE_LITTLE_STAR,
};

uint8_t melody_to_play = TWINKLE_LITTLE_STAR;

void buzzer_init();
void tone_on(uint16_t freq);
void tone_off();
void play_note(uint16_t freq, uint16_t duration);
void play_melody(const note_t melody[], uint8_t length);

void app_main(void) {
    buzzer_init();

    while (1) {
        switch (melody_to_play) {
        case BABY_SHARK:
            play_melody(baby_shark, sizeof(baby_shark) / NOTE_LEN);
            break;
        case JINGLE_BELLS:
            play_melody(jingle_bells, sizeof(jingle_bells) / NOTE_LEN);
            break;
        case WE_WILL_ROCK_YOU:
            play_melody(we_will_rock_you, sizeof(we_will_rock_you) / NOTE_LEN);
            break;
        case TWINKLE_LITTLE_STAR:
            play_melody(twinkle_little_star, sizeof(twinkle_little_star) / NOTE_LEN);
            break;
        
        default:
            play_melody(baby_shark, sizeof(baby_shark) / NOTE_LEN);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void buzzer_init() {
    ledc_timer_config_t t = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t c = {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}

void tone_on(uint16_t freq) {
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, DUTY);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void tone_off() {
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void play_note(uint16_t freq, uint16_t duration) {
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