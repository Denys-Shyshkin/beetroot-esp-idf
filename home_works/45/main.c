#include "driver/uart.h"
#include "driver/uhci.h"
#include "esp_adc/adc_continuous.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

#define POT_CHANNEL ADC_CHANNEL_3 // GPIO 4 is ADC1_3
#define ADC_UNIT ADC_UNIT_1
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BIT_WIDTH SOC_ADC_DIGI_MAX_BITWIDTH
#define ADC_CONV_MODE ADC_CONV_SINGLE_UNIT_1
#define ADC_FORMAT ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define ADC_SAMPLE_RATE_HZ 5000

#define DMA_BUFFER_SIZE 256
#define DMA_MAX_BUF 4096
#define DMA_RESULT_BYTES SOC_ADC_DIGI_RESULT_BYTES
#define SAMPLES_PER_FRAME (DMA_BUFFER_SIZE / DMA_RESULT_BYTES)

// Serial monitor, to see hex values add "monitor_filters = hexlify" to platformio.ini
#define UART_PORT UART_NUM_0
#define UART_BAUD_RATE 115200

#define TX_SAMPLES 512

#define UHCI_RX_MEM_SIZE 1024
#define UHCI_DMA_BURST 32

#define SUPERLOOP_DELAY_MS 10

static const char *TAG = "MAIN";

static adc_continuous_handle_t handle;
static uhci_controller_handle_t uhci;

static uint8_t frame[DMA_BUFFER_SIZE];

static uint16_t buffer_a[TX_SAMPLES];
static uint16_t buffer_b[TX_SAMPLES];
static volatile bool uart_tx_busy = false;

// Pointer to an active buffer
static uint16_t *fill_ptr = buffer_a;
static uint32_t fill_idx = 0;

static void adc_dma_init(void) {
    adc_continuous_handle_cfg_t handle_config = {
        .max_store_buf_size = DMA_MAX_BUF,
        .conv_frame_size = DMA_BUFFER_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_config, &handle));

    adc_digi_pattern_config_t pattern = {
        .atten = ADC_ATTEN,
        .channel = POT_CHANNEL,
        .unit = ADC_UNIT,
        .bit_width = ADC_BIT_WIDTH,
    };

    adc_continuous_config_t adc_config = {
        .pattern_num = 1,
        .adc_pattern = &pattern,
        .sample_freq_hz = ADC_SAMPLE_RATE_HZ,
        .conv_mode = ADC_CONV_MODE,
        .format = ADC_FORMAT,
    };
    ESP_ERROR_CHECK(adc_continuous_config(handle, &adc_config));

    ESP_ERROR_CHECK(adc_continuous_start(handle));

    ESP_LOGI(TAG, "ADC DMA init successful");
}

// Callback function for handling the completion of a transmission
static bool uart_tx_done_cb(uhci_controller_handle_t ctrl, const uhci_tx_done_event_data_t *edata, void *user_ctx) {
    uart_tx_busy = false;
    return false;
}

static void uart_dma_init(void) {
    uart_config_t config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &config));

    uhci_controller_config_t uhci_config = {
        .uart_port = UART_PORT,
        .tx_trans_queue_depth = 1,
        .max_transmit_size = sizeof(buffer_a),
        .max_receive_internal_mem = UHCI_RX_MEM_SIZE,
        .dma_burst_size = UHCI_DMA_BURST,
    };
    ESP_ERROR_CHECK(uhci_new_controller(&uhci_config, &uhci));

    uhci_event_callbacks_t event_callbacks = {
        .on_tx_trans_done = uart_tx_done_cb,
    };
    ESP_ERROR_CHECK(uhci_register_event_callbacks(uhci, &event_callbacks, NULL));

    ESP_LOGI(TAG, "UART DMA init successful");
}

void app_main(void) {
    adc_dma_init();
    uart_dma_init();

    while (1) {
        uint32_t length = 0;

        while (adc_continuous_read(handle, frame, sizeof(frame), &length, 0) == ESP_OK) {
            for (uint32_t i = 0; i < length; i += DMA_RESULT_BYTES) {
                adc_digi_output_data_t *r = (adc_digi_output_data_t *)&frame[i];
                if (r->type2.channel != POT_CHANNEL) {
                    continue;
                }

                fill_ptr[fill_idx] = r->type2.data;
                fill_idx++;

                // Transfer only when the buffer is full
                if (fill_idx >= TX_SAMPLES) {
                    esp_err_t uart_error = uhci_transmit(uhci, (uint8_t *)fill_ptr, sizeof(buffer_a));
                    if (uart_error == ESP_OK) {
                        // Switch buffer
                        fill_ptr = (fill_ptr == buffer_a) ? buffer_b : buffer_a;
                    } else {
                        uart_tx_busy = false;
                    }
                    fill_idx = 0;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY_MS));
    }
}