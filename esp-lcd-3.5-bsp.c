/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "bsp/lcd-3-5-connect.h"
#include "esp_lcd_ili9488.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_lvgl_port.h"
#include "bsp_err_check.h"

static const char *TAG = "LCD35";

esp_err_t bsp_i2c_init(void)
{/*
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
//        .sda_io_num = BSP_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
//        .scl_io_num = BSP_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
//        .master.clk_speed = BSP_I2C_CLK_SPEED_HZ
    };
//    BSP_ERROR_CHECK_RETURN_ERR(i2c_param_config(BSP_I2C_NUM, &i2c_conf));
//    BSP_ERROR_CHECK_RETURN_ERR(i2c_driver_install(BSP_I2C_NUM, i2c_conf.mode, 0, 0, 0));
*/
    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
//    BSP_ERROR_CHECK_RETURN_ERR(i2c_driver_delete(BSP_I2C_NUM));
    return ESP_OK;
}

// Bit number used to represent command and parameter
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

/* The component calls esp_lcd_panel_draw_bitmap API for send data to the screen. There must be called 
  lvgl_port_flush_ready(disp) after each transaction to display. The best way is to use on_color_trans_done 
  callback from esp_lcd IO config structure. 
*/
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    //ESP_LOGD(TAG, "Flush ready");
    lv_disp_t ** disp = (lv_disp_t **)user_ctx;
    lvgl_port_flush_ready(*disp);
    return false;
}


static lv_disp_t *bsp_display_lcd_init(bool asLandscape)
{
    static lv_disp_t *pDisp = NULL;

    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = BSP_LCD_SPI_CLK,
        .mosi_io_num = BSP_LCD_SPI_MOSI,
        .miso_io_num = BSP_LCD_SPI_MISO,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 32768,
    };
    BSP_ERROR_CHECK_RETURN_NULL(spi_bus_initialize(BSP_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BSP_LCD_DC,
        .cs_gpio_num = BSP_LCD_CS,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
		.on_color_trans_done = notify_lvgl_flush_ready,
		.user_ctx = &pDisp
    };

    // Attach the LCD to the SPI bus
    esp_lcd_panel_io_handle_t io_handle = NULL;
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install LCD driver of ili(9488)");
    esp_lcd_panel_handle_t panel_handle = NULL;
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST,
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
        .bits_per_pixel = 18,
    };
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_new_panel_ili9488(io_handle, 
      &panel_config, LCD_BUFFER_SIZE, &panel_handle, asLandscape));

    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_panel_reset(panel_handle));
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_panel_init(panel_handle));
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_panel_disp_on_off(panel_handle, true));

    /* Add LCD screen */
    ESP_LOGI(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_BUFFER_SIZE,
        .double_buffer = true,
        .hres = asLandscape ? BSP_LCD_V_RES : BSP_LCD_H_RES,
        .vres = asLandscape ? BSP_LCD_H_RES : BSP_LCD_V_RES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
        }
    };

    pDisp = lvgl_port_add_disp(&disp_cfg);

    return pDisp;
}

static lv_indev_t *bsp_display_indev_init(lv_disp_t *disp, bool asLandscape)
{
    esp_lcd_touch_handle_t tp;

    const esp_lcd_panel_io_spi_config_t io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(BSP_LCD_TP_CS);
 
    // Initialize touch 
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_H_RES,
        .y_max = BSP_LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = asLandscape ? 1 : 0,
            .mirror_x = asLandscape ? 0 : 0,
            .mirror_y = asLandscape ? 0 : 1,
        },
    };
    
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, &tp_io_handle));
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));
    assert(tp);

    // Add touch input (for selected screen) 
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);
}

esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_display_backlight_off(void)
{
    return bsp_display_brightness_set(0);
}

esp_err_t bsp_display_backlight_on(void)
{
    return bsp_display_brightness_set(100);
}

lv_disp_t *bsp_display_start(bool asLandscape)
{
    lv_disp_t *disp = NULL;
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    BSP_ERROR_CHECK_RETURN_NULL(lvgl_port_init(&lvgl_cfg));
    BSP_NULL_CHECK(disp = bsp_display_lcd_init(asLandscape), NULL);
    BSP_NULL_CHECK(bsp_display_indev_init(disp, asLandscape), NULL);
    return disp;
}

void bsp_display_rotate(lv_disp_t *disp, lv_disp_rot_t rotation)
{
    lv_disp_set_rotation(disp, rotation);
}

bool bsp_display_lock(uint32_t timeout_ms)
{
    return lvgl_port_lock(timeout_ms);
}

void bsp_display_unlock(void)
{
    lvgl_port_unlock();
}
