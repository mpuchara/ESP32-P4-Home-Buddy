idf_component_register(SRCS "screensaver.c" INCLUDE_DIRS "include"
 REQUIRES core PRIV_REQUIRES esp_driver_jpeg esp_psram esp_timer esp_mm
 EMBED_FILES "assets/synthetic.jpg")
