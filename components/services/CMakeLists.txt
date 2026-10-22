idf_component_register(SRCS "services.c" "temperature.c" INCLUDE_DIRS "include" REQUIRES configuration core PRIV_REQUIRES mqtt esp_netif esp_event esp_timer mbedtls json ui)
