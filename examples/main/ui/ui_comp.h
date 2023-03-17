// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.2.1
// LVGL VERSION: 8.2.0
// PROJECT: SquareLine_Project

#ifndef _SQUARELINE_PROJECT_UI_COMP_H
#define _SQUARELINE_PROJECT_UI_COMP_H

#include "ui.h"

lv_obj_t * ui_comp_get_child(lv_obj_t * comp, uint32_t child_idx);
extern uint32_t LV_EVENT_GET_COMP_CHILD;

// COMPONENT Test
#define UI_COMP_TEST_TEST 0
#define UI_COMP_TEST_LABEL2 1
#define _UI_COMP_TEST_NUM 2
lv_obj_t * ui_Test_create(lv_obj_t * comp_parent);
void ui_event_comp_Test_Test(lv_event_t * e);

#endif
