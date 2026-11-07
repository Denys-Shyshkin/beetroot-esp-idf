#include "driver/gpio.h"
#include "driver/uart.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define UART_PORT UART_NUM_1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define UART_BUF_SIZE 256

#define SUPERLOOP_DELAY 1000

static const char *TAG = "MAIN";

uint8_t message[] = {
    0xAA,
    0x01,
    0x02,
    0x50,
    0x80,
    0x7C
};

void app_main(void) {
    uart_config_t config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(UART_PORT, &config);
    uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    while (1) {
        ESP_LOGI(TAG, "UART TEST");

        uart_write_bytes(UART_PORT, message, sizeof(message));

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY));
    }
}