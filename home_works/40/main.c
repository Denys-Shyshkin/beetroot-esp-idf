#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>

#define LED_GPIO GPIO_NUM_5
#define BUTTON_GPIO GPIO_NUM_4
#define BUTTON_DEBOUNCE_US 20000

#define UART_PORT UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_TX_PIN GPIO_NUM_15
#define UART_RX_PIN GPIO_NUM_16
#define UART_BUF_SIZE 256

#define SUPERLOOP_DELAY_MS 10

static const char *TAG = "MAIN";

static bool is_led_on = false;
static bool button_stable_state;
static bool last_button_read;
static int64_t last_button_changed_us;

static void led_init(void) {
    ESP_ERROR_CHECK(gpio_reset_pin(LED_GPIO));
    ESP_ERROR_CHECK(gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, is_led_on));
}

static void button_init(void) {
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));

    last_button_read = gpio_get_level(BUTTON_GPIO);
    button_stable_state = last_button_read;
    last_button_changed_us = esp_timer_get_time();
}

static void uart_init(void) {
    uart_config_t config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &config));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0));
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void app_main(void) {
    led_init();
    button_init();
    uart_init();

    while (1) {
        int64_t now = esp_timer_get_time();

        bool button_read = gpio_get_level(BUTTON_GPIO);

        if (button_read != last_button_read) {
            last_button_changed_us = now;
        }

        if (now - last_button_changed_us >= BUTTON_DEBOUNCE_US) {
            if (button_read != button_stable_state) {
                button_stable_state = button_read;

                if (button_read == 0) {
                    is_led_on = !is_led_on;
                    gpio_set_level(LED_GPIO, is_led_on);

                    uint8_t message = is_led_on ? 0x01 : 0x00;
                    uart_write_bytes(UART_PORT, &message, sizeof(message));

                    ESP_LOGI(TAG, "is_led_on: %d", is_led_on);
                }
            }
        }

        last_button_read = button_read;

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY_MS));
    }
}