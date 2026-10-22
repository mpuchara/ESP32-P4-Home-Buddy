idf_component_register(SRCS "ui.c" "hb_font_20.c" "screen_lifecycle.c" INCLUDE_DIRS "include"
    REQUIRES lvgl PRIV_REQUIRES board core connectivity configuration services screensaver esp_timer esp_psram)
