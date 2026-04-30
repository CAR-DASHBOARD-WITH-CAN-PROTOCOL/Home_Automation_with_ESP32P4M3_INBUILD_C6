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
#include "images.h"

// ─── 1. STRUCTURE TO HOLD CSV DATA IN MEMORY ─────────────────
#include <stdio.h>
#include <string.h>
#include "esp_spiffs.h"

// ─── 1. STRUCTURES TO HOLD CSV DATA IN MEMORY ────────────────


// ============================================================================
// GLOBALS — other devices
// ============================================================================
bool projector_current_state = false;
bool table_lamp_current_state = false;
#define MAX_FAN 10
#define MAX_PROJECTOR 10
#define MAX_LAMP 10
#define MAX_CURTAIN 10
// ============================================================================
// STRUCT
// ============================================================================
typedef struct // fan
{
    int id;
    bool is_on;
    int current_speed;
    char name[32];

    lv_obj_t *ceiling_fan_button;            // root widget
    lv_obj_t *ceiling_fan_speed_slider;      // child 0 of root — arc
    lv_obj_t *ceiling_fan_img;               // child 0 of slider — image (nested inside arc!)
    lv_obj_t *ceiling_fan_speed_label;       // child 1 of root — label
    lv_obj_t *ceiling_fan_minus_s_button;    // child 2 of root — btn
    lv_obj_t *ceiling_fan_minus_speed_label; // child 0 of minus btn
    lv_obj_t *ceiling_fan_plus_s_button;     // child 3 of root — btn
    lv_obj_t *ceiling_fan_plus_speed_label;  // child 0 of plus btn
    lv_obj_t *ceiling_fan_label;             // child 4 of root — label
} fan_device_t;

// ============================================================================
// GLOBALS — fan
// ============================================================================

static fan_device_t my_fans[MAX_FAN];
static int fan_count = 0;

// ============================================================================
// PROJECTOR STRUCT
// ============================================================================
typedef struct
{ // projector
    int id;
    bool is_on;
    int current_level;
    char name[32];

    lv_obj_t *projector_button;         // root
    lv_obj_t *project_slider_cum_light; // child 0 of root — arc
    lv_obj_t *projector_img;            // child 0 of arc — image (nested!)
    lv_obj_t *projector_la;             // child 1 of root — value label
    lv_obj_t *projector_label;          // child 2 of root — name label
} projector_device_t;

// ============================================================================
// TABLE LAMP STRUCT
// ===================== =======================================================
typedef struct
{ // table lamp
    int id;
    bool is_on;
    int current_level;
    char name[32];

    lv_obj_t *table_lamp_button;    // root
    lv_obj_t *table_lamp_arc;       // child 0 of root — arc
    lv_obj_t *table_lamp_img;       // child 0 of arc (nested)
    lv_obj_t *table_lamp_lab_on_ff; // child 1 of root — value label
    lv_obj_t *table_lamp_label;     // child 2 of root — name label
} lamp_device_t;

// ============================================================================
// CURTAIN STRUCT
// ============================================================================
typedef struct
{ // curtain
    int id;
    char name[32];

    lv_obj_t *curtain_button;                    // root
    lv_obj_t *curtain_movement_stop_img_cum_but; // 0
    lv_obj_t *curtain_label;                     // 1
    lv_obj_t *dumbyarc_curt_1_open;         // 2
    lv_obj_t *curtain_img_cum_but_opener;        // 2->0

    lv_obj_t *dumarch_curt_2_close;    // 3
    lv_obj_t *curtain_img_cum_but_closer; // 3->0

    // lv_obj_t *curtain_img_cum_but_opener;        // child 0 — open image
    // lv_obj_t *curtain_img_cum_but_closer;        // child 1 — close image
    // lv_obj_t *curtain_movement_stop_img_cum_but; // child 2 — stop image
    // lv_obj_t *curtain_label;                     // child 3 — name label
} curtain_device_t;

// ============================================================================
// GLOBALS
// ============================================================================

static projector_device_t my_projectors[MAX_PROJECTOR];
static int projector_count = 0;

static lamp_device_t my_lamps[MAX_LAMP];
static int lamp_count = 0;

static curtain_device_t my_curtains[MAX_CURTAIN];
static int curtain_count = 0;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
void fan_screen_init(void);
void table_lamp(void);
void projector(void);
void curtain(void);

void create_projector_widget(const char *proj_name, lv_obj_t *parent_panel);
void create_curtain_widget(const char *curtain_name, lv_obj_t *parent_panel);
void create_lamp_widget(const char *lamp_name, lv_obj_t *parent_panel);
void create_fan_widget(const char *fan_name, lv_obj_t *parent_panel);

void create_projector_widget_big(const char *proj_name, lv_obj_t *parent_panel);
void create_curtain_widget_big(const char *curtain_name, lv_obj_t *parent_panel);
void create_lamp_widget_big(const char *lamp_name, lv_obj_t *parent_panel);
void create_fan_widget_big(const char *fan_name, lv_obj_t *parent_panel);
void turn_off_all_lights(void);// to turn off all lights


static void room_selected_cb(lv_event_t *e)
{
    int location_id = (int)(intptr_t)lv_event_get_user_data(e);
    printf("Room selected! location_id = %d\n", location_id);

    set_selected_location(location_id);

    lv_obj_add_flag(objects.room_selection_screen, LV_OBJ_FLAG_HIDDEN);

    ws_request_devices();
}

// ============================================================================
// PROJECTOR EVENT HANDLER
// ============================================================================
static void projector_event_handler(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    projector_device_t *proj = (projector_device_t *)lv_event_get_user_data(e);
    char buf[8];

    if (target == proj->projector_img)
    {
        proj->is_on = !proj->is_on;
        proj->current_level = proj->is_on ? 5 : 0;
        lv_label_set_text(proj->projector_la, proj->is_on ? "ON" : "OFF");
        lv_arc_set_value(proj->project_slider_cum_light, proj->current_level);
        printf("Projector [%s] %s\n", proj->name, proj->is_on ? "ON" : "OFF");
    }
    else if (target == proj->project_slider_cum_light)
    {
        proj->current_level = lv_arc_get_value(proj->project_slider_cum_light);
        lv_snprintf(buf, sizeof(buf), "%d", proj->current_level);
        lv_label_set_text(proj->projector_la, buf);
    }
}

// ============================================================================
// TABLE LAMP EVENT HANDLER
// ============================================================================
static void table_lamp_event_handler(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    lamp_device_t *lamp = (lamp_device_t *)lv_event_get_user_data(e);
    char buf[8];

    if (target == lamp->table_lamp_img)
    {
        lamp->is_on = !lamp->is_on;
        lamp->current_level = lamp->is_on ? 5 : 0;
        lv_label_set_text(lamp->table_lamp_lab_on_ff, lamp->is_on ? "ON" : "OFF");
        lv_arc_set_value(lamp->table_lamp_arc, lamp->current_level);
        printf("Table lamp [%s] %s\n", lamp->name, lamp->is_on ? "ON" : "OFF");
    }

    // table lamp has no image — arc itself is the toggle
    else if (target == lamp->table_lamp_arc)
    {
        lamp->current_level = lv_arc_get_value(lamp->table_lamp_arc);
        lamp->is_on = lamp->current_level > 0;
        lv_snprintf(buf, sizeof(buf), "%d", lamp->current_level);
        lv_label_set_text(lamp->table_lamp_lab_on_ff,
                          lamp->current_level > 0 ? buf : "OFF");
        printf("Lamp [%s] level=%d\n", lamp->name, lamp->current_level);
    }
}

// ============================================================================
// CURTAIN EVENT HANDLER
// ============================================================================
// static void curtain_event_handler(lv_event_t *e)
// {
//     lv_obj_t *target = lv_event_get_target(e);
//     curtain_device_t *curtain = (curtain_device_t *)lv_event_get_user_data(e);

//     if (target == curtain->curtain_img_cum_but_opener)
//     {
//         lv_obj_clear_state(curtain->curtain_img_cum_but_closer, LV_STATE_CHECKED);
//         lv_obj_invalidate(curtain->curtain_img_cum_but_closer);
//         printf("Curtain [%s] OPEN\n", curtain->name);
//         lv_obj_set_style_bg_color(curtain->dumbyarc_curt_1_open, lv_color_hex(0xffba00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
//         lv_obj_set_style_bg_color(curtain->dumbyarc_curt_1_open, lv_color_hex(0xffba00), LV_PART_KNOB | LV_STATE_DEFAULT);

//     }
//     else if (target == curtain->curtain_img_cum_but_closer)
//     {
//         lv_obj_clear_state(curtain->curtain_img_cum_but_opener, LV_STATE_CHECKED);
//         lv_obj_invalidate(curtain->curtain_img_cum_but_opener);
//         printf("Curtain [%s] CLOSE\n", curtain->name);

//         lv_obj_set_style_bg_color(curtain->dumarch_curt_2_close, lv_color_hex(0xffba00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
//         lv_obj_set_style_bg_color(curtain->dumarch_curt_2_close, lv_color_hex(0xffba00), LV_PART_KNOB | LV_STATE_DEFAULT);

//     }
//     else if (target == curtain->curtain_movement_stop_img_cum_but)
//     {
//         lv_obj_clear_state(curtain->curtain_img_cum_but_opener, LV_STATE_CHECKED);
//         lv_obj_clear_state(curtain->curtain_img_cum_but_closer, LV_STATE_CHECKED);
//         lv_obj_invalidate(curtain->curtain_img_cum_but_opener);
//         lv_obj_invalidate(curtain->curtain_img_cum_but_closer);
//         printf("Curtain [%s] STOP\n", curtain->name);
//     }
// }

static void curtain_event_handler(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    curtain_device_t *curtain = (curtain_device_t *)lv_event_get_user_data(e);

    // Define the active and inactive colors once
    lv_color_t active_color   = lv_color_hex(0xFFBA00);
    lv_color_t inactive_color = lv_color_hex(0x818499); // grey = inactive

    if (target == curtain->curtain_img_cum_but_opener)
    {
        // Deactivate close button and its arc
        lv_obj_clear_state(curtain->curtain_img_cum_but_closer, LV_STATE_CHECKED);
        lv_obj_invalidate(curtain->curtain_img_cum_but_closer);

        // Reset close arc to grey
        lv_obj_set_style_bg_color(curtain->dumarch_curt_2_close,
                                  inactive_color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(curtain->dumarch_curt_2_close,
                                  inactive_color, LV_PART_KNOB | LV_STATE_DEFAULT);

        // Activate open arc to yellow
        lv_obj_set_style_bg_color(curtain->dumbyarc_curt_1_open,
                                  active_color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(curtain->dumbyarc_curt_1_open,
                                  active_color, LV_PART_KNOB | LV_STATE_DEFAULT);

        printf("Curtain [%s] OPEN\n", curtain->name);
    }
    else if (target == curtain->curtain_img_cum_but_closer)
    {
        // Deactivate open button and its arc
        lv_obj_clear_state(curtain->curtain_img_cum_but_opener, LV_STATE_CHECKED);
        lv_obj_invalidate(curtain->curtain_img_cum_but_opener);

        // Reset open arc to grey
        lv_obj_set_style_bg_color(curtain->dumbyarc_curt_1_open,
                                  inactive_color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(curtain->dumbyarc_curt_1_open,
                                  inactive_color, LV_PART_KNOB | LV_STATE_DEFAULT);

        // Activate close arc to yellow
        lv_obj_set_style_bg_color(curtain->dumarch_curt_2_close,
                                  active_color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(curtain->dumarch_curt_2_close,
                                  active_color, LV_PART_KNOB | LV_STATE_DEFAULT);

        printf("Curtain [%s] CLOSE\n", curtain->name);
    }
    else if (target == curtain->curtain_movement_stop_img_cum_but)
    {
        // Deactivate both buttons and reset BOTH arcs to grey
        lv_obj_clear_state(curtain->curtain_img_cum_but_opener, LV_STATE_CHECKED);
        lv_obj_clear_state(curtain->curtain_img_cum_but_closer, LV_STATE_CHECKED);
        lv_obj_invalidate(curtain->curtain_img_cum_but_opener);
        lv_obj_invalidate(curtain->curtain_img_cum_but_closer);

        lv_obj_set_style_bg_color(curtain->dumbyarc_curt_1_open,
                                  inactive_color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(curtain->dumbyarc_curt_1_open,
                                  inactive_color, LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(curtain->dumarch_curt_2_close,
                                  inactive_color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(curtain->dumarch_curt_2_close,
                                  inactive_color, LV_PART_KNOB | LV_STATE_DEFAULT);

        printf("Curtain [%s] STOP\n", curtain->name);
    }
}
// ============================================================================
// CREATE PROJECTOR WIDGET
// ============================================================================
void create_projector_widget(const char *proj_name, lv_obj_t *parent_panel)
{
    if (projector_count >= MAX_PROJECTOR)
    {
        printf("ERROR: MAX_PROJECTOR reached!\n");
        return;
    }

    projector_device_t *proj = &my_projectors[projector_count];
    proj->id = projector_count;
    proj->is_on = false;
    proj->current_level = 0;
    lv_snprintf(proj->name, sizeof(proj->name), "%s", proj_name);

    create_user_widget_projector_button(parent_panel, NULL, 0);
    uint32_t total = lv_obj_get_child_cnt(parent_panel);
    proj->projector_button = lv_obj_get_child(parent_panel, total - 1);

    // Map children
    proj->project_slider_cum_light = lv_obj_get_child(proj->projector_button, 0); // arc
    proj->projector_img = lv_obj_get_child(proj->project_slider_cum_light, 0);    // image inside arc!
    proj->projector_la = lv_obj_get_child(proj->projector_button, 1);             // value label
    proj->projector_label = lv_obj_get_child(proj->projector_button, 2);          // name label

    // Initial values
    lv_arc_set_range(proj->project_slider_cum_light, 0, 5);
    lv_arc_set_value(proj->project_slider_cum_light, 0);
    lv_label_set_text(proj->projector_label, proj->name);
    lv_label_set_text(proj->projector_la, "OFF");

    // Bind events
    lv_obj_add_event_cb(proj->projector_img,
                        projector_event_handler, LV_EVENT_CLICKED, proj);
    lv_obj_add_event_cb(proj->project_slider_cum_light,
                        projector_event_handler, LV_EVENT_VALUE_CHANGED, proj);

    projector_count++;
    printf("Projector widget created: [%s] id=%d\n", proj->name, proj->id);
}

// ============================================================================
// CREATE TABLE LAMP WIDGET
// ============================================================================
void create_lamp_widget(const char *lamp_name, lv_obj_t *parent_panel)
{
    if (lamp_count >= MAX_LAMP)
    {
        printf("ERROR: MAX_LAMP reached!\n");
        return;
    }

    lamp_device_t *lamp = &my_lamps[lamp_count];
    lamp->id = lamp_count;
    lamp->is_on = false;
    lamp->current_level = 0;
    lv_snprintf(lamp->name, sizeof(lamp->name), "%s", lamp_name);

    create_user_widget_table_lamp_button(parent_panel, NULL, 0);
    uint32_t total = lv_obj_get_child_cnt(parent_panel);
    lamp->table_lamp_button = lv_obj_get_child(parent_panel, total - 1);

    // Map children
    lamp->table_lamp_arc = lv_obj_get_child(lamp->table_lamp_button, 0);       // arc
    lamp->table_lamp_img = lv_obj_get_child(lamp->table_lamp_arc, 0);          // child inside child of arc
    lamp->table_lamp_lab_on_ff = lv_obj_get_child(lamp->table_lamp_button, 1); // value label
    lamp->table_lamp_label = lv_obj_get_child(lamp->table_lamp_button, 2);     // name label

    // Initial values
    lv_arc_set_range(lamp->table_lamp_arc, 0, 5);
    lv_arc_set_value(lamp->table_lamp_arc, 0);
    lv_label_set_text(lamp->table_lamp_label, lamp->name);
    lv_label_set_text(lamp->table_lamp_lab_on_ff, "OFF");

    // Bind events
    lv_obj_add_event_cb(lamp->table_lamp_img, table_lamp_event_handler, LV_EVENT_CLICKED, lamp);
    lv_obj_add_event_cb(lamp->table_lamp_arc, table_lamp_event_handler, LV_EVENT_VALUE_CHANGED, lamp);

    lamp_count++;
    printf("Lamp widget created: [%s] id=%d\n", lamp->name, lamp->id);
}

// ============================================================================
// CREATE CURTAIN WIDGET
// ============================================================================

void create_curtain_widget(const char *curtain_name, lv_obj_t *parent_panel)
{
    if (curtain_count >= MAX_CURTAIN)
    {
        printf("ERROR: MAX_CURTAIN reached!\n");
        return;
    }

    curtain_device_t *curtain = &my_curtains[curtain_count];
    curtain->id = curtain_count;
    lv_snprintf(curtain->name, sizeof(curtain->name), "%s", curtain_name);

    create_user_widget_curtain_button(parent_panel, NULL, 0);
    uint32_t total = lv_obj_get_child_cnt(parent_panel);
    curtain->curtain_button = lv_obj_get_child(parent_panel, total - 1);

    // Map children

    curtain->curtain_movement_stop_img_cum_but = lv_obj_get_child(curtain->curtain_button, 0);
    curtain->curtain_label = lv_obj_get_child(curtain->curtain_button, 1);
    curtain->curtain_img_cum_but_opener = lv_obj_get_child(curtain->curtain_button, 2); //
    curtain->curtain_img_cum_but_closer = lv_obj_get_child(curtain->curtain_button, 3); //
    curtain->dumbyarc_curt_1_open = lv_obj_get_child(curtain->curtain_button, 4);
    curtain->dumarch_curt_2_close = lv_obj_get_child(curtain->curtain_button, 5);


    // Initial values
    lv_label_set_text(curtain->curtain_label, curtain->name);

    // Bind events
    lv_obj_add_event_cb(curtain->curtain_img_cum_but_opener,
                        curtain_event_handler, LV_EVENT_VALUE_CHANGED, curtain);
    lv_obj_add_event_cb(curtain->curtain_img_cum_but_closer,
                        curtain_event_handler, LV_EVENT_VALUE_CHANGED, curtain);
    lv_obj_add_event_cb(curtain->curtain_movement_stop_img_cum_but,
                        curtain_event_handler, LV_EVENT_CLICKED, curtain);

    curtain_count++;
    printf("Curtain widget created: [%s] id=%d\n", curtain->name, curtain->id);
}

// ============================================================================
// INIT — call from mid_panel()
// ============================================================================
void projector_screen_init(void)
{
    create_projector_widget("Projector 1", objects.mid_panel);
    create_projector_widget("Projector 2", objects.mid_panel);
    create_projector_widget("Projector 3", objects.mid_panel);
}

void lamp_screen_init(void)
{
    create_lamp_widget("Table Lamp 1", objects.mid_panel);
    create_lamp_widget("Table Lamp 2", objects.mid_panel);
    create_lamp_widget("Table Lamp 3", objects.mid_panel);
}

void curtain_screen_init(void)
{
    create_curtain_widget("Curtain 1", objects.mid_panel);
    create_curtain_widget("Curtain 2", objects.mid_panel);
    create_curtain_widget("Curtain 3", objects.mid_panel);
}

// ============================================================================
// FAN EVENT HANDLER
// ============================================================================
static void ceiling_fan_event_handler(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    fan_device_t *fan = (fan_device_t *)lv_event_get_user_data(e);
    char buf[8];

    if (target == fan->ceiling_fan_plus_s_button)
    {
        if (fan->current_speed < 6)
            fan->current_speed++;
    }
    else if (target == fan->ceiling_fan_minus_s_button)
    {
        if (fan->current_speed > 0)
            fan->current_speed--;
    }
    else if (target == fan->ceiling_fan_speed_slider)
    {
        fan->current_speed = lv_arc_get_value(fan->ceiling_fan_speed_slider);
    }
    else if (target == fan->ceiling_fan_img)
    {
        fan->is_on = !fan->is_on;
        fan->current_speed = fan->is_on ? 1 : 0;
        printf("Fan [%s] %s\n", fan->name, fan->is_on ? "ON" : "OFF");
    }

    // Sync UI
    lv_snprintf(buf, sizeof(buf), "%d", fan->current_speed);
    lv_arc_set_value(fan->ceiling_fan_speed_slider, fan->current_speed);
    if (fan->current_speed == 0)
    {
        lv_label_set_text(fan->ceiling_fan_speed_label, " ");
        lv_arc_set_value(fan->ceiling_fan_speed_slider, 0);
    }
    else
    {
        lv_label_set_text(fan->ceiling_fan_speed_label, buf);
        lv_arc_set_value(fan->ceiling_fan_speed_slider, fan->current_speed);
    }
    // Sync fan img checked state based on speed
    if (fan->current_speed > 0)
        lv_obj_add_state(fan->ceiling_fan_img, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(fan->ceiling_fan_img, LV_STATE_CHECKED);
    lv_obj_invalidate(fan->ceiling_fan_img);
}

void reset_device_widgets(void)
{
    fan_count = 0;
    projector_count = 0;
    lamp_count = 0;
    curtain_count = 0;
    printf("Device widgets reset OK (fan/lamp/curtain/projector = 0)\n");
}

// ============================================================================
// mid_panel — main entry point
// ============================================================================
void mid_panel(void)
{

    ac_screen_init();
    
#ifdef MANUAL

    lamp_screen_init();
    fan_screen_init();
    lamp_screen_init();
    projector_screen_init();
    curtain_screen_init();

#endif
}

// ============================================================================
// update_visibility
// ============================================================================


// ============================================================================
// fan_screen_init — create fan widgets
// ============================================================================
void fan_screen_init(void)
{
    create_fan_widget("Ceiling Fan 1", objects.mid_panel);
    create_fan_widget("Ceiling Fan 2", objects.mid_panel);
    create_fan_widget("Ceiling Fan 3", objects.mid_panel);
}

void create_fan_widget(const char *fan_name, lv_obj_t *parent_panel)
{
    if (fan_count >= MAX_FAN)
    {
        printf("ERROR: MAX_FAN limit reached!\n");
        return;
    }

    fan_device_t *fan = &my_fans[fan_count];
    fan->id = fan_count;
    fan->is_on = false;
    fan->current_speed = 0;
    lv_snprintf(fan->name, sizeof(fan->name), "%s", fan_name);

    // Stamp widget into mid_panel
    create_user_widget_ceiling_fan_button(parent_panel, NULL, 0);
    uint32_t total = lv_obj_get_child_cnt(parent_panel);
    fan->ceiling_fan_button = lv_obj_get_child(parent_panel, total - 1);

    // Map children — based on EEZ widget tree
    fan->ceiling_fan_speed_slider = lv_obj_get_child(fan->ceiling_fan_button, 0);
    fan->ceiling_fan_img = lv_obj_get_child(fan->ceiling_fan_speed_slider, 0); // nested inside arc!
    fan->ceiling_fan_speed_label = lv_obj_get_child(fan->ceiling_fan_button, 1);
    fan->ceiling_fan_minus_s_button = lv_obj_get_child(fan->ceiling_fan_button, 2);
    fan->ceiling_fan_minus_speed_label = lv_obj_get_child(fan->ceiling_fan_minus_s_button, 0);
    fan->ceiling_fan_plus_s_button = lv_obj_get_child(fan->ceiling_fan_button, 3);
    fan->ceiling_fan_plus_speed_label = lv_obj_get_child(fan->ceiling_fan_plus_s_button, 0);
    fan->ceiling_fan_label = lv_obj_get_child(fan->ceiling_fan_button, 4);

    // Initial values
    lv_arc_set_range(fan->ceiling_fan_speed_slider, 0, 6);
    lv_arc_set_value(fan->ceiling_fan_speed_slider, fan->current_speed);
    lv_label_set_text(fan->ceiling_fan_label, fan->name);
    lv_label_set_text(fan->ceiling_fan_speed_label, " ");

    // Bind events
    lv_obj_add_event_cb(fan->ceiling_fan_plus_s_button, ceiling_fan_event_handler,
                        LV_EVENT_CLICKED, fan);
    lv_obj_add_event_cb(fan->ceiling_fan_minus_s_button, ceiling_fan_event_handler,
                        LV_EVENT_CLICKED, fan);
    lv_obj_add_event_cb(fan->ceiling_fan_speed_slider, ceiling_fan_event_handler,
                        LV_EVENT_VALUE_CHANGED, fan);
    lv_obj_add_event_cb(fan->ceiling_fan_img, ceiling_fan_event_handler,
                        LV_EVENT_CLICKED, fan);

    fan_count++;
    printf("Fan widget created OK: [%s] id=%d\n", fan->name, fan->id);
}

void create_projector_widget_big(const char *proj_name, lv_obj_t *parent_panel);
void create_curtain_widget_big(const char *curtain_name, lv_obj_t *parent_panel);
void create_lamp_widget_big(const char *lamp_name, lv_obj_t *parent_panel);
void create_fan_widget_big(const char *fan_name, lv_obj_t *parent_panel)
{
    if (fan_count >= MAX_FAN)
    {
        printf("ERROR: MAX_FAN limit reached!\n");
        return;
    }

    fan_device_t *fan = &my_fans[fan_count];
    fan->id = fan_count;
    fan->is_on = false;
    fan->current_speed = 0;
    lv_snprintf(fan->name, sizeof(fan->name), "%s", fan_name);

    // Stamp widget into mid_panel
    create_user_widget_ceiling_fan_button_big(parent_panel, NULL, 0);
    uint32_t total = lv_obj_get_child_cnt(parent_panel);
    fan->ceiling_fan_button = lv_obj_get_child(parent_panel, total - 1);

    fan->ceiling_fan_speed_slider = lv_obj_get_child(fan->ceiling_fan_button, 0);
    fan->ceiling_fan_img = lv_obj_get_child(fan->ceiling_fan_speed_slider, 0);
    fan->ceiling_fan_minus_s_button = lv_obj_get_child(fan->ceiling_fan_button, 1); // was 2
    fan->ceiling_fan_minus_speed_label = lv_obj_get_child(fan->ceiling_fan_minus_s_button, 0);
    fan->ceiling_fan_speed_label = lv_obj_get_child(fan->ceiling_fan_button, 2);   // was 1
    fan->ceiling_fan_plus_s_button = lv_obj_get_child(fan->ceiling_fan_button, 3); // was 3 
    fan->ceiling_fan_plus_speed_label = lv_obj_get_child(fan->ceiling_fan_plus_s_button, 0);
    fan->ceiling_fan_label = lv_obj_get_child(fan->ceiling_fan_button, 4); // 
    // Initial values
    lv_arc_set_range(fan->ceiling_fan_speed_slider, 0, 6);
    lv_arc_set_value(fan->ceiling_fan_speed_slider, fan->current_speed);
    lv_label_set_text(fan->ceiling_fan_label, fan->name);
    lv_label_set_text(fan->ceiling_fan_speed_label, " ");

    // Bind events
    lv_obj_add_event_cb(fan->ceiling_fan_plus_s_button, ceiling_fan_event_handler,
                        LV_EVENT_CLICKED, fan);
    lv_obj_add_event_cb(fan->ceiling_fan_minus_s_button, ceiling_fan_event_handler,
                        LV_EVENT_CLICKED, fan);
    lv_obj_add_event_cb(fan->ceiling_fan_speed_slider, ceiling_fan_event_handler,
                        LV_EVENT_VALUE_CHANGED, fan);
    lv_obj_add_event_cb(fan->ceiling_fan_img, ceiling_fan_event_handler,
                        LV_EVENT_CLICKED, fan);

    fan_count++;
    printf("Fan widget Big created OK: [%s] id=%d\n", fan->name, fan->id);
}

void create_curtain_widget_big(const char *curtain_name, lv_obj_t *parent_panel)
{
    if (curtain_count >= MAX_CURTAIN)
    {
        printf("ERROR: MAX_CURTAIN reached!\n");
        return;
    }

    curtain_device_t *curtain = &my_curtains[curtain_count];
    curtain->id = curtain_count;
    lv_snprintf(curtain->name, sizeof(curtain->name), "%s", curtain_name);

    create_user_widget_curtain_button_big(parent_panel, NULL, 0);
    uint32_t total = lv_obj_get_child_cnt(parent_panel);
    curtain->curtain_button = lv_obj_get_child(parent_panel, total - 1);

    curtain->curtain_movement_stop_img_cum_but = lv_obj_get_child(curtain->curtain_button, 0);
    curtain->curtain_label = lv_obj_get_child(curtain->curtain_button, 1);
    curtain->curtain_img_cum_but_opener = lv_obj_get_child(curtain->curtain_button, 2); //
    curtain->curtain_img_cum_but_closer = lv_obj_get_child(curtain->curtain_button, 3); //
    curtain->dumbyarc_curt_1_open              = lv_obj_get_child(curtain->curtain_button, 4); //  ADD
     curtain->dumarch_curt_2_close              = lv_obj_get_child(curtain->curtain_button, 5); //  ADD

    // Initial values
    lv_label_set_text(curtain->curtain_label, curtain->name);

    // Bind events
    lv_obj_add_event_cb(curtain->curtain_img_cum_but_opener,
                        curtain_event_handler, LV_EVENT_VALUE_CHANGED, curtain);
    lv_obj_add_event_cb(curtain->curtain_img_cum_but_closer,
                        curtain_event_handler, LV_EVENT_VALUE_CHANGED, curtain);
    lv_obj_add_event_cb(curtain->curtain_movement_stop_img_cum_but,
                        curtain_event_handler, LV_EVENT_CLICKED, curtain);

    curtain_count++;
    printf("Curtain widget created: [%s] id=%d\n", curtain->name, curtain->id);
}

void create_lamp_widget_big(const char *lamp_name, lv_obj_t *parent_panel)
{
    if (lamp_count >= MAX_LAMP)
    {
        printf("ERROR: MAX_LAMP reached!\n");
        return;
    }

    lamp_device_t *lamp = &my_lamps[lamp_count];
    lamp->id = lamp_count;
    lamp->is_on = false;
    lamp->current_level = 0;
    lv_snprintf(lamp->name, sizeof(lamp->name), "%s", lamp_name);

    create_user_widget_table_lamp_button_big(parent_panel, NULL, 0);
    uint32_t total = lv_obj_get_child_cnt(parent_panel);
    lamp->table_lamp_button = lv_obj_get_child(parent_panel, total - 1);

    // Map children
    lamp->table_lamp_arc = lv_obj_get_child(lamp->table_lamp_button, 0);       // arc
    lamp->table_lamp_img = lv_obj_get_child(lamp->table_lamp_arc, 0);          // child inside child of arc
    lamp->table_lamp_lab_on_ff = lv_obj_get_child(lamp->table_lamp_button, 1); // value label
    lamp->table_lamp_label = lv_obj_get_child(lamp->table_lamp_button, 2);     // name label

    // Initial values
    lv_arc_set_range(lamp->table_lamp_arc, 0, 5);
    lv_arc_set_value(lamp->table_lamp_arc, 0);
    lv_label_set_text(lamp->table_lamp_label, lamp->name);
    lv_label_set_text(lamp->table_lamp_lab_on_ff, "OFF");

    // Bind events
    lv_obj_add_event_cb(lamp->table_lamp_img, table_lamp_event_handler, LV_EVENT_CLICKED, lamp);
    lv_obj_add_event_cb(lamp->table_lamp_arc, table_lamp_event_handler, LV_EVENT_VALUE_CHANGED, lamp);

    lamp_count++;
    printf("Lamp widget created: [%s] id=%d\n", lamp->name, lamp->id);
}

void create_projector_widget_big(const char *proj_name, lv_obj_t *parent_panel)
{
    if (projector_count >= MAX_PROJECTOR)
    {
        printf("ERROR: MAX_PROJECTOR reached!\n");
        return;
    }

    projector_device_t *proj = &my_projectors[projector_count];
    proj->id = projector_count;
    proj->is_on = false;
    proj->current_level = 0;
    lv_snprintf(proj->name, sizeof(proj->name), "%s", proj_name);

    create_user_widget_projector_button_big(parent_panel, NULL, 0);
    uint32_t total = lv_obj_get_child_cnt(parent_panel);
    proj->projector_button = lv_obj_get_child(parent_panel, total - 1);

    // Map children
    proj->project_slider_cum_light = lv_obj_get_child(proj->projector_button, 0); // arc
    proj->projector_img = lv_obj_get_child(proj->project_slider_cum_light, 0);    // image inside arc!
    proj->projector_la = lv_obj_get_child(proj->projector_button, 1);             // value label
    proj->projector_label = lv_obj_get_child(proj->projector_button, 2);          // name label

    // Initial values
    lv_arc_set_range(proj->project_slider_cum_light, 0, 5);
    lv_arc_set_value(proj->project_slider_cum_light, 0);
    lv_label_set_text(proj->projector_label, proj->name);
    lv_label_set_text(proj->projector_la, "OFF");

    // Bind events
    lv_obj_add_event_cb(proj->projector_img,
                        projector_event_handler, LV_EVENT_CLICKED, proj);
    lv_obj_add_event_cb(proj->project_slider_cum_light,
                        projector_event_handler, LV_EVENT_VALUE_CHANGED, proj);

    projector_count++;
    printf("Projector widget created: [%s] id=%d\n", proj->name, proj->id);
}


void turn_off_all_lights(void)
{

    // Turn off all Table Lamps (type 1/6/7)
     for (int i = 0; i < lamp_count; i++)
    {
        lamp_device_t *lamp = &my_lamps[i];
        lamp->is_on = false;
        lamp->current_level = 0;

        lv_arc_set_value(lamp->table_lamp_arc, 0);
        lv_label_set_text(lamp->table_lamp_lab_on_ff, "OFF");

        // THIS WAS MISSING — image is nested inside arc
        lv_obj_clear_state(lamp->table_lamp_img, LV_STATE_CHECKED);
        lv_obj_invalidate(lamp->table_lamp_img);
    }

    printf("All Normal lights turned OFF\n");
}

