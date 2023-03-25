#include "esp_log.h"
#include "bsp/esp-lcd-3-5-bsp.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "string.h"

#define TAG "MAIN"

void app_lvgl_display(void)
{
    bsp_lcd_lock(0);

    ui_init();

    bsp_lcd_unlock();
}

void app_main(void)
{
    /* Initialize I2C */
    bsp_lcd_i2c_init();

    /* Initialize display, LVGL, and SD Card */
    lv_disp_t *disp = bsp_lcd_start(false);
    lv_disp_set_rotation(disp, LV_DISP_ROT_90);

    /* Add and show objects on display */
    app_lvgl_display();

    ESP_LOGI(TAG, "Initialization done.");
}
