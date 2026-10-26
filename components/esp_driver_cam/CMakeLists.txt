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
