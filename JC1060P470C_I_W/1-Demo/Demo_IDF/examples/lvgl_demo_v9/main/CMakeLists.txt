set(LV_DEMO_DIR ../managed_components/lvgl__lvgl/demos)
file(GLOB_RECURSE LV_DEMOS_SOURCES ${LV_DEMO_DIR}/*.c)

file(GLOB UI_ASSET_SOURCES "UI/*.c")
file(GLOB LOWER_SCREEN_ASSET_SOURCES "lower_screen/*.c")
file(GLOB UI_EES_ASSET_SOURCES
                    "ui_ees/*.c"
                     "ui_ees/*.cpp")
# -------------------------------------------------

idf_component_register(
    SRCS 
        main.c
        home_screen.c
        ethernet.c
        login.c
        lower_panel.c
        top_panel.c
        ac_screen.c
        mid_panel.c
        cct_lights.c
        data_manager.c
        select_n_order.c
        c6.c
        file.c
        rgb_lights.c
        ${UI_ASSET_SOURCES}
        ${LOWER_SCREEN_ASSET_SOURCES}
        ${UI_EES_ASSET_SOURCES}
        ${UI_SOURCES}
        ${LV_DEMOS_SOURCES}
    INCLUDE_DIRS 
        . 
        UI
        lower_screen
        ui_ees
        ${LV_DEMO_DIR}
        ../managed_components/lvgl__lvgl/src
        ../managed_components/espressif__esp_websocket_client/include
    REQUIRES
)

idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)
target_compile_options(
    ${LVGL_LIB}
    PRIVATE
        -DLV_LVGL_H_INCLUDE_SIMPLE
        -DLV_USE_DEMO_MUSIC
)
