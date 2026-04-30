#include <stdio.h>
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
#include "ui_ees/screens.h"
#include "fonts.h"

#define COLOR_NORMAL lv_color_hex(0xFFFFFF)   // White
#define COLOR_SELECTED lv_color_hex(0xFFBA00) // Orange/Yellow

int brightness_val = 5; // default brightness
static lv_obj_t *settings_dropdown_list = NULL;
lv_obj_t *popup = NULL;
bool is_24h_format = false;

static bool child_locked = false;
static lv_obj_t *lock_overlay = NULL;

/* Forward declarations */
static void overlay_touch_cb(lv_event_t *e);
static void ok_btn_cb(lv_event_t *e);
static void cancel_btn_cb(lv_event_t *e);
static void show_pin_panel(void);
static void hide_pin_panel(void);
static void text_area_cb(lv_event_t *e);

static void list_item_clicked_cb(lv_event_t *e);

// ─────────────────────────────────────────────
// Shared callback for all four buttons
// ─────────────────────────────────────────────
static void format_selection_cb(lv_event_t *e)
{
    lv_obj_t *clicked_btn = lv_event_get_target(e);

    if (clicked_btn == objects.twelveformat_time_button)
    {
        is_24h_format = false;

        lv_obj_set_style_bg_color(objects.twelveformat_time_button, COLOR_SELECTED, 0);
        lv_obj_set_style_bg_color(objects.twentyfourhr_button, COLOR_NORMAL, 0);
    }
    else if (clicked_btn == objects.twentyfourhr_button)
    {
        is_24h_format = true;

        lv_obj_set_style_bg_color(objects.twentyfourhr_button, COLOR_SELECTED, 0);
        lv_obj_set_style_bg_color(objects.twelveformat_time_button, COLOR_NORMAL, 0);
    }
    else if (clicked_btn == objects.time_format_ok_button ||
             clicked_btn == objects.time_format_cancel_button)
    {
        if (objects.time_format_selection_screen != NULL)
        {
            lv_obj_add_flag(objects.time_format_selection_screen, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void slect_device_callback(lv_event_t *e)
{
    lv_obj_t *clicked_btn = lv_event_get_target(e);

    if (clicked_btn == objects.back_button_order_sel_screen)
    {
        lv_obj_add_flag(objects.selec_order_devices, LV_OBJ_FLAG_HIDDEN);
    }

    else if (clicked_btn == objects.slec_order_save_but)
    {
        reset_mid_panel();
        apply_order_to_mid_panel();
        lv_obj_add_flag(objects.selec_order_devices, LV_OBJ_FLAG_HIDDEN);
    }
}

static void cancel_clicked_cb(lv_event_t *e)
{
    printf("Brightness adjustment cancelled.\n");

    // Hide the brightness panel without saving
    if (objects.brightness_panel != NULL)
    {
        lv_obj_add_flag(objects.brightness_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

// --- OK Callback ---ok brightness
static void ok_clicked_cb(lv_event_t *e)
{

    // 3. Hide the brightness panel
    if (objects.brightness_panel != NULL)
    {
        lv_obj_add_flag(objects.brightness_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

// Add this at the bottom of lower_screen.c
void hide_settings_dropdown(void) // for setting label hiding
{
    // If the list exists and is currently visible, hide it!
    if (settings_dropdown_list != NULL && !lv_obj_has_flag(settings_dropdown_list, LV_OBJ_FLAG_HIDDEN))
    {
        lv_obj_add_flag(settings_dropdown_list, LV_OBJ_FLAG_HIDDEN);
    }
}

static void location_btn_cb(lv_event_t *e)
{
    printf("Location button pressed\n");
}

static void light_off_cb(lv_event_t *e)
{
    printf("Light OFF\n");
    turn_off_all_lights();
    turn_off_all_lights_cct();
    turn_off_rgbw_lights();
}

static void scenes_cb(lv_event_t *e)
{
    printf("Scenes button pressed\n");

    if (objects.cct_rgb_lights == NULL)
        return;
    lv_obj_clear_flag(objects.cct_rgb_lights, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(objects.cct_rgb_lights);
}

static void screen_off_cb(lv_event_t *e)
{
    printf("Screen OFF button pressed\n");
    manual_screen_off();
}

static void settings_cb(lv_event_t *e)
{
    lv_obj_t *settings_btn = lv_event_get_target(e);

    // REMOVED the auto-delete block.
    // We want the list to stay in memory so we can toggle it!

    if (settings_dropdown_list == NULL)
    {
        // 1. Create list on the top glass layer
        settings_dropdown_list = lv_list_create(lv_layer_top());

        // 2. Size and Alignment
        lv_obj_set_size(settings_dropdown_list, 250, 270);
        lv_obj_align_to(settings_dropdown_list, settings_btn, LV_ALIGN_OUT_TOP_RIGHT, 90, -10);

        // 3. STYLING THE MAIN LIST (Black Background: 0x070707)
        lv_obj_set_style_bg_color(settings_dropdown_list, lv_color_hex(0x070707), 0);
        lv_obj_set_style_border_width(settings_dropdown_list, 2, 0);
        lv_obj_set_style_border_color(settings_dropdown_list, lv_color_hex(0x333333), 0);
        lv_obj_set_style_pad_all(settings_dropdown_list, 0, 0);

        // Your specific labels
        const char *items[] = {
            "Select devices",
            "Adjust Brightness", "Screen saver",
            "Time format"};

        // 4. Generate the buttons and style them
        for (int i = 0; i < 4; i++)
        {
            lv_obj_t *btn = lv_list_add_btn(settings_dropdown_list, NULL, items[i]);

            lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x070707), 0);
            lv_obj_set_style_border_width(btn, 0, 0);

            lv_obj_set_style_pad_top(btn, 20, 0);
            lv_obj_set_style_pad_bottom(btn, 20, 0);
            lv_obj_set_style_pad_left(btn, 20, 0);

            lv_obj_set_style_text_font(btn, &lv_font_montserrat_20, 0);

            lv_obj_add_event_cb(btn, list_item_clicked_cb, LV_EVENT_CLICKED, NULL);
        }
    }
    else
    {
        // Now this toggle logic will actually run!
        if (lv_obj_has_flag(settings_dropdown_list, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_remove_flag(settings_dropdown_list, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(settings_dropdown_list, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
static void slider_clicked_cb(lv_event_t *e)
{
    // Get the object that triggered the event (the slider)
    lv_obj_t *slider = lv_event_get_target(e);

    // Grab the current value
    brightness_val = lv_slider_get_value(slider);

    //  (Optional) If you must add 2 in code, cap it at 100 to prevent BSP errors
    brightness_val += 2;
    if (brightness_val > 100)
        brightness_val = 100;

    // Apply the brightness
    bsp_display_brightness_set(brightness_val);

    printf("Brightness dynamically set to: %d%%\n", brightness_val);
}

// This callback handles the clicks for all the items in the list
static void list_item_clicked_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    // Grab the text of the button so we know which one was clicked
    const char *item_text = lv_list_get_btn_text(settings_dropdown_list, btn);
    printf("Selected: %s\n", item_text);

    // Hide the list immediately after a selection is made
    if (settings_dropdown_list != NULL)
    {
        lv_obj_add_flag(settings_dropdown_list, LV_OBJ_FLAG_HIDDEN);
    }

    if (item_text != NULL)
    {
        // Check if the text matches exactly what you typed in your items[] array
        if (strcmp(item_text, "Adjust Brightness") == 0)
        {

            /// brightness screen
            if (objects.ok != NULL) // for brightness screen
            {
                lv_obj_set_ext_click_area(objects.ok, 20); // to increase touch screen
                lv_obj_add_event_cb(objects.ok, ok_clicked_cb, LV_EVENT_CLICKED, NULL);
            }

            if (objects.cancel != NULL)
            {
                lv_obj_set_ext_click_area(objects.cancel, 20); // to increase touch screen
                lv_obj_add_event_cb(objects.cancel, cancel_clicked_cb, LV_EVENT_CLICKED, NULL);
            }
            if (objects.slider != NULL)
            {
                lv_obj_add_event_cb(objects.slider, slider_clicked_cb, LV_EVENT_VALUE_CHANGED, NULL);
            }
            //

            // Make the EEZ Studio brightness panel visible
            if (objects.brightness_panel != NULL)
            {
                lv_obj_clear_flag(objects.brightness_panel, LV_OBJ_FLAG_HIDDEN);

                // Bring it to the front just in case it's rendering behind your other panels
                lv_obj_move_foreground(objects.brightness_panel);
            }
        }
        else if (strcmp(item_text, "Refresh") == 0)
        {
            // Code for Refresh goes here
            printf("Running refresh logic...\n");
        }
        else if (strcmp(item_text, "Time format") == 0)
        {
            // Attach the exact same callback to both buttons
            lv_obj_add_event_cb(objects.twelveformat_time_button, format_selection_cb, LV_EVENT_CLICKED, NULL);

            lv_obj_add_event_cb(objects.twentyfourhr_button, format_selection_cb, LV_EVENT_CLICKED, NULL);
            lv_obj_set_ext_click_area(objects.time_format_ok_button, 20);     // to increase touch scree
            lv_obj_set_ext_click_area(objects.time_format_cancel_button, 20); // to increase touch screen

            lv_obj_add_event_cb(objects.time_format_ok_button, format_selection_cb, LV_EVENT_CLICKED, NULL);
            lv_obj_add_event_cb(objects.time_format_cancel_button, format_selection_cb, LV_EVENT_CLICKED, NULL);

            if (objects.time_format_selection_screen != NULL)
            {
                lv_obj_clear_flag(objects.time_format_selection_screen, LV_OBJ_FLAG_HIDDEN);

                lv_obj_move_foreground(objects.time_format_selection_screen);

                // Reflect whichever format is currently active
                if (is_24h_format)
                {
                    lv_obj_set_style_bg_color(objects.twentyfourhr_button, COLOR_SELECTED, 0);
                    lv_obj_set_style_bg_color(objects.twelveformat_time_button, COLOR_NORMAL, 0);
                }
                else
                {
                    // Default: 12H is highlighted
                    lv_obj_set_style_bg_color(objects.twelveformat_time_button, COLOR_SELECTED, 0);
                    lv_obj_set_style_bg_color(objects.twentyfourhr_button, COLOR_NORMAL, 0);
                }
            }
        }

        else if (strcmp(item_text, "Screen saver") == 0)
        {
            hide_settings_dropdown();
            wohnux_main_ui_create();
            set_screensaver_active(true);
        }

        else if (strcmp(item_text, "Select devices") == 0)
        {

            lv_obj_add_event_cb(objects.back_button_order_sel_screen, slect_device_callback, LV_EVENT_CLICKED, NULL);

            lv_obj_add_event_cb(objects.slec_order_save_but, slect_device_callback, LV_EVENT_CLICKED, NULL);

            if (objects.selec_order_devices != NULL)
            {
                populate_order_list();
                build_select_order_widgets();
                lv_obj_clear_flag(objects.selec_order_devices, LV_OBJ_FLAG_HIDDEN);

                // Bring it to the front just in case it's rendering behind your other panels
                lv_obj_move_foreground(objects.selec_order_devices);
            }
        }
    }
}

/* ── Touch on black screen → show PIN panel + keyboard ──────────────────── */
static void overlay_touch_cb(lv_event_t *e)
{
    show_pin_panel();
}

/* ── Lock button callback ────────────────────────────────────────────────── */
static void lock_cb(lv_event_t *e)
{
    printf("Lock pressed\n");

    if (child_locked) return;
    child_locked = true;

    lock_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(lock_overlay, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(lock_overlay, 0, 0);
    lv_obj_set_style_bg_color(lock_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(lock_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(lock_overlay, 0, 0);
    lv_obj_set_style_radius(lock_overlay, 0, 0);
    lv_obj_clear_flag(lock_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_move_foreground(lock_overlay);  // ← ADD THIS — force it to top

    /* Also cover any dropdowns or popups that might be open */
    if (settings_dropdown_list != NULL)
        lv_obj_add_flag(settings_dropdown_list, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_parent(objects.child_lock_unlock_panel, lock_overlay);
    lv_obj_set_parent(objects.keyboard, lock_overlay);

    lv_obj_add_flag(objects.child_lock_unlock_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.keyboard, LV_OBJ_FLAG_HIDDEN);

    lv_keyboard_set_textarea(objects.keyboard, objects.textarea_pin_enter);
    lv_keyboard_set_mode(objects.keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_textarea_set_password_mode(objects.textarea_pin_enter, true);   // ← ADD
    lv_textarea_set_password_show_time(objects.textarea_pin_enter, 0); // ← ADD

    lv_obj_remove_event_cb(objects.ok_child_lock_panel, ok_btn_cb);
    lv_obj_remove_event_cb(objects.cancel_child_lock_panel, cancel_btn_cb);
    lv_obj_add_event_cb(lock_overlay, overlay_touch_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.ok_child_lock_panel, ok_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.cancel_child_lock_panel, cancel_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.textarea_pin_enter, text_area_cb, LV_EVENT_CLICKED, NULL);

    printf("Lock overlay setup complete\n");
}


/* ── Show PIN panel ──────────────────────────────────────────────────────── */
static void show_pin_panel(void)
{
    printf("Showing PIN panel\n");

    lv_textarea_set_text(objects.textarea_pin_enter, "");
    lv_textarea_set_password_mode(objects.textarea_pin_enter, true);  // ← ADD
    lv_textarea_set_password_show_time(objects.textarea_pin_enter, 0);

    lv_obj_clear_flag(objects.child_lock_unlock_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(objects.child_lock_unlock_panel);
    lv_obj_remove_event_cb(lock_overlay, overlay_touch_cb);
}

/* ── Hide PIN panel ──────────────────────────────────────────────────────── */
static void hide_pin_panel(void)
{
    printf("Hiding PIN panel\n");

    lv_obj_add_flag(objects.child_lock_unlock_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(objects.keyboard, LV_OBJ_FLAG_HIDDEN);

    /* Re-enable overlay click */
    lv_obj_add_event_cb(lock_overlay, overlay_touch_cb, LV_EVENT_CLICKED, NULL);
}

/* ── OK pressed → validate PIN ───────────────────────────────────────────── */
static void ok_btn_cb(lv_event_t *e)
{

    const char *entered = lv_textarea_get_text(objects.textarea_pin_enter);
    printf("PIN entered: %s\n", entered);

    char str[20]; // Ensure buffer is large enough

    // Syntax: snprintf(buffer, size, format, variable)
    snprintf(str, sizeof(str), "%d", g_config.childlock);

    if (strcmp(entered, str) == 0)
    {
        printf("PIN correct - unlocking\n");

        lv_obj_add_flag(objects.invalid_child_lock_label, LV_OBJ_FLAG_HIDDEN);

        /* Move panel and keyboard back to the real main screen */
        lv_obj_set_parent(objects.child_lock_unlock_panel, real_main_screen);
        lv_obj_set_parent(objects.keyboard, real_main_screen);

        /* Hide them */
        lv_obj_add_flag(objects.child_lock_unlock_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.keyboard, LV_OBJ_FLAG_HIDDEN);

        /* Destroy overlay */
        lv_obj_del(lock_overlay);
        lock_overlay = NULL;
        child_locked = false;
    }
    else
    {
        printf("Wrong PIN - try again\n");
        lv_textarea_set_text(objects.textarea_pin_enter, "");
        lv_obj_clear_flag(objects.invalid_child_lock_label, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ── Cancel pressed → back to black screen ───────────────────────────────── */
static void cancel_btn_cb(lv_event_t *e)
{
    hide_pin_panel();
}

static void text_area_cb(lv_event_t *e)
{
    lv_obj_clear_flag(objects.keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(objects.keyboard);
    lv_obj_set_pos(objects.keyboard, 0, 300);
    
    lv_textarea_set_password_mode(objects.textarea_pin_enter, true);  // ← ADD THIS
    lv_textarea_set_password_show_time(objects.textarea_pin_enter, 0);
}


void lower_panel(void)
{

    settings_dropdown_list = NULL;
    // lv_obj_set_style_pad_right(objects.lower_panel, 10, 0);

    lv_obj_add_event_cb(objects.obj0, location_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(objects.obj2, light_off_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(objects.obj3, scenes_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(objects.obj4, screen_off_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(objects.obj5, lock_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(objects.obj6, settings_cb, LV_EVENT_CLICKED, NULL);

    // Time format screen

    //
}

// 2. The Callback Function
