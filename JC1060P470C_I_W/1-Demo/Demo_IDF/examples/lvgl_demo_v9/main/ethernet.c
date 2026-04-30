
#include "esp_eth.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_eth_mac_esp.h"
#include "esp_http_client.h"
#include "esp_sntp.h"
#include "esp_websocket_client.h"
#include "header.h"
#include "screens.h"
#include "images.h"

#include <stdio.h>
#include "ui.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"
#include "lv_demos.h"

// ============================================================
// STATE MACHINE
// ============================================================
typedef enum
{
    WS_STATE_IDLE,
    WS_STATE_WAIT_LOGIN,
    WS_STATE_WAIT_LOCATIONS,
    WS_STATE_WAIT_DEVICES,
    WS_STATE_READY
} ws_state_t;

static ws_state_t ws_state = WS_STATE_IDLE;
char dev_ip_bufer[50] = {0};

// ============================================================
// GLOBALS
// ============================================================
static esp_websocket_client_handle_t ws_client = NULL;
static char ws_buffer[16384] = {0};
static int ws_buf_len = 0;
static char auth_token[64] = {0};

void set_selected_location(int location_id);
int get_selected_location(void);
void ws_send_login(const char *login_id, const char *pass_key);
void ws_request_locations(void);
void parse_locations_csv(const char *csv);
void ws_request_devices(void);
void set_locations_ready(bool val);

static int selected_location_id = -1;

bool locations_ready = false;
bool g_eth_connected = false;
bool g_ws_connected = false;

void set_locations_ready(bool val)
{
    locations_ready = val;
}

bool is_locations_ready(void)
{
    return locations_ready;
}

void set_selected_location(int location_id)
{
    selected_location_id = location_id;
    save_selected_location_to_nvs(location_id); // ← save immediately
    printf("Selected location: %d\n", location_id);
}

int get_selected_location(void)
{
    return selected_location_id;
}
// ============================================================
// PUBLIC API
// ============================================================
bool is_data_received(void)
{
    return (ws_state == WS_STATE_READY);
}

void ws_send_login(const char *login_id, const char *pass_key)
{
    char msg[128];
    snprintf(msg, sizeof(msg),
             "{\"data\":[{\"fxn\":508,\"loginId\":\"%s\",\"passKey\":\"%s\"}]}",
             login_id, pass_key);

    esp_websocket_client_send_text(ws_client, msg, strlen(msg), portMAX_DELAY);
    ws_state = WS_STATE_WAIT_LOGIN;
    printf("Login sent via WebSocket\n");
}

void ws_request_locations(void)
{
    const char *req = "{\"data\":[{\"fxn\":16,\"name\":\"locations.csv\"}]}";
    esp_websocket_client_send_text(ws_client, req, strlen(req), portMAX_DELAY);
    ws_state = WS_STATE_WAIT_LOCATIONS;
#ifdef PRINTFOFF
    printf("Requested locations.csv\n");
#endif
}

void ws_request_devices(void)
{
    const char *req_dev = "{\"data\":[{\"fxn\":16,\"name\":\"devices.csv\"}]}";
    esp_websocket_client_send_text(ws_client, req_dev, strlen(req_dev), portMAX_DELAY);

    const char *req_ac = "{\"data\":[{\"fxn\":16,\"name\":\"ac.csv\"}]}";
    esp_websocket_client_send_text(ws_client, req_ac, strlen(req_ac), portMAX_DELAY);

    const char *req_scenes = "{\"data\":[{\"fxn\":16,\"name\":\"scenes.csv\"}]}";
    esp_websocket_client_send_text(ws_client, req_scenes, strlen(req_scenes), portMAX_DELAY);

    ws_state = WS_STATE_WAIT_DEVICES;
#ifdef PRINTFOFF
    printf("Requested devices.csv and ac.csv and scenes\n");
#endif
}

// ============================================================
// PROCESS COMPLETE MESSAGE
// ============================================================
static void process_message(const char *msg)
{
    // ── fxn:508 LOGIN RESPONSE ──
    if (strstr(msg, "\"fxn\":508"))
    {
        if (strstr(msg, "\"err\":0") || strstr(msg, "\"err\":\"0\""))
        {
            printf("Login SUCCESS!\n");
            lv_obj_add_flag(objects.invalid_login_label, LV_OBJ_FLAG_HIDDEN);

            // Extract token
            char *tok = strstr(msg, "\"token\":\"");
            if (tok)
            {
                tok += 9;
                char *end = strchr(tok, '"');
                if (end)
                {
                    int len = end - tok;
                    strncpy(auth_token, tok, len);
                    auth_token[len] = '\0';
                    printf("Token: %s\n", auth_token);
                }
            }

            // Notify login screen — success
            //  on_login_success();

            // Request locations
            //  ws_request_locations();

            // Also check if we have a saved location
            int saved_loc = load_selected_location_from_nvs();
            if (saved_loc != -1)
            {
                printf("Found saved location: %d — skipping room selection\n", saved_loc);
                set_selected_location(saved_loc);
                ws_request_devices(); // go straight to devices
            }
            else
            {
                ws_request_locations(); // show room selection
            }
        }
        else
        {
            printf("Login FAILED!\n");
            lv_obj_clear_flag(objects.invalid_login_label, LV_OBJ_FLAG_HIDDEN);

            //  on_login_failed();
        }
        return;
    }

    // ── fxn:16 locations.csv ──
    if (strstr(msg, "locations.csv") && strstr(msg, "contents"))
    {
        printf("locations.csv received!\n");
        char *contents = strstr(msg, "\"contents\":\"");
        if (contents)
        {
            contents += 12;
            char *end = strrchr(contents, '"');
            if (end)
                *end = '\0';
            parse_locations_csv(contents);
            locations_ready = true; // ← just set flag, NO LVGL here
        }
        return;
    }
    // ── fxn:16 devices.csv ──
    if (strstr(msg, "devices.csv") && strstr(msg, "contents"))
    {
        printf("devices.csv received!\n");
        char *contents = strstr(msg, "\"contents\":\"");
        if (contents)
        {
            contents += 12;
            char *end = strrchr(contents, '"');
            if (end)
                *end = '\0';
            parse_devices_csv(contents);
        }
        return;
    }

    // ── fxn:16 ac.csv ──
    if (strstr(msg, "ac.csv") && strstr(msg, "contents"))
    {
        printf("ac.csv received!\n");
        char *contents = strstr(msg, "\"contents\":\"");
        if (contents)
        {
            contents += 12;
            char *end = strrchr(contents, '"');
            if (end)
                *end = '\0';
            parse_ac_csv(contents);
            ws_state = WS_STATE_READY;
            // Notify UI to build screen
            //  on_devices_ready();
        }
        return;
    }
    if (strstr(msg, "scenes.csv") && strstr(msg, "contents"))
    {
        printf("scenes.csv received!\n");
        char *contents = strstr(msg, "\"contents\":\"");
        if (contents)
        {
            contents += 12;
            char *end = strrchr(contents, '"');
            if (end)
                *end = '\0';
            parse_scenes_csv(contents);
            ws_state = WS_STATE_READY;
            // Notify UI to build screen
            //  on_devices_ready();
        }
        return;
    }
}

// ============================================================
// WEBSOCKET EVENT HANDLER
// ============================================================
static void websocket_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id)
    {

    case WEBSOCKET_EVENT_CONNECTED:
        printf("WebSocket connected!\n");
        ws_state = WS_STATE_IDLE;
        g_ws_connected = true; // ← flag only, no UI call
        on_websocket_connected();
        if (check_login_status())
        {
            printf("Auto login from NVS\n");
            ws_send_login("Admin", "Compac");
        }
        break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        printf("WebSocket disconnected!\n");
        g_ws_connected = false; // ← flag only, no UI call
        ws_state = WS_STATE_IDLE;
        break;

    case WEBSOCKET_EVENT_DATA:
        if (data->data_len > 0)
        {
            if (ws_buf_len + data->data_len < sizeof(ws_buffer) - 1)
            {
                memcpy(ws_buffer + ws_buf_len, data->data_ptr, data->data_len);
                ws_buf_len += data->data_len;
                ws_buffer[ws_buf_len] = '\0';
            }
            else
            {
                printf("ERROR: ws_buffer overflow!\n");
                ws_buf_len = 0;
                ws_buffer[0] = '\0';
                break;
            }

            // Check if message complete
            if (ws_buffer[ws_buf_len - 1] == '}')
            {
                process_message(ws_buffer);
                ws_buf_len = 0;
                ws_buffer[0] = '\0';
            }
        }
        break;

    case WEBSOCKET_EVENT_ERROR:
        printf("WebSocket error!\n");
        break;

    default:
        break;
    }
}

// ============================================================
// START WEBSOCKET
// ============================================================
static void start_websocket(void)
{
    esp_websocket_client_config_t ws_cfg = {
        .uri = "ws://192.168.29.230:3031",
        .reconnect_timeout_ms = 5000,
        .network_timeout_ms = 10000,
    };

    ws_client = esp_websocket_client_init(&ws_cfg);
    esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY,
                                  websocket_event_handler, NULL);
    esp_websocket_client_start(ws_client);
    printf("WebSocket client started\n");
    // WebSocket connected

    // create_login_ui();
}

// ============================================================
// SNTP
// ============================================================
void initialize_sntp(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    setenv("TZ", "IST-5:30", 1);
    tzset();
}

// ============================================================
// GOT IP
// ============================================================
// got_ip_event_handler
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

    printf("Got IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));

    snprintf(dev_ip_bufer, sizeof(dev_ip_bufer), IPSTR, IP2STR(&event->ip_info.ip));
    

    g_eth_connected = true; // ← flag only, no UI call
    initialize_sntp();
    start_websocket();
}

// eth_event_handler
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        printf("Ethernet cable connected!\n");
        break;

    case ETHERNET_EVENT_DISCONNECTED:
        printf("Ethernet cable disconnected!\n");
        g_eth_connected = false; // ← flag only
        g_ws_connected = false;  // ← flag only
        break;

    case ETHERNET_EVENT_START:
        printf("Ethernet started\n");
        break;

    case ETHERNET_EVENT_STOP:
        printf("Ethernet stopped\n");
        g_eth_connected = false; // ← flag only
        g_ws_connected = false;  // ← flag only
        break;

    default:
        break;
    }
}

// ============================================================
// ETHERNET SETUP
// ============================================================
void config_mac_phy_ether(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);

    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);

    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 1;
    phy_config.reset_gpio_num = -1;
    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                               &got_ip_event_handler, NULL));

    // ── ETH event — cable plug/unplug ────────────────────────────────────
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &eth_event_handler,
                                               NULL));
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}
