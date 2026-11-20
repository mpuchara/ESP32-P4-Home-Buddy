idf_component_register(SRCS "brightness_service.c" INCLUDE_DIRS "include"
    REQUIRES core PRIV_REQUIRES board camera nvs_flash esp_timer)
