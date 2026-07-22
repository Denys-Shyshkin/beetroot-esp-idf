#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

#include "servo.h"

static const char *TAG = "POT_SERVO";

#define POT_ADC_UNIT ADC_UNIT_1
#define POT_ADC_CHANNEL ADC_CHANNEL_3
#define POT_ADC_ATTEN ADC_ATTEN_DB_12
#define POT_ADC_BITWIDTH ADC_BITWIDTH_12
#define POT_RAW_MAX 4095

#define SERVO_UPDATE_PERIOD_US 50000
#define LOG_PERIOD_US 500000

#define ADC_SAMPLES 16
#define ANGLE_DEADBAND 2.0f
#define FILTER_ENABLED 1

static adc_oneshot_unit_handle_t s_adc_handle = NULL;

static int64_t s_last_servo_update = 0;
static int64_t s_last_log = 0;
static float s_last_sent_angle = -1000.0f;
static int s_last_raw = 0;
static float s_last_angle_calc = 0.0f;
static uint32_t s_update_count = 0;

/* ---------------------------------------------------------------- */

static void pot_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = POT_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = POT_ADC_ATTEN,
        .bitwidth = POT_ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle,
                                               POT_ADC_CHANNEL,
                                               &chan_cfg));

    ESP_LOGI(TAG, "ADC1 CH%d initialized (12 dB, 12 bit)", POT_ADC_CHANNEL);
}

static int pot_read_raw(void)
{
#if FILTER_ENABLED
    int sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++)
    {
        int raw = 0;
        if (adc_oneshot_read(s_adc_handle, POT_ADC_CHANNEL, &raw) == ESP_OK)
        {
            sum += raw;
        }
    }
    return sum / ADC_SAMPLES;
#else
    int raw = 0;
    adc_oneshot_read(s_adc_handle, POT_ADC_CHANNEL, &raw);
    return raw;
#endif
}

static float raw_to_angle(int raw)
{
    if (raw < 0)
        raw = 0;
    if (raw > POT_RAW_MAX)
        raw = POT_RAW_MAX;

    return ((float)raw / (float)POT_RAW_MAX) * 180.0f;
}

static void servo_control_update(void)
{
    int64_t now = esp_timer_get_time();
    if (now - s_last_servo_update >= SERVO_UPDATE_PERIOD_US)
    {
        s_last_servo_update = now;
        return;
    }
    
    s_last_raw = pot_read_raw();
    s_last_angle_calc = raw_to_angle(s_last_raw);

#if FILTER_ENABLED
    if (fabsf(s_last_angle_calc - s_last_sent_angle) > ANGLE_DEADBAND)
    {
        s_last_sent_angle = s_last_angle_calc;
        servo_write_angle(s_last_angle_calc);
        s_update_count++;
    }
#else
    s_last_sent_angle = s_last_angle_calc;
    servo_write_angle(s_last_angle_calc);
    s_update_count++;
#endif
}

static void log_update(void)
{
    int64_t now = esp_timer_get_time();
    if (now - s_last_log < LOG_PERIOD_US)
    {
        return;
    }
    s_last_log = now;

    printf("raw=%4d  calc=%6.1f  servo=%6.1f  moves=%lu\n",
           s_last_raw,
           s_last_angle_calc,
           servo_get_angle(),
           (unsigned long)s_update_count);
}

/* ---------------------------------------------------------------- */

void app_main(void)
{
    ESP_LOGI(TAG, "Task 4: potentiometer -> servo");
    ESP_LOGI(TAG, "Filter: %s", FILTER_ENABLED ? "ON" : "OFF");

    servo_init();
    pot_init();

    int64_t t0 = esp_timer_get_time();
    s_last_servo_update = t0;
    s_last_log = t0;

    while (1)
    {
        servo_control_update();
        log_update();
        vTaskDelay(1);
    }
}
