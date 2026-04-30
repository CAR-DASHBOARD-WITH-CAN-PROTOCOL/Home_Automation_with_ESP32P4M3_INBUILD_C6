#include <stdio.h>
#include "ui.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "lv_demos.h"
#include "header.h"
#include "screens.h"
// ─── NEW INCLUDES FOR UART ─────────────────────────────────
#include "driver/uart.h"
#include "driver/gpio.h"
#include <time.h>
#include <sys/time.h>


// ─── UART CONFIGURATION FOR P4 (INTERNAL ROUTING) ──────────
#define P4_UART_PORT      UART_NUM_1 
#define P4_UART_BAUD      115200
#define P4_UART_TX_PIN    15         // P4 TX routed to C6 RX (Pin 21)
#define P4_UART_RX_PIN    14         // P4 RX routed from C6 TX (Pin 20)
#define P4_UART_BUF_SIZE  1024

static const char *TAG = "P4_MAIN";
void p4_uart_init(void);
// ─── UART INITIALIZATION ───────────────────────────────────


static void apply_time_from_c6(const char *msg)
{
    // Expected format: "FXN:6,TIME:16:42:27,DATE:24/04/26,WDAY:6\n"
    int fxn, hour, min, sec, date, mon, year, wday;

    int parsed = sscanf(msg,
        "FXN:%d,TIME:%d:%d:%d,DATE:%d/%d/%d,WDAY:%d",
        &fxn, &hour, &min, &sec, &date, &mon, &year, &wday);

    if (parsed != 8) {
        printf("ERROR: Time parse failed, got %d fields\n", parsed);
        return;
    }

    // Build struct tm and set system time
    struct tm timeinfo = {0};  // inner struct which set time 
    timeinfo.tm_hour  = hour;
    timeinfo.tm_min   = min;
    timeinfo.tm_sec   = sec;
    timeinfo.tm_mday  = date;
    timeinfo.tm_mon   = mon - 1;       // tm_mon is 0-based
    timeinfo.tm_year  = year + 100;    // tm_year is years since 1900, year=26 means 2026
    timeinfo.tm_wday  = wday;

    time_t t = mktime(&timeinfo);
    if (t == -1) {
        printf("ERROR: mktime failed\n");
        return;
    }

    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&tv, NULL);

    printf("System time set → %02d:%02d:%02d %02d/%02d/20%02d\n",
           hour, min, sec, date, mon, year);
}



void p4_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate  = P4_UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_param_config(P4_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(P4_UART_PORT, P4_UART_TX_PIN, P4_UART_RX_PIN, 
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(P4_UART_PORT, P4_UART_BUF_SIZE, P4_UART_BUF_SIZE, 0, NULL, 0));
    
    ESP_LOGI(TAG, "Internal UART Listener initialized (TX=%d, RX=%d)", P4_UART_TX_PIN, P4_UART_RX_PIN);
}

// ─── UART LISTENER TASK ────────────────────────────────────
void c6_listener_task(void *arg)
{
    uint8_t rx_buffer[P4_UART_BUF_SIZE];

    while (1) {
        int rx_bytes = uart_read_bytes(P4_UART_PORT, rx_buffer,
                                       P4_UART_BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (rx_bytes > 0) {
            rx_buffer[rx_bytes] = '\0';
            ESP_LOGI(TAG, "← C6: %s", rx_buffer);

            if (strstr((char *)rx_buffer, "CONNECTION_SUCCESSFUL")) {
                ESP_LOGI(TAG, "C6 WiFi connected!");
            }
            else if (strstr((char *)rx_buffer, "WIFI_FAIL")) {
                ESP_LOGW(TAG, "C6 WiFi failed!");
            }
            else if (strstr((char *)rx_buffer, "C6_BOOTING")) {
                ESP_LOGI(TAG, "C6 booting...");
            }
            else if (strstr((char *)rx_buffer, "FXN:6")) {  // ← time message
                apply_time_from_c6((char *)rx_buffer);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

