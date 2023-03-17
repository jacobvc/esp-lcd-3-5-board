idf_component_register(
    SRCS "lcd-3-5-connect.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES driver
    PRIV_REQUIRES esp_timer esp_lcd esp_lcd_touch
)
