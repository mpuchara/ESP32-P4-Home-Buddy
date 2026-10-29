cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(PROJECT_VER "0.11.9-dev")
project(home_buddy)
if(NOT CONFIG_PARTITION_TABLE_OFFSET EQUAL 0x10000 OR
   NOT CONFIG_PARTITION_TABLE_CUSTOM_FILENAME STREQUAL "partitions.csv" OR
   CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE)
    message(FATAL_ERROR "Use a fresh 0.6.1 single-image sdkconfig: table 0x10000, partitions.csv, no OTA rollback. See docs/USB_FLASHING.md")
endif()
add_custom_target(hb_isr_placement ALL
    COMMAND "${PYTHON}" "${PROJECT_DIR}/tools/check_isr_placement.py"
            "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf"
            "${CMAKE_BINARY_DIR}/config/sdkconfig.json"
    DEPENDS "${CMAKE_PROJECT_NAME}.elf")
