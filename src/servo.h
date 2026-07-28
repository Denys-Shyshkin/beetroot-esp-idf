#pragma once

void servo_init(int gpio_num);
void servo_set_angle(int mv, int *val_deg, int *duty);