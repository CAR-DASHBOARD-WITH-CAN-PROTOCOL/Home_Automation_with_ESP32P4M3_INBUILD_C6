// #include <stdio.h>
// #include "ui.h"
// #include "lvgl.h"
// #include "header.h"
// #include "screens.h"

// // ============================================================================
// // STRUCT
// typedef struct {
//     int  id;
//     bool is_on;
//     int  current_temp;
//     char name[32];

//     lv_obj_t *obj0;                      // widget root = the button itself
//     lv_obj_t *obj0__ac_temp_slider;      // child 0 — arc
//     lv_obj_t *obj0__ac_temp_l;           // child 1 — label
//     lv_obj_t *obj0__ac_seting_img;       // child 2 — image (gear)
//     lv_obj_t *obj0__ac_plus_button;      // child 3 — btn
//     lv_obj_t *obj0__ac_minus_button;     // child 4 — btn
//     lv_obj_t *obj0__main_screen_ac_img;  // child 5 — image (power)
//     lv_obj_t *obj0__ac_lab;              // child 6 — label
// } ac_device_t;
// // ============================================================================
// // GLOBALS
// // ============================================================================
// #define MAX_AC 10

// static ac_device_t  my_acs[MAX_AC]; // array variable to store all data to the ac structure one by one
// static int          ac_count          = 0;
// static ac_device_t *current_active_ac = NULL;  // pointer for telling which ac is pointing to towards screen

// // ============================================================================
// // FORWARD DECLARATIONS
// // ============================================================================
// void ac_screen_init(void);
// void create_ac_widget(const char *ac_name, lv_obj_t *parent_panel);
// void create_ac_widget_big(const char *ac_name, lv_obj_t *parent_panel);

// // ============================================================================
// // SYNC BIG SCREEN
// // ============================================================================
// static void sync_big_screen(ac_device_t *ac)
// {
//     if (current_active_ac != ac)    return;
//     if (objects.ac_screen  == NULL) return;

//     char buf[8];
//     if (ac->current_temp > 0)
//         lv_snprintf(buf, sizeof(buf), "%d°C", ac->current_temp);
//     else
//         lv_snprintf(buf, sizeof(buf), "OFF");

//     lv_label_set_text(objects.temperature_label_on_ac_pic, buf);
//     lv_label_set_text(objects.ac_screen_label, ac->name);

//     // Sync big screen img checked state
//     if (ac->is_on)
//         lv_obj_add_state(objects.ac_screen, LV_STATE_CHECKED);
//     else
//         lv_obj_clear_state(objects.ac_screen, LV_STATE_CHECKED);
//     lv_obj_invalidate(objects.ac_screen);
// }

// // ============================================================================
// // WIDGET CALLBACKS
// // ============================================================================
// static void gear_clicked_cb(lv_event_t *e)
// {
//     ac_device_t *ac = (ac_device_t *)lv_event_get_user_data(e);
//     current_active_ac = ac;

//     sync_big_screen(ac);

//     lv_obj_clear_flag(objects.ac_screen, LV_OBJ_FLAG_HIDDEN);
//     lv_obj_move_foreground(objects.ac_screen);
// }
// static void widget_temp_cb(lv_event_t *e)
// {
//     lv_obj_t    *target = lv_event_get_target(e);
//     ac_device_t *ac     = (ac_device_t *)lv_event_get_user_data(e);

//     if (target == ac->obj0__ac_plus_button) {
//         if (ac->current_temp == 0)
//             ac->current_temp = 16;        // OFF → jump to 16
//         else if (ac->current_temp < 30)
//             ac->current_temp++;
//     }
//     else if (target == ac->obj0__ac_minus_button) {
//         if (ac->current_temp == 16)
//             ac->current_temp = 0;         // 16 → jump to OFF
//         else if (ac->current_temp > 16)
//             ac->current_temp--;
//     }
//     else if (target == ac->obj0__ac_temp_slider) {
//         ac->current_temp = lv_arc_get_value(ac->obj0__ac_temp_slider);
//     }
//     else if (target == ac->obj0__main_screen_ac_img) {
//         ac->is_on        = !ac->is_on;
//         ac->current_temp = ac->is_on ? 17 : 0;
//     }

//    // Single source of truth
//     ac->is_on = (ac->current_temp > 0);

//     // Arc — park at 15 when OFF, otherwise show actual temp
//     if (ac->current_temp > 0)
//         lv_arc_set_value(ac->obj0__ac_temp_slider, ac->current_temp);
//     else
//         lv_arc_set_value(ac->obj0__ac_temp_slider, 16);

//     // Label
//     char buf[8];
//     if (ac->current_temp > 0)
//         lv_snprintf(buf, sizeof(buf), "%d°C", ac->current_temp);
//     else
//         lv_snprintf(buf, sizeof(buf), "OFF");
//     lv_label_set_text(ac->obj0__ac_temp_l, buf);

//     // Img
//     if (ac->is_on)
//         lv_obj_add_state(ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
//     else
//         lv_obj_clear_state(ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
//     lv_obj_invalidate(ac->obj0__main_screen_ac_img);

//     sync_big_screen(ac);
// }
// // ============================================================================
// // BIG SCREEN CALLBACKS
// // ============================================================================
// static void big_screen_plus_cb(lv_event_t *e)
// {
//     if (!current_active_ac) return;
//     if (current_active_ac->current_temp == 0)
//         current_active_ac->current_temp = 16;
//     else if (current_active_ac->current_temp < 30)
//         current_active_ac->current_temp++;

//     current_active_ac->is_on = (current_active_ac->current_temp > 0);

//     char buf[8];
//     if (current_active_ac->current_temp > 0)
//         lv_snprintf(buf, sizeof(buf), "%d°C", current_active_ac->current_temp);
//     else
//         lv_snprintf(buf, sizeof(buf), "OFF");

//     lv_label_set_text(objects.temperature_label_on_ac_pic, buf);
//     lv_label_set_text(current_active_ac->obj0__ac_temp_l, buf);
//     lv_arc_set_value(current_active_ac->obj0__ac_temp_slider, current_active_ac->current_temp > 0 ? current_active_ac->current_temp : 15);

//     if (current_active_ac->is_on)
//         lv_obj_add_state(current_active_ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
//     else
//         lv_obj_clear_state(current_active_ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
//     lv_obj_invalidate(current_active_ac->obj0__main_screen_ac_img);
// }

// static void big_screen_minus_cb(lv_event_t *e)
// {
//     if (!current_active_ac) return;
//     if (current_active_ac->current_temp == 16)
//         current_active_ac->current_temp = 0;
//     else if (current_active_ac->current_temp > 16)
//         current_active_ac->current_temp--;

//     current_active_ac->is_on = (current_active_ac->current_temp > 0);

//     char buf[8];
//     if (current_active_ac->current_temp > 0)
//         lv_snprintf(buf, sizeof(buf), "%d°C", current_active_ac->current_temp);
//     else
//         lv_snprintf(buf, sizeof(buf), "OFF");

//     lv_label_set_text(objects.temperature_label_on_ac_pic, buf);
//     lv_label_set_text(current_active_ac->obj0__ac_temp_l, buf);
//     lv_arc_set_value(current_active_ac->obj0__ac_temp_slider, current_active_ac->current_temp > 0 ? current_active_ac->current_temp : 15);

//     if (current_active_ac->is_on)
//         lv_obj_add_state(current_active_ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
//     else
//         lv_obj_clear_state(current_active_ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
//     lv_obj_invalidate(current_active_ac->obj0__main_screen_ac_img);
// }

// static void back_ac_button_clicked_cb(lv_event_t *e)
// {
//     lv_obj_add_flag(objects.ac_screen, LV_OBJ_FLAG_HIDDEN);
//     current_active_ac = NULL;
// }

// // ============================================================================
// // AC BIG SCREEN — Fan / Mode / Swing callbacks with mutual exclusion
// // ============================================================================
// static void ac_fan_sp_auto_but_callback(lv_event_t *e)
// {
//     lv_obj_clear_state(objects.ac_fan_sp_slow_img, LV_STATE_CHECKED);
//     lv_obj_clear_state(objects.ac_f_s_fast_off_img, LV_STATE_CHECKED);
//     lv_obj_clear_state(objects.ac_fan_sp_med_img,   LV_STATE_CHECKED);
//     lv_obj_invalidate(objects.ac_fan_sp_slow_img);
//     lv_obj_invalidate(objects.ac_f_s_fast_off_img);
//     lv_obj_invalidate(objects.ac_fan_sp_med_img);
//     printf("fan: auto\n");
// }

// static void ac_fan_sp_slow_but_1_callback(lv_event_t *e)
// {
//     lv_obj_clear_state(objects.ac_fan_sp_auto_img,  LV_STATE_CHECKED);
//     lv_obj_clear_state(objects.ac_f_s_fast_off_img, LV_STATE_CHECKED);
//     lv_obj_clear_state(objects.ac_fan_sp_med_img,   LV_STATE_CHECKED);
//     lv_obj_invalidate(objects.ac_fan_sp_auto_img);
//     lv_obj_invalidate(objects.ac_f_s_fast_off_img);
//     lv_obj_invalidate(objects.ac_fan_sp_med_img);
//     printf("fan: slow\n");
// }

// static void ac_fan_sp_fast_but_callback(lv_event_t *e)
// {
//     lv_obj_clear_state(objects.ac_fan_sp_auto_img, LV_STATE_CHECKED);
//     lv_obj_clear_state(objects.ac_fan_sp_slow_img, LV_STATE_CHECKED);
//     lv_obj_clear_state(objects.ac_fan_sp_med_img,  LV_STATE_CHECKED);
//     lv_obj_invalidate(objects.ac_fan_sp_auto_img);
//     lv_obj_invalidate(objects.ac_fan_sp_slow_img);
//     lv_obj_invalidate(objects.ac_fan_sp_med_img);
//     printf("fan: fast\n");
// }

// static void ac_fan_sp_med_but_callback(lv_event_t *e)
// {
//     lv_obj_clear_state(objects.ac_fan_sp_auto_img,  LV_STATE_CHECKED);
//     lv_obj_clear_state(objects.ac_fan_sp_slow_img,  LV_STATE_CHECKED);
//     lv_obj_clear_state(objects.ac_f_s_fast_off_img, LV_STATE_CHECKED);
//     lv_obj_invalidate(objects.ac_fan_sp_auto_img);
//     lv_obj_invalidate(objects.ac_fan_sp_slow_img);
//     lv_obj_invalidate(objects.ac_f_s_fast_off_img);
//     printf("fan: med\n");
// }

// static void ac_cool_m_button_callback(lv_event_t *e)
// {
//     lv_obj_clear_state(objects.dummy_ac_warm_b_for_img_but, LV_STATE_CHECKED);
//     lv_obj_invalidate(objects.dummy_ac_warm_b_for_img_but);
//     printf("mode: cool\n");
// }

// static void warm_ac_mode_b_callback(lv_event_t *e)
// {
//     lv_obj_clear_state(objects.dummy_ac_cool_b_for_img_but, LV_STATE_CHECKED);
//     lv_obj_invalidate(objects.dummy_ac_cool_b_for_img_but);
//     printf("mode: warm\n");
// }

// static void ac_swing_horizontal_but_callback(lv_event_t *e)
// {
//     lv_obj_clear_state(objects.ac_v_swing_img, LV_STATE_CHECKED);
//     lv_obj_invalidate(objects.ac_v_swing_img);
//     printf("swing: H\n");
// }

// static void ac_vert_swing_button_callback(lv_event_t *e)
// {
//     lv_obj_clear_state(objects.ac_h_swing_img, LV_STATE_CHECKED);
//     lv_obj_invalidate(objects.ac_h_swing_img);
//     printf("swing: V\n");
// }


// void reset_ac_widgets(void)
// {
//     ac_count = 0;
//     lv_obj_clean(objects.mid_panel);
//     lv_obj_clean(objects.top_plus_mid_panel);
//     printf("AC reset OK, ac_count=0\n");

// }
// // ============================================================================
// // ac_screen_init() — wire the shared big screen ONCE
// // ============================================================================
// void ac_screen_init(void)
// {

//     lv_obj_add_event_cb(objects.back_ac_button,          back_ac_button_clicked_cb,        LV_EVENT_CLICKED, NULL);
//     lv_obj_add_event_cb(objects.plus_ac_botton_on_img,   big_screen_plus_cb,               LV_EVENT_CLICKED, NULL);
//     lv_obj_add_event_cb(objects.minus_but_on_ac_pic,     big_screen_minus_cb,              LV_EVENT_CLICKED, NULL);
//     lv_obj_add_event_cb(objects.ac_fan_sp_auto_img,      ac_fan_sp_auto_but_callback,      LV_EVENT_VALUE_CHANGED, NULL);
//     lv_obj_add_event_cb(objects.ac_fan_sp_slow_img,    ac_fan_sp_slow_but_1_callback,    LV_EVENT_VALUE_CHANGED, NULL);
//     lv_obj_add_event_cb(objects.ac_f_s_fast_off_img,      ac_fan_sp_fast_but_callback,      LV_EVENT_VALUE_CHANGED, NULL);
//     lv_obj_add_event_cb(objects.ac_fan_sp_med_img,       ac_fan_sp_med_but_callback,       LV_EVENT_VALUE_CHANGED, NULL);
//     lv_obj_add_event_cb(objects.dummy_ac_cool_b_for_img_but,        ac_cool_m_button_callback,        LV_EVENT_VALUE_CHANGED, NULL);
//     lv_obj_add_event_cb(objects.dummy_ac_warm_b_for_img_but,          warm_ac_mode_b_callback,          LV_EVENT_VALUE_CHANGED, NULL);
//     lv_obj_add_event_cb(objects.ac_h_swing_img, ac_swing_horizontal_but_callback, LV_EVENT_VALUE_CHANGED, NULL);
//     lv_obj_add_event_cb(objects.ac_v_swing_img,    ac_vert_swing_button_callback,    LV_EVENT_VALUE_CHANGED, NULL);


// #ifdef MANUAL
// create_ac_widget("Bedroom",objects.mid_panel);       // creates 1st AC
// create_ac_widget("Living Room",objects.mid_panel);   // creates 2nd AC
// create_ac_widget("Office",objects.mid_panel); 
// #endif
// }

// // ============================================================================
// // create_ac_widget() — call once per AC needed
// // ============================================================================
// void create_ac_widget(const char *ac_name,lv_obj_t *parent_panel)
// {
//     if (ac_count >= MAX_AC) {
//         printf("ERROR: MAX_AC limit reached!\n");
//         return;
//     }

//     ac_device_t *ac  = &my_acs[ac_count];
//     ac->id           = ac_count;
//     ac->is_on        = false;
//     ac->current_temp = 0;
//     lv_snprintf(ac->name, sizeof(ac->name), "%s", ac_name);

//     // Stamp widget into mid_panel
//    // Stamp widget into mid_panel
//     create_user_widget_ac_button(parent_panel, NULL, 0);
//     uint32_t total = lv_obj_get_child_cnt(parent_panel);
//     ac->obj0 = lv_obj_get_child(parent_panel, total - 1);

//     // FIX: obj0 IS the button — no extra child level needed
//     // Direct children of obj0:
//     ac->obj0__ac_temp_slider     = lv_obj_get_child(ac->obj0, 0);  // arc
//     ac->obj0__ac_temp_l          = lv_obj_get_child(ac->obj0, 1);  // label
//     ac->obj0__ac_seting_img      = lv_obj_get_child(ac->obj0, 2);  // image
//     ac->obj0__ac_plus_button     = lv_obj_get_child(ac->obj0, 3);  // btn
//     ac->obj0__ac_minus_button    = lv_obj_get_child(ac->obj0, 4);  // btn
//     ac->obj0__main_screen_ac_img = lv_obj_get_child(ac->obj0, 5);  // image
//     ac->obj0__ac_lab             = lv_obj_get_child(ac->obj0, 6);  // label

    
//     lv_arc_set_range(ac->obj0__ac_temp_slider, 16, 30);
//     lv_arc_set_value(ac->obj0__ac_temp_slider, 16);     // arc starts at 16
//   lv_label_set_text(ac->obj0__ac_lab,    ac->name);
//    lv_label_set_text(ac->obj0__ac_temp_l, "OFF");      // shows OFF initially

//     // Bind events
//     lv_obj_add_event_cb(ac->obj0__ac_plus_button,     widget_temp_cb,  LV_EVENT_CLICKED,       ac);
//     lv_obj_add_event_cb(ac->obj0__ac_minus_button,    widget_temp_cb,  LV_EVENT_CLICKED,       ac);
//     lv_obj_add_event_cb(ac->obj0__ac_temp_slider,     widget_temp_cb,  LV_EVENT_VALUE_CHANGED, ac);
//     lv_obj_add_event_cb(ac->obj0__main_screen_ac_img, widget_temp_cb,  LV_EVENT_CLICKED,       ac);
//     lv_obj_add_event_cb(ac->obj0__ac_seting_img,      gear_clicked_cb, LV_EVENT_CLICKED,       ac);

//     ac_count++;
//     printf("AC widget created OK: [%s] id=%d\n", ac->name, ac->id);
// }

// // ============================================================================
// // create_ac_widget_big() — for top_plus_mid_panel when no scenes
// // Same struct, same callbacks, only widget stamp differs
// // ============================================================================
// void create_ac_widget_big(const char *ac_name, lv_obj_t *parent_panel)
// {
//     if (ac_count >= MAX_AC) {
//         printf("ERROR: MAX_AC limit reached!\n");
//         return;
//     }

//     ac_device_t *ac  = &my_acs[ac_count];
//     ac->id           = ac_count;
//     ac->is_on        = false;
//     ac->current_temp = 0;
//     lv_snprintf(ac->name, sizeof(ac->name), "%s", ac_name);

//     // Stamp BIG widget instead of small one
//     create_user_widget_ac_button_big(parent_panel, NULL, 0);
//     uint32_t total = lv_obj_get_child_cnt(parent_panel);
//     ac->obj0 = lv_obj_get_child(parent_panel, total - 1);

//     // Child mapping — identical indices to small widget
//     ac->obj0__ac_temp_slider     = lv_obj_get_child(ac->obj0, 0);  // arc
//     ac->obj0__ac_temp_l          = lv_obj_get_child(ac->obj0, 1);  // label
//     ac->obj0__ac_seting_img      = lv_obj_get_child(ac->obj0, 2);  // gear image
//     ac->obj0__ac_plus_button     = lv_obj_get_child(ac->obj0, 3);  // plus image (clickable)
//     ac->obj0__ac_minus_button    = lv_obj_get_child(ac->obj0, 4);  // minus image (clickable)
//     ac->obj0__main_screen_ac_img = lv_obj_get_child(ac->obj0, 5);  // imgbutton on/off
//     ac->obj0__ac_lab             = lv_obj_get_child(ac->obj0, 6);  // name label
    
//     // child 7 and 8 are decorative arcs — ignore them

//     // Initial values — identical to small widget
//     lv_arc_set_range(ac->obj0__ac_temp_slider, 16, 30);
//     lv_arc_set_value(ac->obj0__ac_temp_slider, 16);
//     lv_label_set_text(ac->obj0__ac_lab,    ac->name);
//     lv_label_set_text(ac->obj0__ac_temp_l, "OFF");

//     // Bind events — exact same callbacks, nothing changes
//     lv_obj_add_event_cb(ac->obj0__ac_plus_button,     widget_temp_cb,  LV_EVENT_CLICKED,       ac);
//     lv_obj_add_event_cb(ac->obj0__ac_minus_button,    widget_temp_cb,  LV_EVENT_CLICKED,       ac);
//     lv_obj_add_event_cb(ac->obj0__ac_temp_slider,     widget_temp_cb,  LV_EVENT_VALUE_CHANGED, ac);
//     lv_obj_add_event_cb(ac->obj0__main_screen_ac_img, widget_temp_cb,  LV_EVENT_CLICKED,       ac);
//     lv_obj_add_event_cb(ac->obj0__ac_seting_img,      gear_clicked_cb, LV_EVENT_CLICKED,       ac);

//     ac_count++;
//     printf("AC BIG widget created OK: [%s] id=%d\n", ac->name, ac->id);
// }


#include <stdio.h>
#include "ui.h"
#include "lvgl.h"
#include "header.h"
#include "screens.h"

// ============================================================================
// STRUCT
// ============================================================================
typedef struct {
    int  id;
    bool is_on;
    int  current_temp;
    int  fan_speed;   // 0=auto, 1=slow, 2=med, 3=fast
    int  mode;        // 0=cool, 1=warm
    int  swing;       // 0=none, 1=horizontal, 2=vertical
    char name[32];

    lv_obj_t *obj0;
    lv_obj_t *obj0__ac_temp_slider;
    lv_obj_t *obj0__ac_temp_l;
    lv_obj_t *obj0__ac_seting_img;
    lv_obj_t *obj0__ac_plus_button;
    lv_obj_t *obj0__ac_minus_button;
    lv_obj_t *obj0__main_screen_ac_img;
    lv_obj_t *obj0__ac_lab;
} ac_device_t;

// ============================================================================
// GLOBALS
// ============================================================================
#define MAX_AC 10

static ac_device_t  my_acs[MAX_AC];
static int          ac_count          = 0;
static ac_device_t *current_active_ac = NULL;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
void ac_screen_init(void);
void create_ac_widget(const char *ac_name, lv_obj_t *parent_panel);
void create_ac_widget_big(const char *ac_name, lv_obj_t *parent_panel);

// ============================================================================
// SYNC BIG SCREEN — restores ALL per-widget state when opening big screen
// ============================================================================
static void sync_big_screen(ac_device_t *ac)
{
    if (current_active_ac != ac)    return;
    if (objects.ac_screen  == NULL) return;

    // 1. Sync name label
    lv_label_set_text(objects.ac_screen_label, ac->name);

    // 2. Sync temperature label
    char buf[8];
    if (ac->current_temp > 0)
        lv_snprintf(buf, sizeof(buf), "%d\xc2\xb0""C", ac->current_temp);
    else
        lv_snprintf(buf, sizeof(buf), "OFF");
    lv_label_set_text(objects.temperature_label_on_ac_pic, buf);

    // 3. Sync arc to this widget's actual saved temp
    if (ac->current_temp > 0)
        lv_arc_set_value(ac->obj0__ac_temp_slider, ac->current_temp);
    else
        lv_arc_set_value(ac->obj0__ac_temp_slider, 16); // park at min when OFF

    // 4. Sync big screen on/off image state
    if (ac->is_on)
        lv_obj_add_state(objects.ac_screen, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(objects.ac_screen, LV_STATE_CHECKED);
    lv_obj_invalidate(objects.ac_screen);

    // 5. Sync fan speed — clear all first, then set the saved one
    lv_obj_clear_state(objects.ac_fan_sp_auto_img,   LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_fan_sp_slow_img,   LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_fan_sp_med_img,    LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_f_s_fast_off_img,  LV_STATE_CHECKED);
    switch (ac->fan_speed) {
        case 0: lv_obj_add_state(objects.ac_fan_sp_auto_img,  LV_STATE_CHECKED); break;
        case 1: lv_obj_add_state(objects.ac_fan_sp_slow_img,  LV_STATE_CHECKED); break;
        case 2: lv_obj_add_state(objects.ac_fan_sp_med_img,   LV_STATE_CHECKED); break;
        case 3: lv_obj_add_state(objects.ac_f_s_fast_off_img, LV_STATE_CHECKED); break;
        default: break;
    }
    lv_obj_invalidate(objects.ac_fan_sp_auto_img);
    lv_obj_invalidate(objects.ac_fan_sp_slow_img);
    lv_obj_invalidate(objects.ac_fan_sp_med_img);
    lv_obj_invalidate(objects.ac_f_s_fast_off_img);

    // 6. Sync mode — clear all first, then set the saved one
    lv_obj_clear_state(objects.dummy_ac_cool_b_for_img_but, LV_STATE_CHECKED);
    lv_obj_clear_state(objects.dummy_ac_warm_b_for_img_but, LV_STATE_CHECKED);
    switch (ac->mode) {
        case 0: lv_obj_add_state(objects.dummy_ac_cool_b_for_img_but, LV_STATE_CHECKED); break;
        case 1: lv_obj_add_state(objects.dummy_ac_warm_b_for_img_but, LV_STATE_CHECKED); break;
        default: break;
    }
    lv_obj_invalidate(objects.dummy_ac_cool_b_for_img_but);
    lv_obj_invalidate(objects.dummy_ac_warm_b_for_img_but);

    // 7. Sync swing — clear all first, then set the saved one
    lv_obj_clear_state(objects.ac_h_swing_img, LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_v_swing_img, LV_STATE_CHECKED);
    switch (ac->swing) {
        case 1: lv_obj_add_state(objects.ac_h_swing_img, LV_STATE_CHECKED); break;
        case 2: lv_obj_add_state(objects.ac_v_swing_img, LV_STATE_CHECKED); break;
        default: break;
    }
    lv_obj_invalidate(objects.ac_h_swing_img);
    lv_obj_invalidate(objects.ac_v_swing_img);
}

// ============================================================================
// WIDGET CALLBACKS
// ============================================================================
static void gear_clicked_cb(lv_event_t *e)
{
    ac_device_t *ac   = (ac_device_t *)lv_event_get_user_data(e);
    current_active_ac = ac;

    // Restore this widget's full state on the shared big screen
    sync_big_screen(ac);

    lv_obj_clear_flag(objects.ac_screen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(objects.ac_screen);
}

static void widget_temp_cb(lv_event_t *e)
{
    lv_obj_t    *target = lv_event_get_target(e);
    ac_device_t *ac     = (ac_device_t *)lv_event_get_user_data(e);

    if (target == ac->obj0__ac_plus_button) {
        if (ac->current_temp == 0)
            ac->current_temp = 16;
        else if (ac->current_temp < 30)
            ac->current_temp++;
    }
    else if (target == ac->obj0__ac_minus_button) {
        if (ac->current_temp == 16)
            ac->current_temp = 0;
        else if (ac->current_temp > 16)
            ac->current_temp--;
    }
    else if (target == ac->obj0__ac_temp_slider) {
        ac->current_temp = lv_arc_get_value(ac->obj0__ac_temp_slider);
    }
    else if (target == ac->obj0__main_screen_ac_img) {
        ac->is_on        = !ac->is_on;
        ac->current_temp = ac->is_on ? 17 : 0;
    }

    // Single source of truth
    ac->is_on = (ac->current_temp > 0);

    // Arc — park at 16 when OFF, otherwise show actual temp
    if (ac->current_temp > 0)
        lv_arc_set_value(ac->obj0__ac_temp_slider, ac->current_temp);
    else
        lv_arc_set_value(ac->obj0__ac_temp_slider, 16);

    // Temperature label on widget
    char buf[8];
    if (ac->current_temp > 0)
        lv_snprintf(buf, sizeof(buf), "%d\xc2\xb0""C", ac->current_temp);
    else
        lv_snprintf(buf, sizeof(buf), "OFF");
    lv_label_set_text(ac->obj0__ac_temp_l, buf);

    // Power image on widget
    if (ac->is_on)
        lv_obj_add_state(ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
    lv_obj_invalidate(ac->obj0__main_screen_ac_img);

    // If this widget's big screen is open, keep it in sync too
    sync_big_screen(ac);
}

// ============================================================================
// BIG SCREEN CALLBACKS
// ============================================================================
static void big_screen_plus_cb(lv_event_t *e)
{
    if (!current_active_ac) return;

    if (current_active_ac->current_temp == 0)
        current_active_ac->current_temp = 16;
    else if (current_active_ac->current_temp < 30)
        current_active_ac->current_temp++;

    current_active_ac->is_on = (current_active_ac->current_temp > 0);

    // Update big screen temperature label
    char buf[8];
    if (current_active_ac->current_temp > 0)
        lv_snprintf(buf, sizeof(buf), "%d\xc2\xb0""C", current_active_ac->current_temp);
    else
        lv_snprintf(buf, sizeof(buf), "OFF");
    lv_label_set_text(objects.temperature_label_on_ac_pic, buf);

    // Keep widget label in sync
    lv_label_set_text(current_active_ac->obj0__ac_temp_l, buf);

    // Sync arc — use 16 (not 15) when OFF since range min is 16
    lv_arc_set_value(current_active_ac->obj0__ac_temp_slider,
                     current_active_ac->current_temp > 0
                     ? current_active_ac->current_temp : 16);

    // Sync widget power image
    if (current_active_ac->is_on)
        lv_obj_add_state(current_active_ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(current_active_ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
    lv_obj_invalidate(current_active_ac->obj0__main_screen_ac_img);

    // Sync big screen on/off state
    if (current_active_ac->is_on)
        lv_obj_add_state(objects.ac_screen, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(objects.ac_screen, LV_STATE_CHECKED);
    lv_obj_invalidate(objects.ac_screen);
}

static void big_screen_minus_cb(lv_event_t *e)
{
    if (!current_active_ac) return;

    if (current_active_ac->current_temp == 16)
        current_active_ac->current_temp = 0;
    else if (current_active_ac->current_temp > 16)
        current_active_ac->current_temp--;

    current_active_ac->is_on = (current_active_ac->current_temp > 0);

    // Update big screen temperature label
    char buf[8];
    if (current_active_ac->current_temp > 0)
        lv_snprintf(buf, sizeof(buf), "%d\xc2\xb0""C", current_active_ac->current_temp);
    else
        lv_snprintf(buf, sizeof(buf), "OFF");
    lv_label_set_text(objects.temperature_label_on_ac_pic, buf);

    // Keep widget label in sync
    lv_label_set_text(current_active_ac->obj0__ac_temp_l, buf);

    // Sync arc — use 16 (not 15) when OFF since range min is 16
    lv_arc_set_value(current_active_ac->obj0__ac_temp_slider,
                     current_active_ac->current_temp > 0
                     ? current_active_ac->current_temp : 16);

    // Sync widget power image
    if (current_active_ac->is_on)
        lv_obj_add_state(current_active_ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(current_active_ac->obj0__main_screen_ac_img, LV_STATE_CHECKED);
    lv_obj_invalidate(current_active_ac->obj0__main_screen_ac_img);

    // Sync big screen on/off state
    if (current_active_ac->is_on)
        lv_obj_add_state(objects.ac_screen, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(objects.ac_screen, LV_STATE_CHECKED);
    lv_obj_invalidate(objects.ac_screen);
}

static void back_ac_button_clicked_cb(lv_event_t *e)
{
    lv_obj_add_flag(objects.ac_screen, LV_OBJ_FLAG_HIDDEN);
    current_active_ac = NULL;
}

// ============================================================================
// FAN / MODE / SWING CALLBACKS — now save state per widget
// ============================================================================
static void ac_fan_sp_auto_but_callback(lv_event_t *e)
{
    if (current_active_ac)
        current_active_ac->fan_speed = 0;

    lv_obj_clear_state(objects.ac_fan_sp_slow_img,   LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_f_s_fast_off_img,  LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_fan_sp_med_img,    LV_STATE_CHECKED);
    lv_obj_invalidate(objects.ac_fan_sp_slow_img);
    lv_obj_invalidate(objects.ac_f_s_fast_off_img);
    lv_obj_invalidate(objects.ac_fan_sp_med_img);
    printf("fan: auto\n");
}

static void ac_fan_sp_slow_but_1_callback(lv_event_t *e)
{
    if (current_active_ac)
        current_active_ac->fan_speed = 1;

    lv_obj_clear_state(objects.ac_fan_sp_auto_img,   LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_f_s_fast_off_img,  LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_fan_sp_med_img,    LV_STATE_CHECKED);
    lv_obj_invalidate(objects.ac_fan_sp_auto_img);
    lv_obj_invalidate(objects.ac_f_s_fast_off_img);
    lv_obj_invalidate(objects.ac_fan_sp_med_img);
    printf("fan: slow\n");
}

static void ac_fan_sp_fast_but_callback(lv_event_t *e)
{
    if (current_active_ac)
        current_active_ac->fan_speed = 3;

    lv_obj_clear_state(objects.ac_fan_sp_auto_img,  LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_fan_sp_slow_img,  LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_fan_sp_med_img,   LV_STATE_CHECKED);
    lv_obj_invalidate(objects.ac_fan_sp_auto_img);
    lv_obj_invalidate(objects.ac_fan_sp_slow_img);
    lv_obj_invalidate(objects.ac_fan_sp_med_img);
    printf("fan: fast\n");
}

static void ac_fan_sp_med_but_callback(lv_event_t *e)
{
    if (current_active_ac)
        current_active_ac->fan_speed = 2;

    lv_obj_clear_state(objects.ac_fan_sp_auto_img,   LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_fan_sp_slow_img,   LV_STATE_CHECKED);
    lv_obj_clear_state(objects.ac_f_s_fast_off_img,  LV_STATE_CHECKED);
    lv_obj_invalidate(objects.ac_fan_sp_auto_img);
    lv_obj_invalidate(objects.ac_fan_sp_slow_img);
    lv_obj_invalidate(objects.ac_f_s_fast_off_img);
    printf("fan: med\n");
}

static void ac_cool_m_button_callback(lv_event_t *e)
{
    if (current_active_ac)
        current_active_ac->mode = 0;

    lv_obj_clear_state(objects.dummy_ac_warm_b_for_img_but, LV_STATE_CHECKED);
    lv_obj_invalidate(objects.dummy_ac_warm_b_for_img_but);
    printf("mode: cool\n");
}

static void warm_ac_mode_b_callback(lv_event_t *e)
{
    if (current_active_ac)
        current_active_ac->mode = 1;

    lv_obj_clear_state(objects.dummy_ac_cool_b_for_img_but, LV_STATE_CHECKED);
    lv_obj_invalidate(objects.dummy_ac_cool_b_for_img_but);
    printf("mode: warm\n");
}

static void ac_swing_horizontal_but_callback(lv_event_t *e)
{
    if (current_active_ac)
        current_active_ac->swing = 1;

    lv_obj_clear_state(objects.ac_v_swing_img, LV_STATE_CHECKED);
    lv_obj_invalidate(objects.ac_v_swing_img);
    printf("swing: H\n");
}

static void ac_vert_swing_button_callback(lv_event_t *e)
{
    if (current_active_ac)
        current_active_ac->swing = 2;

    lv_obj_clear_state(objects.ac_h_swing_img, LV_STATE_CHECKED);
    lv_obj_invalidate(objects.ac_h_swing_img);
    printf("swing: V\n");
}

// ============================================================================
// RESET
// ============================================================================
void reset_ac_widgets(void)
{
    ac_count          = 0;
    current_active_ac = NULL;
    lv_obj_clean(objects.mid_panel);
    lv_obj_clean(objects.top_plus_mid_panel);
    printf("AC reset OK, ac_count=0\n");
}

// ============================================================================
// ac_screen_init() — wire the shared big screen ONCE
// ============================================================================
void ac_screen_init(void)
{
    lv_obj_add_event_cb(objects.back_ac_button,               back_ac_button_clicked_cb,        LV_EVENT_CLICKED,       NULL);
    lv_obj_set_ext_click_area(objects.back_ac_button, 20);
    lv_obj_add_event_cb(objects.plus_ac_botton_on_img,        big_screen_plus_cb,               LV_EVENT_CLICKED,       NULL);
    lv_obj_add_event_cb(objects.minus_but_on_ac_pic,          big_screen_minus_cb,              LV_EVENT_CLICKED,       NULL);
    lv_obj_add_event_cb(objects.ac_fan_sp_auto_img,           ac_fan_sp_auto_but_callback,      LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.ac_fan_sp_slow_img,           ac_fan_sp_slow_but_1_callback,    LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.ac_f_s_fast_off_img,          ac_fan_sp_fast_but_callback,      LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.ac_fan_sp_med_img,            ac_fan_sp_med_but_callback,       LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.dummy_ac_cool_b_for_img_but,  ac_cool_m_button_callback,        LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.dummy_ac_warm_b_for_img_but,  warm_ac_mode_b_callback,          LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.ac_h_swing_img,               ac_swing_horizontal_but_callback, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.ac_v_swing_img,               ac_vert_swing_button_callback,    LV_EVENT_VALUE_CHANGED, NULL);

#ifdef MANUAL
    create_ac_widget("Bedroom",     objects.mid_panel);
    create_ac_widget("Living Room", objects.mid_panel);
    create_ac_widget("Office",      objects.mid_panel);
#endif
}

// ============================================================================
// SHARED WIDGET WIRING — used by both create functions
// ============================================================================
static void wire_ac_widget(ac_device_t *ac)
{
    lv_arc_set_range(ac->obj0__ac_temp_slider, 16, 30);
    lv_arc_set_value(ac->obj0__ac_temp_slider, 16);
    lv_label_set_text(ac->obj0__ac_lab,    ac->name);
    lv_label_set_text(ac->obj0__ac_temp_l, "OFF");

    lv_obj_add_event_cb(ac->obj0__ac_plus_button,     widget_temp_cb,  LV_EVENT_CLICKED,       ac);
    lv_obj_add_event_cb(ac->obj0__ac_minus_button,    widget_temp_cb,  LV_EVENT_CLICKED,       ac);
    lv_obj_add_event_cb(ac->obj0__ac_temp_slider,     widget_temp_cb,  LV_EVENT_VALUE_CHANGED, ac);
    lv_obj_add_event_cb(ac->obj0__main_screen_ac_img, widget_temp_cb,  LV_EVENT_CLICKED,       ac);
    lv_obj_add_event_cb(ac->obj0__ac_seting_img,      gear_clicked_cb, LV_EVENT_CLICKED,       ac);
}

// ============================================================================
// create_ac_widget() — small widget
// ============================================================================
void create_ac_widget(const char *ac_name, lv_obj_t *parent_panel)
{
    if (ac_count >= MAX_AC) {
        printf("ERROR: MAX_AC limit reached!\n");
        return;
    }

    ac_device_t *ac  = &my_acs[ac_count];
    ac->id           = ac_count;
    ac->is_on        = false;
    ac->current_temp = 0;
    ac->fan_speed    = 0;   // default: auto
    ac->mode         = 0;   // default: cool
    ac->swing        = 0;   // default: none
    lv_snprintf(ac->name, sizeof(ac->name), "%s", ac_name);

    create_user_widget_ac_button(parent_panel, NULL, 0);
    uint32_t total = lv_obj_get_child_cnt(parent_panel);
    ac->obj0 = lv_obj_get_child(parent_panel, total - 1);

    ac->obj0__ac_temp_slider     = lv_obj_get_child(ac->obj0, 0);
    ac->obj0__ac_temp_l          = lv_obj_get_child(ac->obj0, 1);
    ac->obj0__ac_seting_img      = lv_obj_get_child(ac->obj0, 2);
    ac->obj0__ac_plus_button     = lv_obj_get_child(ac->obj0, 3);
    ac->obj0__ac_minus_button    = lv_obj_get_child(ac->obj0, 4);
    ac->obj0__main_screen_ac_img = lv_obj_get_child(ac->obj0, 5);
    ac->obj0__ac_lab             = lv_obj_get_child(ac->obj0, 6);

    wire_ac_widget(ac);

    ac_count++;
    printf("AC widget created OK: [%s] id=%d\n", ac->name, ac->id);
}

// ============================================================================
// create_ac_widget_big() — big widget
// ============================================================================
void create_ac_widget_big(const char *ac_name, lv_obj_t *parent_panel)
{
    if (ac_count >= MAX_AC) {
        printf("ERROR: MAX_AC limit reached!\n");
        return;
    }

    ac_device_t *ac  = &my_acs[ac_count];
    ac->id           = ac_count;
    ac->is_on        = false;
    ac->current_temp = 0;
    ac->fan_speed    = 0;   // default: auto
    ac->mode         = 0;   // default: cool
    ac->swing        = 0;   // default: none
    lv_snprintf(ac->name, sizeof(ac->name), "%s", ac_name);

    create_user_widget_ac_button_big(parent_panel, NULL, 0);
    uint32_t total = lv_obj_get_child_cnt(parent_panel);
    ac->obj0 = lv_obj_get_child(parent_panel, total - 1);

    ac->obj0__ac_temp_slider     = lv_obj_get_child(ac->obj0, 0);
    ac->obj0__ac_temp_l          = lv_obj_get_child(ac->obj0, 1);
    ac->obj0__ac_seting_img      = lv_obj_get_child(ac->obj0, 2);
    ac->obj0__ac_plus_button     = lv_obj_get_child(ac->obj0, 3);
    ac->obj0__ac_minus_button    = lv_obj_get_child(ac->obj0, 4);
    ac->obj0__main_screen_ac_img = lv_obj_get_child(ac->obj0, 5);
    ac->obj0__ac_lab             = lv_obj_get_child(ac->obj0, 6);
    // child 7 and 8 are decorative arcs — intentionally ignored

    wire_ac_widget(ac);

    ac_count++;
    printf("AC BIG widget created OK: [%s] id=%d\n", ac->name, ac->id);
}