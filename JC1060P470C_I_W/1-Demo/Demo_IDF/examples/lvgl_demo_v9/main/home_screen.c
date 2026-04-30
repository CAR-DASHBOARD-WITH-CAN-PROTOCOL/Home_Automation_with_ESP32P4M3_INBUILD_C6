#include "lvgl.h"
#include <stdio.h>
#include "header.h"
#include <time.h>
#include "images.h"
#include "fonts.h"
#include "screens.h"

static lv_obj_t *time_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *splash_screen_page = NULL;
static lv_timer_t *clock_timer = NULL;
static lv_obj_t *main_screen_ref = NULL; // real main screen saved here


// In home_screen.c — add these statics
static lv_obj_t *ss_eth_on_img   = NULL;  // ss = screensaver
static lv_obj_t *ss_eth_off_img  = NULL;
static lv_obj_t *ss_ws_conn_lab  = NULL;
static lv_obj_t *ss_ws_disc_lab  = NULL;



void update_connection_status_ui(bool eth_connected, bool ws_connected)
{
    // Called from LVGL timer — already in LVGL task, no lock needed
    // ── Login screen objects ──────────────────────────────────────────────
    if (objects.internet_on_img != NULL)
    {
        if (eth_connected)
        {
            lv_obj_clear_flag(objects.internet_on_img,    LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.internet_off_image,   LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(objects.internet_on_img,      LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.internet_off_image, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (objects.socket_connected_lab != NULL)
    {
        if (ws_connected)
        {
            lv_obj_clear_flag(objects.socket_connected_lab,      LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(objects.socket_disconnected_label,   LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(objects.socket_connected_lab,        LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(objects.socket_disconnected_label, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // ── Screensaver objects ───────────────────────────────────────────────
    if (ss_eth_on_img != NULL)
    {
        if (eth_connected)
        {
             // Connected — hide everything, show nothing
            lv_obj_add_flag(ss_eth_on_img,  LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ss_eth_off_img, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(ss_eth_on_img,    LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ss_eth_off_img, LV_OBJ_FLAG_HIDDEN);
        }

        if (ws_connected)
        {
          // Connected — hide everything
            lv_obj_add_flag(ss_ws_conn_lab, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ss_ws_disc_lab, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(ss_ws_conn_lab,   LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ss_ws_disc_lab, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void create_status_on_screensaver(lv_obj_t *parent)
{
    //Ethernet ON image
    ss_eth_on_img = lv_image_create(parent);                  //       can make visible internet visiblity by uncommenting this 
    lv_image_set_src(ss_eth_on_img, &img_internet_on);   // same image src as EEZ
    lv_obj_set_pos(ss_eth_on_img, 934, 3);
    lv_obj_add_flag(ss_eth_on_img, LV_OBJ_FLAG_HIDDEN);  // hidden by default

    // Ethernet OFF image
    ss_eth_off_img = lv_image_create(parent);
    lv_image_set_src(ss_eth_off_img, &img_internet_off); // same image src as EEZ
    lv_obj_set_pos(ss_eth_off_img, 928, 3);
    lv_obj_add_flag(ss_eth_off_img, LV_OBJ_FLAG_HIDDEN); // hidden by default

   // Socket connected label   //  can make visible internet visiblity by uncommenting this 
    ss_ws_conn_lab = lv_label_create(parent);
    lv_label_set_text(ss_ws_conn_lab, "Connected");
    lv_obj_set_style_text_color(ss_ws_conn_lab, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(ss_ws_conn_lab, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ss_ws_conn_lab, 890, 60);
    lv_obj_add_flag(ss_ws_conn_lab, LV_OBJ_FLAG_HIDDEN); // hidden by default

    // Socket disconnected label
    ss_ws_disc_lab = lv_label_create(parent);
    lv_label_set_text(ss_ws_disc_lab, "Disconnected");
    lv_obj_set_style_text_color(ss_ws_disc_lab, lv_color_hex(0xFF4444), 0);
    lv_obj_set_style_text_font(ss_ws_disc_lab, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ss_ws_disc_lab, 875, 61);
    lv_obj_add_flag(ss_ws_disc_lab, LV_OBJ_FLAG_HIDDEN); // hidden by default
}


// ── called from ui_start() once at boot ──────────────────────────────────────
void set_main_screen_ref(lv_obj_t *scr)
{
    main_screen_ref = scr;
    printf("main_screen_ref : %p\n", main_screen_ref);
    printf("screen size     : w=%d h=%d\n",
           lv_obj_get_width(main_screen_ref),
           lv_obj_get_height(main_screen_ref));
    printf("screen pos      : x=%d y=%d\n",
           lv_obj_get_x(main_screen_ref),
           lv_obj_get_y(main_screen_ref));
}

// ── wake up = load main screen exactly like splash loads itself ───────────────
void hide_splash_screen(void)
{
    printf("hide_splash_screen: loading %p\n", main_screen_ref);
    if (main_screen_ref != NULL)
        lv_scr_load(main_screen_ref);
}

// ── clock tick ────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────
// Clock — respects is_24h_format flag
// ─────────────────────────────────────────────
static void update_time_cb(lv_timer_t *timer)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char time_str[16];

    if (is_24h_format)
        strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);   // 14:30
    else
        strftime(time_str, sizeof(time_str), "%I:%M", &timeinfo);   // 02:30

    if (time_label)
        lv_label_set_text(time_label, time_str);

    char date_str[100];

    if (is_24h_format)
        strftime(date_str, sizeof(date_str), "%A\n%B %d, %Y", &timeinfo);      // no AM/PM
    else
        strftime(date_str, sizeof(date_str), "%A\n%B %d, %Y\n%p", &timeinfo);  // with AM/PM

    if (date_label)
        lv_label_set_text(date_label, date_str);
}


// ── build clock UI directly on splash_screen_page (NO tileview) ──────────────
static void create_splash_content(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    // Logo
    lv_obj_t *img_logo = lv_image_create(parent);
    lv_image_set_src(img_logo, &img_wohnux_logo);
    lv_obj_align(img_logo, LV_ALIGN_BOTTOM_MID, 0, -120);

    // Divider line
    static lv_point_precise_t line_points[] = {{0, 0}, {0, 200}};
    lv_obj_t *divider = lv_line_create(parent);
    lv_line_set_points(divider, line_points, 2);
    lv_obj_set_style_line_width(divider, 2, 0);
    lv_obj_set_style_line_color(divider, lv_color_white(), 0);
    lv_obj_align(divider, LV_ALIGN_TOP_MID, 0, 120);

    time_label = lv_label_create(parent);

    lv_obj_set_width(time_label, 360); // FIXED WIDTH
    lv_obj_set_style_text_font(time_label, &ui_font_bold_120, 0);
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_RIGHT, 0);

    lv_obj_align_to(time_label, divider, LV_ALIGN_OUT_LEFT_MID, -20, 0);

    // Date label
    date_label = lv_label_create(parent);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_line_space(date_label, 10, 0);
    lv_obj_set_style_text_align(date_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align_to(date_label, divider, LV_ALIGN_OUT_RIGHT_MID, 30, -10);

    // Clock timer — only create once
    update_time_cb(NULL);
    if (clock_timer == NULL)
        clock_timer = lv_timer_create(update_time_cb, 1000, NULL);


        
}

// ── main entry point ──────────────────────────────────────────────────────────
void wohnux_main_ui_create(void)
{
    if (splash_screen_page == NULL)
    {
        // FIX: create screen and put content DIRECTLY on it
        // NO tileview — tileview was causing the partial screen restore bug
        splash_screen_page = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(splash_screen_page, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(splash_screen_page, LV_OPA_COVER, 0);

        // Remove default padding so content fills full screen
        lv_obj_set_style_pad_all(splash_screen_page, 0, 0);
        lv_obj_set_style_border_width(splash_screen_page, 0, 0);

        create_splash_content(splash_screen_page);
        create_status_on_screensaver(splash_screen_page); // ← add this

    }

    lv_scr_load(splash_screen_page);
}

