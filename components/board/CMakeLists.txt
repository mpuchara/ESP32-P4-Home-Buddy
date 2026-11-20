idf_component_register(SRCS "board.c" "vendor/board_p4.c"
    INCLUDE_DIRS "include" "vendor"
    REQUIRES esp_lcd esp_lcd_touch_gt911
    PRIV_REQUIRES esp_driver_gpio esp_driver_i2c esp_driver_ledc esp_driver_ppa
                  esp_hw_support esp_mm esp_psram)
