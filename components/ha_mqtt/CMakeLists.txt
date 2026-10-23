idf_component_register(SRCS "mqtt_service.c" "mqtt_config.c" "mqtt_payload.c" INCLUDE_DIRS "include"
    REQUIRES esp_common PRIV_REQUIRES identity mqtt json esp_timer esp_hw_support mbedtls nvs_flash)
