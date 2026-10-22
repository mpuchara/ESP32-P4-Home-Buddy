idf_component_register(SRCS "app_main.c" INCLUDE_DIRS "."
    REQUIRES board ui connectivity configuration services ubox_client nvs_flash esp_psram esp_timer esp_app_format spi_flash)
