cmake_minimum_required(VERSION 3.16)
project(home_buddy_host_tests C)
enable_testing()
add_executable(test_geometry test_geometry.c ../components/core/geometry.c)
target_include_directories(test_geometry PRIVATE ../components/core/include)
if(MSVC)
  target_compile_options(test_geometry PRIVATE /W4 /WX)
else()
  target_compile_options(test_geometry PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME geometry COMMAND test_geometry)

set(LVGL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl" CACHE PATH "Pinned LVGL 8.3.11 source")
set(HB_FONT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../components/ui/hb_font_20.c" CACHE FILEPATH "Font under test")
if(NOT EXISTS "${LVGL_SOURCE_DIR}/src/font/lv_font_fmt_txt.c")
  message(FATAL_ERROR "Resolve firmware dependencies first or set LVGL_SOURCE_DIR to LVGL 8.3.11")
endif()
add_executable(test_font test_font.c "${HB_FONT_SOURCE}"
  "${LVGL_SOURCE_DIR}/src/font/lv_font.c"
  "${LVGL_SOURCE_DIR}/src/font/lv_font_fmt_txt.c"
  "${LVGL_SOURCE_DIR}/src/misc/lv_utils.c")
target_include_directories(test_font PRIVATE "${LVGL_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(test_font PRIVATE LV_CONF_INCLUDE_SIMPLE LV_KCONFIG_IGNORE)
add_test(NAME font_bitmaps COMMAND test_font)
