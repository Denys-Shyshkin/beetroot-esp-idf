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

static int16_t servo_angle = 90;

typedef struct {
    gpio_num_t gpio;
    bool btn_state;
    bool last_btn_state;
    uint64_t s_last_btn_pressed;
    bool is_btn_pressed;
} Button;

#define DEFAULT_BUTTON (Button){ .gpio = -1, .btn_state = 1, .last_btn_state = 1, .s_last_btn_pressed = 0, .is_btn_pressed = 0 }

Button btn_1 = DEFAULT_BUTTON;
Button btn_2 = DEFAULT_BUTTON;

static void button_init(Button *btn, gpio_num_t gpio) {
    btn->gpio = gpio;

    gpio_config_t io_config = {
        .pin_bit_mask = 1ULL << btn->gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_config);
}

static void get_is_button_pressed(Button *btn) {
    int64_t now = esp_timer_get_time();

    bool button_read = gpio_get_level(btn->gpio);

    if (button_read != btn->last_btn_state) {
        btn->s_last_btn_pressed = now;
    }

    if (now - btn->s_last_btn_pressed >= BUTTON_DEBOUNCE_US) {
        if (button_read != btn->btn_state) {
            btn->btn_state = button_read;

            if (button_read == 0) {
                btn->is_btn_pressed = 1;
            }
        }
    }

    btn->last_btn_state = button_read;
}

static void buttons_reading() {
    get_is_button_pressed(&btn_1);
    get_is_button_pressed(&btn_2);
}

static void servo_control_update(void) {
    if (btn_1.is_btn_pressed) {
        btn_1.is_btn_pressed = 0;

        servo_angle += ANGLE_STEP;
    } 
    if (btn_2.is_btn_pressed) {
        btn_2.is_btn_pressed = 0;

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

    button_init(&btn_1, BUTTON_1);
    button_init(&btn_2, BUTTON_2);

    while (1) {
        buttons_reading();
        servo_control_update();
        log_update();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}