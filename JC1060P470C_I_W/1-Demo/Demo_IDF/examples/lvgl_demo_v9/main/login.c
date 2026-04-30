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


static lv_obj_t *error_label;
static bool password_visible = false;

void check_and_wipe_nvs_if_needed(void);
void update_connection_status_ui(bool eth_connected, bool ws_connected);


//  Change this number whenever you want to force an NVS wipe on the next flash!




void check_and_wipe_nvs_if_needed(void) 
{
    // 1. Standard NVS Init so we can actually read the stored flag
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // 2. Open NVS to check our custom version flag
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error opening NVS handle!\n");
        return;
    }

    int32_t stored_version = 0; // Default to 0
    err = nvs_get_i32(my_handle, "nvs_ver", &stored_version);

    // 3. Compare stored version with current code version
    if (err == ESP_ERR_NVS_NOT_FOUND || stored_version != CURRENT_NVS_VERSION) {
        printf("NVS Version mismatch! Stored: %ld, Code: %d. Wiping NVS...\n", stored_version, CURRENT_NVS_VERSION);

        // We must close the handle before nuking the memory
        nvs_close(my_handle);

        //  WIPE THE ENTIRE NVS 
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());

        // Re-open and save the new version so it doesn't wipe next reboot
        ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle));
        ESP_ERROR_CHECK(nvs_set_i32(my_handle, "nvs_ver", CURRENT_NVS_VERSION));
        ESP_ERROR_CHECK(nvs_commit(my_handle));
        
        printf("NVS completely wiped and updated to version %d!\n", CURRENT_NVS_VERSION);
    } else {
        printf("NVS Version matches (%d). Booting with saved data intact.\n", CURRENT_NVS_VERSION);
    }

    // 4. Always close the handle when done
    nvs_close(my_handle);
}



void on_websocket_connected(void)
{
    printf("WebSocket ready — show login screen\n");
    // Login screen is already shown — just enable the login button
    // or show a "connected" indicator
}

void on_login_success(void)
{
    printf("Login success — hiding login screen\n");
    lv_obj_add_flag(objects.login_screen, LV_OBJ_FLAG_HIDDEN);
}

void on_login_failed(void)
{
    printf("Login failed — show error\n");
    lv_obj_remove_flag(error_label, LV_OBJ_FLAG_HIDDEN);
    lv_textarea_set_text(objects.username, "");
    lv_textarea_set_text(objects.password, "");
}


/* ── credential check ── */
bool verify_credentials(const char *user, const char *pass)
{
    if (strcmp(user, "Admin") == 0 && strcmp(pass, "Compac") == 0)


        return true;
    return false;
}

/* ── keyboard event — tick button hides keyboard ── */
static void keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    /* LV_EVENT_READY fires when user presses the tick (OK) button */
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL)
    {
        lv_keyboard_set_textarea(objects.keyboard, NULL);
        lv_obj_add_flag(objects.keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ── textarea focus — show keyboard and point it to correct field ── */
static void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);

    if (code == LV_EVENT_FOCUSED)
    {
        lv_keyboard_set_textarea(objects.keyboard, ta);
        lv_obj_clear_flag(objects.keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(error_label, LV_OBJ_FLAG_HIDDEN);

        // ── Get textarea's Y position on screen ──
        lv_coord_t ta_y = lv_obj_get_y(ta);
        lv_coord_t ta_height = lv_obj_get_height(ta);
        lv_coord_t kb_height = lv_obj_get_height(objects.keyboard);
        lv_coord_t scr_h = lv_display_get_vertical_resolution(NULL);

        // ── Position keyboard just below the textarea ──
        lv_coord_t kb_y = ta_y + ta_height + 10; // 10px gap below textarea

        // ── If keyboard goes off screen bottom, push it up ──
        if (kb_y + kb_height > scr_h)
            kb_y = scr_h - kb_height - 5; // 5px margin from bottom

        lv_obj_set_pos(objects.keyboard, 0, kb_y);
    }

    if (code == LV_EVENT_DEFOCUSED)
    {
        lv_keyboard_set_textarea(objects.keyboard, NULL);
        lv_obj_add_flag(objects.keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

static void eye_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
        return;

    password_visible = !password_visible;

    /* true  = show plain text
       false = hide with dots  */
    lv_textarea_set_password_mode(objects.password, !password_visible);
}

/* ── login button ── */
static void login_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    const char *user_str = lv_textarea_get_text(objects.username);
    const char *pass_str = lv_textarea_get_text(objects.password);

    // Send via WebSocket — server validates
    ws_send_login(user_str, pass_str);
}
/* ════════════════════════════════════════════════
   MAIN LOGIN UI BUILDER
   ════════════════════════════════════════════════ */
void create_login_ui(void)
{
    ui_init(); // initialise all objects first

    // top_panel();
    // mid_panel();
    // lower_panel();
    if (objects.login_screen == NULL)
    {
        printf("ERROR: login_screen NULL!\n");
        return;
    }
    lv_obj_clear_flag(objects.login_screen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(objects.login_screen);

    /* ── keyboard hidden on start, pointing to nothing ── */
    lv_keyboard_set_textarea(objects.keyboard, NULL);
    lv_obj_add_flag(objects.keyboard, LV_OBJ_FLAG_HIDDEN);

    // In create_login_ui() after hiding keyboard
    lv_obj_set_size(objects.keyboard,
                    lv_display_get_horizontal_resolution(NULL), // full width
                    200);                                       // fixed height — adjust to your screen
    lv_textarea_set_password_mode(objects.password, true);
        /* ── tick button closes keyboard ── */
    lv_obj_add_event_cb(objects.keyboard, keyboard_event_cb,
                            LV_EVENT_ALL, NULL);

    /* ── Fix 1: bigger font so text fills the textarea height ── */
    lv_obj_set_style_text_color(objects.username,
                                lv_color_hex(0xFFFFFF),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(objects.username,
                               &lv_font_montserrat_20,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(objects.password,
                                lv_color_hex(0xFFFFFF),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(objects.password,
                               &lv_font_montserrat_20,
                               LV_PART_MAIN | LV_STATE_DEFAULT);

    /* cursor visible */
    lv_obj_set_style_bg_color(objects.username,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_CURSOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.password,
                              lv_color_hex(0xFFFFFF),
                              LV_PART_CURSOR | LV_STATE_DEFAULT);

    /* ── eye toggle stays visible, click toggles password mode ── */
    lv_obj_add_flag(objects.visible_eye_password, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(objects.visible_eye_password, eye_btn_event_cb,
                        LV_EVENT_CLICKED, NULL);

    /* ── attach keyboard to both textareas ── */
    lv_obj_add_event_cb(objects.username, ta_event_cb,
                        LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(objects.password, ta_event_cb,
                        LV_EVENT_ALL, NULL);

    /* ── login button ── */
    lv_obj_add_event_cb(objects.login_button, login_btn_event_cb, // here login button will send data to socket to to check credentials
                        LV_EVENT_ALL, NULL); 

    /* ── Fix 2: error label directly below password, not floating ── */
    error_label = lv_label_create(objects.login_screen);
    lv_label_set_text(error_label, "Invalid Username or Password!");
    lv_obj_set_style_text_color(error_label,
                                lv_color_hex(0xFF4C4C), 0);
    lv_obj_set_style_text_font(error_label,
                               &lv_font_montserrat_16, 0);
    lv_obj_align_to(error_label, objects.password,
                    LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);
    lv_obj_add_flag(error_label, LV_OBJ_FLAG_HIDDEN);

    real_main_screen = lv_scr_act();
}