#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "header.h"
#include "screens.h"
#include "images.h"
#include "nvs_flash.h"
#include "nvs.h"

// ============================================================
// STORAGE
// ============================================================
#define MAX_LOCATIONS 20
#define MAX_DEVICES 100

static location_t locations[MAX_LOCATIONS];
static int location_count = 0;

static device_t devices[MAX_DEVICES];
static int device_count = 0;

static ac_data_t ac_list[MAX_AC_DATA];
static int ac_count = 0;

static scene_t scenes[MAX_SCENES];
static int scene_count = 0;

static bool devices_ready = false;


void set_devices_ready(bool val);
bool is_devices_ready(void);

void save_selected_location_to_nvs(int location_id);
int load_selected_location_from_nvs(void);
// ============================================================
// GETTERS
// ============================================================



void save_selected_location_to_nvs(int location_id)
{
    nvs_handle_t handle;
    if (nvs_open("storage", NVS_READWRITE, &handle) == ESP_OK)
    {
        nvs_set_i32(handle, "selected_loc", location_id);
        nvs_commit(handle);
        nvs_close(handle);
        printf("Saved location %d to NVS\n", location_id);
    }
}

int load_selected_location_from_nvs(void)
{
    nvs_handle_t handle;
    int32_t location_id = -1;
    if (nvs_open("storage", NVS_READONLY, &handle) == ESP_OK)
    {
        nvs_get_i32(handle, "selected_loc", &location_id);
        nvs_close(handle);
        printf("Loaded location %d from NVS\n", location_id);
    }
    return location_id;
}



int get_location_count(void)
{

    return location_count;
}
location_t *get_location(int index)
{
    return &locations[index];
}

int get_device_count(void)
{
    return device_count;
}
device_t *get_device(int index)
{
    return &devices[index];
}

int get_ac_count(void)
{
    return ac_count;
}
ac_data_t *get_ac(int index)
{
    return &ac_list[index];
}

int get_scene_count(void)
{
    return scene_count;
}
scene_t *get_scene(int index)
{
    return &scenes[index];
}

bool is_devices_ready(void)
{

    return devices_ready;
}
void set_devices_ready(bool val)
{

    devices_ready = val;
}

scene_t *get_scene_by_name_and_location(const char *name, int location_id)
{
    for (int i = 0; i < scene_count; i++)
    {
        if (scenes[i].location_id == location_id &&
            strcmp(scenes[i].name, name) == 0)
            return &scenes[i];
    }
    return NULL;
}

// ============================================================
// HELPER — replace literal \n with real newline
// ============================================================
static void replace_newlines(char *buf)
{
    char *p = buf;
    while (*p)
    {
        if (p[0] == '\\' && p[1] == 'n')
        {
            p[0] = '\n';
            p[1] = ' ';
        }
        p++;
    }
}

// ============================================================
// PARSE locations.csv
// ============================================================
void parse_locations_csv(const char *csv)
{
    location_count = 0;
    char *buf = strdup(csv);
    if (!buf)
        return;

    replace_newlines(buf);

    char *line = strtok(buf, "\n");
    line = strtok(NULL, "\n"); // skip header

    while (line != NULL && location_count < MAX_LOCATIONS)
    {
        location_t *loc = &locations[location_count];
        char name_buf[32] = {0};

        int parsed = sscanf(line, "%d,%d,%d,%31[^,]",
                            &loc->location_id,
                            &loc->floor_id,
                            (int *)&loc->is_active,
                            name_buf);
        char buf[20];
        lv_snprintf(buf, sizeof(buf), "Floor No:%d", loc->floor_id);

        lv_label_set_text(objects.floot_button_label, buf);
        if (parsed >= 4)
        {
            snprintf(loc->name, sizeof(loc->name), "%s", name_buf);
            #ifdef PRINTFOFF
            printf("Location parsed: [%s] id=%d\n", loc->name, loc->location_id);
            #endif
            location_count++;
        }
        line = strtok(NULL, "\n");
    }

    free(buf);
    printf("Total locations: %d\n", location_count);
}

// ============================================================
// PARSE devices.csv
// ============================================================
void parse_devices_csv(const char *csv)
{
    device_count = 0;
    char *buf = strdup(csv);
    if (!buf)
        return;

    replace_newlines(buf);

    char *line = strtok(buf, "\n");
    line = strtok(NULL, "\n"); // skip header

    while (line != NULL && device_count < MAX_DEVICES)
    {
        device_t *dev = &devices[device_count];
        char name_buf[32] = {0};
        int dimmable = 0, active = 0, in_scenes = 0;
        int FloorId, dummy2, dummy3, dummy4, dummy5, dummy6;

        int parsed = sscanf(line,
                            "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%31[^,],%d,%d",
                            &dev->device_id,
                            &dev->floor_id, // FloorId
                            &dev->location_id,
                            &dummy2, // Comm_Channel
                            &dummy3, // SlaveType
                            &dummy4, // SlaveId
                            &dummy5, // reg5
                            &dummy6, // reg4
                            &dummy3, // reg3
                            &dummy4, // reg2
                            &dummy5, // reg1
                            &dummy6, // Reg_Coil_Address
                            &dev->device_type,
                            &in_scenes,
                            name_buf,
                            &dimmable,
                            &active);

        if (parsed >= 17)
        {
            snprintf(dev->name, sizeof(dev->name), "%s", name_buf);
            dev->dimmable = dimmable;
            dev->active = active;
            dev->in_scenes = in_scenes;

            
            #ifdef PRINTFOFF
            printf("Device parsed: [%s] id=%d type=%d loc=%d Inscene:%d\n",
                   dev->name, dev->device_id, dev->device_type,
                   dev->location_id, dev->in_scenes);
#endif
            // // Create widget only for selected location
            // if (dev->location_id == get_selected_location() && dev->active)
            // {
            //     if (dev->in_scenes == 1)
            //     {
            //         // Goes to scenes/CCT panel
            //         if (dev->device_type == 10 || dev->device_type == 11 || dev->device_type == 14)
            //         {
            //             create_cct_widget(dev->name, objects.cct_light_panel);
            //             printf("CCT widget created in scenes panel: [%s]\n", dev->name);
            //         }
            //     }
            //     else
            //     {
            //         // Goes to mid panel — use device_type not name
            //         switch (dev->device_type)
            //         {
            //         case 5: // Fan
            //             create_fan_widget(dev->name, objects.mid_panel);
            //             printf("Fan widget created: [%s]\n", dev->name);
            //             break;
            //         case 8: // Projector
            //             create_projector_widget(dev->name, objects.mid_panel);
            //             printf("Projector widget created: [%s]\n", dev->name);
            //             break;
            //         case 9: // Curtain
            //             create_curtain_widget(dev->name, objects.mid_panel);
            //             printf("Curtain widget created: [%s]\n", dev->name);
            //             break;
            //         case 1: // Simple relay/lamp
            //             create_lamp_widget(dev->name, objects.mid_panel);
            //             printf("Lamp widget created: [%s]\n", dev->name);
            //             break;
            //         default:
            //             printf("Unknown device type %d for [%s]\n",
            //                    dev->device_type, dev->name);
            //             break;
            //         }
            //     }
            // }
            device_count++;
        }
        line = strtok(NULL, "\n");
    }

    free(buf);
    printf("Total devices: %d\n", device_count);
}

// ============================================================
// PARSE ac.csv
// ============================================================
void parse_ac_csv(const char *csv)
{
    ac_count = 0;
    char *buf = strdup(csv);
    if (!buf)
        return;

    replace_newlines(buf);

    char *line = strtok(buf, "\n");
    line = strtok(NULL, "\n"); // skip header

    while (line != NULL && ac_count < MAX_AC_DATA)
    {
        ac_data_t *ac = &ac_list[ac_count];
        char base[16] = {0};
        char name[32] = {0};
        int active = 0;

        int parsed = sscanf(line, "%d,%d,%d,%d,%15[^,],%31[^,],%d",
                            &ac->id,
                            &ac->slave_id,
                            &ac->location_id,
                            &ac->type,
                            base,
                            name,
                            &active);

        if (parsed >= 7)
        {
            snprintf(ac->base_addr, sizeof(ac->base_addr), "%s", base);
            snprintf(ac->name, sizeof(ac->name), "%s", name);
            ac->active = active;
            #ifdef PRINTFOFF
            printf("AC parsed: [%s] id=%d loc=%d\n", ac->name, ac->id, ac->location_id);
            #endif
            ac_count++;

            // if (ac->location_id == get_selected_location() && ac->active)
            // {
            //     create_ac_widget(ac->name, objects.mid_panel);
            //     printf("AC widget created: [%s]\n", ac->name);
            // }
        }
        line = strtok(NULL, "\n");
    }

    free(buf);
    printf("Total ACs: %d\n", ac_count);
}

// ============================================================
// PARSE scenes.csv
// ============================================================
void parse_scenes_csv(const char *csv)
{
    scene_count = 0;

    // Reset all scene buttons first
    btn_cfg.welcome_button = false;
    btn_cfg.bright_button = false;
    btn_cfg.day_button = false;
    btn_cfg.sleep_b = false;
    btn_cfg.relax_button = false;
    btn_cfg.dawn_button = false;

    char *buf = strdup(csv);
    if (!buf)
        return;

    replace_newlines(buf);

    char *line = strtok(buf, "\n");
    line = strtok(NULL, "\n"); // skip header

    while (line != NULL && scene_count < MAX_SCENES)
    {
        scene_t *sc = &scenes[scene_count];
        memset(sc, 0, sizeof(scene_t));

        char name_buf[16] = {0};
        int active = 0, reset = 0;

        int parsed = sscanf(line, "%d,%d,%d,%d,%15[^,]",
                            &sc->scene_id,
                            &sc->location_id,
                            &active,
                            &reset,
                            name_buf);

        if (parsed < 5)
        {
            line = strtok(NULL, "\n");
            continue;
        }

        sc->active = active;
        snprintf(sc->name, sizeof(sc->name), "%s", name_buf);

        // Parse device/value pairs
        char *ptr = line;
        int comma_count = 0;
        while (*ptr && comma_count < 5)
        {
            if (*ptr == ',')
                comma_count++;
            ptr++;
        }

        sc->device_count = 0;
        while (*ptr && sc->device_count < MAX_SCENE_DEVICES)
        {
            if (*ptr == ',')
            {
                ptr++;
                continue;
            }

            int dev_id = 0, dev_val = 0;
            int n = sscanf(ptr, "%d,%d", &dev_id, &dev_val);
            if (n == 2 && dev_id > 0)
            {
                sc->device_ids[sc->device_count] = dev_id;
                sc->device_vals[sc->device_count] = dev_val;
                sc->device_count++;
            }

            int skip = 2;
            while (*ptr && skip > 0)
            {
                if (*ptr == ',')
                    skip--;
                ptr++;
            }
        }
#ifdef PRINTFOFF
        printf("Scene parsed: [%s] id=%d loc=%d devices=%d\n",
               sc->name, sc->scene_id, sc->location_id, sc->device_count);
#endif
        // Enable scene buttons for selected location
        if (sc->location_id == get_selected_location())
        {
            if (strcmp(sc->name, "Welcome") == 0)
                btn_cfg.welcome_button = true;
            else if (strcmp(sc->name, "Bright") == 0)
                btn_cfg.bright_button = true;
            else if (strcmp(sc->name, "Day") == 0)
                btn_cfg.day_button = true;
            else if (strcmp(sc->name, "Sleep") == 0)
                btn_cfg.sleep_b = true;
            else if (strcmp(sc->name, "Relax") == 0)
                btn_cfg.relax_button = true;
            else if (strcmp(sc->name, "Dawn") == 0)
                btn_cfg.dawn_button = true;
        }

        scene_count++;
        line = strtok(NULL, "\n");
    }

    free(buf);
    #ifdef PRINTFOFF
    printf("Total scenes: %d\n", scene_count);
#endif
    // Set flag — UI will be built in LVGL timer
    devices_ready = true;
}