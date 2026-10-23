idf_component_register(SRCS "face_policy.c" "presence.c" "face_worker.cpp" INCLUDE_DIRS "include"
 REQUIRES esp_common PRIV_REQUIRES camera compute_budget esp_timer esp_psram espressif__human_face_detect)
idf_component_get_property(model_dir espressif__human_face_detect COMPONENT_DIR)
file(SHA256 "${model_dir}/models/p4/espdet_pico_416_416_face.espdl" digest)
if(NOT digest STREQUAL "a8a0ccfe808fc1c11902340d902f06d8483456e882af683aa23e91108c55e951")
 message(FATAL_ERROR "Unexpected ESPDet416 face detection weights")
endif()
