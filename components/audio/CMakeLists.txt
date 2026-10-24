idf_component_register(SRCS "audio_service.c" "audio_pcm.c" INCLUDE_DIRS "include"
 REQUIRES esp_common PRIV_REQUIRES esp_driver_i2s esp_driver_i2c esp_timer esp_codec_dev)
