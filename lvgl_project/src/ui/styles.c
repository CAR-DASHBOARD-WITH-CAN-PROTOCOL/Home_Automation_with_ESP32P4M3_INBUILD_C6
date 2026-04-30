#include "styles.h"
#include "images.h"
#include "fonts.h"

#include "ui.h"
#include "screens.h"

//
// Style: 
//

void init_style___MAIN_DEFAULT(lv_style_t *style) {
    lv_style_set_align(style, LV_ALIGN_CENTER);
    lv_style_set_layout(style, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(style, LV_FLEX_FLOW_COLUMN);
    lv_style_set_flex_main_place(style, LV_FLEX_ALIGN_CENTER);
    lv_style_set_flex_cross_place(style, LV_FLEX_ALIGN_CENTER);
    lv_style_set_bg_color(style, lv_color_hex(0xffffb000));
    lv_style_set_bg_opa(style, 255);
    lv_style_set_bg_grad_dir(style, LV_GRAD_DIR_NONE);
};

lv_style_t *get_style___MAIN_DEFAULT() {
    static lv_style_t *style;
    if (!style) {
        style = (lv_style_t *)lv_malloc(sizeof(lv_style_t));
        lv_style_init(style);
        init_style___MAIN_DEFAULT(style);
    }
    return style;
};

void add_style__(lv_obj_t *obj) {
    (void)obj;
    lv_obj_add_style(obj, get_style___MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

void remove_style__(lv_obj_t *obj) {
    (void)obj;
    lv_obj_remove_style(obj, get_style___MAIN_DEFAULT(), LV_PART_MAIN | LV_STATE_DEFAULT);
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style__,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style__,
    };
    remove_style_funcs[styleIndex](obj);
}