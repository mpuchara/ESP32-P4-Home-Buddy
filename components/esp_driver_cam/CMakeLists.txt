# SPDX-License-Identifier: Apache-2.0
# Project-local adapter over the pinned native driver; never edits the installed SDK.
file(TO_CMAKE_PATH "$ENV{IDF_PATH}/components/esp_driver_cam" native)
file(READ "${native}/csi/src/esp_cam_ctlr_csi.c" csi)
string(REPLACE "\r\n" "\n" csi "${csi}")
string(SHA256 source_hash "${csi}")
if(NOT source_hash STREQUAL "c6d1ca38d50dafe17acfb0fbc647ab1843fae81506e4bd572fa505791332fc5d")
    message(FATAL_ERROR "Camera DMA adapter requires the audited ESP-IDF5.5.1 CSI source")
endif()
string(REPLACE "ctlr->h_res * ctlr->v_res * ctlr->in_bpp / 64"
               "ctlr->h_res * ctlr->v_res * ctlr->out_bpp / 64" csi "${csi}")
string(REPLACE "ctlr->trans.buffer), ctlr->trans.received_size, ESP_CACHE_MSYNC_FLAG_DIR_M2C"
               "ctlr->trans.buffer), ctlr->fb_size_in_bytes, ESP_CACHE_MSYNC_FLAG_DIR_M2C" csi "${csi}")
# RGB888 payload ends 64 bytes into a 128-byte L2 cache line. Pad the backing
# allocation and cache maintenance, not the DMA transfer or published length.
string(REPLACE "heap_caps_aligned_alloc(alignment, ctlr->fb_size_in_bytes, MALLOC_CAP_SPIRAM)"
               "heap_caps_aligned_alloc(alignment, (ctlr->fb_size_in_bytes + 127U) & ~127U, MALLOC_CAP_SPIRAM)" csi "${csi}")
string(REPLACE "ctlr->backup_buffer), ctlr->fb_size_in_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M"
               "ctlr->backup_buffer), (ctlr->fb_size_in_bytes + 127U) & ~127U, ESP_CACHE_MSYNC_FLAG_DIR_C2M" csi "${csi}")
string(REPLACE "ctlr->trans.buffer), ctlr->fb_size_in_bytes, ESP_CACHE_MSYNC_FLAG_DIR_M2C"
               "ctlr->trans.buffer), (ctlr->fb_size_in_bytes + 127U) & ~127U, ESP_CACHE_MSYNC_FLAG_DIR_M2C" csi "${csi}")
string(REPLACE "heap_caps_calloc(1, size, buf_caps)"
               "heap_caps_aligned_calloc(128, 1, (size + 127U) & ~127U, buf_caps)" csi "${csi}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/esp_cam_ctlr_csi.c" "${csi}")
idf_component_register(
    SRCS "${native}/esp_cam_ctlr.c" "${native}/dvp_share_ctrl.c"
         "${native}/isp_dvp/src/esp_cam_ctlr_isp_dvp.c"
         "${CMAKE_CURRENT_BINARY_DIR}/esp_cam_ctlr_csi.c"
    INCLUDE_DIRS "${native}/include" "${native}/interface" "${native}/csi/include"
                 "${native}/isp_dvp/include"
    PRIV_INCLUDE_DIRS "${native}" "${native}/csi/src"
    REQUIRES esp_driver_isp esp_mm PRIV_REQUIRES esp_driver_gpio
    LDFRAGMENTS "dw_gdma_cache.lf")
target_compile_definitions(${COMPONENT_LIB} PUBLIC HB_CAMERA_DMA_OUTPUT_BITS=1)
