#include "buzzer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "servo.h"
#include <stdio.h>

#define ENC_CLK GPIO_NUM_4
#define ENC_DT GPIO_NUM_5
#define ENC_SW GPIO_NUM_6
#define BUZZER GPIO_NUM_7
#define SERVO GPIO_NUM_18

#define COUNTER_DEBOUNCE_US 300000
#define BTN_DEBOUNCE_US 50000
#define OPTIONS_LENGTH 4
#define CODE_LENGTH 4
#define ATTEMPTS 3

const char *TAG = "ENCODE";

int8_t count = 0;
int8_t attempt_number = 1;
bool direction = 1;
bool prev_direction = 1;
bool code_check_success = 0;
uint64_t s_last_count = 0;
uint8_t last = 0b0000;
uint8_t curr_index = 0;
bool btn_state = 1;
bool last_btn_state = 1;
uint64_t s_last_btn_press = 0;
bool is_btn_pressed = 0;

const uint8_t cw_options[OPTIONS_LENGTH] = {0b1101, 0b0100, 0b0010, 0b1011};
const uint8_t c_cw_options[OPTIONS_LENGTH] = {0b1110, 0b0111, 0b0001, 0b1000};

const uint8_t safe_code[CODE_LENGTH] = {2, 7, 4, 9};
uint8_t user_input[CODE_LENGTH] = {0};

const note_t game_over[] = {{NOTE_G4, EIGHTH}, {NOTE_E4, EIGHTH}, {NOTE_D4, EIGHTH}, {NOTE_C4, HALF}};
const note_t victory[] = {{NOTE_G4, EIGHTH}, {NOTE_G4, EIGHTH}, {NOTE_A4, QUARTER}, {NOTE_G4, EIGHTH}, {NOTE_C5, HALF}};

void encoder_init() {
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << ENC_CLK) | (1ULL << ENC_DT) | (1ULL << ENC_SW),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    last = (gpio_get_level(ENC_CLK) << 1) | gpio_get_level(ENC_DT);
}

bool is_contain(const uint8_t arr[], uint8_t value) {
    for (uint8_t i = 0; i < OPTIONS_LENGTH; i++) {
        if (arr[i] == value) {
            return 1;
        }
    }
    return 0;
}

void terminal_output() {
    for (int i = 0; i < CODE_LENGTH; i++) {
        if (i == curr_index) {
            printf("[");
            printf("%d", user_input[i]);
            printf("]");
        } else {
            printf(" %d ", user_input[i]);
        }

        if (i == CODE_LENGTH - 1) {
            printf(" | %d/%d", attempt_number, ATTEMPTS);
            printf("\n");
        }
    }
}

void read_encoder_rotation_inputs() {
    int64_t now = esp_timer_get_time();

    uint8_t a = gpio_get_level(ENC_CLK);
    uint8_t b = gpio_get_level(ENC_DT);
    uint8_t cur = (a << 1) | b;

    if (cur != last) {
        uint8_t sum = (last << 2) | cur;

        if (now - s_last_count >= COUNTER_DEBOUNCE_US) {
            s_last_count = now;

            if (is_contain(cw_options, sum)) {
                count++;
                direction = 1;
            } else if (is_contain(c_cw_options, sum)) {
                count--;
                direction = 0;
            }
        }

        last = cur;
    }
}

void read_encoder_btn_inputs() {
    uint64_t now = esp_timer_get_time();

    bool btn_read = gpio_get_level(ENC_SW);

    if (btn_read != last_btn_state) {
        s_last_btn_press = now;
    }

    if (now - s_last_btn_press >= BTN_DEBOUNCE_US) {
        if (btn_read != btn_state) {
            btn_state = btn_read;

            if (btn_read == 0) {
                is_btn_pressed = 1;
            }
        }
    }

    last_btn_state = btn_read;
}

void handle_encoder_rotation_data() {
    if (direction != prev_direction) {
        curr_index++;
        count = 0;

        prev_direction = direction;
    } else {
        if (count > 9) {
            count = 0;
        }
        if (count < 0) {
            count = 9;
        }
    }

    if (curr_index < CODE_LENGTH) {
        user_input[curr_index] = count;
    }
}

void next_attempt() {
    attempt_number++;
    curr_index = 0;
    count = 0;
    direction = 1;
    prev_direction = 1;

    for (int i = 0; i < CODE_LENGTH; i++) {
        user_input[i] = 0;
    }
}

void handle_encoder_btn_data() {
    if (is_btn_pressed) {
        next_attempt();

        is_btn_pressed = 0;
    }
}

void check_user_input() {
    if (curr_index == CODE_LENGTH) {
        for (int i = 0; i < CODE_LENGTH; i++) {
            if (user_input[i] != safe_code[i]) {
                next_attempt();

                return;
            }
        }
        code_check_success = 1;
    }
}

void freeze_mode() {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void handle_failed_result() {
    if (attempt_number > ATTEMPTS) {
        ESP_LOGW(TAG, "Too many failed attempts, terminal is locked");

        play_melody(game_over, sizeof(game_over) / NOTE_LEN);

        freeze_mode();
    }
}

void handle_success_result() {
    if (code_check_success) {
        ESP_LOGI(TAG, "Code is correct");

        play_melody(victory, sizeof(victory) / NOTE_LEN);
        servo_write_angle(170);

        freeze_mode();
    }
}

void app_main(void) {
    encoder_init();
    buzzer_init(BUZZER);
    servo_init(SERVO);

    while (1) {
        read_encoder_rotation_inputs();
        read_encoder_btn_inputs();

        handle_encoder_rotation_data();
        handle_encoder_btn_data();

        check_user_input();

        handle_failed_result();
        handle_success_result();

        terminal_output();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}