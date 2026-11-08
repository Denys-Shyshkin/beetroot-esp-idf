#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_PORT I2C_NUM_0
#define I2C_SDA_GPIO GPIO_NUM_4
#define I2C_SCL_GPIO GPIO_NUM_5
#define I2C_FREQ_HZ 100000
#define I2C_TIMEOUT_MS 200

#define DISPLAY_ADDRESS 0x3C

#define BUTTON_GPIO GPIO_NUM_6
#define BUTTON_DEBOUNCE_US 20000

#define SUPERLOOP_DELAY_MS 10

static const char *TAG = "MAIN";

static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t display;
static bool display_state = false;

static uint8_t button_prev_read = 1;
static uint8_t button_stable_state = 1;
static int64_t button_last_changed_us = 0;

static void i2c_bus_init(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    ESP_LOGI(TAG, "I2C bus init successful - PORT: %d | SDA GPIO: %d | SCL GPIO: %d", I2C_PORT, I2C_SDA_GPIO, I2C_SCL_GPIO);
}

static void i2c_device_config(void) {
    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DISPLAY_ADDRESS,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &device_config, &display));

    ESP_LOGI(TAG, "I2C device config successful - FREQ: %d", I2C_FREQ_HZ);
}

static esp_err_t display_setup(void) {
    uint8_t setup_buffer[] = {
        0x00,       // control byte: commands follow
        0xAE,       // display off while configuring
        0xD5, 0x80, // clock divide ratio / oscillator frequency (default)
        0xA8, 0x3F, // multiplex ratio: 64 rows
        0xD3, 0x00, // display offset: 0
        0x40,       // display start line: 0
        0x8D, 0x14, // charge pump on
        0xA1,       // mirror columns
        0xC8,       // flip rows
        0xDA, 0x12, // COM pins configuration for 128x64
        0x81, 0x7F, // contrast (0x00..0xFF)
        0xA6,       // normal display (not inverted)
        0xA5,       // all pixels on
    };

    return i2c_master_transmit(display, setup_buffer, sizeof(setup_buffer), I2C_TIMEOUT_MS);
}

static esp_err_t display_on(void) {
    uint8_t control_buffer[] = {
        0x00, // control byte: commands follow
        0xAF  // display on
    };

    esp_err_t control_error = i2c_master_transmit(display, control_buffer, sizeof(control_buffer), I2C_TIMEOUT_MS);
    if (control_error != ESP_OK) {
        ESP_LOGE(TAG, "DISPLAY ON control error: %s", esp_err_to_name(control_error));
        return control_error;
    }

    ESP_LOGI(TAG, "DISPLAY is ON");
    return ESP_OK;
}

static esp_err_t display_off(void) {
    uint8_t control_buffer[] = {
        0x00, // control byte: commands follow
        0xAE  // display off
    };

    esp_err_t control_error = i2c_master_transmit(display, control_buffer, sizeof(control_buffer), I2C_TIMEOUT_MS);
    if (control_error != ESP_OK) {
        ESP_LOGE(TAG, "DISPLAY OFF control error: %s", esp_err_to_name(control_error));
        return control_error;
    }

    ESP_LOGI(TAG, "DISPLAY is OFF");
    return ESP_OK;
}

static void button_init(void) {
    gpio_config_t config = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&config));

    button_prev_read = gpio_get_level(BUTTON_GPIO);
    button_stable_state = button_prev_read;
    button_last_changed_us = esp_timer_get_time();

    ESP_LOGI(TAG, "BUTTON init successful - GPIO: %d", BUTTON_GPIO);
}

void app_main(void) {
    i2c_bus_init();

    esp_err_t probe_error = i2c_master_probe(i2c_bus, DISPLAY_ADDRESS, I2C_TIMEOUT_MS);
    if (probe_error != ESP_OK) {
        ESP_LOGE(TAG, "I2C device probe error: %s", esp_err_to_name(probe_error));
        ESP_LOGE(TAG, "Terminating the process...");
        return;
    }

    ESP_LOGI(TAG, "I2C device probe successful - ADDRESS: 0x%02X", DISPLAY_ADDRESS);

    i2c_device_config();

    esp_err_t display_setup_error = display_setup();
    if (display_setup_error != ESP_OK) {
        ESP_LOGE(TAG, "DISPLAY setup error: %s", esp_err_to_name(display_setup_error));
        ESP_LOGE(TAG, "Terminating the process...");
        return;
    }

    ESP_LOGI(TAG, "DISPLAY setup successful");

    esp_err_t display_on_error = display_on();
    if (display_on_error == ESP_OK) {
        display_state = true;
    }

    button_init();

    while (1) {
        int64_t now = esp_timer_get_time();

        uint8_t button_read_raw = gpio_get_level(BUTTON_GPIO);

        if (button_read_raw != button_prev_read) {
            button_last_changed_us = now;
        }

        if (now - button_last_changed_us >= BUTTON_DEBOUNCE_US) {
            if (button_read_raw != button_stable_state) {
                button_stable_state = button_read_raw;

                if (button_stable_state == 0) {
                    ESP_LOGI(TAG, "BUTTON press registered");

                    bool new_display_state = !display_state;

                    esp_err_t display_error = new_display_state ? display_on() : display_off();
                    if (display_error == ESP_OK) {
                        display_state = new_display_state;
                    }
                }
            }
        }

        button_prev_read = button_read_raw;

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY_MS));
    }
}