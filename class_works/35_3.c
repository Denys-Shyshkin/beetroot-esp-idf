#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define ENC_CLK GPIO_NUM_9   
#define ENC_DT  GPIO_NUM_10  

#define POLLS_PER_YIELD 2000

static int cnt_cw    = 0;
static int cnt_ccw   = 0;
static int cnt_error = 0;

static void print_transition(uint8_t last, uint8_t cur, uint8_t sum, const char *dir)
{
    printf("%d%d -> %d%d   sum=0b%d%d%d%d (%2d)  %s\n",
           (last >> 1) & 1, last & 1,
           (cur  >> 1) & 1, cur  & 1,
           (sum >> 3) & 1, (sum >> 2) & 1, (sum >> 1) & 1, sum & 1,
           sum, dir);
}

void app_main(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << ENC_CLK) | (1ULL << ENC_DT),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    uint8_t last = (gpio_get_level(ENC_CLK) << 1) | gpio_get_level(ENC_DT);

    printf("\n=== Квадратура наживо ===\n");
    printf("Стан = AB.\n");
    printf("Один клац фіксатора = 4 переходи (повний цикл).\n\n");

    int poll_counter = 0;
    int report_counter = 0;

    while (1) {
        uint8_t a = gpio_get_level(ENC_CLK);
        uint8_t b = gpio_get_level(ENC_DT);
        uint8_t cur = (a << 1) | b;

        if (cur != last) {
            uint8_t sum = (last << 2) | cur;

            if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
                cnt_cw++;
                print_transition(last, cur, sum, "CW");
            }
            else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
                cnt_ccw++;
                print_transition(last, cur, sum, "CCW");
            }
            else {
                cnt_error++;
                print_transition(last, cur, sum, "??? ПОМИЛКА");
            }

            last = cur;
        }

        if (++report_counter >= 500000) {
            report_counter = 0;
            printf("\n--- CW: %d | CCW: %d | ERR: %d ---\n\n",
                   cnt_cw, cnt_ccw, cnt_error);
        }

        if (++poll_counter >= POLLS_PER_YIELD) {
            poll_counter = 0;
            vTaskDelay(1);
        }
    }
}