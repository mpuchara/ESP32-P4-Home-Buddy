idf_component_register(SRCS "ha_policy.c" "ha_payload.c" "ha_configuration.c" "ha_http_client.c"
 INCLUDE_DIRS "include" REQUIRES esp_common identity
 PRIV_REQUIRES esp_http_client esp_timer esp_hw_support mbedtls nvs_flash json)
