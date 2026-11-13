#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "spi.h"
#include "sensor.h"

static const char *TAG = "SENSOR";

spi_device_handle_t sensor_handle;

#define SENSOR_RESET_TIMEOUT_US 100000
#define SENSOR_POLL_INTERVAL_US 2000

#define SENSOR_RESET_REG 0xE0
#define SENSOR_RESET_CMD 0xB6

#define SENSOR_STATUS_REG 0xF3
#define SENSOR_STATUS_IM_UPDATE (1 << 0)

#define SENSOR_CALIB_T_REG 0x88
#define SENSOR_CALIB_P_REG 0x8E
#define SENSOR_CALIB_H1_REG 0xA1
#define SENSOR_CALIB_H_REG 0xE1

#define SENSOR_CTRL_HUM_REG 0xF2
#define SENSOR_CTRL_MEAS_REG 0xF4

#define SENSOR_OSRS_H_X1 0x01
#define SENSOR_OSRS_T_X1 (0x01 << 5)
#define SENSOR_OSRS_P_X1 (0x01 << 2)

#define SENSOR_MODE_NORMAL 0x03

#define SENSOR_DATA_REG 0xF7

typedef struct {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;

    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;

    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;

} sensor_calibration_t;

static int32_t t_fine;

static sensor_calibration_t calibration;

static esp_err_t sensor_status_check(void) {
    uint8_t status = 0;

    ESP_RETURN_ON_ERROR(spi_read_regs(sensor_handle, SENSOR_STATUS_REG, &status, 1), TAG, "Status check failed");

    ESP_RETURN_ON_FALSE((status & SENSOR_STATUS_IM_UPDATE) == 0, ESP_ERR_INVALID_STATE, TAG, "Status - BUSY");

    ESP_LOGI(TAG, "Status - READY");

    return ESP_OK;
}

static void sensor_reset(void) {
    ESP_ERROR_CHECK(spi_write_reg(sensor_handle, SENSOR_RESET_REG, SENSOR_RESET_CMD));

    ESP_LOGI(TAG, "Reset start");

    int64_t start_time = esp_timer_get_time();
    while (esp_timer_get_time() - start_time < SENSOR_RESET_TIMEOUT_US) {
        esp_err_t error = sensor_status_check();
        if (error == ESP_OK) {
            ESP_LOGI(TAG, "Reset successful");
            return;
        }

        if (error != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "Reset failed: %s", esp_err_to_name(error));
            abort();
        }

        esp_rom_delay_us(SENSOR_POLL_INTERVAL_US);
    }

    ESP_LOGE(TAG, "Reset failed on timeout");
    abort();
}

static esp_err_t sensor_read_temp_calibration(void) {
    uint8_t data[6] = {0};

    ESP_RETURN_ON_ERROR(spi_read_regs(sensor_handle, SENSOR_CALIB_T_REG, data, sizeof(data)), TAG, "Read temperature calibration failed");

    calibration.dig_T1 = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    calibration.dig_T2 = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
    calibration.dig_T3 = (int16_t)((uint16_t)data[4] | ((uint16_t)data[5] << 8));

    ESP_LOGI(TAG, "Temperature calib values: T1=%u T2=%d T3=%d", calibration.dig_T1, calibration.dig_T2, calibration.dig_T3);

    return ESP_OK;
}

static esp_err_t sensor_read_pressure_calibration(void) {
    uint8_t data[18] = {0};

    ESP_RETURN_ON_ERROR(spi_read_regs(sensor_handle, SENSOR_CALIB_P_REG, data, sizeof(data)), TAG, "Read pressure calibration failed");

    calibration.dig_P1 = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    calibration.dig_P2 = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
    calibration.dig_P3 = (int16_t)((uint16_t)data[4] | ((uint16_t)data[5] << 8));
    calibration.dig_P4 = (int16_t)((uint16_t)data[6] | ((uint16_t)data[7] << 8));
    calibration.dig_P5 = (int16_t)((uint16_t)data[8] | ((uint16_t)data[9] << 8));
    calibration.dig_P6 = (int16_t)((uint16_t)data[10] | ((uint16_t)data[11] << 8));
    calibration.dig_P7 = (int16_t)((uint16_t)data[12] | ((uint16_t)data[13] << 8));
    calibration.dig_P8 = (int16_t)((uint16_t)data[14] | ((uint16_t)data[15] << 8));
    calibration.dig_P9 = (int16_t)((uint16_t)data[16] | ((uint16_t)data[17] << 8));

    ESP_LOGI(TAG, "Pressure calib values: P1=%u P2=%d P3=%d P4=%d P5=%d P6=%d P7=%d P8=%d P9=%d", calibration.dig_P1, calibration.dig_P2, calibration.dig_P3, calibration.dig_P4, calibration.dig_P5,
             calibration.dig_P6, calibration.dig_P7, calibration.dig_P8, calibration.dig_P9);

    return ESP_OK;
}

static esp_err_t sensor_read_humidity_calibration(void) {
    uint8_t h1 = 0;
    uint8_t data[7] = {0};

    ESP_RETURN_ON_ERROR(spi_read_regs(sensor_handle, SENSOR_CALIB_H1_REG, &h1, 1), TAG, "Read H1 calibration failed");
    ESP_RETURN_ON_ERROR(spi_read_regs(sensor_handle, SENSOR_CALIB_H_REG, data, sizeof(data)), TAG, "Read humidity calibration failed");

    calibration.dig_H1 = h1;
    calibration.dig_H2 = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
    calibration.dig_H3 = data[2];

    calibration.dig_H4 = (int16_t)(((int8_t)data[3] << 4) | (data[4] & 0x0F));
    calibration.dig_H5 = (int16_t)(((int8_t)data[5] << 4) | (data[4] >> 4));
    calibration.dig_H6 = (int8_t)data[6];

    ESP_LOGI(TAG, "Humidity calib values: H1=%u H2=%d H3=%u H4=%d H5=%d H6=%d", calibration.dig_H1, calibration.dig_H2, calibration.dig_H3, calibration.dig_H4, calibration.dig_H5, calibration.dig_H6);

    return ESP_OK;
}

static void sensor_read_calibration(void) {
    ESP_ERROR_CHECK(sensor_read_temp_calibration());
    ESP_ERROR_CHECK(sensor_read_pressure_calibration());
    ESP_ERROR_CHECK(sensor_read_humidity_calibration());

    ESP_LOGI(TAG, "Calibration read successful");
}

static void sensor_config(void) {
    ESP_ERROR_CHECK(spi_write_reg(sensor_handle, SENSOR_CTRL_HUM_REG, SENSOR_OSRS_H_X1));
    ESP_ERROR_CHECK(spi_write_reg(sensor_handle, SENSOR_CTRL_MEAS_REG, SENSOR_OSRS_T_X1 | SENSOR_OSRS_P_X1 | SENSOR_MODE_NORMAL));

    ESP_LOGI(TAG, "Config successful");
}

void sensor_setup(void) {
    sensor_reset();
    sensor_read_calibration();
    sensor_config();

    ESP_LOGI(TAG, "Setup successful");
}

esp_err_t sensor_read_raw(uint32_t *adc_P, uint32_t *adc_T, uint16_t *adc_H) {
    uint8_t data[8] = {0};

    ESP_RETURN_ON_ERROR(spi_read_regs(sensor_handle, SENSOR_DATA_REG, data, sizeof(data)), TAG, "Read raw failed");

    *adc_P = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | ((uint32_t)data[2] >> 4);
    *adc_T = ((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | ((uint32_t)data[5] >> 4);
    *adc_H = ((uint16_t)data[6] << 8) | (uint16_t)data[7];

    return ESP_OK;
}

static int32_t sensor_convert_temp(uint32_t adc_T) {
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)calibration.dig_T1 << 1))) * (int32_t)calibration.dig_T2) >> 11;

    int32_t d = (adc_T >> 4) - (int32_t)calibration.dig_T1;
    int32_t v2 = (((d * d) >> 12) * (int32_t)calibration.dig_T3) >> 14;

    t_fine = v1 + v2;
    return (t_fine * 5 + 128) >> 8;
}

static uint32_t sensor_convert_pressure(int32_t adc_P) {
    int64_t v1 = (int64_t)t_fine - 128000;
    int64_t v2 = v1 * v1 * (int64_t)calibration.dig_P6;

    v2 += (v1 * (int64_t)calibration.dig_P5) << 17;
    v2 += ((int64_t)calibration.dig_P4) << 35;

    v1 = ((v1 * v1 * (int64_t)calibration.dig_P3) >> 8) + ((v1 * (int64_t)calibration.dig_P2) << 12);
    v1 = ((((int64_t)1 << 47) + v1) * (int64_t)calibration.dig_P1) >> 33;

    if (v1 == 0)
        return 0;

    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - v2) * 3125) / v1;

    v1 = ((int64_t)calibration.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    v2 = ((int64_t)calibration.dig_P8 * p) >> 19;

    return (uint32_t)(((p + v1 + v2) >> 8) + ((int64_t)calibration.dig_P7 << 4));
}

static uint32_t sensor_convert_humidity(int32_t adc_H) {
    int32_t v = t_fine - 76800;

    v = (((((adc_H << 14) - ((int32_t)calibration.dig_H4 << 20) - ((int32_t)calibration.dig_H5 * v)) + 16384) >> 15) *
         (((((((v * (int32_t)calibration.dig_H6) >> 10) * (((v * (int32_t)calibration.dig_H3) >> 11) + 32768)) >> 10) + 2097152) * (int32_t)calibration.dig_H2 + 8192) >> 14));

    v -= (((((v >> 15) * (v >> 15)) >> 7) * (int32_t)calibration.dig_H1) >> 4);

    if (v < 0)
        v = 0;
    if (v > 419430400)
        v = 419430400;

    return (uint32_t)(v >> 12);
}

esp_err_t sensor_read_measurements(sensor_measurements_t *data) {
    uint32_t adc_P;
    uint32_t adc_T;
    uint16_t adc_H;

    ESP_RETURN_ON_ERROR(sensor_read_raw(&adc_P, &adc_T, &adc_H), TAG, "Read measurements failed");

    data->temp_c = sensor_convert_temp(adc_T) / 100;
    data->pressure_hpa = sensor_convert_pressure(adc_P) / 25600;
    data->humidity_pct = sensor_convert_humidity(adc_H) / 1024;

    return ESP_OK;
}