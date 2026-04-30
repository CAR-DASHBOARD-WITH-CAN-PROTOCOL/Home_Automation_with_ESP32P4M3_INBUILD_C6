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

#include "esp_spiffs.h"
#include <stdio.h>
#include <string.h>

csv_device_t parsed_devices[MAX_CSV_DEVICES];
csv_scene_t parsed_scenes[MAX_CSV_SCENES];

int total_parsed_devices = 0;
int total_parsed_scenes = 0;
int total_config_data = 0;
bool has_scenes = 0;
app_config_t g_config = {
    .sleepT = 0, // safe defaults if CSV missing
    .location = 0,
    .showDevicesOnly = 0,
    .oldLcd = 0,
    .showData = 0,
    .childlock = 0,
};

void init_spiffs(void);
void create_ui_from_csv(void);

// ─── 1. MOUNT THE FLASH DRIVE ────────────────────────────────
void init_spiffs(void)
{
    ESP_LOGI("SPIFFS", "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL, // Uses the first spiffs partition found
        .max_files = 5,
        .format_if_mount_failed = true // Formats on first boot
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        ESP_LOGE("SPIFFS", "Failed to mount or format filesystem");
        return;
    }
    ESP_LOGI("SPIFFS", "SPIFFS mounted successfully");
}

// ─── 2. PARSE THE DEVICES CSV ────────────────────────────────
void create_ui_from_csv(void)
{
    FILE *f = fopen("/spiffs/devices.csv", "r");
    if (f == NULL)
    {
        ESP_LOGE("CSV", "Failed to open device.csv");
        return;
    }
    char line[128];

    // Read and ignore the first line (the Header: Id,locId,Type,...)
    fgets(line, sizeof(line), f);

    // Read the file line by line
    while (fgets(line, sizeof(line), f) != NULL)
    {
        int id, locId, type, inScenes, dimmable;
        char name[64];

        // Parse the CSV format: 173,7,1,0,0,COVE1
        // %[^\r\n] ensures it reads the string even if it has spaces (like "BED LIGHT 1")
        if (sscanf(line, "%d,%d,%d,%d,%d,%[^\r\n]", &id, &locId, &type, &inScenes, &dimmable, name) == 6)
        {

            ESP_LOGI("CSV", "Parsed: ID:%d, Name:%s, Type:%d", id, name, type);
        }
    }
    fclose(f);
}

// // ─── 2. NEW SCREEN CREATION LOGIC ────────────────────────────
void screen_creation(void)
{

    FILE *fc = fopen("/spiffs/config.csv", "r");
    if (!fc)
    {
        ESP_LOGE("CSV", "Failed to open config.csv! Using defaults.");
        return;
    }

    char line[128];
    fgets(line, sizeof(line), fc); // Skip header line "name,value"

    while (fgets(line, sizeof(line), fc) != NULL)
    {
        char key[64];
        int value;
        line[strcspn(line, "\r\n")] = 0;
        // Each line is:  sleepT,30
        if (sscanf(line, "%63[^,],%d", key, &value) == 2)
        {
            ESP_LOGI("CSV", "Config: key=%s  value=%d", key, value);

            if (strcmp(key, "sleepT") == 0)
                g_config.sleepT = value;
            else if (strcmp(key, "Location") == 0)
                g_config.location = value;
            else if (strcmp(key, "ShowDevicesOnly") == 0)
                g_config.showDevicesOnly = value;
            else if (strcmp(key, "OldLcd") == 0)
                g_config.oldLcd = value;
            else if (strcmp(key, "ShowData") == 0)
                g_config.showData = value;
            else if (strcmp(key, "childlock") == 0)
                g_config.childlock = value;

            else
            {
                ESP_LOGW("CSV", "Unknown config key: %s", key);
            }
        }
        
    }

    fclose(fc);

    // Print all parsed values
    ESP_LOGI("CFG", "sleepT=%d  location=%d  showDevicesOnly=%d  oldLcd=%d  showData=%d",
             g_config.sleepT, g_config.location, g_config.showDevicesOnly,
             g_config.oldLcd, g_config.showData, g_config.childlock);

    // --- STEP A: READ DEVICES.CSV INTO MEMORY ---
    total_parsed_devices = 0;
    FILE *fd = fopen("/spiffs/devices.csv", "r");
    if (fd)
    {
        char line[128];
        fgets(line, sizeof(line), fd); // Skip header

        while (fgets(line, sizeof(line), fd) != NULL && total_parsed_devices < MAX_CSV_DEVICES) // next line
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
    // --- STEP B: READ SCENES.CSV INTO MEMORY ---
    btn_cfg.welcome_button = false;
    btn_cfg.bright_button = false;
    btn_cfg.day_button = false;
    btn_cfg.sleep_b = false;
    btn_cfg.relax_button = false;
    btn_cfg.dawn_button = false;

    total_parsed_scenes = 0;
    FILE *fs = fopen("/spiffs/scenes.csv", "r");
    if (fs)
    {
        char line[128];
        fgets(line, sizeof(line), fs); // Skip header

        while (fgets(line, sizeof(line), fs) != NULL && total_parsed_scenes < MAX_CSV_SCENES)
        {
            csv_scene_t *scn = &parsed_scenes[total_parsed_scenes];

            // Try comma separation first, fallback to tab/space separation if Excel saved it differently
            if (sscanf(line, "%d,%d,%[^,],%[^\r\n]", &scn->id, &scn->locId, scn->scn_ic, scn->name) == 4 ||
                sscanf(line, "%d %d %s %[^\r\n]", &scn->id, &scn->locId, scn->scn_ic, scn->name) == 4)
            {

                ESP_LOGI("CSV", "Parsed Scene: %s (Icon: %s)", scn->name, scn->scn_ic);
                total_parsed_scenes++;

                if (strcmp(scn->name, "Welcome") == 0)
                    btn_cfg.welcome_button = true;
                else if (strcmp(scn->name, "Bright") == 0)
                    btn_cfg.bright_button = true;
                else if (strcmp(scn->name, "Day") == 0)
                    btn_cfg.day_button = true;
                else if (strcmp(scn->name, "Sleep") == 0)
                    btn_cfg.sleep_b = true;
                else if (strcmp(scn->name, "Relax") == 0)
                    btn_cfg.relax_button = true;
                else if (strcmp(scn->name, "Dawn") == 0)
                    btn_cfg.dawn_button = true;
            }
        }
        fclose(fs);
    }
    else
    {
        ESP_LOGW("CSV", "Failed to open scenes.csv, proceeding without scenes.");
    }

    // --- STEP C: LAYOUT DECISIONS & BASE PANELS ---
    has_scenes = (total_parsed_scenes > 0);
    printf("has_scenes = %d\n", has_scenes);

    
    cct_rgb_light();
    rgbw_light_init();
    lower_panel();

    if (has_scenes)
    {
        top_panel();
        mid_panel();
        lv_obj_add_flag(objects.top_plus_mid_panel, LV_OBJ_FLAG_HIDDEN);

        for (int i = 0; i < total_parsed_scenes; i++)
        {
            csv_scene_t *scn = &parsed_scenes[i];
            printf("Creating scene widget for: %s\n", scn->name);
        }
    }
    else
    {
        lv_obj_add_flag(objects.top_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(objects.mid_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(objects.top_plus_mid_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(objects.top_plus_mid_panel);

        // Reposition scrollbar below items to facilitate page-like scrolling
        lv_obj_set_scrollbar_mode(objects.top_plus_mid_panel, LV_SCROLLBAR_MODE_AUTO);

        lv_obj_add_flag(objects.lower_panel, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_set_pos(objects.lower_panel, 0, 520);
        lv_obj_move_foreground(objects.lower_panel);
    }

    bool has_inscenes_devices = false;
    for (int i = 0; i < total_parsed_devices; i++)
    {
        if (parsed_devices[i].inScenes == 1)
        {
            has_inscenes_devices = true;
            break;
        }
    }

    if (has_inscenes_devices)
        lv_obj_clear_flag(objects.obj3, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(objects.obj3, LV_OBJ_FLAG_HIDDEN);

    // --- STEP D: CREATE AC WIDGETS ---
    for (int i = 0; i < total_parsed_devices; i++)
    {
        csv_device_t *dev = &parsed_devices[i];

        if (dev->type == 4 && !(parsed_devices[i].inScenes == 1))
        {
            if (has_scenes)
                create_ac_widget(dev->name, objects.mid_panel);
            else
                create_ac_widget_big(dev->name, objects.top_plus_mid_panel);
        }
        else if (dev->type == 4 && (parsed_devices[i].inScenes == 1))
        {
            create_ac_widget(dev->name, objects.cct_rgb_light_panel);
        }
    }

    // --- STEP E: CREATE DEVICE WIDGETS ---
    for (int i = 0; i < total_parsed_devices; i++)
    {
        csv_device_t *dev = &parsed_devices[i];
        if (dev->type == 4)
            continue;

        lv_obj_t *target;
        bool use_big;

        if (dev->inScenes == 1)
        {
            target = objects.cct_rgb_light_panel;
            use_big = false;
        }
        else if (has_scenes)
        {
            target = objects.mid_panel;
            use_big = false;
        }
        else
        {
            target = objects.top_plus_mid_panel;
            use_big = true;
        }

        switch (dev->type)
        {
        case 1:
        case 6:
        case 7:
            use_big ? create_lamp_widget_big(dev->name, target) : create_lamp_widget(dev->name, target);
            break;
        case 2:
        case 3:
        case 10:
        case 13:
            use_big ? create_cct_widget_big(dev->name, target) : create_cct_widget(dev->name, target);
            break;
        case 5:
            use_big ? create_fan_widget_big(dev->name, target) : create_fan_widget(dev->name, target);
            break;
        case 8:
            use_big ? create_projector_widget_big(dev->name, target) : create_projector_widget(dev->name, target);
            break;
        case 9:
            use_big ? create_curtain_widget_big(dev->name, target) : create_curtain_widget(dev->name, target);
            break;
        case 11:
        case 12:
        case 14:
            create_rgb_widget(dev->name, target);            //printf("RGB type %d [%s] — widget not yet implemented\n", dev->type, dev->name);
            break;
        default:
            printf("Unhandled type %d [%s]\n", dev->type, dev->name);
            break;
        }
    }

    // --- STEP F: FINISH ---
    printf("Screen built from SPIFFS and saved to NVS\n");

    real_main_screen = lv_scr_act();
    lv_display_trigger_activity(NULL);
}
