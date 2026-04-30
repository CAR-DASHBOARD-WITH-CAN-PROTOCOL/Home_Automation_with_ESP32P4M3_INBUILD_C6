idf_component_register(
    SRCS "main.c" "socket.c"
    PRIV_REQUIRES 
        nvs_flash 
        esp_event 
        esp_netif 
        esp_wifi 
        driver 
        esp_driver_uart
        esp_websocket_client
        json                    # ← cJSON lives here
    INCLUDE_DIRS "."
)