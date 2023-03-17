// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.2.1
// LVGL VERSION: 8.2.0
// PROJECT: SquareLine_Project


#include "ui.h"
#include "ui_helpers.h"
#include "ui_comp.h"

uint32_t LV_EVENT_GET_COMP_CHILD;

typedef struct {
    uint32_t child_idx;
    lv_obj_t * child;
} ui_comp_get_child_t;

lv_obj_t * ui_comp_get_child(lv_obj_t * comp, uint32_t child_idx)
{
    ui_comp_get_child_t info;
    info.child = NULL;
    info.child_idx = child_idx;
    lv_event_send(comp, LV_EVENT_GET_COMP_CHILD, &info);
    return info.child;
}

void get_component_child_event_cb(lv_event_t * e)
{
    lv_obj_t ** c = lv_event_get_user_data(e);
    ui_comp_get_child_t * info = lv_event_get_param(e);
    info->child = c[info->child_idx];
}

void del_component_child_event_cb(lv_event_t * e)
{
    lv_obj_t ** c = lv_event_get_user_data(e);
    lv_mem_free(c);
}


void ui_event_comp_Test_Test(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    lv_obj_t ** comp_Test = lv_event_get_user_data(e);
    if(event_code == LV_EVENT_CLICKED) {
        Btn2ClickedEvent(e);
    }
}

// COMPONENT Test

lv_obj_t * ui_Test_create(lv_obj_t * comp_parent)
{

    lv_obj_t * cui_Test;
    cui_Test = lv_btn_create(comp_parent);
    lv_obj_set_width(cui_Test, 100);
    lv_obj_set_height(cui_Test, 50);
    lv_obj_set_x(cui_Test, 2);
    lv_obj_set_y(cui_Test, -58);
    lv_obj_set_align(cui_Test, LV_ALIGN_CENTER);
    lv_obj_add_flag(cui_Test, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(cui_Test, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    lv_obj_t * cui_Label2;
    cui_Label2 = lv_label_create(cui_Test);
    lv_obj_set_width(cui_Label2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(cui_Label2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(cui_Label2, LV_ALIGN_CENTER);
    lv_label_set_text(cui_Label2, "Press me");

    lv_obj_t ** children = lv_mem_alloc(sizeof(lv_obj_t *) * _UI_COMP_TEST_NUM);
    children[UI_COMP_TEST_TEST] = cui_Test;
    children[UI_COMP_TEST_LABEL2] = cui_Label2;
    lv_obj_add_event_cb(cui_Test, get_component_child_event_cb, LV_EVENT_GET_COMP_CHILD, children);
    lv_obj_add_event_cb(cui_Test, del_component_child_event_cb, LV_EVENT_DELETE, children);
    lv_obj_add_event_cb(cui_Test, ui_event_comp_Test_Test, LV_EVENT_ALL, children);
    return cui_Test;
}

