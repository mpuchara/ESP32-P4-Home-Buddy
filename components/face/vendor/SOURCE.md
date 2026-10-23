# Diagnostic face detection sources

Registry pins: esp-dl3.3.14, human_face_detect0.5.1, dl_fft0.8.0,
esp_new_jpeg1.0.2; exact integrity records are in dependencies.lock.
ESP-DL upstream commit5acd1c9992af4afb0fa80c221a676982b11381b8;
model-component source17e9abb1c59cd66b77b4bdb95587abcfafbb124a.
ESP-DL and model component declare MIT, with their exact registry licenses here.
FFT declares MIT; its registry archive omits LICENSE, so the upstream root MIT at
source1ab1e264d4270bfad6a45760ea19273dca0cedfc is retained here.
ESP-New-JPEG has an Espressif-product-restricted MIT-style license, retained
verbatim. This target is ESP32-P4. No ESPHome or external inference service.

Selected P4 ESPDet Pico Face 416x416, 578,160 bytes, embedded in application rodata:
SHA256 a8a0ccfe808fc1c11902340d902f06d8483456e882af683aa23e91108c55e951
CMake verifies the weight hash. This replaces the MSR/MNP selection from 0.4.0;
registry/source pins are unchanged. Build-time registry downloads are required;
device inference and its weights have no network dependency. No household images
or biometric templates are bundled. Code is experimental detection, not identity.
