
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "bsp/esp-lcd-3-5-bsp.h"
#include "esp_lcd_ili9488.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_lvgl_port.h"
#include "bsp_err_check.h"

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"

static const char *TAG = "ESP LCD 3.5 DIY";

/*
 * Backlight definitions
 */
static const ledc_mode_t BACKLIGHT_LEDC_MODE = LEDC_LOW_SPEED_MODE;
static const ledc_channel_t BACKLIGHT_LEDC_CHANNEL = LEDC_CHANNEL_0;
static const ledc_timer_t BACKLIGHT_LEDC_TIMER = LEDC_TIMER_1;
static const ledc_timer_bit_t BACKLIGHT_LEDC_TIMER_RESOLUTION = LEDC_TIMER_10_BIT;
static const uint32_t BACKLIGHT_LEDC_FRQUENCY = 5000;

/*
 * SD Card definitions
 */
sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
sdmmc_host_t host = SDSPI_HOST_DEFAULT();
sdmmc_card_t *bsp_sdcard = NULL;    // Global SD card handler

/*
 * I2C Support
 */
esp_err_t bsp_lcd_i2c_init(void)
{
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_LCD_I2C_SDA_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = BSP_LCD_I2C_SCL_PIN,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = BSP_LCD_I2C_CLK_SPEED_HZ
    };
    BSP_ERROR_CHECK_RETURN_ERR(i2c_param_config(BSP_LCD_I2C_NUM, &i2c_conf));
    BSP_ERROR_CHECK_RETURN_ERR(i2c_driver_install(BSP_LCD_I2C_NUM, i2c_conf.mode, 0, 0, 0));

    return ESP_OK;
}

esp_err_t bsp_lcd_i2c_deinit(void)
{
    BSP_ERROR_CHECK_RETURN_ERR(i2c_driver_delete(BSP_LCD_I2C_NUM));
    return ESP_OK;
}

/*
 * SD Card support
 */
sdmmc_card_t *bsp_lcd_sdcard_mount(const char *mount_point, esp_err_t *pErr)
{
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        //.format_if_mount_failed = true,
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    *pErr = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &bsp_sdcard);

    if (*pErr != ESP_OK) {
        if (*pErr == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(*pErr));
        }
        return NULL;
    }
    return bsp_sdcard;
}

esp_err_t bsp_lcd_sdcard_unmount(sdmmc_card_t* card, const char *mount_point)
{
    esp_vfs_fat_sdcard_unmount(mount_point, card);

    return ESP_OK;
}

void sdcard_init(void)
{
    esp_err_t err;
    spi_device_handle_t spi;
    ESP_LOGI(TAG, "Adding SD card to SHARED SPI");

    spi_device_interface_config_t devcfg;
    memset(&devcfg, 0, sizeof(devcfg));
    
    devcfg.clock_speed_hz = 10 * 1000 * 1000, //Clock out at 10 MHz
    devcfg.mode = 0,                          //SPI mode 0
    devcfg.spics_io_num = BSP_SDCARD_CS_PIN,  //CS pin
    devcfg.queue_size = 7,                    //We want to be able to queue 7 transactions at a time
    // devcfg.pre_cb Not used
 
    // Initialize the slot without card detect (CD)
    host.slot = BSP_SHARED_SPI_HOST;
    host.max_freq_khz = SDMMC_FREQ_PROBING;

    slot_config.gpio_cs = BSP_SDCARD_CS_PIN;
    slot_config.host_id = BSP_SHARED_SPI_HOST;

    err = spi_bus_add_device(BSP_SHARED_SPI_HOST, &devcfg, &spi);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    char *mount_point = "/sdcard";
    sdmmc_card_t *card = bsp_lcd_sdcard_mount(mount_point, &err);
    if (card) {
        // Card has been initialized, print its properties
        sdmmc_card_print_info(stdout, card);

        bsp_lcd_sdcard_unmount(card, mount_point);
    }
}

/*
 * LCD Support
 */
// Number of bits used to represent command and parameter
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    //ESP_LOGD(TAG, "Flush ready");
    lv_disp_t ** disp = (lv_disp_t **)user_ctx;
    lvgl_port_flush_ready(*disp);
    return false;
}


static lv_disp_t *display_lcd_init()
{
    static lv_disp_t *pDisp = NULL;

    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = BSP_LCD_SPI_CLK_PIN,
        .mosi_io_num = BSP_LCD_SPI_MOSI_PIN,
        .miso_io_num = BSP_LCD_SPI_MISO_PIN,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 32768,
    };
    BSP_ERROR_CHECK_RETURN_NULL(spi_bus_initialize(BSP_SHARED_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BSP_LCD_DC_PIN,
        .cs_gpio_num = BSP_LCD_CS_PIN,
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
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_SHARED_SPI_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install LCD driver of ili(9488)");
    esp_lcd_panel_handle_t panel_handle = NULL;
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST_PIN,
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
        .bits_per_pixel = 18,
    };
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_new_panel_ili9488(io_handle, 
      &panel_config, LCD_BUFFER_SIZE, &panel_handle));

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
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
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

/*
 * Backlight Support
 */
static void backlight_init() 
{
    
    const ledc_channel_config_t LCD_backlight_channel =
    {
        .gpio_num = (gpio_num_t)BSP_LCD_BACKLIGHT_PIN,
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .channel = BACKLIGHT_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BACKLIGHT_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .flags = 
        {
            .output_invert = 0
        }
    };
    const ledc_timer_config_t LCD_backlight_timer =
    {
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .duty_resolution = BACKLIGHT_LEDC_TIMER_RESOLUTION,
        .timer_num = BACKLIGHT_LEDC_TIMER,
        .freq_hz = BACKLIGHT_LEDC_FRQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_LOGI(TAG, "Initializing LEDC for backlight pin: %d", BSP_LCD_BACKLIGHT_PIN);

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));
}

void bsp_lcd_set_brightness(int brightness_percentage)
{
    if (brightness_percentage > 100)
    {
        brightness_percentage = 100;
    }    
    else if (brightness_percentage < 0)
    {
        brightness_percentage = 0;
    }
    ESP_LOGI(TAG, "Setting backlight to %d%%", brightness_percentage);

    // LEDC resolution set to 10bits, thus: 100% = 1023
    uint32_t duty_cycle = (1023 * brightness_percentage) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL));
}

static lv_indev_t *display_indev_init(lv_disp_t *disp)
{
    esp_lcd_touch_handle_t tp;

    const esp_lcd_panel_io_spi_config_t io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(BSP_TOUCH_CS_PIN);
 
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
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 1,
        },
    };
    
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_SHARED_SPI_HOST, &io_config, &tp_io_handle));
    BSP_ERROR_CHECK_RETURN_NULL(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));
    assert(tp);

    // Add touch input (for selected screen) 
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = tp,
    };

    return lvgl_port_add_touch(&touch_cfg);
}

/*
 * LCD Start and launch
 */
lv_disp_t *bsp_lcd_start(int extra_stack)
{
    lv_disp_t *disp = NULL;
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_stack += extra_stack;
    BSP_ERROR_CHECK_RETURN_NULL(lvgl_port_init(&lvgl_cfg));
    BSP_NULL_CHECK(disp = display_lcd_init(), NULL);
    BSP_NULL_CHECK(display_indev_init(disp), NULL);
    backlight_init();

    sdcard_init();
    return disp;
}

bool bsp_lcd_lock(uint32_t timeout_ms)
{
    return lvgl_port_lock(timeout_ms);
}

void bsp_lcd_unlock(void)
{
    lvgl_port_unlock();
}
