#include "ldr.h"
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_log.h>

#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;

void ldr_init(adc_unit_t adc_unit, adc_channel_t adc_channel) {
    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = adc_unit,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &adc_handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, adc_channel, &channel_config));

    adc_cali_curve_fitting_config_t cfg = {
        .unit_id = adc_unit,
        .chan = adc_channel,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cfg, &cali_handle));
}

void ldr_read_raw(adc_channel_t adc_channel, int *out_raw) {
    adc_oneshot_read(adc_handle, adc_channel, out_raw);
}

void ldr_read_voltage(int *out_raw, int *voltage) {
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, *out_raw, voltage));
}