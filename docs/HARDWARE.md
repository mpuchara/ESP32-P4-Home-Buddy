# Hardware compatibility

Assessment date: 2026-10-22. Physical USB identity: COM5, ESP32-P4 rev. 1.0,
16 MB flash, 40 MHz crystal, USB Serial/JTAG. No C6 firmware was overwritten.

| Peripheral | Evidence | Status for this unit |
|---|---|---|
| P4 / flash | esptool 4.12.0 read on COM5 | Rev 1.0 / 16 MB confirmed |
| ST7701S 480×800 DSI | Physical boot: DSI/PPA startup and LVGL ready | Visible image/orientation observation pending |
| GT911 | Physical probe 0x5D, chip ID 911, configuration 250 | Detected; four-corner touch test pending |
| 32 MB PSRAM | Physical boot: 32 MB at 80 MHz, SPI SRAM memory test OK | Capacity and startup test confirmed |
| C6 SDIO | Physical slot1 40 MHz init; ESP32-C6 detected | Link confirmed; firmware version RPC times out, network acceptance blocked |
| OV02C10 camera | supplied assumption | Exact sensor not verified; not in inspected esp_cam_sensor supported table |
| ES8311 / microphone | upstream pins and schematic sheets | Routing, codec identity and PCM capture not verified |
| Ambient light sensor | no confirmed part / schematic evidence | Do not assume one exists |
| microSD | upstream VO4 supply description | Not mounted by Home Buddy |

## Experimental pin map

Source: [ultramcu BSP](https://github.com/ultramcu/guition-jc4880p4-bsp/tree/324970bade0d1f4e52880fe8016580368bc1e06e)
and [field notes](https://github.com/ultramcu/guition-jc4880p443c-i-w).
The upstream sources disagree on some points: older notes say GPIO-only backlight,
newer BSP uses 5 kHz PWM; touch reset 3 vs alternate community value 22.
This implementation uses the pinned BSP values, never invented pins.

| Block | Signals |
|---|---|
| DSI | 2 lanes, 500 Mbit/s, 34 MHz DPI; reset GPIO5 |
| DSI power | internal LDO3, 2500 mV |
| Backlight | GPIO23; PWM 5 kHz, 8-bit, nonzero floor 26 |
| GT911 | I2C SDA7 / SCL8, reset3, INT unused; probe 0x5D then 0x14 |
| C6 SDIO slot1 | CLK18 CMD19 D0=14 D1=15 D2=16 D3=17 RESET54 |
| microSD slot0 | CLK43 CMD44 D0..3=39..42; LDO4 3300 mV |
| ES8311 reference only | MCLK13 BCLK12 LRCK10 DOUT9 DIN48; PA11 |

The inherited BSP also acquires the SD power rail during display init, but does
not mount/format a card. This side effect is documented, not proof of SD operation.

## Silicon revision

The upstream Arduino platform reports some units as ES rev 1.3. This unit reports
1.0. Arduino's `esp32p4_es` board setting is not an ESP-IDF Kconfig option.
IDF 5.5.1 supports P4 up to revision 1.99 in its checked-in Kconfig. Do not select
a newer production-only toolchain by assuming the suffix identifies the silicon.
Boot logs and actual chip interrogation are authoritative for the connected unit.

## Hardware acceptance

Record boot/reset logs, PSRAM size and test outcome. Observe an 800×480 landscape
screen, tap all four corners, test slider minimum/maximum, cold power-on and repeated
menu transitions. Then test C6 version, scan, association/DHCP, disconnect/reconnect.
A serial "panel initialized" message alone does not establish visible image quality.
Camera capture, audio capture, AI inference, OTA and 72-hour soak are NOT VERIFIED
ON HARDWARE until separately logged.
