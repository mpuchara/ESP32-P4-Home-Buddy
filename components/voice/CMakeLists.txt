# Pinned libfvad sources; no network dependency at build time.
set(HB_FVAD_SOURCES
    vendor/libfvad/src/fvad.c
    vendor/libfvad/src/signal_processing/division_operations.c
    vendor/libfvad/src/signal_processing/energy.c
    vendor/libfvad/src/signal_processing/get_scaling_square.c
    vendor/libfvad/src/signal_processing/resample_48khz.c
    vendor/libfvad/src/signal_processing/resample_by_2_internal.c
    vendor/libfvad/src/signal_processing/resample_fractional.c
    vendor/libfvad/src/signal_processing/spl_inl.c
    vendor/libfvad/src/vad/vad_core.c
    vendor/libfvad/src/vad/vad_filterbank.c
    vendor/libfvad/src/vad/vad_gmm.c
    vendor/libfvad/src/vad/vad_sp.c
)
idf_component_register(SRCS "voice_core.c" "voice_embedding.cpp" "voice_diagnostic.cpp" "voice_vad.c" ${HB_FVAD_SOURCES} INCLUDE_DIRS "include"
 PRIV_INCLUDE_DIRS "vendor/libfvad/include" REQUIRES esp_common PRIV_REQUIRES audio compute_budget esp_timer esp_system esp_psram esp_partition spi_flash mbedtls espressif__esp-dl)
