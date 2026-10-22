# Pinned native CSI DMA corrections

This project-only CMake adapter reads ESP-IDF5.5.1 CSI source, normalizes CRLF and
requires SHA256 `c6d1ca38d50dafe17acfb0fbc647ab1843fae81506e4bd572fa505791332fc5d`.
It generates a patched build-directory source, leaving the installed SDK intact.
Other camera API/ISP-DVP sources, headers and Kconfig come from that same IDF.
The adapter targets this project's ESP32-P4 only.

Upstream source: `components/esp_driver_cam/csi/src/esp_cam_ctlr_csi.c` in
ESP-IDF commit `fcae32885b0296b32044cb99ecbdc50d98dddb83`.
Copyright2023-2025 Espressif Systems, Apache-2.0; original notices are retained
in the generated source. Root Apache-2.0 LICENSE is distributed with firmware.

Two exact changes:

1. DW-GDMA transfer count uses **output** bits per pixel. Original uses RAW10 input
   bits while ISP writes a different format. RAW10->RGB565 filled only5/8 of
   output; RAW10->RAW8 would overrun the output buffer. Both require output bits.
2. Completion invalidates the entire known output buffer. Original invalidates
   `received_size` before assigning the completed size, often zero.

Unknown/changed SDK source fails configuration instead of applying a fuzzy patch.
Reassess and remove this adapter when upgrading IDF. Full-buffer/frequency/color
acceptance on the connected P4 is recorded separately in DEVELOPMENT_LOG.md.
