idf_component_register(SRCS "voice_core.c" "voice_embedding.cpp" "voice_diagnostic.cpp" INCLUDE_DIRS "include"
 REQUIRES esp_common PRIV_REQUIRES audio compute_budget esp_timer esp_system esp_psram esp_partition spi_flash mbedtls espressif__esp-dl)
