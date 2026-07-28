#pragma once

#include <esp_adc/adc_cali.h>

void ldr_init(adc_unit_t adc_unit, adc_channel_t adc_channel);
void ldr_read_raw(adc_channel_t adc_channel, int *out_raw);
void ldr_read_voltage(int *out_raw, int *voltage);