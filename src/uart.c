#include "uart.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include <string.h>

#define UART_PORT UART_NUM_0
#define UART_BAUD_RATE 115200
#define UART_READ_TIMEOUT 100

static const char *TAG = "UART";

const char *uart_valid_cmds[VALID_CMDS_NUM] = {"PRES", "TEMP", "HUM"};

static char rx_line[UART_BUFFER_SIZE];
static uint8_t rx_line_len = 0;

void uart_init(void) {
    uart_config_t config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &config));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUFFER_SIZE, UART_BUFFER_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "Init successful - PORT: %d | BAUD_RATE: %d", UART_PORT, UART_BAUD_RATE);
}

static bool uart_is_cmd_valid(void) {
    for (uint8_t i = 0; i < VALID_CMDS_NUM; i++) {
        if (strcmp(rx_line, uart_valid_cmds[i]) == 0) {
            return true;
        }
    }

    return false;
}

void uart_print_cmds(void) {
    const char *title = "Available commands: ";
    uart_write_bytes(UART_PORT, title, strlen(title));

    for (size_t i = 0; i < VALID_CMDS_NUM; i++) {
        uart_write_bytes(UART_PORT, uart_valid_cmds[i], strlen(uart_valid_cmds[i]));

        if (i == VALID_CMDS_NUM - 1) {
            uart_write_bytes(UART_PORT, "\n", 1);
        } else {
            uart_write_bytes(UART_PORT, " | ", 3);
        }
    }
}

void uart_print(const void *src, size_t size) {
    uart_write_bytes(UART_PORT, src, size);
}

void uart_read_cmd(char *read_cmd, char *current_cmd) {
    int len = uart_read_bytes(UART_PORT, &rx_line[rx_line_len], UART_BUFFER_SIZE - rx_line_len - 1, pdMS_TO_TICKS(UART_READ_TIMEOUT));
    if (len < 0) {
        ESP_LOGE(TAG, "UART read ERROR");
    } else {
        rx_line_len += len;

        for (int i = rx_line_len - len; i < rx_line_len; i++) {
            if (rx_line[i] == '\n') {
                rx_line[i] = '\0';

                if (uart_is_cmd_valid()) {
                    if (strcmp(current_cmd, rx_line) == 0) {
                        const char *cmd_in_use_string = "This command already active, try another one. \n";
                        uart_print(cmd_in_use_string, strlen(cmd_in_use_string));
                    } else {
                        strncpy(read_cmd, rx_line, sizeof(rx_line) - 1);
                    }
                } else {
                    const char *error_string = "Invalid commnad, try again. \n";
                    uart_print(error_string, strlen(error_string));
                }

                uart_print("\n", 1);
                uart_print_cmds();

                rx_line_len = 0;
            }
        }
    }
}