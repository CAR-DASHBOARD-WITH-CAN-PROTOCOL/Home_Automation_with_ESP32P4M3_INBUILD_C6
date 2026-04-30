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

// ============================================================================
// TEMPERATURE ENUM — maps each img button to a CCT value
// ============================================================================
typedef enum {
    CCT_TEMP_2500K = 2500,
    CCT_TEMP_3000K = 3000,
    CCT_TEMP_3500K = 3500,
    CCT_TEMP_4000K = 4000,
    CCT_TEMP_4500K = 4500,
    CCT_TEMP_5000K = 5000,
    CCT_TEMP_5500K = 5500,
    CCT_TEMP_6500K = 6500,
} cct_temperature_t;


// DELETE these two dead declarations at the top:
// static lv_obj_t *home_temp_btns[] = { NULL, };
// static cct_temperature_t home_temps[] = { ... };

// ADD this missing helper before temperature_intensity_callback:
static cct_temperature_t obj_to_temperature(lv_obj_t *target)
{
    if (target == objects.sixtyfive_k_cct_lights)   return CCT_TEMP_6500K;
    if (target == objects.fifty_five_k_cct_lights)  return CCT_TEMP_5500K;
    if (target == objects.five_k_cct_lights)        return CCT_TEMP_5000K;
    if (target == objects.four_five_k_cct_lights)   return CCT_TEMP_4500K;
    if (target == objects.four_k_cct_lights)        return CCT_TEMP_4000K;
    if (target == objects.three_k_five_cct_lights)  return CCT_TEMP_3500K;
    if (target == objects.three_k_cct_lights)       return CCT_TEMP_3000K;
    if (target == objects.two_k_five_cct_lights)    return CCT_TEMP_2500K;
    return CCT_TEMP_4000K;
}
// ============================================================================
// STRUCT
// ============================================================================
typedef struct {
    int               id;
    bool              is_on;
    int               current_intensity;
    cct_temperature_t current_temperature;   // <-- NEW
    char              name[32];

    lv_obj_t *cct_light_button;
    lv_obj_t *cct_light_indicator;
    lv_obj_t *button_cum_img_on_off_cct;
    lv_obj_t *cct_light_intensity_label;
    lv_obj_t *cct_light_home_screen_img_button;
    lv_obj_t *cct_light_intensity_plus_button;
    lv_obj_t *cct_light_minus_intensity_button;
    lv_obj_t *cct_light_main_name_label;
} cct_device_t;

// ============================================================================
// GLOBALS
// ============================================================================
#define MAX_CCT 20

static cct_device_t  my_ccts[MAX_CCT];
static int           cct_count          = 0;
static cct_device_t *current_ccttive_cct = NULL;  // points to cct whose panel is open

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
void create_cct_widget(const char *cct_name, lv_obj_t *parent_panel);
void create_cct_widget_big(const char *cct_name, lv_obj_t *parent_panel);

void turn_off_all_lights_cct(void);
// ============================================================================
// HELPER — clear ALL home panel temp buttons, then check only one
// ============================================================================
static void home_temp_select(lv_obj_t *selected)
{
    lv_obj_t *btns[] = {
        objects.sixtyfive_k_cct_lights,
        objects.fifty_five_k_cct_lights,
        objects.five_k_cct_lights,
        objects.four_five_k_cct_lights,
        objects.four_k_cct_lights,
        objects.three_k_five_cct_lights,
        objects.three_k_cct_lights,
        objects.two_k_five_cct_lights,
    };
    for (int i = 0; i < 8; i++) {
        if (btns[i] == selected) {
            lv_obj_add_state(btns[i], LV_STATE_CHECKED);
            // here you button light scren temperature intensity button , write logic hre send the data for that
        } else {
            lv_obj_clear_state(btns[i], LV_STATE_CHECKED);
            // clear all state of the button which were earlier pressed
        }
        lv_obj_invalidate(btns[i]); // force imgbtn to redraw with correct src
    }
}

static void screen_temp_select(lv_obj_t *selected)
{
    lv_obj_t *btns[] = {
        objects.six_k_five_img_button_cct_screen,
        objects.five_k_five_img_button_cct_screen,
        objects.five_k_img_button_cct_screen,
        objects.fourty_k_five_img_button_cct_screen,
        objects.four_k_img_button_cct_screen,
        objects.three_k_five_img_button_cct_screen,
        objects.three_k_img_button_cct_screen,
        objects.two_k_five_img_button_cct_screen,
    };
    for (int i = 0; i < 8; i++) {
        if (btns[i] == selected) {
            lv_obj_add_state(btns[i], LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btns[i], LV_STATE_CHECKED);
        }
        lv_obj_invalidate(btns[i]); // force imgbtn to redraw with correct src
    }
}


// ============================================================================
// HELPER — map screen panel object → temperature
// ============================================================================
static cct_temperature_t screen_obj_to_temperature(lv_obj_t *target)
{
    if (target == objects.six_k_five_img_button_cct_screen)    return CCT_TEMP_6500K;
    if (target == objects.five_k_five_img_button_cct_screen)   return CCT_TEMP_5500K;
    if (target == objects.five_k_img_button_cct_screen)        return CCT_TEMP_5000K;
    if (target == objects.fourty_k_five_img_button_cct_screen) return CCT_TEMP_4500K;
    if (target == objects.four_k_img_button_cct_screen)        return CCT_TEMP_4000K;
    if (target == objects.three_k_five_img_button_cct_screen)  return CCT_TEMP_3500K;
    if (target == objects.three_k_img_button_cct_screen)       return CCT_TEMP_3000K;
    if (target == objects.two_k_five_img_button_cct_screen)    return CCT_TEMP_2500K;
    return CCT_TEMP_4000K;
}

// ============================================================================
// HELPER — apply temperature to one widget
// ============================================================================
static void apply_temperature_to_widget(cct_device_t *cct, cct_temperature_t temp)
{
    cct->current_temperature = temp;
    printf("cct [%s] temperature → %dK\n", cct->name, (int)temp);
    // TODO: drive hardware here using cct->current_temperature + cct->current_intensity
}

// ============================================================================
// sync_big_screen — populate cct_screen_panel for a specific cct
// ============================================================================
static void sync_big_screen(cct_device_t *cct)
{
    if (cct == NULL) return;

    // FIX: title label
    lv_label_set_text(objects.cct_light_screen_main_label, cct->name);

    // FIX: static intensity label — always "Set Intensity:"
    lv_label_set_text(objects.intensity_label_cct_screen, "Set Intensity:");

    // FIX: slider range locked to 0–10, then set value
    lv_slider_set_range(objects.cct_light_intensity_slider_main_screen, 0, 10);
    lv_slider_set_value(objects.cct_light_intensity_slider_main_screen,
                        cct->current_intensity, LV_ANIM_OFF);

    // FIX: sync screen temp buttons — clear all, check only matching
    lv_obj_t *btns[] = {
        objects.six_k_five_img_button_cct_screen,
        objects.five_k_five_img_button_cct_screen,
        objects.five_k_img_button_cct_screen,
        objects.fourty_k_five_img_button_cct_screen,
        objects.four_k_img_button_cct_screen,
        objects.three_k_five_img_button_cct_screen,
        objects.three_k_img_button_cct_screen,
        objects.two_k_five_img_button_cct_screen,
    };
    cct_temperature_t temps[] = {
        CCT_TEMP_6500K, CCT_TEMP_5500K, CCT_TEMP_5000K, CCT_TEMP_4500K,
        CCT_TEMP_4000K, CCT_TEMP_3500K, CCT_TEMP_3000K, CCT_TEMP_2500K,
    };
    for (int i = 0; i < 8; i++) {
    if (temps[i] == cct->current_temperature)
        lv_obj_add_state(btns[i], LV_STATE_CHECKED);
    else
    lv_obj_clear_state(btns[i], LV_STATE_CHECKED);
    lv_obj_invalidate(btns[i]); // ADD this line here too
}
}
// ============================================================================
// CALLBACKS
// ============================================================================

static void gear_clicked_cb(lv_event_t *e)
{
    cct_device_t *cct    = (cct_device_t *)lv_event_get_user_data(e);
    current_ccttive_cct  = cct;

    printf("settings opened for cct [%s]\n", cct->name);
    sync_big_screen(cct);

    lv_obj_clear_flag(objects.cct_screen_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(objects.cct_screen_panel);
}


static void widget_intensity_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    cct_device_t *cct = (cct_device_t *)lv_event_get_user_data(e);

    if (target == cct->cct_light_intensity_plus_button)
    {
        if (cct->current_intensity == 0)
            cct->current_intensity = 1;
        else if (cct->current_intensity < 10)
            cct->current_intensity++;
    }
    else if (target == cct->cct_light_minus_intensity_button)
    {
        if (cct->current_intensity == 1)
            cct->current_intensity = 0;
        else if (cct->current_intensity > 1)
            cct->current_intensity--;
    }
    else if (target == cct->cct_light_indicator)
    {
        cct->current_intensity = lv_arc_get_value(cct->cct_light_indicator);
    }
    else if (target == cct->button_cum_img_on_off_cct)
    {
        cct->is_on = !cct->is_on;
        cct->current_intensity = cct->is_on ? 10 : 0;
        printf("cct [%s] %s\n", cct->name, cct->is_on ? "ON" : "OFF");
    }

    // Single source of truth
    cct->is_on = (cct->current_intensity > 0);

    // Sync arc
    lv_arc_set_value(cct->cct_light_indicator, cct->current_intensity);

    // Sync label — empty when OFF
    if (cct->current_intensity == 0)
        lv_label_set_text(cct->cct_light_intensity_label, " ");
    else
    {
        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%d", cct->current_intensity);
        lv_label_set_text(cct->cct_light_intensity_label, buf);
    }

    // Sync img checked state
    if (cct->is_on)
        lv_obj_add_state(cct->button_cum_img_on_off_cct, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(cct->button_cum_img_on_off_cct, LV_STATE_CHECKED);
    lv_obj_invalidate(cct->button_cum_img_on_off_cct);

    // Sync big screen if open for this cct
    if (current_ccttive_cct == cct)
        sync_big_screen(cct);
}


static void temperature_intensity_callback(lv_event_t *e)
{
    lv_obj_t         *target = lv_event_get_target(e);
    cct_temperature_t temp   = obj_to_temperature(target);

    // Force radio behaviour — one ON rest OFF
    home_temp_select(target);

    // Apply to all cct widgets
    for (int i = 0; i < cct_count; i++) {
        apply_temperature_to_widget(&my_ccts[i], temp);
    }
    printf("Global temp → %dK for all %d CCT lights\n", (int)temp, cct_count);
}

// FIX: screen panel temp buttons — same radio fix
static void screen_temperature_cb(lv_event_t *e)
{
    if (current_ccttive_cct == NULL) return;

    lv_obj_t         *target = lv_event_get_target(e);
    cct_temperature_t temp   = screen_obj_to_temperature(target);

    // Force radio behaviour — one ON rest OFF
    screen_temp_select(target);

    apply_temperature_to_widget(current_ccttive_cct, temp);
}


static void screen_slider_cb(lv_event_t *e)
{
    if (current_ccttive_cct == NULL)
        return;

    int val = lv_slider_get_value(objects.cct_light_intensity_slider_main_screen);
    current_ccttive_cct->current_intensity = val;
    current_ccttive_cct->is_on = (val > 0);

    // Sync arc
    lv_arc_set_value(current_ccttive_cct->cct_light_indicator, val);

    // Sync label — empty when OFF
    if (val == 0)
        lv_label_set_text(current_ccttive_cct->cct_light_intensity_label, " ");
    else
    {
        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%d", val);
        lv_label_set_text(current_ccttive_cct->cct_light_intensity_label, buf);
    }

    // Sync img
    if (current_ccttive_cct->is_on)
        lv_obj_add_state(current_ccttive_cct->button_cum_img_on_off_cct, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(current_ccttive_cct->button_cum_img_on_off_cct, LV_STATE_CHECKED);
    lv_obj_invalidate(current_ccttive_cct->button_cum_img_on_off_cct);

    // Sync big screen label
    char buf2[24];
    if (val == 0)
        lv_label_set_text(objects.intensity_label_cct_screen, "Set Intensity:");
    else
    {
        lv_snprintf(buf2, sizeof(buf2), "Set Intensity: %d", val);
        lv_label_set_text(objects.intensity_label_cct_screen, buf2);
    }

    printf("cct [%s] intensity (slider) → %d\n", current_ccttive_cct->name, val);
}


static void cct_but_to_main_scr_callbcctk(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    if (target == objects.cct_back_but_to_main_scr) {
        lv_obj_add_flag(objects.cct_rgb_lights, LV_OBJ_FLAG_HIDDEN);
        current_ccttive_cct = NULL;
    } else if (target == objects.back_button_cct_screen) {
        lv_obj_add_flag(objects.cct_screen_panel, LV_OBJ_FLAG_HIDDEN);
        current_ccttive_cct = NULL;
    }
}

void reset_cct_widgets(void)
{
    cct_count = 0;
    lv_obj_clean(objects.cct_rgb_light_panel);
    printf("CCT reset OK, cct_count=0\n");
}

// ============================================================================
// INIT
// ============================================================================
void cct_rgb_light(void)
{
    // Slider range locked once at boot
    lv_slider_set_range(objects.cct_light_intensity_slider_main_screen, 0, 10);

    // Static label set once — never changes
    
    lv_obj_add_event_cb(objects.cct_back_but_to_main_scr, cct_but_to_main_scr_callbcctk, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.back_button_cct_screen,   cct_but_to_main_scr_callbcctk, LV_EVENT_CLICKED, NULL);

    
    lv_obj_add_event_cb(objects.sixtyfive_k_cct_lights,  temperature_intensity_callback, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.fifty_five_k_cct_lights, temperature_intensity_callback, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.five_k_cct_lights,       temperature_intensity_callback, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.four_five_k_cct_lights,  temperature_intensity_callback, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.four_k_cct_lights,       temperature_intensity_callback, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.three_k_five_cct_lights, temperature_intensity_callback, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.three_k_cct_lights,      temperature_intensity_callback, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.two_k_five_cct_lights,   temperature_intensity_callback, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_event_cb(objects.cct_light_intensity_slider_main_screen, screen_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // FIX: use LV_EVENT_VALUE_CHANGED for screen panel checkable imgbuttons too
    lv_obj_add_event_cb(objects.six_k_five_img_button_cct_screen,    screen_temperature_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.five_k_five_img_button_cct_screen,   screen_temperature_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.five_k_img_button_cct_screen,        screen_temperature_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.fourty_k_five_img_button_cct_screen, screen_temperature_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.four_k_img_button_cct_screen,        screen_temperature_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.three_k_five_img_button_cct_screen,  screen_temperature_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.three_k_img_button_cct_screen,       screen_temperature_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(objects.two_k_five_img_button_cct_screen,    screen_temperature_cb, LV_EVENT_VALUE_CHANGED, NULL);

    #ifdef MANUAL
    
    create_cct_widget("CCT Light 1",objects.cct_rgb_light_panel);
    create_cct_widget("CCT Light 2",objects.cct_rgb_light_panel);
    create_cct_widget("TV Lamp 1",objects.cct_rgb_light_panel);
    create_cct_widget("CCT Light 3",objects.cct_rgb_light_panel);
    create_cct_widget("Wall Light 1",objects.cct_rgb_light_panel);
    create_cct_widget("Wall Light 2",objects.cct_rgb_light_panel);
    create_cct_widget("CCT Light 4",objects.cct_rgb_light_panel);
    create_cct_widget("CCT Light 5",objects.cct_rgb_light_panel);
    create_cct_widget("CCT Light 6",objects.cct_rgb_light_panel);
    create_cct_widget("Window Light 6",objects.cct_rgb_light_panel);
    create_ac_widget("AC one",objects.cct_rgb_light_panel);
    create_curtain_widget("Curtain one",objects.cct_rgb_light_panel);

    #endif
}
// ============================================================================
// WIDGET FACTORY
// ============================================================================
void create_cct_widget(const char *cct_name,lv_obj_t *parent_panel)
{
    if (cct_count >= MAX_CCT) {
        printf("ERROR: MAX_CCT limit reached!\n");
        return;
    }

    cct_device_t *cct      = &my_ccts[cct_count];
    cct->id                = cct_count;
    cct->is_on             = false;
    cct->current_intensity = 0;
    cct->current_temperature = CCT_TEMP_4000K;  // default warm-neutral
    lv_snprintf(cct->name, sizeof(cct->name), "%s", cct_name);

    create_user_widget_cct_light_but(parent_panel, NULL, 0);
    uint32_t total            = lv_obj_get_child_cnt(parent_panel);
    cct->cct_light_button     = lv_obj_get_child(parent_panel, total - 1);


    cct->cct_light_indicator              = lv_obj_get_child(cct->cct_light_button, 0);
    cct->button_cum_img_on_off_cct        = lv_obj_get_child(cct->cct_light_indicator, 0);
    cct->cct_light_intensity_label        = lv_obj_get_child(cct->cct_light_button, 1);
    cct->cct_light_home_screen_img_button = lv_obj_get_child(cct->cct_light_button, 2);
    cct->cct_light_intensity_plus_button  = lv_obj_get_child(cct->cct_light_button, 3);
    cct->cct_light_minus_intensity_button = lv_obj_get_child(cct->cct_light_button, 4);
    cct->cct_light_main_name_label        = lv_obj_get_child(cct->cct_light_button, 5);

    // Initial UI state
    lv_arc_set_range(cct->cct_light_indicator, 0, 10);
    lv_arc_set_value(cct->cct_light_indicator, cct->current_intensity);
    lv_label_set_text(cct->cct_light_main_name_label, cct->name);
    lv_label_set_text(cct->cct_light_intensity_label, " ");

    // Bind widget events
    lv_obj_add_event_cb(cct->cct_light_intensity_plus_button,  widget_intensity_cb, LV_EVENT_CLICKED,       cct);
    lv_obj_add_event_cb(cct->cct_light_minus_intensity_button, widget_intensity_cb, LV_EVENT_CLICKED,       cct);
    lv_obj_add_event_cb(cct->cct_light_indicator,              widget_intensity_cb, LV_EVENT_VALUE_CHANGED, cct);
    lv_obj_add_event_cb(cct->button_cum_img_on_off_cct,        widget_intensity_cb, LV_EVENT_CLICKED,       cct);
    lv_obj_add_event_cb(cct->cct_light_home_screen_img_button, gear_clicked_cb,     LV_EVENT_CLICKED,       cct);

    cct_count++;
    printf("CCT widget created: [%s] id=%d\n", cct->name, cct->id);
}

void create_cct_widget_big(const char *cct_name,lv_obj_t *parent_panel)
{
    if (cct_count >= MAX_CCT) {
        printf("ERROR: MAX_CCT limit reached!\n");
        return;
    }

    cct_device_t *cct      = &my_ccts[cct_count];
    cct->id                = cct_count;
    cct->is_on             = false;
    cct->current_intensity = 0;
    cct->current_temperature = CCT_TEMP_4000K;  // default warm-neutral
    lv_snprintf(cct->name, sizeof(cct->name), "%s", cct_name);

    create_user_widget_cct_light_but_big(parent_panel, NULL, 0);
    uint32_t total            = lv_obj_get_child_cnt(parent_panel);
    cct->cct_light_button     = lv_obj_get_child(parent_panel, total - 1);


    cct->cct_light_indicator              = lv_obj_get_child(cct->cct_light_button, 0);
    cct->button_cum_img_on_off_cct        = lv_obj_get_child(cct->cct_light_indicator, 0);
    cct->cct_light_intensity_label        = lv_obj_get_child(cct->cct_light_button, 1);
    cct->cct_light_home_screen_img_button = lv_obj_get_child(cct->cct_light_button, 2);
    cct->cct_light_intensity_plus_button  = lv_obj_get_child(cct->cct_light_button, 3);
    cct->cct_light_minus_intensity_button = lv_obj_get_child(cct->cct_light_button, 4);
    cct->cct_light_main_name_label        = lv_obj_get_child(cct->cct_light_button, 5);

    // Initial UI state
    lv_arc_set_range(cct->cct_light_indicator, 0, 10);
    lv_arc_set_value(cct->cct_light_indicator, cct->current_intensity);
    lv_label_set_text(cct->cct_light_main_name_label, cct->name);
    lv_label_set_text(cct->cct_light_intensity_label, " ");

    // Bind widget events
    lv_obj_add_event_cb(cct->cct_light_intensity_plus_button,  widget_intensity_cb, LV_EVENT_CLICKED,       cct);
    lv_obj_add_event_cb(cct->cct_light_minus_intensity_button, widget_intensity_cb, LV_EVENT_CLICKED,       cct);
    lv_obj_add_event_cb(cct->cct_light_indicator,              widget_intensity_cb, LV_EVENT_VALUE_CHANGED, cct);
    lv_obj_add_event_cb(cct->button_cum_img_on_off_cct,        widget_intensity_cb, LV_EVENT_CLICKED,       cct);
    lv_obj_add_event_cb(cct->cct_light_home_screen_img_button, gear_clicked_cb,     LV_EVENT_CLICKED,       cct);

    cct_count++;
    printf("CCT widget created: [%s] id=%d\n", cct->name, cct->id);
}

void turn_off_all_lights_cct(void)
{
       for (int i = 0; i < cct_count; i++)
    {
        cct_device_t *cct = &my_ccts[i];
        cct->is_on = false;
        cct->current_intensity = 0;

        lv_arc_set_value(cct->cct_light_indicator, 0);
        lv_label_set_text(cct->cct_light_intensity_label, " ");

        // Clear checked state on the ON/OFF image button
        lv_obj_clear_state(cct->button_cum_img_on_off_cct, LV_STATE_CHECKED);
        lv_obj_invalidate(cct->button_cum_img_on_off_cct);
    }
    printf("Turned Off all CCT lights\n");
}