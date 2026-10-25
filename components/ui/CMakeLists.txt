idf_component_register(SRCS "ui.c" "product_menu.c" "hb_font_20.c" "screen_lifecycle.c" INCLUDE_DIRS "include"
    REQUIRES lvgl core PRIV_REQUIRES audio voice home_assistant identity face board camera brightness connectivity configuration services screensaver esp_timer esp_psram)
