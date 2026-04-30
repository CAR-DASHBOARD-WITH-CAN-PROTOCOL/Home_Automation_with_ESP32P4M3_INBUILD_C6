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
#include <math.h>
#include "images.h"

// Globals to keep track of the selected color and the visual cursor
static lv_color_t current_selected_color;
static lv_obj_t *touch_cursor_obj = NULL;
static lv_obj_t *white_touch_cursor_obj = NULL;

static bool syncing_ui = false; // ← ADD THIS ONE LINE
void turn_off_rgbw_lights(void);

typedef struct
{
    int id;
    bool is_on;
    int current_intensity;
    int white_color_intensity;
    int temperature_light_intensity;
    rgb_current_t color;
    char name[32];

    lv_obj_t *rgbw_button;
    lv_obj_t *rgbw_label;
    lv_obj_t *rgbw_light_indicator;
    lv_obj_t *button_img_on_off_rgbw;
    lv_obj_t *reg_rgb_img;
    lv_obj_t *green_rgbw_img;
    lv_obj_t *blue_rgbw_img;
    lv_obj_t *color_panel_opening_img;
} rgb_device_t;

#define MAX_RGB 20
static rgb_device_t my_rgbs[MAX_RGB];
static int rgb_count = 0;
static rgb_device_t *active_rgb = NULL;

void create_rgb_widget(const char *rgb_name, lv_obj_t *parent_panel);
void create_rgb_widget_big(const char *rgb_name, lv_obj_t *parent_panel);
void rgbw_light_init(void);
static void sync_widget_ui(rgb_device_t *rgb);

// ─── Callback for the Big White Circle ───────────────────────────────────────
static void white_circle_event_cb(lv_event_t *e)
{
    lv_obj_t *img = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING)
    {
        lv_indev_t *indev = lv_indev_active();
        if (indev == NULL)
            return;

        lv_point_t p;
        lv_indev_get_point(indev, &p);

        // Get image coordinates and dimensions
        lv_area_t img_coords;
        lv_obj_get_coords(img, &img_coords);

        int img_w = lv_obj_get_width(img);
        int img_h = lv_obj_get_height(img);

        // 1. Calculate local X, Y relative to the image
        int touch_x = p.x - img_coords.x1;
        int touch_y = p.y - img_coords.y1;

        int center_x = img_w / 2;
        int center_y = img_h / 2;
        int dx = touch_x - center_x;
        int dy = touch_y - center_y;

        // 2. HITBOX CHECK: Only allow touches INSIDE the circle
        int radius = (img_w < img_h ? img_w : img_h) / 2;
        if (dx * dx + dy * dy > radius * radius)
        {
            return; // Clicked the black corners outside the circle, ignore it!
        }

        // 3. Update or Create the Touch Spot Cursor
        if (white_touch_cursor_obj == NULL)
        {
            white_touch_cursor_obj = lv_obj_create(objects.white_screen_rgb);
            lv_obj_set_size(white_touch_cursor_obj, 24, 24);
            lv_obj_set_style_radius(white_touch_cursor_obj, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_opa(white_touch_cursor_obj, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(white_touch_cursor_obj, 3, 0);
            lv_obj_set_style_border_color(white_touch_cursor_obj, lv_color_black(), 0);
            lv_obj_set_style_shadow_width(white_touch_cursor_obj, 5, 0);
            lv_obj_set_style_shadow_color(white_touch_cursor_obj, lv_color_white(), 0);
            lv_obj_clear_flag(white_touch_cursor_obj, LV_OBJ_FLAG_CLICKABLE);
        }

        // Move the cursor exactly to where the finger is
        lv_obj_set_pos(white_touch_cursor_obj, p.x - 12, p.y - 12);

        if (active_rgb != NULL)
        {
            // 4. Calculate the vertical percentage (0.0 = Top, 1.0 = Bottom)
            float percent = (float)touch_y / (float)img_h;
            if (percent < 0.0f)
                percent = 0.0f;
            if (percent > 1.0f)
                percent = 1.0f;

            // 5. Interpolate between Warm (2500K) and Cool (6500K)
            // Warm White RGB (Top of circle - 2500K Deep Amber)
            int warm_r = 255, warm_g = 161, warm_b = 72;

            // Cool White RGB (Bottom of circle - 6500K Cool Daylight)
            int cool_r = 220, cool_g = 235, cool_b = 255;

            int final_r = warm_r + (int)((cool_r - warm_r) * percent);
            int final_g = warm_g + (int)((cool_g - warm_g) * percent);
            int final_b = warm_b + (int)((cool_b - warm_b) * percent);

            // Calculate the Kelvin value for your backend logs
            int current_kelvin = 2500 + (int)(percent * (6500 - 2500));
            printf("[BACKEND LOG] Color Temp changed to: %dK\n", current_kelvin);

            // 6. Push to the Active Light and Sync the UI
            active_rgb->color.r = final_r;
            active_rgb->color.g = final_g;
            active_rgb->color.b = final_b;

            // Auto-turn on the light if it was off
            active_rgb->temperature_light_intensity = (int)(percent * 10.0f);
            active_rgb->is_on = true;

            // maps 0.0–1.0 → 0–10

            // Sync all UI elements (Slider, Widget Arc, etc.)
            sync_widget_ui(active_rgb);
        }
    }
}
// ─── Callback for clicking a historical Color Replica ────────────────────────
static void replica_click_cb(lv_event_t *e)
{
    lv_obj_t *replica = lv_event_get_target(e);

    // 1. Extract the color directly from this replica's style!
    lv_color_t saved_color = lv_obj_get_style_image_recolor(replica, LV_PART_MAIN);

    // 2. Update the global color
    current_selected_color = saved_color;

    // 3. Push this color to the active light and sync the UI
    if (active_rgb != NULL)
    {
        lv_color32_t c32 = lv_color_to_32(current_selected_color, 0xFF);
        active_rgb->color.r = c32.red;
        active_rgb->color.g = c32.green;
        active_rgb->color.b = c32.blue;

        // Turn the light on if it was off
        active_rgb->is_on = true;
        if (active_rgb->current_intensity == 0)
        {
            // active_rgb->current_intensity = 10;
        }

        // Master sync!
        sync_widget_ui(active_rgb);
    }
}

// ── arc colour from active channel ──────────────────────────────────────────
static lv_color_t channel_to_color(rgb_device_t *rgb)
{
    if (rgb->color.r > 0 && rgb->color.g == 0 && rgb->color.b == 0)
        return lv_color_hex(0xFF0000);
    if (rgb->color.g > 0 && rgb->color.r == 0 && rgb->color.b == 0)
        return lv_color_hex(0x00FF00);
    if (rgb->color.b > 0 && rgb->color.r == 0 && rgb->color.g == 0)
        return lv_color_hex(0x0000FF);
    return lv_color_hex(0xFFBA00);
}

// ── sync widget UI (The Single Source of Truth) ──────────────────────────────
static void sync_widget_ui(rgb_device_t *rgb)
{
    if (!rgb)
        return;

    // 1. Generate the exact LVGL color from the struct's custom RGB values
    lv_color_t col = lv_color_hex((rgb->color.r << 16) | (rgb->color.g << 8) | rgb->color.b);

    // If the light is ON but the color is completely zero, default to warm yellow
    if (rgb->is_on && rgb->color.r == 0 && rgb->color.g == 0 && rgb->color.b == 0)
    {
        col = lv_color_hex(0xFFBA00);
    }

    // 2. Sync the Arc (Snap to 10 if ON, 0 if OFF. Ignore partial intensity)
    lv_arc_set_value(rgb->rgbw_light_indicator, rgb->is_on ? 10 : 0);
    lv_obj_set_style_arc_color(rgb->rgbw_light_indicator, col, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // 3. Sync the On/Off Button Image
    if (rgb->is_on)
    {
        lv_obj_add_state(rgb->button_img_on_off_rgbw, LV_STATE_CHECKED);
        lv_obj_set_style_image_recolor(rgb->button_img_on_off_rgbw, col, 0);
        lv_obj_set_style_image_recolor_opa(rgb->button_img_on_off_rgbw, LV_OPA_COVER, 0); // LV_OPA_COVER is 255
    }
    else
    {
        lv_obj_clear_state(rgb->button_img_on_off_rgbw, LV_STATE_CHECKED);
        lv_obj_set_style_image_recolor(rgb->button_img_on_off_rgbw, lv_color_hex(0x818499), 0);
        lv_obj_set_style_image_recolor_opa(rgb->button_img_on_off_rgbw, LV_OPA_COVER, 0);
    }

    // 5. Sync the Big Screen components IF this is the active RGB being edited
    if (active_rgb == rgb)
    {
        // 1. ALWAYS update the physical knob positions of all 3 sliders
        syncing_ui = true; // ← ADD BEFORE

        lv_slider_set_value(objects.color_panel_color_slider, rgb->current_intensity, LV_ANIM_OFF);
        lv_slider_set_value(objects.white_scn_slider, rgb->temperature_light_intensity, LV_ANIM_OFF);
        lv_slider_set_value(objects.white_intensity_slider, rgb->white_color_intensity, LV_ANIM_OFF);
        syncing_ui = false; // ← ADD AFTER

        // 2. ALWAYS force the pure white slider to stay white visually
        lv_obj_set_style_bg_color(objects.white_intensity_slider, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(objects.white_intensity_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);

        // 3. Handle the dynamic colors for the other two sliders
        bool has_color = (rgb->color.r > 0 || rgb->color.g > 0 || rgb->color.b > 0);

        if (has_color)
        {
            // OVERWRITE WITH NEW COLOR: Apply the exact color they picked
            lv_obj_set_style_image_recolor(objects.color_picker_image, col, LV_PART_MAIN);
            lv_obj_set_style_image_recolor_opa(objects.color_picker_image, LV_OPA_COVER, LV_PART_MAIN);

            // Sync RGB slider color
            lv_obj_set_style_bg_color(objects.color_panel_color_slider, col, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(objects.color_panel_color_slider, col, LV_PART_KNOB | LV_STATE_DEFAULT);

            // Sync White Temperature slider color
            lv_obj_set_style_bg_color(objects.white_scn_slider, col, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(objects.white_scn_slider, col, LV_PART_KNOB | LV_STATE_DEFAULT);
        }
        else
        {
            // NO COLOR PICKED YET: Remove the C-code styles so the EEZ Studio default shows!
            lv_obj_set_style_image_recolor_opa(objects.color_picker_image, 0, LV_PART_MAIN);

            // Set the sliders back to neutral grey/white
            lv_obj_set_style_bg_color(objects.color_panel_color_slider, lv_color_hex(0x000000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(objects.color_panel_color_slider, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_DEFAULT);

            lv_obj_set_style_bg_color(objects.white_scn_slider, lv_color_hex(0xffffff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(objects.white_scn_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
        }
    }

    printf("rgb [%s] on=%d R=%d G=%d B=%d Color Intensity=%d White Color Intensity=%d,Temp light intensity=%d\n",
           rgb->name, rgb->is_on, rgb->color.r, rgb->color.g, rgb->color.b, rgb->current_intensity,
           rgb->white_color_intensity, rgb->temperature_light_intensity);
}

static void sync_big_screen(rgb_device_t *rgb)
{
    if (!rgb)
        return;

    // Update the title on the RGB Color panel
    lv_label_set_text(objects.rgbw_screen_label, rgb->name);

    // Update the title on the new White Color panel
    lv_label_set_text(objects.label_white_rgb_screen, rgb->name);
}
// ── widget button callback ───────────────────────────────────────────────────
static void rgb_widget_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    rgb_device_t *rgb = (rgb_device_t *)lv_event_get_user_data(e);

    if (target == rgb->button_img_on_off_rgbw)
    {
        rgb->is_on = !rgb->is_on;

        // FIX: Handle intensity without erasing color
        if (rgb->is_on && rgb->current_intensity == 0)
        {
            // rgb->current_intensity = 10;
        }
        else if (!rgb->is_on)
        {
            rgb->current_intensity = 0;
        }
    }
    else if (target == rgb->reg_rgb_img)
    {
        rgb->color.r = 255;
        rgb->color.g = 0;
        rgb->color.b = 0;
        rgb->is_on = true;
        rgb->current_intensity = 0; // Start at 0
    }
    else if (target == rgb->green_rgbw_img)
    {
        rgb->color.r = 0;
        rgb->color.g = 255;
        rgb->color.b = 0;
        rgb->is_on = true;
        rgb->current_intensity = 0; // Start at 0
    }
    else if (target == rgb->blue_rgbw_img)
    {
        rgb->color.r = 0;
        rgb->color.g = 0;
        rgb->color.b = 255;
        rgb->is_on = true;
        rgb->current_intensity = 0; // Start at 0
    }
    else if (target == rgb->color_panel_opening_img)
    {
        active_rgb = rgb;
        sync_big_screen(rgb);

        // Ensure the global color matches this widget before opening screen
        current_selected_color = lv_color_hex((rgb->color.r << 16) | (rgb->color.g << 8) | rgb->color.b);

        sync_widget_ui(rgb); // Force update to the slider & picker

        lv_obj_clear_flag(objects.rgb_screen_container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(objects.rgb_screen_container);
        return;
    }

    // Keep the global color variable perfectly in sync with our R/G/B buttons
    current_selected_color = lv_color_hex((rgb->color.r << 16) | (rgb->color.g << 8) | rgb->color.b);

    // Call the master sync!
    sync_widget_ui(rgb);
}
// ── arc drag: snap full or zero ──────────────────────────────────────────────
// ── arc drag: snap full or zero ──────────────────────────────────────────────
static void rgb_arc_cb(lv_event_t *e)
{
    rgb_device_t *rgb = (rgb_device_t *)lv_event_get_user_data(e);
    int val = lv_arc_get_value(rgb->rgbw_light_indicator);

    rgb->is_on = (val > 1); // 5 is a better threshold for a 10-point arc

    // FIX: If turned on, give it intensity. If off, zero the intensity.
    if (rgb->is_on && rgb->current_intensity == 0)
    {
        // rgb->current_intensity = 10;
    }
    else if (!rgb->is_on)
    {
        rgb->current_intensity = 0;
    }

    // FIX: Removed the code that erased rgb->color to 0! Never erase color memory!

    sync_widget_ui(rgb);
    if (active_rgb == rgb)
        sync_big_screen(rgb);
}

static void rgb_screen_slider_cb(lv_event_t *e)
{
    if (syncing_ui)
        return; // ← ADD THIS ONE LINE — blocks recursive ca

    if (!active_rgb)
        return;
    lv_obj_t *target = lv_event_get_target(e);
    if (target == objects.color_panel_color_slider)
    {

        int val = lv_slider_get_value(target);

        active_rgb->current_intensity = val;
        active_rgb->is_on = (val > 0);
    }
    else if (target == objects.white_intensity_slider)
    {

        int val_white = lv_slider_get_value(target);

        active_rgb->white_color_intensity = val_white;
        active_rgb->is_on = (val_white > 0);
    }
    else if (target == objects.white_scn_slider) // Handles your new white screen slider
    {
        int val_white_white_sc = lv_slider_get_value(target);
        active_rgb->temperature_light_intensity = val_white_white_sc;
        active_rgb->is_on = (val_white_white_sc > 0);
        printf("White Intensity changed to: %d\n", val_white_white_sc);
    }

    sync_widget_ui(active_rgb);
}

static void rgb_callback(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    if (target == objects.back_button_label_rgb_screen)
    {
        lv_obj_add_flag(objects.rgb_screen_container, LV_OBJ_FLAG_HIDDEN);
        active_rgb = NULL;
    }
    else if (target == objects.white_panel_back_button_label)
    {
        lv_obj_add_flag(objects.white_screen_rgb, LV_OBJ_FLAG_HIDDEN);
    }
    else if (target == objects.small_white_img)
    {
        lv_obj_clear_flag(objects.white_screen_rgb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(objects.white_screen_rgb);
    }
    else if (target == objects.color_img_inside_white_panel)
    {
        lv_obj_add_flag(objects.white_screen_rgb, LV_OBJ_FLAG_HIDDEN);

        lv_obj_clear_flag(objects.rgb_screen_container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(objects.rgb_screen_container);
    }
}

void reset_rgb_widgets(void)
{
    rgb_count = 0;
    active_rgb = NULL;
    lv_obj_clean(objects.cct_rgb_light_panel);
    printf("RGB reset OK\n");
}

// ─── Callback for the Big Color Wheel ───────────────────────────────────────
static void color_circle_event_cb(lv_event_t *e)
{
    lv_obj_t *img = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING)
    {
        lv_indev_t *indev = lv_indev_active();
        if (indev == NULL)
            return;

        lv_point_t p;
        lv_indev_get_point(indev, &p);

        // Get image coordinates and dimensions
        lv_area_t img_coords;
        lv_obj_get_coords(img, &img_coords);

        int img_w = lv_obj_get_width(img);
        int img_h = lv_obj_get_height(img);

        // 1. Calculate local X, Y relative to the image
        int touch_x = p.x - img_coords.x1;
        int touch_y = p.y - img_coords.y1;
        // 2. Math to find Hue (Angle) and Saturation (Distance from center)
        int center_x = img_w / 2;
        int center_y = img_h / 2;
        int dx = touch_x - center_x;
        int dy = touch_y - center_y;

        // Calculate Hue (0 to 359 degrees)
        int angle = (int)(atan2(dy, dx) * 180.0f / 3.14159265f);
        if (angle < 0)
            angle += 360;
        uint16_t hue = angle;

        // Calculate Saturation and distance
        int radius = (int)sqrt(dx * dx + dy * dy);
        int max_radius = img_w / 2;
        int sat = (radius * 100) / max_radius;
        if (sat > 100)
            sat = 100; // Cap saturation at edges

        // Convert HSV to LVGL RGB Color
        current_selected_color = lv_color_hsv_to_rgb(hue, sat, 100);

        // ---> NEW MATH: Constrain the cursor to the circle boundary <---
        int constrained_screen_x = p.x;
        int constrained_screen_y = p.y;

        if (radius > max_radius)
        {
            // Finger is outside the circle! Calculate where the edge is.
            float ratio = (float)max_radius / (float)radius;
            constrained_screen_x = img_coords.x1 + center_x + (int)(dx * ratio);
            constrained_screen_y = img_coords.y1 + center_y + (int)(dy * ratio);
        }

        // Update or Create the Touch Spot Cursor
        if (touch_cursor_obj == NULL)
        {
            // ... (keep your cursor creation code here unchanged) ...
            touch_cursor_obj = lv_obj_create(objects.rgb_screen_container);
            lv_obj_set_size(touch_cursor_obj, 24, 24);
            lv_obj_set_style_radius(touch_cursor_obj, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_opa(touch_cursor_obj, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(touch_cursor_obj, 3, 0);
            lv_obj_set_style_border_color(touch_cursor_obj, lv_color_white(), 0);
            lv_obj_set_style_shadow_width(touch_cursor_obj, 5, 0);
            lv_obj_set_style_shadow_color(touch_cursor_obj, lv_color_black(), 0);
            lv_obj_clear_flag(touch_cursor_obj, LV_OBJ_FLAG_CLICKABLE);
        }

        // ---> FIX: Use the mathematically constrained coordinates, NOT p.x/p.y <---
        lv_obj_set_pos(touch_cursor_obj, constrained_screen_x - 12, constrained_screen_y - 12);
        // Simply update the active_rgb logic, then let the sync function do the UI styling
        if (active_rgb != NULL)
        {
            // Convert to 32-bit to easily grab R, G, B
            lv_color32_t c32 = lv_color_to_32(current_selected_color, 0xFF);
            active_rgb->color.r = c32.red;
            active_rgb->color.g = c32.green;
            active_rgb->color.b = c32.blue;

            // If they pick a color, automatically turn the light on
            active_rgb->is_on = true;

            if (active_rgb->current_intensity == 0)
            {
                // active_rgb->current_intensity = 10;
            }

            // Call our master sync function!
            sync_widget_ui(active_rgb);
        }
    }
}

// ─── Callback for the Plus/Picker Image ──────────────────────────────────────
static void color_picker_img_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED)
    {
        // Create a new image inside the flex container
        lv_obj_t *replica = lv_image_create(objects.color_selected_flex_container);
        lv_image_set_src(replica, &img_color_picker_change_img);

        // Apply the newly selected color to the replica
        lv_obj_set_style_image_recolor(replica, current_selected_color, LV_PART_MAIN);
        lv_obj_set_style_image_recolor_opa(replica, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_pad_right(replica, 10, LV_PART_MAIN);

        // ---> NEW: Make the replica a clickable button! <---
        lv_obj_add_flag(replica, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(replica, replica_click_cb, LV_EVENT_CLICKED, NULL);

        // Move this replica to index 1 (right after the main picker button)
        lv_obj_move_to_index(replica, 1);
    }
}

void rgbw_light_init(void)
{
    lv_obj_add_event_cb(objects.back_button_label_rgb_screen, rgb_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_set_ext_click_area(objects.back_button_label_rgb_screen, 20);
    lv_obj_add_event_cb(objects.white_panel_back_button_label, rgb_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_set_ext_click_area(objects.white_panel_back_button_label, 20);
    lv_obj_add_event_cb(objects.small_white_img, rgb_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(objects.color_img_inside_white_panel, rgb_callback, LV_EVENT_CLICKED, NULL);

    // Make sure this image is clickable (it is in your code, but double check)
    lv_obj_add_flag(objects.color_circle_img, LV_OBJ_FLAG_CLICKABLE);

    // Attach the dragging event to the big wheel
    lv_obj_add_event_cb(objects.color_circle_img, color_circle_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(objects.color_circle_img, color_circle_event_cb, LV_EVENT_PRESSING, NULL);

    // Make the color picker target image clickable
    lv_obj_add_flag(objects.color_picker_image, LV_OBJ_FLAG_CLICKABLE);

    // Attach the click event to clone it into the flex box
    lv_obj_add_event_cb(objects.color_picker_image, color_picker_img_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(objects.color_panel_color_slider, rgb_screen_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_ext_click_area(objects.color_panel_color_slider, 20); // ← ADD
    lv_slider_set_range(objects.color_panel_color_slider, 0, 10);

    lv_obj_add_event_cb(objects.white_intensity_slider, rgb_screen_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_ext_click_area(objects.white_intensity_slider, 20); // ← ADD

    lv_slider_set_range(objects.white_intensity_slider, 0, 255);

    lv_obj_add_event_cb(objects.white_scn_slider, rgb_screen_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_ext_click_area(objects.white_scn_slider, 20); // ← ADD

    lv_slider_set_range(objects.white_scn_slider, 0, 10);
    lv_obj_add_event_cb(objects.while_cirler_big_img, white_circle_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(objects.while_cirler_big_img, white_circle_event_cb, LV_EVENT_PRESSING, NULL);
}

void create_rgb_widget(const char *rgb_name, lv_obj_t *parent_panel)
{
    if (rgb_count >= MAX_RGB)
    {
        printf("ERROR: MAX_RGB limit reached!\n");
        return;
    }

    // Snapshot index BEFORE building, so _rgb_setup uses the correct slot
    int this_idx = rgb_count;

    create_user_widget_rgbw_button(parent_panel, NULL, 0);

    // Count children AFTER adding the widget
    uint32_t total = lv_obj_get_child_cnt(parent_panel);

    rgb_device_t *rgb = &my_rgbs[this_idx];
    rgb->id = this_idx;
    rgb->is_on = false;
    rgb->current_intensity = 0;
    rgb->white_color_intensity = 0;
    rgb->temperature_light_intensity = 0;
    rgb->color.r = 0;
    rgb->color.g = 0;
    rgb->color.b = 0;
    lv_snprintf(rgb->name, sizeof(rgb->name), "%s", rgb_name);

    // Always grab the last child — the one just added
    rgb->rgbw_button = lv_obj_get_child(parent_panel, total - 1);

    rgb->rgbw_label = lv_obj_get_child(rgb->rgbw_button, 0);
    rgb->rgbw_light_indicator = lv_obj_get_child(rgb->rgbw_button, 1);
    rgb->button_img_on_off_rgbw = lv_obj_get_child(rgb->rgbw_light_indicator, 1);
    rgb->reg_rgb_img = lv_obj_get_child(rgb->rgbw_button, 2);
    rgb->green_rgbw_img = lv_obj_get_child(rgb->rgbw_button, 3);
    rgb->blue_rgbw_img = lv_obj_get_child(rgb->rgbw_button, 4);
    rgb->color_panel_opening_img = lv_obj_get_child(rgb->rgbw_button, 5);

    lv_label_set_text(rgb->rgbw_label, rgb->name);
    lv_arc_set_range(rgb->rgbw_light_indicator, 0, 10);
    lv_arc_set_value(rgb->rgbw_light_indicator, 0);

    // Pass rgb (pointing to my_rgbs[this_idx]) — stable for lifetime of widget
    lv_obj_add_event_cb(rgb->button_img_on_off_rgbw, rgb_widget_cb, LV_EVENT_CLICKED, rgb);
    lv_obj_add_event_cb(rgb->reg_rgb_img, rgb_widget_cb, LV_EVENT_CLICKED, rgb);
    lv_obj_add_event_cb(rgb->green_rgbw_img, rgb_widget_cb, LV_EVENT_CLICKED, rgb);
    lv_obj_add_event_cb(rgb->blue_rgbw_img, rgb_widget_cb, LV_EVENT_CLICKED, rgb);
    lv_obj_add_event_cb(rgb->color_panel_opening_img, rgb_widget_cb, LV_EVENT_CLICKED, rgb);
    lv_obj_add_event_cb(rgb->rgbw_light_indicator, rgb_arc_cb, LV_EVENT_VALUE_CHANGED, rgb);

    rgb_count++; // increment AFTER everything is wired up
    printf("RGB widget created: [%s] id=%d parent_child_count=%d\n",
           rgb->name, rgb->id, (int)total); // this total will give total child cound under parent
}

void turn_off_rgbw_lights(void)
{

    for (int i = 0; i < rgb_count; i++)
    {
        rgb_device_t *rgb_off = &my_rgbs[i];
        rgb_off->is_on = false;
        rgb_off->current_intensity = 0;
        rgb_off->white_color_intensity = 0;
        rgb_off->temperature_light_intensity = 0;
        lv_arc_set_value(rgb_off->rgbw_light_indicator, 0);
        lv_obj_set_style_arc_color(rgb_off->rgbw_light_indicator, lv_color_hex(0x818499), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    
        lv_obj_clear_state(rgb_off->button_img_on_off_rgbw, LV_STATE_CHECKED);
        lv_obj_set_style_image_recolor(rgb_off->button_img_on_off_rgbw, lv_color_hex(0x818499), 0);
        lv_obj_set_style_image_recolor_opa(rgb_off->button_img_on_off_rgbw, LV_OPA_COVER, 0);
    }



    lv_obj_set_style_bg_color(objects.white_intensity_slider, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.white_intensity_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(objects.color_panel_color_slider, lv_color_hex(0x000000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.color_panel_color_slider, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(objects.white_scn_slider, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.white_scn_slider, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);

    printf("Turned Off all RGBW lights\n");
}