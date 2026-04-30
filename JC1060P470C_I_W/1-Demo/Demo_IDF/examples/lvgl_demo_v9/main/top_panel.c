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
#include "screens.h"
#include "images.h"

// globals — one image object per button
static lv_obj_t *welcome_img = NULL;
static lv_obj_t *bright_img  = NULL;
static lv_obj_t *day_img     = NULL;
static lv_obj_t *relax_img   = NULL;
static lv_obj_t *sleep_img   = NULL;
static lv_obj_t *dawn_img    = NULL;


// ← ADD THESE
static lv_obj_t *welcome_label = NULL;
static lv_obj_t *bright_label  = NULL;
static lv_obj_t *day_label     = NULL;
static lv_obj_t *relax_label   = NULL;
static lv_obj_t *sleep_label   = NULL;
static lv_obj_t *dawn_label    = NULL;


button_config_t btn_cfg = {
    .welcome_button = false,
    .bright_button  = false,
    .day_button     = false,
    .sleep_b        = false,
    .relax_button   = false,
    .dawn_button    = false
};


void update_button_visibility_top_panel(void);

// ============================================================
// HELPER — reset all to OFF
// ============================================================
static void reset_all_to_off(void)
{
    if (welcome_img) { lv_img_set_src(welcome_img, &img_welcome_img_off);     lv_obj_invalidate(welcome_img);  }
    if (bright_img)  { lv_img_set_src(bright_img,  &img_white_bright_img);    lv_obj_invalidate(bright_img);   }
    if (day_img)     { lv_img_set_src(day_img,     &img_day);                 lv_obj_invalidate(day_img);      }
    if (relax_img)   { lv_img_set_src(relax_img,   &img_relax);               lv_obj_invalidate(relax_img);    }
    if (sleep_img)   { lv_img_set_src(sleep_img,   &img_sleep_while_img_off); lv_obj_invalidate(sleep_img);    }
    if (dawn_img)    { lv_img_set_src(dawn_img,    &img_dawn);                lv_obj_invalidate(dawn_img);     }

    // ← ADD: grey all labels
    lv_color_t off_col = lv_color_hex(0x818499);
    if (welcome_label) lv_obj_set_style_text_color(welcome_label, off_col, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (bright_label)  lv_obj_set_style_text_color(bright_label,  off_col, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (day_label)     lv_obj_set_style_text_color(day_label,     off_col, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (relax_label)   lv_obj_set_style_text_color(relax_label,   off_col, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (sleep_label)   lv_obj_set_style_text_color(sleep_label,   off_col, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (dawn_label)    lv_obj_set_style_text_color(dawn_label,    off_col, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// ============================================================
// CALLBACKS
// ============================================================
static void welcome_clicked_cb(lv_event_t *e)
{
    reset_all_to_off();
    lv_img_set_src(welcome_img, &img_welcom_img_on_);
    if (welcome_label)
        lv_obj_set_style_text_color(welcome_label, lv_color_hex(0xFFBA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    printf("Welcome button pressed.\n");
}

static void bright_b_clicked_cb(lv_event_t *e)
{
    reset_all_to_off();
    lv_img_set_src(bright_img, &img_yallow_bright_img);
    if (bright_label)
        lv_obj_set_style_text_color(bright_label, lv_color_hex(0xFFBA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    printf("Bright button pressed.\n");
}

static void day_button_clicked_cb(lv_event_t *e)
{
    reset_all_to_off();
    lv_img_set_src(day_img, &img_day_yellow_img);
    if (day_label)
        lv_obj_set_style_text_color(day_label, lv_color_hex(0xFFBA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    printf("Day button pressed.\n");
}

static void sleep_b_clicked_cb(lv_event_t *e)
{
    reset_all_to_off();
    lv_img_set_src(sleep_img, &img_sleep_yallow_on);
    if (sleep_label)
        lv_obj_set_style_text_color(sleep_label, lv_color_hex(0xFFBA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    printf("Sleep button pressed.\n");
}

static void relax_b_clicked_cb(lv_event_t *e)
{
    reset_all_to_off();
    lv_img_set_src(relax_img, &img_relax_yallow_img);
    if (relax_label)
        lv_obj_set_style_text_color(relax_label, lv_color_hex(0xFFBA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    printf("Relax button pressed.\n");
}

static void dawn_clicked_cb(lv_event_t *e)
{
    reset_all_to_off();
    lv_img_set_src(dawn_img, &img_dawn_yallow_img);
    if (dawn_label)
        lv_obj_set_style_text_color(dawn_label, lv_color_hex(0xFFBA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    printf("Dawn button pressed.\n");
}


// ============================================================
// update_button_visibility_top_panel
// ============================================================
void update_button_visibility_top_panel(void)
{
    if (btn_cfg.welcome_button)
    {

        lv_obj_clear_flag(objects.welcome_button, LV_OBJ_FLAG_HIDDEN);
         welcome_label = lv_obj_get_child(objects.welcome_button, 0); // ← ADD
        lv_obj_set_style_text_color(welcome_label, lv_color_hex(0x818499), LV_PART_MAIN | LV_STATE_DEFAULT); // ← ADD default grey

        welcome_img = lv_img_create(objects.welcome_button);
        lv_img_set_src(welcome_img, &img_welcome_img_off);
        lv_obj_align(welcome_img, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_move_background(welcome_img);
        lv_obj_add_event_cb(objects.welcome_button, welcome_clicked_cb, LV_EVENT_CLICKED, NULL);
    }
    else
        lv_obj_add_flag(objects.welcome_button, LV_OBJ_FLAG_HIDDEN);

    if (btn_cfg.bright_button)
    {
        lv_obj_clear_flag(objects.bright_button_nc, LV_OBJ_FLAG_HIDDEN);
         bright_label = lv_obj_get_child(objects.bright_button_nc, 0); // ← ADD
    lv_obj_set_style_text_color(bright_label, lv_color_hex(0x818499), LV_PART_MAIN | LV_STATE_DEFAULT); // ← ADD

        bright_img = lv_img_create(objects.bright_button_nc);
        lv_img_set_src(bright_img, &img_white_bright_img);
        lv_obj_align(bright_img, LV_ALIGN_TOP_MID, 0, -7);
        lv_obj_move_background(bright_img);
        lv_obj_add_event_cb(objects.bright_button_nc, bright_b_clicked_cb, LV_EVENT_CLICKED, NULL);
    }
    else
        lv_obj_add_flag(objects.bright_button_nc, LV_OBJ_FLAG_HIDDEN);

    if (btn_cfg.day_button)
    {
        lv_obj_clear_flag(objects.day_button_nc, LV_OBJ_FLAG_HIDDEN);
         day_label = lv_obj_get_child(objects.day_button_nc, 0); // ← ADD
         lv_obj_set_style_text_color(day_label, lv_color_hex(0x818499), LV_PART_MAIN | LV_STATE_DEFAULT); // ← ADD
        day_img = lv_img_create(objects.day_button_nc);
        lv_img_set_src(day_img, &img_day);
        lv_obj_align(day_img, LV_ALIGN_TOP_MID, 0, -7);
        lv_obj_move_background(day_img);
        lv_obj_add_event_cb(objects.day_button_nc, day_button_clicked_cb, LV_EVENT_CLICKED, NULL);
    }
    else
        lv_obj_add_flag(objects.day_button_nc, LV_OBJ_FLAG_HIDDEN);

    if (btn_cfg.sleep_b)
    {
        lv_obj_clear_flag(objects.sleep_b_nc, LV_OBJ_FLAG_HIDDEN);
         sleep_label = lv_obj_get_child(objects.sleep_b_nc, 0); // ← ADD
         lv_obj_set_style_text_color(sleep_label, lv_color_hex(0x818499), LV_PART_MAIN | LV_STATE_DEFAULT); // ← ADD


        sleep_img = lv_img_create(objects.sleep_b_nc);
        lv_img_set_src(sleep_img, &img_sleep_while_img_off);
        lv_obj_align(sleep_img, LV_ALIGN_TOP_MID, 0, -7);
        lv_obj_move_background(sleep_img);
        lv_obj_add_event_cb(objects.sleep_b_nc, sleep_b_clicked_cb, LV_EVENT_CLICKED, NULL);
    }
    else
        lv_obj_add_flag(objects.sleep_b_nc, LV_OBJ_FLAG_HIDDEN);

    if (btn_cfg.relax_button)
    {
        lv_obj_clear_flag(objects.relax_button_nc, LV_OBJ_FLAG_HIDDEN);
         relax_label = lv_obj_get_child(objects.relax_button_nc, 0); // ← ADD
         lv_obj_set_style_text_color(relax_label, lv_color_hex(0x818499), LV_PART_MAIN | LV_STATE_DEFAULT); // ← ADD
        relax_img = lv_img_create(objects.relax_button_nc);
        lv_img_set_src(relax_img, &img_relax);
        lv_obj_align(relax_img, LV_ALIGN_TOP_MID, 0, -7);
        lv_obj_move_background(relax_img);
        lv_obj_add_event_cb(objects.relax_button_nc, relax_b_clicked_cb, LV_EVENT_CLICKED, NULL);
    }
    else
        lv_obj_add_flag(objects.relax_button_nc, LV_OBJ_FLAG_HIDDEN);

    if (btn_cfg.dawn_button)
    {
        lv_obj_clear_flag(objects.dawn_button_nc, LV_OBJ_FLAG_HIDDEN);
         dawn_label = lv_obj_get_child(objects.dawn_button_nc, 0); // ← ADD
         lv_obj_set_style_text_color(dawn_label, lv_color_hex(0x818499), LV_PART_MAIN | LV_STATE_DEFAULT); // ← ADD
        dawn_img = lv_img_create(objects.dawn_button_nc);
        lv_img_set_src(dawn_img, &img_dawn);
        lv_obj_align(dawn_img, LV_ALIGN_TOP_MID, 0, -7);
        lv_obj_move_background(dawn_img);
        lv_obj_add_event_cb(objects.dawn_button_nc, dawn_clicked_cb, LV_EVENT_CLICKED, NULL);
    }
    else
        lv_obj_add_flag(objects.dawn_button_nc, LV_OBJ_FLAG_HIDDEN);
}

// ============================================================
void top_panel(void)
{
    update_button_visibility_top_panel();
}

