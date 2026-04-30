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
#include "esp_sntp.h"
#include "esp_websocket_client.h"
#include "header.h"
#include "cJSON.h"

// ============================================================
// GLOBALS
// ============================================================
static esp_websocket_client_handle_t ws_client = NULL;
static char ws_buffer[16384] = {0};  // clean power of 2
static int  ws_buf_len = 0;

// ─── SNTP ──────────────────────────────────────────────────
void initialize_sntp(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    setenv("TZ", "IST-5:30", 1);
    tzset();

    // Wait for time sync (max 20 seconds)
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    while (timeinfo.tm_year < (2020 - 1900) && retry < 10) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
        retry++;
        printf("Waiting for SNTP sync... (%d/10)\n", retry);
    }
    printf("Time synced: %s", asctime(&timeinfo));
}

// ─── JSON PARSER ───────────────────────────────────────────
static void parse_and_send_ws_message(const char *json_str)
{
    // Skip any leading non-JSON characters
    const char *json_start = strchr(json_str, '{');
    if (json_start == NULL) {
        printf("ERROR: No JSON object found\n");
        return;
    }

    cJSON *root = cJSON_Parse(json_start);
    if (root == NULL) {
        printf("ERROR: cJSON parse failed\n");
        return;
    }

    cJSON *data_array = cJSON_GetObjectItem(root, "data");
    if (!cJSON_IsArray(data_array)) {
        printf("ERROR: 'data' not found or not array\n");
        cJSON_Delete(root);
        return;
    }

    cJSON *item = cJSON_GetArrayItem(data_array, 0);
    if (item == NULL) {
        printf("ERROR: data array is empty\n");
        cJSON_Delete(root);
        return;
    }

    // Extract fxn first to decide what to do
    cJSON *fxn_obj = cJSON_GetObjectItem(item, "fxn");
    if (fxn_obj == NULL) {
        printf("ERROR: 'fxn' field missing\n");
        cJSON_Delete(root);
        return;
    }
    int fxn = fxn_obj->valueint;

    switch (fxn)
    {
        case 6: // Time sync message
        {
            int hour = cJSON_GetObjectItem(item, "hour") ? cJSON_GetObjectItem(item, "hour")->valueint : -1;
            int min  = cJSON_GetObjectItem(item, "min")  ? cJSON_GetObjectItem(item, "min")->valueint  : -1;
            int sec  = cJSON_GetObjectItem(item, "sec")  ? cJSON_GetObjectItem(item, "sec")->valueint  : -1;
            int date = cJSON_GetObjectItem(item, "date") ? cJSON_GetObjectItem(item, "date")->valueint : -1;
            int mon  = cJSON_GetObjectItem(item, "mon")  ? cJSON_GetObjectItem(item, "mon")->valueint  : -1;
            int year = cJSON_GetObjectItem(item, "year") ? cJSON_GetObjectItem(item, "year")->valueint : -1;
            int wday = cJSON_GetObjectItem(item, "wday") ? cJSON_GetObjectItem(item, "wday")->valueint : -1;

            printf("Time → %02d:%02d:%02d  Date → %02d/%02d/%02d  Wday → %d\n",
                   hour, min, sec, date, mon, year, wday);

            char msg[128];
            snprintf(msg, sizeof(msg),
                "FXN:%d,TIME:%02d:%02d:%02d,DATE:%02d/%02d/%02d,WDAY:%d\n",
                fxn, hour, min, sec, date, mon, year, wday);

            uart_data_send(msg);
            break;
        }

        default:
            printf("Unhandled fxn: %d\n", fxn);
            break;
    }

    cJSON_Delete(root);
}

// ─── WEBSOCKET EVENT HANDLER ────────────────────────────────
static void websocket_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        printf("WebSocket connected!\n");
        ws_buf_len = 0;
        ws_buffer[0] = '\0';
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        printf("WebSocket disconnected!\n");
        ws_buf_len = 0;
        ws_buffer[0] = '\0';
        break;

    case WEBSOCKET_EVENT_DATA:
        if (data->data_len <= 0)
            break;

        // Buffer overflow guard
        if (ws_buf_len + data->data_len >= (int)sizeof(ws_buffer) - 1) {
            printf("ERROR: ws_buffer overflow! Resetting.\n");
            ws_buf_len = 0;
            ws_buffer[0] = '\0';
            break;
        }

        // Accumulate data
        memcpy(ws_buffer + ws_buf_len, data->data_ptr, data->data_len);
        ws_buf_len += data->data_len;
        ws_buffer[ws_buf_len] = '\0';

        // Trim trailing whitespace to handle \n or \r after }
        while (ws_buf_len > 0 &&
               (ws_buffer[ws_buf_len - 1] == '\n' ||
                ws_buffer[ws_buf_len - 1] == '\r' ||
                ws_buffer[ws_buf_len - 1] == ' ')) {
            ws_buf_len--;
            ws_buffer[ws_buf_len] = '\0';
        }

        // Check if complete JSON message
        if (ws_buf_len > 0 && ws_buffer[ws_buf_len - 1] == '}') {
           // printf("WS RX: %s\n", ws_buffer);
            parse_and_send_ws_message(ws_buffer);
            ws_buf_len = 0;
            ws_buffer[0] = '\0';
        }
        break;

    case WEBSOCKET_EVENT_ERROR:
        printf("WebSocket error!\n");
        break;

    default:
        break;
    }
}

// ─── START WEBSOCKET ────────────────────────────────────────
static void start_websocket(void)
{
    esp_websocket_client_config_t ws_cfg = {
        .uri                  = WS_SERVER_URI,
        .reconnect_timeout_ms = 5000,
        .network_timeout_ms   = 10000,
    };

    ws_client = esp_websocket_client_init(&ws_cfg);
    esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY,
                                  websocket_event_handler, NULL);
    esp_websocket_client_start(ws_client);
    printf("WebSocket client started → %s\n", WS_SERVER_URI);
}

// ─── PUBLIC INIT ────────────────────────────────────────────
void socket_init_c6(void)
{
    initialize_sntp();
    start_websocket();
}