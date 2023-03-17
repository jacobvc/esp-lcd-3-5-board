// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.2.1
// LVGL VERSION: 8.2.0
// PROJECT: SquareLine_Project

#include "ui.h"
#include "ui_helpers.h"
#include "ui_comp.h"

///////////////////// VARIABLES ////////////////////
lv_obj_t * ui_Screen1;
lv_obj_t * ui_btnPressMe;
lv_obj_t * ui_sldCount;
lv_obj_t * ui_txaNotes;

///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif
#if LV_COLOR_16_SWAP !=0
    #error "LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings"
#endif

///////////////////// ANIMATIONS ////////////////////

///////////////////// FUNCTIONS ////////////////////

///////////////////// SCREENS ////////////////////
void ui_Screen1_screen_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_btnPressMe = ui_Test_create(ui_Screen1);
    lv_obj_set_x(ui_btnPressMe, 2);
    lv_obj_set_y(ui_btnPressMe, -58);

    ui_sldCount = lv_slider_create(ui_Screen1);
    lv_obj_set_width(ui_sldCount, 150);
    lv_obj_set_height(ui_sldCount, 10);
    lv_obj_set_align(ui_sldCount, LV_ALIGN_CENTER);

    ui_txaNotes = lv_textarea_create(ui_Screen1);
    lv_obj_set_width(ui_txaNotes, 150);
    lv_obj_set_height(ui_txaNotes, 70);
    lv_obj_set_x(ui_txaNotes, -4);
    lv_obj_set_y(ui_txaNotes, 83);
    lv_obj_set_align(ui_txaNotes, LV_ALIGN_CENTER);
    lv_textarea_set_text(ui_txaNotes, "Empty");
    lv_textarea_set_placeholder_text(ui_txaNotes, "Placeholder...");

}

void ui_init(void)
{
    LV_EVENT_GET_COMP_CHILD = lv_event_register_id();

    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                               false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    ui_Screen1_screen_init();
    lv_disp_load_scr(ui_Screen1);
}
