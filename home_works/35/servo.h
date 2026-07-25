#pragma once

#include <stdint.h>

void servo_init(uint8_t gpio_num);
void servo_write_us(uint32_t us);
void servo_write_angle(float deg);
void servo_stop(void);
float servo_get_angle(void);