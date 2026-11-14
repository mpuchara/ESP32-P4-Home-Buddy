idf_component_register(SRCS "ubox_client.c" INCLUDE_DIRS "include"
 REQUIRES configuration core PRIV_REQUIRES identity esp_http_client screensaver ui esp_psram esp_timer)
