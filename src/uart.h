#pragma once

#define UART_BUFFER_SIZE 256

typedef enum {
    CMD_PRES,
    CMD_TEMP,
    CMD_HUM,
    VALID_CMDS_NUM
} uart_cmd_t;

extern const char *uart_valid_cmds[VALID_CMDS_NUM];

void uart_init(void);
void uart_read_cmd(char *read_cmd, char *current_cmd);
void uart_print_cmds(void);