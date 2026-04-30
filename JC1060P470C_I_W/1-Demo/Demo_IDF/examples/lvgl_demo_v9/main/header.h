#ifndef HOME_SCREEN_H
#define HOME_SCREEN_H

#pragma once      // Prevents double inclusion
#include "lvgl.h" // ADD THIS LINE

//// macros

#define STORAGE_NAMESPACE "storage"
#define GLOBAL_TAG "LVGL_DISPLAY"
#define esp_logs(format, ...) ESP_LOGI(GLOBAL_TAG, format, ##__VA_ARGS__)

#define LV_INDEV_DEF_READ_PERIOD 20
#define LV_INDEV_DEF_DRAG_LIMIT 10
#define LV_INDEV_DEF_DRAG_THROW 10

#define MAX_AC_DATA 10

#define MAX_SCENES 30
#define MAX_SCENE_DEVICES 20
#define CURRENT_NVS_VERSION 2 // for erasing flash data

// #define MANUAL   TO Commend hardcoded widget and enable dynamic widgets
// #define PRINTFOFF   // for commenting debug statments
#define listen // for listening data from C6


// extra added img and font declarations
LV_FONT_DECLARE(clock_font_96);       // clock font 96
LV_FONT_DECLARE(clock_font_120_bold); // clock font 120
// Declare your external image arrays
LV_IMAGE_DECLARE(location);
LV_IMAGE_DECLARE(lightbulb_off);
LV_IMAGE_DECLARE(secenes); // Keeping your exact spelling!
LV_IMAGE_DECLARE(screen_off);
LV_IMAGE_DECLARE(settings32);
///////////////////////strutures

typedef struct
{
    // Buttons from your array
    bool welcome_button;
    bool bright_button;
    bool day_button;
    bool sleep_b;
    bool relax_button;
    bool dawn_button;
    bool ceiling_fan_button;
} button_config_t;

// Global instance with default settings (all visible)
extern button_config_t btn_cfg;

// data manager .c
// for login .csv
typedef struct
{
    int location_id;
    int floor_id;
    bool is_active;
    char name[32];
} location_t;

// for location .csv
typedef struct
{
    int device_id;
    int floor_id;
    int location_id;
    int device_type;
    bool dimmable;
    bool active;
    bool in_scenes;
    char name[32];
} device_t;

extern bool locations_ready;

// structure defining the way we receive AC unit data in json format

typedef struct
{
    int id;
    int slave_id;
    int location_id;
    int type;
    char base_addr[16];
    char name[32];
    bool active;
} ac_data_t;

// structure defining the way we receive scens data in json format
typedef struct
{
    int scene_id;
    int location_id;
    bool active;
    char name[16];
    int device_ids[MAX_SCENE_DEVICES];
    int device_vals[MAX_SCENE_DEVICES];
    int device_count;
} scene_t;

// select_order.h
#define MAX_ORDER_DEVICES 20

typedef struct {
    char     name[64];
    int      device_type;
    int      device_index;   // index in original device_list / ac_list
    bool     visible;        // checkbox state
    int      order;          // position 0=top
    lv_obj_t *widget_btn;    // pointer to the created button widget
    lv_obj_t *checkbox;
    lv_obj_t *label;
    lv_obj_t *up_label;
    lv_obj_t *down_label;
} order_item_t;


typedef struct {
    int id;
    int locId;
    int type;
    int inScenes;
    int dimmable;
    char name[64];
} csv_device_t;

typedef struct {
    int id;
    int locId;
    char scn_ic[32]; // e.g., "scn1013"
    char name[64];   // e.g., "Welcome"
} csv_scene_t;
#define MAX_CSV_DEVICES 100
#define MAX_CSV_SCENES 20
extern csv_device_t parsed_devices[MAX_CSV_DEVICES];
extern csv_scene_t parsed_scenes[MAX_CSV_SCENES];
// header.h

typedef struct {
    int  sleepT;
    int  location;
    int  showDevicesOnly;
    int  oldLcd;
    int  showData;
    int  childlock;
} app_config_t;


// Declare as extern so any .c file can use it
extern app_config_t g_config;


typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_current_t;


extern void open_select_order_panel(void);
extern void apply_order_to_mid_panel(void);
extern void populate_order_list(void);
extern void build_select_order_widgets(void);

////////////////////Variable declarations start////////////////////////////

extern lv_obj_t *tile_splash;
extern lv_obj_t *tileview;
extern lv_obj_t *real_main_screen;
extern int brightness_val;
extern bool is_data_received(void); // only declare function, not variable
extern bool first_login;
extern bool g_eth_connected;
extern bool g_ws_connected;
extern char dev_ip_bufer[50];
extern bool manual_screensaver_cooldown;

// for getting data for scenes csv  from server
extern int get_scene_count(void);
extern scene_t *get_scene(int index);
extern scene_t *get_scene_by_name_and_location(const char *name, int location_id);
extern void parse_scenes_csv(const char *csv);

// fpr gettomg data for ac csv from server
extern int get_ac_count(void);
extern ac_data_t *get_ac(int index);
extern void parse_ac_csv(const char *csv);      // for parsing data from socket for ac unit
extern void check_and_wipe_nvs_if_needed(void); // for earsing flash memory when first time boot
extern bool is_24h_format;

// ── Counters you likely have in your widget files ─────────────────
// (wherever your MAX_LAMP, MAX_AC etc counters live)
extern int ac_widget_count;
extern int lamp_widget_count;
extern int curtain_widget_count;
extern int fan_widget_count;
extern int projector_widget_count;
extern int cct_widget_count;
extern int total_parsed_devices;
extern int total_parsed_scenes;
extern int total_config_data;
extern  bool has_scenes;
////////////////////Variable declarations end ////////////////////////////

// functino declarations
extern void reset_mid_panel(void);
void wohnux_main_ui_create(void);
extern void top_panel(void);
extern void mid_panel(void);
extern void lower_panel(void);
extern void create_settings_screen(lv_obj_t *parent);
extern void create_home_buttons(lv_obj_t *parent);
extern void create_login_ui(void);
void save_login_status(bool status);
extern void create_splash_screen(lv_obj_t *parent);
extern void hide_settings_dropdown(void);
extern void manual_screen_off(void);
extern void ac_screen_init(void);
extern void set_main_screen_ref(lv_obj_t *scr);
extern void hide_splash_screen(void);
extern void cct_rgb_light(void);
extern void create_cct_widget(const char *cct_name, lv_obj_t *parent_panel);
extern void create_projector_widget(const char *proj_name, lv_obj_t *parent_panel);
extern void create_curtain_widget(const char *curtain_name, lv_obj_t *parent_panel);
extern void create_lamp_widget(const char *lamp_name, lv_obj_t *parent_panel);
extern void create_fan_widget(const char *fan_name, lv_obj_t *parent_panel);
extern void create_ac_widget(const char *ac_name, lv_obj_t *parent_panel);
void create_ac_widget(const char *ac_name, lv_obj_t *parent_panel);
extern void parse_locations_csv(const char *csv);
extern void parse_devices_csv(const char *csv);

extern int get_location_count(void);
extern location_t *get_location(int index);

extern int get_device_count(void);
extern device_t *get_device(int index);

extern void set_selected_location(int location_id);
extern int get_selected_location(void);

extern void ws_send_login(const char *login_id, const char *pass_key);
extern void ws_request_locations(void);
extern void ws_request_devices(void);
extern bool is_locations_ready(void);
extern void update_button_visibility_top_panel(void);

// Callbacks — implement these in your UI files
extern void on_websocket_connected(void);
extern void on_login_success(void);
extern void on_login_failed(void);
extern void on_locations_ready(void);
extern void on_devices_ready(void);
extern void set_devices_ready(bool val);
extern bool is_devices_ready(void);

// new project
extern void config_mac_phy_ether(void);
extern void set_locations_ready(bool val);
extern void save_selected_location_to_nvs(int location_id);
extern int load_selected_location_from_nvs(void);
bool check_login_status();
extern void create_ac_widget_big(const char *ac_name, lv_obj_t *parent_panel);
extern void create_cct_widget_big(const char *cct_name, lv_obj_t *parent_panel);
extern void create_projector_widget_big(const char *proj_name, lv_obj_t *parent_panel);
extern void create_curtain_widget_big(const char *curtain_name, lv_obj_t *parent_panel);
extern void create_lamp_widget_big(const char *lamp_name, lv_obj_t *parent_panel);
extern void create_fan_widget_big(const char *fan_name, lv_obj_t *parent_panel);
extern void screen_creation(void);
extern void location_ready_check(void);
extern void update_connection_status_ui(bool eth_connected, bool ws_connected);
extern void set_screensaver_active(bool val);   // ← add this

extern void reset_mid_panel(void);
extern void reset_ac_widgets(void);
extern void reset_cct_widgets(void);
extern void reset_device_widgets(void);
extern void p4_uart_init(void);
extern void c6_listener_task(void *arg);
extern void init_spiffs(void);
extern void create_ui_from_csv(void);
extern void turn_off_all_lights(void);
extern void turn_off_all_lights_cct(void);


extern void create_rgb_widget(const char *rgb_name, lv_obj_t *parent_panel);
extern void create_rgb_widget_big(const char *rgb_name, lv_obj_t *parent_panel);
extern void reset_rgb_widgets(void);
extern void rgbw_light_init(void);
extern void turn_off_rgbw_lights(void);


#endif