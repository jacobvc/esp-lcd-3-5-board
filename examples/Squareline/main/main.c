#include "esp_log.h"
#include "bsp/esp-lcd-3-5-bsp.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "string.h"

#define TAG "MAIN"
#define APP_DISP_DEFAULT_BRIGHTNESS 50

#include <stdio.h>
#include "dirent.h"
#define MAX_DIR_BYTES 4096
#define MOUNT_POINT "/sd"

void sdcard_ls(sdmmc_card_t* card, const char *path)
{
    if (card) {
        DIR* dir = opendir(path);
        struct dirent* de = readdir(dir);
        lv_label_set_text(ui_lblPath, path);
        lv_textarea_set_text(ui_txaFiles, "");
        while (de) {
            lv_textarea_add_text(ui_txaFiles, de->d_name);
            if (de->d_type == DT_DIR) {
                lv_textarea_add_text(ui_txaFiles, "/");
            }
            lv_textarea_add_text(ui_txaFiles, "\n");
            de = readdir(dir);
        }
    }
    else {
        lv_label_set_text(ui_lblPath, "Can't read SD Card");
    }
}

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

    /* Initialize display and LVGL */
    lv_disp_t *disp = bsp_lcd_start(false);
    lv_disp_set_rotation(disp, LV_DISP_ROT_90);

    /* Add and show objects on display */
    app_lvgl_display();

    ESP_LOGI(TAG, "Initialization done.");

    esp_err_t err;
    sdmmc_card_t *card = bsp_lcd_sdcard_mount(MOUNT_POINT, &err);
    sdcard_ls(card, MOUNT_POINT);
    bsp_lcd_sdcard_unmount(card, MOUNT_POINT);

}
