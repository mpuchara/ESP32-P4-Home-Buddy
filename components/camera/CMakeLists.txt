idf_component_register(SRCS "camera_probe.c" "camera_capture.c" "camera_health.c" "frame_pool.c" "light_meter.c" "camera_preview.c" "raw_frame.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_i2c esp_driver_cam esp_driver_isp esp_timer esp_psram esp_mm)
