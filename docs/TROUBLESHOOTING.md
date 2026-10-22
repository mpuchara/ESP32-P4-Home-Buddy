# Troubleshooting

* IDF 5.1 cannot target P4: activate the dedicated IDF 5.5.1 terminal.
* On Windows set PYTHONIOENCODING=utf-8 before idf.py; upstream build output includes
  Unicode which can fail under the system legacy codepage.
* COM5 enumeration alone is insufficient: use esptool 4.12.0 with --before usb_reset.
  Older esptool 4.7 failed to complete this unit's handshake during this session.
* Default firmware has no visible UI: confirm hardware then select the bringup
  profile; check which sdkconfig/build directory you flash.
* Black display: capture serial output for PSRAM/LDO/DSI/PPA errors, confirm actual
  panel/reset/backlight pins before changing them. Do not guess a similar model.
* Wrong touch orientation: test all corners; change image rotation and inverse touch
  mapping together. Applying both LVGL rotation and manual touch rotation is wrong.
* Wi-Fi: opt-in diagnostics, inspect C6 handshake and exact version, verify SDIO pins.
  Do not blindly flash C6 as a connectivity fix.
* NVS corruption: preserve original flash, diagnose and recover explicitly. Firmware
  deliberately does not automatically erase NVS.
* No camera/audio/AI/OTA: those services are not implemented yet.
