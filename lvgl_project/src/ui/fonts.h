#ifndef EEZ_LVGL_UI_FONTS_H
#define EEZ_LVGL_UI_FONTS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_font_t ui_font_urbanist;
extern const lv_font_t ui_font_u_font;
extern const lv_font_t ui_font_bold;
extern const lv_font_t ui_font_bold_big;
extern const lv_font_t ui_font_extra_bold_18;
extern const lv_font_t ui_font_bold_36;
extern const lv_font_t ui_font_bold_26;
extern const lv_font_t ui_font_bold_120;
extern const lv_font_t ui_font_bold_20;
extern const lv_font_t ui_font_18_bold;
extern const lv_font_t ui_font_bold_22;
extern const lv_font_t ui_font_bold_28;
extern const lv_font_t ui_font_font_32;
extern const lv_font_t ui_font_bold_24;

#ifndef EXT_FONT_DESC_T
#define EXT_FONT_DESC_T
typedef struct _ext_font_desc_t {
    const char *name;
    const void *font_ptr;
} ext_font_desc_t;
#endif

extern ext_font_desc_t fonts[];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_FONTS_H*/