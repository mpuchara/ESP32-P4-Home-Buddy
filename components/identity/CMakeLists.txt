idf_component_register(SRCS "identity_core.c" "identity_service.cpp" "identity_model_storage.c"
 INCLUDE_DIRS "include" REQUIRES esp_common face
 PRIV_REQUIRES camera board compute_budget esp_timer esp_psram esp_system mbedtls fatfs sdmmc esp_driver_sdmmc
 espressif__human_face_recognition espressif__human_face_detect)
