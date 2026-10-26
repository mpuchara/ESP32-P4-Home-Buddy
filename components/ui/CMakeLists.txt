idf_component_register(SRCS "ui.c" "product_menu.c" "hb_font_20.c" "screen_lifecycle.c" INCLUDE_DIRS "include"
    REQUIRES lvgl core PRIV_REQUIRES audio voice home_assistant identity face board camera brightness connectivity configuration services screensaver esp_timer esp_psram)

# Verify the real target compiler's frames, including the receipt -> enrollment path.
set_source_files_properties(ui.c PROPERTIES COMPILE_OPTIONS "-fstack-usage")
add_custom_target(hb_ui_stack_budget ALL
    COMMAND "${PYTHON}" "${PROJECT_DIR}/tools/check_ui_stack.py" "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS ${COMPONENT_LIB}
    VERBATIM)
