idf_component_register(
    SRCS "esp-lcd-3-5-bsp.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES driver
    PRIV_REQUIRES fatfs esp_timer esp_lcd esp_lcd_touch
)
