idf_component_register(SRCS "enrollment_flow.c" "identity_core.c" "identity_fusion.c" "identity_service.cpp" "identity_model_storage.c"
 "identity_model_format.c" "identity_events.c" "profile_store.c" "profile_flash.c" "household_voice.c" "profile_store_v2.c"
 INCLUDE_DIRS "include" REQUIRES esp_common face voice
 PRIV_REQUIRES configuration camera board compute_budget esp_timer esp_psram esp_system mbedtls esp_partition spi_flash bootloader_support
 espressif__human_face_recognition espressif__human_face_detect)
