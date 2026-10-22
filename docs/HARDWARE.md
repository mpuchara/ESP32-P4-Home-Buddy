# Hardware compatibility

Assessment date: 2026-10-22. Physical USB identity: COM5, ESP32-P4 rev. 1.0,
16 MB flash, 40 MHz crystal, USB Serial/JTAG. No C6 firmware was overwritten.

| Peripheral | Evidence | Status for this unit |
|---|---|---|
| P4 / flash | esptool 4.12.0 read on COM5 | Rev 1.0 / 16 MB confirmed |
| ST7701S 480×800 DSI | Physical boot and operator observation of saver/menu text | UI, UBox colors and orientation confirmed; cold-power test pending |
| GT911 | Physical probe 0x5D, chip ID 911, configuration 250; operator opens menu by touch | Operator confirmed touch diagnostic and navigation |
| 32 MB PSRAM | Physical boot: 32 MB at 80 MHz, SPI SRAM memory test OK | Capacity and startup test confirmed |
| C6 SDIO | Physical slot1 20 MHz and station/DHCP | Working station; C6 version RPC times out, firmware untouched |
| OV02C10 camera | SCCB0x36 ID0x5602 and native capture | RAW8 colors/detail and upright preview operator-confirmed |
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

## Iteration 2 evidence, 2026-10-22

0.2.0-dev wifi profile built and was flashed/hash-verified on COM5. Startup reports
LVGL ready and ESP-Hosted scan ESP_OK with 8 records. C6 version metadata still
times out; legacy station scan works with untouched C6 flash. Host software checks
do not establish physical JPEG colors, touch corners, brightness or live DHCP.
Operator tests for the synthetic JPEG, greeting, corners/drag and provisioning
have been requested. Preserve their pending status until an actual response.

Operator subsequently confirmed Test JPEG, touch, greeting and brightness all OK;
Wi-Fi connected with an IP and the clock showed correct local time. Hardware JPEG
1280x720 decode+cover measured 156 ms, free PSRAM 28575716 and largest 28311552
during decode, aligned input 51743, raw RGB565 1843200, final 768000 bytes.
This confirms the offline decode path and station/SNTP workflow, not live UBox or
MQTT. Cold boot/reconnect, repeated network failures and soak remain pending.

##0.2.1-dev follow-up

Operator confirmed a real UBox image, corrected colors after native RGB565 endian
selection, visible token field, and Open-Meteo locality search/selection with a
temperature displayed on the saver. Passive logs show304accepted with the existing
image. During later network traffic, SDIO returned0x109(ESP_ERR_INVALID_CRC) at
40MHz and a snapshot was rejected while retaining the old frame. The host clock
is being reduced to20MHz using the supported Kconfig option, with unchanged pins
and untouched C6 flash. This mitigation still requires verification and is not
proof of long-term transport stability.

The20MHz restart sample subsequently verified actual clock, saved Wi-Fi/DHCP,
Open-Meteo TLS/accepted current response, and a real640x360 JPEG HTTP200accepted
with matching101730-byte transfer. No C6 write was performed. Clock mitigation is
not a guarantee against recurrence; long-term transport testing remains open.

Final0.2.1-dev implementationdd3d444 passed all clean CI profiles in run39807653198
and was flashed/hash-verified on COM5. Post-flash station/DHCP, restored locality,
verified-TLS weather and complete UBox640x360/304 responses were observed. The
new2304x1296 bound/layout is host-tested; no new frame of that size arrived in the
final short sample. Full-resolution hardware acceptance and long-term stability
remain open; no LTE capture was initiated merely to obtain test evidence.

## Iteration 3 physical camera identification

On2026-10-22 COM5 returned OV02C10 ID0x5602 at SCCB0x36 through existing I2C0
SDA7/SCL8, without reset/power writes. Native CSI/ISP subsequently produced
1288x728 RAW8 GBRG at about30fps. The initial RGB565 preview was rejected for
green/pale/noisy images. Operator accepted RAW8 colors/detail; native module head
points right, so portrait preview now rotates CCW90. Operator confirmed upright head in landscape device orientation. No model/inference or actual day/night calibration is accepted yet.
No separate ALS was identified in inspected documentation/BSP; no arbitrary
ALS driver is enabled. See LOCAL_CAMERA.md for source wiring and limits.
