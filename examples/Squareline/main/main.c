#include "esp_log.h"
#include "bsp/esp-lcd-3-5-bsp.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "string.h"

#define TAG "MAIN"

/***
 *      ____   ____     ____                 _   _____           _   
 *     / ___| |  _ \   / ___| __ _  _ __  __| | |_   _|___  ___ | |_ 
 *     \___ \ | | | | | |    / _` || '__|/ _` |   | | / _ \/ __|| __|
 *      ___) || |_| | | |___| (_| || |  | (_| |   | ||  __/\__ \| |_ 
 *     |____/ |____/   \____|\__,_||_|   \__,_|   |_| \___||___/ \__|
 *                                                                   
 */
#include <stdio.h>
#include "dirent.h"
#define MAX_DIR_BYTES 4096
#define MOUNT_POINT "/sd"

void BtnLsClicked(lv_event_t * e)
{
    const char *path = MOUNT_POINT;
    esp_err_t err = 0;
    lv_textarea_set_text(ui_txaFiles, "");
    sdmmc_card_t *card = bsp_lcd_sdcard_mount(MOUNT_POINT, &err);
    if (card)
    {
        DIR *dir = opendir(path);
        struct dirent *de = readdir(dir);
        lv_label_set_text(ui_lblPath, path);
        while (de)
        {
            lv_textarea_add_text(ui_txaFiles, de->d_name);
            if (de->d_type == DT_DIR)
            {
                lv_textarea_add_text(ui_txaFiles, "/");
            }
            lv_textarea_add_text(ui_txaFiles, "\n");
            de = readdir(dir);
        }
        bsp_lcd_sdcard_unmount(card, MOUNT_POINT);
    }
    else
    {
        lv_label_set_text(ui_lblPath, "Can't read SD Card");
    }
}

/***
 *      ___  ____    ____   _____           _   
 *     |_ _||___ \  / ___| |_   _|___  ___ | |_ 
 *      | |   __) || |       | | / _ \/ __|| __|
 *      | |  / __/ | |___    | ||  __/\__ \| |_ 
 *     |___||_____| \____|   |_| \___||___/ \__|
 *                                              
 */
#include "bmx280.h"

bmx280_t* bmx280 = NULL;

esp_err_t bmp280Init()
{
    bmx280 = bmx280_create(BSP_LCD_I2C_NUM);

    if (!bmx280) { 
        ESP_LOGE(TAG, "Could not create bmx280 driver.");
        return -1;
    }

    esp_err_t err = bmx280_init(bmx280);
    if (err == ESP_OK) {
        bmx280_config_t bmx_cfg = BMX280_DEFAULT_CONFIG;
        err = bmx280_configure(bmx280, &bmx_cfg);
    }
    if (err != ESP_OK) {
        bmx280_close(bmx280);
        bmx280 = NULL;
    }
    return err;
}

void BtnSampleClicked(lv_event_t * e)
{
    if (bmx280) {
        ESP_ERROR_CHECK(bmx280_setMode(bmx280, BMX280_MODE_FORCE));
        do {
            vTaskDelay(pdMS_TO_TICKS(1));
        } while(bmx280_isSampling(bmx280));

        float temp = 0, pres = 0, hum = 0;
        ESP_ERROR_CHECK(bmx280_readoutFloat(bmx280, &temp, &pres, &hum));
        char buf[80];
        sprintf(buf, "%f", pres);
        lv_label_set_text(ui_lblPressure, buf);
        sprintf(buf, "%f", temp);
        lv_label_set_text(ui_lblTemp, buf);
    }
}

void SldBacklightChanged(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    char buf[8];
    //lv_snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(slider));
    //lv_label_set_text(slider_label, buf);
    //lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    bsp_lcd_set_brightness((int)lv_slider_get_value(slider));
}

/***
 *                        _        
 *      _ __ ___    __ _ (_) _ __  
 *     | '_ ` _ \  / _` || || '_ \ 
 *     | | | | | || (_| || || | | |
 *     |_| |_| |_| \__,_||_||_| |_|
 *                                 
 */
void app_main(void)
{
    // Initialize display and LVGL
    lv_disp_t *disp = bsp_lcd_start(false);
    lv_disp_set_rotation(disp, LV_DISP_ROT_90); // Landscape

    // Lock the display and iniitialize the SquareLine ui
    bsp_lcd_lock(0);
    ui_init();
    bsp_lcd_unlock();

    lv_slider_set_value(ui_sldBacklight, 50, LV_ANIM_OFF);
    bsp_lcd_set_brightness(50);

    ESP_LOGI(TAG, "Display Initialization done.");

    // Initialize I2C and BMP280
    bsp_lcd_i2c_init();
    bmp280Init();
    if (!bmx280) {
        lv_label_set_text(ui_lblPressure, "BMP280 NOT FOUND");
        lv_label_set_text(ui_lblTemp, "BMP280 NOT FOUND");
    }
}
