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
#include <stdio.h>
#include <string.h>
#include "esp_spiffs.h"

static order_item_t order_items[MAX_ORDER_DEVICES];
static int order_item_count = 0;

int ac_widget_count = 0;
int lamp_widget_count = 0;
int curtain_widget_count = 0;
int fan_widget_count = 0;
int projector_widget_count = 0;
int cct_widget_count = 0;

void reset_mid_panel(void);
void reset_ac_widgets(void);
void reset_cct_widgets(void);
void reset_device_widgets(void);

// ── Forward refs ─────────────────────────────────────────────────
static void order_up_cb(lv_event_t *e);
static void order_down_cb(lv_event_t *e);
static void order_checkbox_cb(lv_event_t *e);

void open_select_order_panel(void);
void action_save_order(lv_event_t *e);
void action_cancel_order(lv_event_t *e);

void apply_order_to_mid_panel(void);
void populate_order_list(void);
void build_select_order_widgets(void);
void reset_mid_panel(void);

void populate_order_list(void)
{
    order_item_count = 0;

    // --- STEP A: READ DEVICES.CSV INTO MEMORY ---
    total_parsed_devices = 0;
    FILE *fd = fopen("/spiffs/devices.csv", "r");
    if (fd)
    {
        char line[128];
        fgets(line, sizeof(line), fd); // Skip header

        while (fgets(line, sizeof(line), fd) != NULL && total_parsed_devices < MAX_CSV_DEVICES)
        {
            csv_device_t *dev = &parsed_devices[total_parsed_devices];
            if (sscanf(line, "%d,%d,%d,%d,%d,%[^\r\n]",
                       &dev->id, &dev->locId, &dev->type, &dev->inScenes, &dev->dimmable, dev->name) == 6)
            {
                total_parsed_devices++;
            }
        }

        fclose(fd);
    }
    else
    {
        ESP_LOGE("CSV", "Failed to open devices.csv!");
    }

    // --- STEP D: CREATE AC WIDGETS ---
    for (int i = 0; i < total_parsed_devices; i++)
    {
        csv_device_t *dev = &parsed_devices[i];

        if (dev->type == 4 && !(parsed_devices[i].inScenes == 1))
        {
            if (has_scenes)
            {
                order_item_t *item = &order_items[order_item_count];
                strncpy(item->name, dev->name, sizeof(item->name));
                item->device_type = 4; // AC type
                item->device_index = i;
                item->visible = true;
                item->order = order_item_count;
                item->widget_btn = NULL;
                order_item_count++;
            }
        }
    } // <--- THIS WAS THE MISSING BRACE!

    // ── All other devices (mid panel only — skip in_scenes) ──────
    for (int i = 0; i < total_parsed_devices; i++)
    {
        csv_device_t *dev = &parsed_devices[i];

        if (dev->inScenes == 1)
            continue; // skip cct panel devices
        if (dev->type == 4)
            continue; // AC already added

        order_item_t *item = &order_items[order_item_count];
        strncpy(item->name, dev->name, sizeof(item->name));
        item->device_type = dev->type;
        item->device_index = i;
        item->visible = true;
        item->order = order_item_count;
        item->widget_btn = NULL;
        order_item_count++;
    }
}
void build_select_order_widgets(void)
{
    reset_mid_panel();
    // Clear any previously created children
    lv_obj_clean(objects.slec_ord_widget_container);

    for (int i = 0; i < order_item_count; i++)
    {
        order_item_t *item = &order_items[i];

        // ── Call your EEZ generated function ─────────────────────
        create_user_widget_selec_order_widget_button(
            objects.slec_ord_widget_container, NULL, i);

        // ── Grab the last created child (your widget) ─────────────
        lv_obj_t *btn = lv_obj_get_child(
            objects.slec_ord_widget_container,
            lv_obj_get_child_count(objects.slec_ord_widget_container) - 1);

        item->widget_btn = btn; // parent

        lv_obj_t *cb = lv_obj_get_child(btn, 0); // button itself
        lv_obj_t *lbl = lv_obj_get_child(btn, 1);
        lv_obj_t *lbl_up = lv_obj_get_child(btn, 2);
        lv_obj_t *lbl_dn = lv_obj_get_child(btn, 3);

        item->checkbox = cb;
        item->label = lbl;
        item->up_label = lbl_up;
        item->down_label = lbl_dn;

        // ── Set device name ───────────────────────────────────────
        lv_label_set_text(lbl, item->name);

        // ── Set checkbox state ────────────────────────────────────
        if (item->visible)
            lv_obj_add_state(cb, LV_STATE_CHECKED);
        else
            lv_obj_clear_state(cb, LV_STATE_CHECKED);

        // ── Up/Down label symbols ─────────────────────────────────
        lv_label_set_text(lbl_up, LV_SYMBOL_UP);
        lv_label_set_text(lbl_dn, LV_SYMBOL_DOWN);

        // ── Make up/down labels clickable ─────────────────────────
        lv_obj_add_flag(lbl_up, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(lbl_dn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(lbl_up, 10);
        lv_obj_set_ext_click_area(lbl_dn, 10);

        // ── Attach callbacks with index as user data ──────────────
        lv_obj_add_event_cb(cb, order_checkbox_cb,
                            LV_EVENT_VALUE_CHANGED, (void *)(uintptr_t)i);
        lv_obj_add_event_cb(lbl_up, order_up_cb,
                            LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_add_event_cb(lbl_dn, order_down_cb,
                            LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        // ── Dim if hidden ─────────────────────────────────────────
        if (!item->visible)
            lv_obj_set_style_opa(btn, LV_OPA_40, 0);
    }
}

static void swap_order_items(int a, int b)
{
    order_item_t tmp = order_items[a];
    order_items[a] = order_items[b];
    order_items[b] = tmp;
}

static void order_up_cb(lv_event_t *e)
{
    int i = (int)(uintptr_t)lv_event_get_user_data(e);
    if (i == 0)
        return;

    swap_order_items(i, i - 1);
    build_select_order_widgets(); // rebuild UI in new order
}

static void order_down_cb(lv_event_t *e)
{
    int i = (int)(uintptr_t)lv_event_get_user_data(e);
    if (i >= order_item_count - 1)
        return;

    swap_order_items(i, i + 1);
    build_select_order_widgets(); // rebuild UI in new order
}

static void order_checkbox_cb(lv_event_t *e)
{
    int i = (int)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *cb = lv_event_get_target(e);
    bool checked = lv_obj_has_state(cb, LV_STATE_CHECKED);

    // Guard — at least 1 must remain visible
    if (!checked)
    {
        int visible_count = 0;
        for (int j = 0; j < order_item_count; j++)
            if (order_items[j].visible)
                visible_count++;

        if (visible_count <= 1)
        {
            lv_obj_add_state(cb, LV_STATE_CHECKED); // force re-check
            return;
        }
    }

    order_items[i].visible = checked;

    lv_obj_t *btn = order_items[i].widget_btn;
    lv_obj_set_style_opa(btn,
                         checked ? LV_OPA_COVER : LV_OPA_40, 0);
}

void apply_order_to_mid_panel(void)
{

    // 1. Reset all backend counters to 0
    reset_mid_panel();

    // 2. Clean BOTH panels so we don't duplicate or overlap widgets
    lv_obj_clean(objects.mid_panel);
    lv_obj_clean(objects.cct_rgb_light_panel);

    // 3. Rebuild the MID PANEL based on your custom order array
    for (int i = 0; i < order_item_count; i++)
    {
        order_item_t *item = &order_items[i];
        if (!item->visible)
            continue; // skip hidden devices

        // Recreate widget in new order
        switch (item->device_type)
        {
        case 4:
            create_ac_widget(item->name, objects.mid_panel);
            break;
        case 1:
        case 6:
        case 7:
            create_lamp_widget(item->name, objects.mid_panel);
            break;
        case 2:
        case 3:
        case 10:
        case 13:
            create_cct_widget(item->name, objects.mid_panel);
            break;
        case 5:
            create_fan_widget(item->name, objects.mid_panel);
            break;
        case 8:
            create_projector_widget(item->name, objects.mid_panel);
            break;
        case 9:
            create_curtain_widget(item->name, objects.mid_panel);
            break;
        case 11:
        case 12:
        case 14:
            create_rgb_widget(item->name, objects.mid_panel);
            break;
        }
    }

    // 4. Rebuild the RGB/CCT PANEL (Since we skipped them in the order list)
    for (int i = 0; i < total_parsed_devices; i++)
    {
        csv_device_t *dev = &parsed_devices[i];

        // Only target devices meant for the RGB panel in the current location
        if (dev->inScenes == 1)
        {
            if (dev->type == 4)
            {
                create_ac_widget(dev->name, objects.cct_rgb_light_panel);
            }
            else
            {
                switch (dev->type)
                {
                case 1:
                case 6:
                case 7:
                    create_lamp_widget(dev->name, objects.cct_rgb_light_panel);
                    break;
                case 2:
                case 3:
                case 10:
                case 13:
                    create_cct_widget(dev->name, objects.cct_rgb_light_panel);
                    break;
                case 5:
                    create_fan_widget(dev->name, objects.cct_rgb_light_panel);
                    break;
                case 8:
                    create_projector_widget(dev->name, objects.cct_rgb_light_panel);
                    break;
                case 9:
                    create_curtain_widget(dev->name, objects.cct_rgb_light_panel);
                    break;
                case 11:
                case 12:
                case 14:
                    create_rgb_widget(dev->name, objects.cct_rgb_light_panel);

                    break;
                }
            }
        }
    }

    // Finish up UI logic
    mid_panel();
}

void reset_mid_panel(void)
{
    // These functions reset the static counters INSIDE each file
    // and clean their panels — the only correct way since all
    // counters are static (private) to their own .c files
    reset_ac_widgets();     // resets ac_count in ac_screen.c
    reset_cct_widgets();    // resets cct_count in cct_rgb_lights.c
    reset_device_widgets(); // resets fan/lamp/curtain/projector counts in mid_panel.c
    reset_rgb_widgets();
    printf("=== reset_mid_panel done ===\n");
}
