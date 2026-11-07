#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define BUZZER_GPIO     16
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_DUTY_RES   LEDC_TIMER_10_BIT

#define REST 0
#define NOTE_C7  2093
#define NOTE_D7  2349
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_G7  3136
#define NOTE_A7  3520
#define NOTE_B7  3951
#define NOTE_C8  4186

typedef struct {
    uint16_t freq;    
    uint16_t duration;  
} note_t;


static const note_t melody[] = {
    {NOTE_E7, 400}, {REST, 50},
    {NOTE_E7, 200}, {NOTE_G7, 400}, {NOTE_E7, 400},
    {NOTE_D7, 400}, {NOTE_C7, 600}, {REST, 100},
    {NOTE_C7, 800}, {REST, 300},    
    {NOTE_E7, 200}, {NOTE_G7, 400}, {NOTE_E7, 400},
    {NOTE_D7, 400}, {NOTE_C7, 600}, {REST, 10},{NOTE_C7, 300},
    {NOTE_C7, 300}, {NOTE_C7, 800}, {REST, 300},
};
#define MELODY_LEN (sizeof(melody) / sizeof(note_t))

static void buzzer_init(void)
{
    ledc_timer_config_t t = {
        .speed_mode      = LEDC_MODE,
        .timer_num       = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz         = 2700,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&t));

    ledc_channel_config_t c = {
        .gpio_num   = BUZZER_GPIO,
        .speed_mode = LEDC_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&c));
}

static void tone_on(uint16_t freq)
{
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 512);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

static void tone_off(void)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

static void play_note(uint16_t freq, uint16_t duration)
{
    uint16_t play = duration * 85 / 100;
    uint16_t gap  = duration - play;

    if (freq == REST) {
        tone_off();
        vTaskDelay(pdMS_TO_TICKS(duration));
        return;
    }
    tone_on(freq);
    vTaskDelay(pdMS_TO_TICKS(play));
    tone_off();
    vTaskDelay(pdMS_TO_TICKS(gap));
}

void app_main(void)
{
    buzzer_init();

    while (1) {
        for (int i = 0; i < MELODY_LEN; i++) {
            play_note(melody[i].freq, melody[i].duration);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}