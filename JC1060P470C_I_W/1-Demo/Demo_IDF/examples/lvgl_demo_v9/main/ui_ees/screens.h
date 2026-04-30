#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *main_screen;
    lv_obj_t *top_panel;
    lv_obj_t *welcome_button;
    lv_obj_t *welcome_l;
    lv_obj_t *bright_button_nc;
    lv_obj_t *bright_l;
    lv_obj_t *day_button_nc;
    lv_obj_t *day_label;
    lv_obj_t *relax_button_nc;
    lv_obj_t *relax_label;
    lv_obj_t *sleep_b_nc;
    lv_obj_t *sleep_l;
    lv_obj_t *dawn_button_nc;
    lv_obj_t *dawn_l;
    lv_obj_t *mid_panel;
    lv_obj_t *lower_panel;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *scenes;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *cct_rgb_lights;
    lv_obj_t *advance_lights;
    lv_obj_t *cct_back_but_to_main_scr;
    lv_obj_t *back_button_cct_lights_screen;
    lv_obj_t *temperature_light_indicators;
    lv_obj_t *sixtyfive_k_cct_lights;
    lv_obj_t *fifty_five_k_cct_lights;
    lv_obj_t *five_k_cct_lights;
    lv_obj_t *four_five_k_cct_lights;
    lv_obj_t *four_k_cct_lights;
    lv_obj_t *three_k_five_cct_lights;
    lv_obj_t *three_k_cct_lights;
    lv_obj_t *two_k_five_cct_lights;
    lv_obj_t *cct_rgb_light_panel;
    lv_obj_t *cct_screen_panel;
    lv_obj_t *cct_light_screen_main_label;
    lv_obj_t *back_button_cct_screen;
    lv_obj_t *back_button_arrow_img_cct_screen;
    lv_obj_t *case_temperature_intensity_button_for_sider;
    lv_obj_t *brightness_full_img;
    lv_obj_t *brightness_min_img;
    lv_obj_t *cct_light_intensity_slider_main_screen;
    lv_obj_t *intensity_label_cct_screen;
    lv_obj_t *predefined_colors_label_cct_screen;
    lv_obj_t *six_k_five_img_button_cct_screen;
    lv_obj_t *six_k_five_img_label_cct_screen;
    lv_obj_t *five_k_five_img_button_cct_screen;
    lv_obj_t *five_k_five_img_label_cct_screen;
    lv_obj_t *five_k_img_button_cct_screen;
    lv_obj_t *five_k_label_cct_screen;
    lv_obj_t *fourty_k_five_img_button_cct_screen;
    lv_obj_t *fourty_k_five_img_label_cct_screen;
    lv_obj_t *four_k_img_button_cct_screen;
    lv_obj_t *four_k_label_cct_screen;
    lv_obj_t *three_k_five_img_button_cct_screen;
    lv_obj_t *three_k_five_img_label_cct_screen;
    lv_obj_t *three_k_img_button_cct_screen;
    lv_obj_t *three_k_label_cct_screen;
    lv_obj_t *two_k_five_img_button_cct_screen;
    lv_obj_t *two_k_five_img_label_cct_screen;
    lv_obj_t *brightness_panel;
    lv_obj_t *cancel;
    lv_obj_t *ok;
    lv_obj_t *obj7;
    lv_obj_t *slider;
    lv_obj_t *ac_screen;
    lv_obj_t *back_ac_button;
    lv_obj_t *back_button_img;
    lv_obj_t *ac_screen_label;
    lv_obj_t *temperature_lab;
    lv_obj_t *ac_img;
    lv_obj_t *plus_ac_botton_on_img;
    lv_obj_t *plus_img_ac;
    lv_obj_t *minus_but_on_ac_pic;
    lv_obj_t *minuy_ac_img;
    lv_obj_t *temperature_label_on_ac_pic;
    lv_obj_t *ac_mode_label;
    lv_obj_t *warm_ac_mode_b;
    lv_obj_t *ac_hot_mode_img1;
    lv_obj_t *dummy_ac_warm_b_for_img_but;
    lv_obj_t *warm_mode_ac_buttom_l;
    lv_obj_t *ac_cool_m_button;
    lv_obj_t *ac_cool_mode_img;
    lv_obj_t *dummy_ac_cool_b_for_img_but;
    lv_obj_t *ac_cool_mode_label;
    lv_obj_t *ac_swing_label;
    lv_obj_t *ac_vert_swing_button;
    lv_obj_t *ac_v_swing_img;
    lv_obj_t *ac_swing_horizontal_but;
    lv_obj_t *ac_h_swing_img;
    lv_obj_t *acfabspeed;
    lv_obj_t *ac_fan_sp_auto_but;
    lv_obj_t *ac_fan_sp_auto_img;
    lv_obj_t *ac_fan_sp_auto_label;
    lv_obj_t *ac_fan_sp_slow_but_1;
    lv_obj_t *ac_fan_sp_slow_img;
    lv_obj_t *ac_fan_sp_slow_label;
    lv_obj_t *ac_fan_sp_med_but;
    lv_obj_t *ac_fan_sp_med_img;
    lv_obj_t *ac_fan_sp_med_lab;
    lv_obj_t *ac_fan_sp_fast_but;
    lv_obj_t *ac_f_s_fast_off_img;
    lv_obj_t *ac_fan_sp_fast_lab;
    lv_obj_t *room_selection_screen;
    lv_obj_t *floor_button;
    lv_obj_t *floot_button_label;
    lv_obj_t *select_location_l;
    lv_obj_t *room_selection_container;
    lv_obj_t *login_screen;
    lv_obj_t *username;
    lv_obj_t *username_img;
    lv_obj_t *password;
    lv_obj_t *visible_eye_password;
    lv_obj_t *password_img;
    lv_obj_t *welcome_wohnux_screen_label;
    lv_obj_t *label_2_smarting_enriching;
    lv_obj_t *login_button;
    lv_obj_t *login_label;
    lv_obj_t *keyboard;
    lv_obj_t *invalid_login_label;
    lv_obj_t *internet_on_img;
    lv_obj_t *internet_off_image;
    lv_obj_t *socket_connected_lab;
    lv_obj_t *socket_disconnected_label;
    lv_obj_t *top_plus_mid_panel;
    lv_obj_t *time_format_selection_screen;
    lv_obj_t *twentyfourhr_button;
    lv_obj_t *twelvehour_label;
    lv_obj_t *twelveformat_time_button;
    lv_obj_t *time_format_label;
    lv_obj_t *twentyfourhr_format_label;
    lv_obj_t *time_format_ok_button;
    lv_obj_t *time_format_cancel_button;
    lv_obj_t *selec_order_devices;
    lv_obj_t *back_button_order_sel_screen;
    lv_obj_t *slec_ord_widget_container;
    lv_obj_t *select_and_order_devices_screen_main_label;
    lv_obj_t *slec_order_save_but;
    lv_obj_t *slec_order_save_but_label;
    lv_obj_t *child_lock_unlock_panel;
    lv_obj_t *textarea_pin_enter;
    lv_obj_t *cancel_child_lock_panel;
    lv_obj_t *ok_child_lock_panel;
    lv_obj_t *child_lock_unlock_panel_label;
    lv_obj_t *invalid_child_lock_label;
    lv_obj_t *white_screen_rgb;
    lv_obj_t *white_color_img_while_panel_scr;
    lv_obj_t *white_panel_back_button_label;
    lv_obj_t *color_img_inside_white_panel;
    lv_obj_t *label_white_rgb_screen;
    lv_obj_t *while_cirler_big_img;
    lv_obj_t *white_screen_slider_panel;
    lv_obj_t *brightness_dim_whilet_rgb_slider_img;
    lv_obj_t *brightness_full_whilet_rgb_slider_img;
    lv_obj_t *white_scn_slider;
    lv_obj_t *white_scn_slider_lab;
    lv_obj_t *rgb_screen_container;
    lv_obj_t *color_circle_img;
    lv_obj_t *small_color_img;
    lv_obj_t *small_white_img;
    lv_obj_t *color_circle_intensity_slider_panel;
    lv_obj_t *brightness_dim_rgb_slider_panel;
    lv_obj_t *brightness_full_rgb_slider_panel;
    lv_obj_t *color_slider_panel_label;
    lv_obj_t *color_panel_color_slider;
    lv_obj_t *white_color_slider_panel_inside_rgb_sc;
    lv_obj_t *white_intensity_slider;
    lv_obj_t *white_slider_label_rgb_sc;
    lv_obj_t *brightness_dim_white_slider_panel_color_sc;
    lv_obj_t *brightness_bright_white_slider_panel_color_sc;
    lv_obj_t *rgbw_screen_label;
    lv_obj_t *color_selected_flex_container;
    lv_obj_t *color_picker_image;
    lv_obj_t *plus_color_lable_on_slect_color_img;
    lv_obj_t *back_button_label_rgb_screen;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void create_user_widget_ac_button(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_ac_button(void *flowState, int startWidgetIndex);

void create_user_widget_ceiling_fan_button(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_ceiling_fan_button(void *flowState, int startWidgetIndex);

void create_user_widget_curtain_button(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_curtain_button(void *flowState, int startWidgetIndex);

void create_user_widget_table_lamp_button(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_table_lamp_button(void *flowState, int startWidgetIndex);

void create_user_widget_projector_button(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_projector_button(void *flowState, int startWidgetIndex);

void create_user_widget_cct_light_but(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_cct_light_but(void *flowState, int startWidgetIndex);

void create_user_widget_selection_room_button(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_selection_room_button(void *flowState, int startWidgetIndex);

void create_user_widget_ac_button_big(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_ac_button_big(void *flowState, int startWidgetIndex);

void create_user_widget_ceiling_fan_button_big(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_ceiling_fan_button_big(void *flowState, int startWidgetIndex);

void create_user_widget_table_lamp_button_big(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_table_lamp_button_big(void *flowState, int startWidgetIndex);

void create_user_widget_projector_button_big(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_projector_button_big(void *flowState, int startWidgetIndex);

void create_user_widget_curtain_button_big(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_curtain_button_big(void *flowState, int startWidgetIndex);

void create_user_widget_cct_light_but_big(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_cct_light_but_big(void *flowState, int startWidgetIndex);

void create_user_widget_selec_order_widget_button(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_selec_order_widget_button(void *flowState, int startWidgetIndex);

void create_user_widget_rgbw_button(lv_obj_t *parent_obj, void *flowState, int startWidgetIndex);
void tick_user_widget_rgbw_button(void *flowState, int startWidgetIndex);

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/