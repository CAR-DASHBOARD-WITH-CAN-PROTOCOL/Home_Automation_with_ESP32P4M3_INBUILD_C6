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

static bool is_backlight_on = true;
static bool is_screensaver_active = false;
lv_obj_t *scr = NULL;
lv_obj_t *real_main_screen = NULL;
bool manual_screensaver_cooldown = false;

static void (*original_read_cb)(lv_indev_t *indev, lv_indev_data_t *data) = NULL;
void login_screen(void); // screen startup and checking credential of server.
void hide_settings_dropdown(void);
void manual_screen_off(void);
void ui_start(void);
void top_panel(void);
void mid_panel(void);
void lower_panel(void);
void save_buttons_to_file(void); // file data saving for dynamic button
void load_buttons_from_file(void);
void set_main_screen_ref(lv_obj_t *scr);
void hide_splash_screen(void);
bool check_login_status();
void screen_creation(void);
void location_ready_check(void);
void c6_listener_task(void *arg);


void set_screensaver_active(bool val)
{
    is_screensaver_active = val;
    manual_screensaver_cooldown = val;
}

bool check_login_status()
{
    nvs_handle_t my_handle;
    int32_t is_logged_in = 0; // Default to not logged in

    if (nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle) == ESP_OK)
    {
        nvs_get_i32(my_handle, "logged_in", &is_logged_in);
        nvs_close(my_handle);
    }
    printf("is logged in %d\n", is_logged_in);
    return (is_logged_in == 1);
    // return false;
}

// Save login status after successful HTTP response
void save_login_status(bool status)
{
    nvs_handle_t my_handle;
    if (nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle) == ESP_OK)
    {
        nvs_set_i32(my_handle, "logged_in", status ? 1 : 0);
        nvs_commit(my_handle);
        nvs_close(my_handle);
        printf("saved login status\n");
    }
}
// bool is_backlight_on = true;

// 2. Add this function so the UI can safely shut down the screen
void manual_screen_off(void)
{
    printf("Manually turning off backlight...\n");
    bsp_display_backlight_off();
    is_backlight_on = false;

    // Reset the inactivity timer so the touch driver doesn't instantly wake it up
    lv_display_trigger_activity(NULL);
}
// 2. Our custom interceptor function to fix the GT911 scaling for LVGL v9
static void custom_touchpad_read_wrapper(lv_indev_t *indev, lv_indev_data_t *data)
{
    // ONLY touch handling — nothing else!
    if (original_read_cb)
        original_read_cb(indev, data);

    if (data->state == LV_INDEV_STATE_PRESSED)
    {
        if (!is_backlight_on)
        {
            bsp_display_brightness_set(brightness_val);
            is_backlight_on = true;
            lv_display_trigger_activity(NULL);
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        }
    }
}

static void screen_saver_timer_cb(lv_timer_t *timer)
{
    uint32_t inactive_time = lv_display_get_inactive_time(NULL);

    // 1. AUTO-START SCREENSAVER (30 seconds)
     int time_saver=0;
     time_saver =  g_config.sleepT*1000;
    if (inactive_time >= time_saver && !is_screensaver_active)
    {
        hide_settings_dropdown();
        wohnux_main_ui_create();
        is_screensaver_active = true;
        manual_screensaver_cooldown = false; // No cooldown needed for auto
    }
    
    else if (is_screensaver_active && manual_screensaver_cooldown)
    {
        if (inactive_time > 1500)
        {
            manual_screensaver_cooldown = false;
        }
    }
    else if (is_screensaver_active && !manual_screensaver_cooldown)
    {
        if (inactive_time < 1000)
        {
            lv_display_trigger_activity(NULL);
            lv_scr_load(real_main_screen);
            is_screensaver_active = false;
        }
    }
}

void app_main(void)
{
    // Start display
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = true,
            .sw_rotate = false,
        }};
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();

       // ─── START UART COMMUNICATION WITH C6 ───
    p4_uart_init();
    
    // Create the listener task (4096 bytes stack is usually enough for UART parsing)
    xTaskCreate(c6_listener_task, "c6_listener", 4096, NULL, 5, NULL);

    init_spiffs();

    ui_start(); // show login screen immediately
    bsp_display_unlock();
}

void ui_start(void)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    if (indev)
    {
        original_read_cb = lv_indev_get_read_cb(indev);
        lv_indev_set_read_cb(indev, custom_touchpad_read_wrapper);
        printf("Touch wrapper injected!\n");
    }

    bsp_display_lock(0);
    lv_timer_create(screen_saver_timer_cb, 1000, NULL);
    scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    wohnux_main_ui_create();

    ui_init(); // initialise all objects first

    reset_mid_panel();
    reset_ac_widgets();
    reset_cct_widgets();
    reset_device_widgets();
    reset_rgb_widgets();
    
    screen_creation();

    set_main_screen_ref(scr);

    bsp_display_unlock();
    // Always show login screen — WebSocket validates credentials
}
