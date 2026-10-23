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
* Camera unavailable: inspect SCCB ID, frame/error counters and PSRAM/DMA largest block.
  Do not alter touch reset, C6 reset or MIPI power pins speculatively. Bounded retries
  leave manual brightness/UBox usable. Audio/AI/OTA are not implemented yet.

* Slow/stale local preview: capture FPS is not visible FPS. IDF5.5.1 output-DMA/cache length
  adapter and preview timings are documented in LOCAL_CAMERA.md; do not change
  global RGB565/JPEG/DSI byte order to fix only the local camera.
* Green/pale local preview: use the accepted RAW10->RAW8 pipeline with the pinned
  CSI adapter. Shared frames remain native GBRG; black subtraction, thumbnail-only
  white balance/gamma and CCW90 orientation are explicit stages. Do not apply the
  UBox JPEG byte-order setting to Bayer data. Preview is not calibrated color.

* AUTO disabled: calibrate two valid points at least0.5 relative stops apart.
  Rejected/old samples cannot calibrate. A camera fault holds last safe brightness;
  use MANUAL at any time. Calibration NVS is separate, never erase Wi-Fi/weather.

* Camera ID status RESTART: an uncertain native driver lifetime has quarantined
  camera resources. Manual brightness remains available; a board reboot is needed
  to retry this state. Ordinary3s capture timeouts retry with bounded backoff.
  Keep physical fault recovery acceptance separate from host pool/policy tests.

* Night calibration refused in darkness:0.3.3-dev distinguishes a fresh dark-limit
  reference from unavailable data. Leave the lens uncovered in real night light,
  wait for exposure to settle and select Kalibruj noc when ciemno (prog) appears.
  Negative relative values are normal; missing/stale data has no numeric value.

* Face diagnostic: use good normal lighting and a front-facing face close enough
  for the low-resolution detector. No boxes is not evidence of an unknown identity.
  AI: brak pamieci is a fail-closed admission; exit/retry after current network work.
  Old boxes expire and page close disables inference. Preserve UBox/brightness
  diagnostics if the experimental model cannot run. See FACE_DETECTION.md.
