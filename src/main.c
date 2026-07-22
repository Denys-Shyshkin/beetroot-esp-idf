#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "servo.h"

static const char *TAG = "SERVO";

#define BUTTON_1 GPIO_NUM_4
#define BUTTON_2 GPIO_NUM_5

#define LOG_PERIOD_US 500000
#define BUTTON_DEBOUNCE_US 50000

#define ANGLE_STEP 10

static uint64_t s_last_log = 0;
static uint64_t s_last_btn_1_pressed = 0;
static uint64_t s_last_btn_2_pressed = 0;

static bool btn_1_state = 1;
static bool btn_2_state = 1;

static bool is_btn_1_pressed = 1;
static bool is_btn_2_pressed = 1;

static bool last_btn_1_state = 1;
static bool last_btn_2_state = 1;

int16_t servo_angle = 90;

static void button_init(gpio_num_t gpio) {
    gpio_config_t io_config = {
        .pin_bit_mask = 1ULL << gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_config);
}

static bool get_is_button_pressed(gpio_num_t gpio, bool *btn_state, bool *last_btn_state, uint64_t *s_last_btn_pressed) {
    int64_t now = esp_timer_get_time();

    bool button_read = gpio_get_level(gpio);

    if (button_read != *last_btn_state) {
        *s_last_btn_pressed = now;
    }

    if (now - *s_last_btn_pressed >= BUTTON_DEBOUNCE_US) {
        if (button_read != *btn_state) {
            *btn_state = button_read;

            if (button_read == 0) {
                return true;
            }
        }
    }

    *last_btn_state = button_read;

    return false;
}

static void buttons_reading() {
    is_btn_1_pressed = get_is_button_pressed(BUTTON_1, &btn_1_state, &last_btn_1_state, &s_last_btn_1_pressed);
    is_btn_2_pressed = get_is_button_pressed(BUTTON_2, &btn_2_state, &last_btn_2_state, &s_last_btn_2_pressed);
}

static void servo_control_update(void) {
    if (is_btn_1_pressed) {
        servo_angle += ANGLE_STEP;
    } 
    if (is_btn_2_pressed) {
        servo_angle -= ANGLE_STEP;
    }

    if (servo_angle > 180) {
        servo_angle = 180;
    }
    if (servo_angle < 0) {
        servo_angle = 0;
    }

    servo_write_angle(servo_angle);
}

static void log_update(void) {
    int64_t now = esp_timer_get_time();

    if (now - s_last_log >= LOG_PERIOD_US) {
        s_last_log = now;

        ESP_LOGI(TAG, "Current servo angle: %d \n", servo_angle);
    }
}


void app_main(void) {
    servo_init();

    button_init(BUTTON_1);
    button_init(BUTTON_2);

    while (1) {
        buttons_reading();
        servo_control_update();
        log_update();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}