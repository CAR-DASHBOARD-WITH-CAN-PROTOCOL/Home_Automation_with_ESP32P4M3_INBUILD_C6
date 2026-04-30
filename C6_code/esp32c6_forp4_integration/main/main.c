#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "header.h"

// ─── CONFIG ────────────────────────────────────────────────
#define WIFI_SSID       "Wohnux"
#define WIFI_PASSWORD   "Wohnux@427"
#define WIFI_MAX_RETRY  5

#define UART_PORT       UART_NUM_1        // ← NOT UART_NUM_0
#define UART_BAUD       115200
#define UART_TX_PIN     20
#define UART_RX_PIN     21
#define UART_BUF_SIZE   1024

static const char *TAG = "C6_MAIN";

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int retry_count = 0;

// ─── UART ──────────────────────────────────────────────────
static void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate  = UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,   // ← required for C6
    };
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0);
    ESP_LOGI(TAG, "UART initialized TX=%d RX=%d", UART_TX_PIN, UART_RX_PIN);
}

// Public — called from csix.c via header.h
void uart_data_send(const char *msg)
{
    uart_write_bytes(UART_PORT, msg, strlen(msg));
    ESP_LOGI(TAG, "→ P4: %s", msg);
}

static void uart_send_to_p4(const char *msg)
{
    uart_write_bytes(UART_PORT, msg, strlen(msg));
    uart_write_bytes(UART_PORT, "\n", 1);
    ESP_LOGI(TAG, "→ P4: %s", msg);
}

// ─── WIFI ──────────────────────────────────────────────────
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGI(TAG, "Retrying WiFi (%d/%d)", retry_count, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            uart_send_to_p4("WIFI_FAIL");
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        char ip_str[32];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", ip_str);
        retry_count = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);

        char msg[64];
        snprintf(msg, sizeof(msg), "CONNECTION_SUCCESSFUL|IP:%s\n", ip_str);
        uart_send_to_p4(msg);
    }
}

static bool wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg  = { .capable = true, .required = false },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE, pdMS_TO_TICKS(15000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected!");
        return true;
    }
    ESP_LOGE(TAG, "WiFi failed");
    return false;
}

// ─── UART RX TASK ──────────────────────────────────────────
static void uart_rx_task(void *arg)
{
    uint8_t buf[UART_BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_PORT, buf,
                                  UART_BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            buf[len] = '\0';
            ESP_LOGI(TAG, "← P4: %s", (char *)buf);

            if (strstr((char *)buf, "PING")) {
                uart_send_to_p4("PONG");
            }
            else if (strstr((char *)buf, "GET_STATUS")) {
                uart_send_to_p4("STATUS:WIFI_OK");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ─── APP MAIN ──────────────────────────────────────────────
void app_main(void)
{
    printf("C6 starting...\n");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    uart_init();
    uart_send_to_p4("C6_BOOTING");

    ESP_LOGI(TAG, "Connecting to WiFi: %s", WIFI_SSID);
    uart_send_to_p4("WIFI_CONNECTING");

    bool connected = wifi_init_sta();
    if (!connected) {
        uart_send_to_p4("WIFI_FAILED");
        // Don't start websocket if no WiFi
        xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL, 5, NULL);
        while (1) vTaskDelay(pdMS_TO_TICKS(10000));
    }

    socket_init_c6();   // ← only starts after WiFi confirmed

    xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL, 5, NULL);

}