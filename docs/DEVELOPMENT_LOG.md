# Development log

## 2026-10-22: initial foundation

Repository began at 0c361ab with LICENSE only. Work is on
codex/initial-hardware-foundation. HA_UBOX_bridge was read through GitHub only;
no checkout, file mutation or commit was made in that repository.

### Research and implementation

Inspected both requested ultramcu repositories, their source APIs and licenses,
the existing UBox snapshot handler, official ESP-DL speaker example and the
esp_cam_sensor supported sensor list. Selected IDF 5.5.1, LVGL 8.3.11 and exact
managed dependency pins. Created architecture, partition/PSRAM budget, roadmap,
hardware/API/privacy/build/recovery documentation, CI and draft-release workflows.

Implemented serial diagnostics, non-destructive NVS init, an experimental DSI/PPA
LVGL port, GT911 inverse coordinate mapping, Polish font, touch-open menu, manual
brightness and inactivity return, plus optional version-gated C6 scan diagnostics.
The menu explicitly labels unfinished product features. No fabricated recognition,
time, weather, network connection or camera image is presented.

### Confirmed physical evidence

* esptool 4.12.0, COM5: ESP32-P4 rev. 1.0, 40 MHz crystal, 16 MB flash.
* Full original P4 flash backup completed, 16,777,216 bytes, SHA256
  d951fe69eeb2e297c75e76ad970bf5caeafd4b7a71ab88cdf6167f1bd6c47c15.
  Private backup remains in ignored local/original-p4-flash.bin; never upload it.
* Initial diagnostics image compiled, flashed and hash-verified. Serial boot:
  IDF v5.5.1; 32 MB PSRAM detected; SPI SRAM memory test OK; app_main reached.
* Initial default image initialized ESP-Hosted bookkeeping via its constructor.
  Final configuration disables the component in diagnostics/display profiles;
  C6 is enabled only in the explicit Wi-Fi profile. No C6 firmware was flashed.

### Verification completed so far

* CMake/MSVC host test: cover/crop aspect ratios, overflow, coverage over many
  dimensions, invalid inputs and touch corners passed (CTest 1/1).
* Python configuration suite: dependency lock parity, flash partitions and all
  Polish glyphs passed (3/3).
* clang-format 19.1.7 application checks and actionlint 1.7.12 passed.
* Diagnostic and first experimental display firmware builds passed. Final
  display profile with ESP-Hosted disabled also compiled and flashed successfully
  (0x8b070-byte app). Effective configuration checker passed.
* Display-profile boot: DSI panel initialized, two 480×800 native buffers allocated,
  PPA rotation registered, GPIO23 LEDC enabled, GT911 chip ID 911 read at 0x5D,
  and LVGL 800×480 task started. Initial free memory: internal 429123 bytes,
  PSRAM 31245164 bytes. Twelve-second capture contains no reset or display error.
  This is initialization evidence; visual/touch acceptance remains separate.
* GT911 driver warns "Unable to initialize the I2C address" with no INT line,
  then reads chip ID/configuration successfully. Functional touch remains to test.
* Local Git push had no noninteractive credentials. Published the same file tree
  through the authorized GitHub connector, then reconciled the local branch to it;
  no credentials were extracted. Draft PR: https://github.com/mpuchara/ESP32-P4-Home-Buddy/pull/1.
* Wi-Fi profile built locally (0xf1330 bytes), effective GPIO/profile check passed,
  and all three profiles packaged with public-only outputs and license notices.
* Physical C6 diagnostic: slot1 / 4-bit / 40 MHz SDIO initialized, ESP32-C6 and WLAN
  identified. Version mismatch reported; firmware-version RPC timed out and the
  app reported ESP_FAIL without rebooting the UI during the 30 s capture. Installed
  version remains UNKNOWN (do not interpret the transport's 0.0.0 as a real version).
  No scans/association or C6 writes occurred. Display-only P4 image restored.
* Initial GitHub run compiled all three images and passed host checks, then failed
  in post-build Git repository discovery. Replaced that lock guard with a direct
  before/after byte comparison and explicit workspace paths. Full rerun passed:
  https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39752014808
  (host checks and all three firmware build/configuration/package jobs).
  Verified implementation commit: 1b967073fceb8db5b143348d839a81fac8e5c84d.
* That documentation-only evidence update used [skip ci]; firmware/workflows were
  unchanged at that point. No production release or merge was made.

### Acceptance still open

Visible display, four-corner touch, backlight behavior, cold-boot/repeated-menu
stability remain pending physical observations. C6 transport is confirmed, but
firmware identification/compatibility must be resolved before scan/association.
Wi-Fi provisioning/association/DHCP are not implemented yet. Camera, microphone,
face/speaker inference, profiles/PIN, UBox transport/JPEG display, NTP, temperature,
MQTT, automatic brightness and OTA are not implemented.
72-hour stability: NOT VERIFIED ON HARDWARE. This is not a completed product.

Next gate: visible 800×480 LVGL and peripheral verification, then fullscreen UBox
snapshot transport and rendering. Follow docs/DEVELOPMENT_PLAN.md.

## 2026-10-22: missing text reported during physical display test

The operator observed a dark saver and, after touch, menu rectangles and a slider
without any labels. This confirms a visible display path and touch navigation,
but does not confirm slider brightness or complete four-corner calibration.

Root cause: hb_font_20 used compressed bitmap_format=1, while the resolved LVGL
configuration has LV_USE_FONT_COMPRESSED=0. LVGL therefore returns NULL for glyph
bitmaps even though their descriptors/codepoint maps exist. The earlier static
glyph-coverage test did not exercise decoding and did not detect this mismatch.

Regenerated the same Noto Sans subset with --no-compress (bitmap_format=0), keeping
the current LVGL configuration. Added a host regression using the real pinned
LVGL 8.3.11 font decoder with compression disabled. It rejects the old compressed
font at bitmap != NULL and passes the corrected font for ASCII and all Polish
letters. Added target startup probes for ASCII/Polish glyph descriptors and bitmap
pointers so this failure is reported through serial rather than silently accepted.

Corrected display firmware built (0x8b9e0 bytes), flashed/hash-verified on COM5,
and booted. Target log: "Font bitmaps ready: ASCII and Polish", then LVGL ready.
Host CTest: 2/2 passed; Python checks 3/3, formatting and actionlint passed.
The operator subsequently confirmed that saver text and menu labels are visible.
That follow-up exposed cramped text in the four menu tiles. Increased tile height
from 70 to 112 pixels, provided a 52-pixel title area and a separate bottom-aligned
status, reduced padding to fit the full status and moved the controls below.
The actual font advances give "W przygotowaniu" a 162-pixel width, fitting the
168-pixel content width. The operator then confirmed "Tak, kafle są czytelne":
the corrected saver/menu text and separated tile labels are verified on the
physical device. This does not establish slider operation or four-corner touch.
The font fix passed all jobs in CI run 39763215780. The final layout implementation
18d6dab752873a8236fa3ea2e34f7a9bf591f1c2 passed host tests plus all three firmware
build/configuration/package jobs in CI run 39763775980:
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39763775980.
The subsequent documentation-only receipt uses [skip ci]. The corrected display
profile remains installed on COM5; no C6 changes were needed for these UI fixes.


## Iteration 2: 0.2.0-dev, 2026-10-22

Inspected existing open/unmerged PR #1 and preserved its branch and working BSP,
font and coordinate transform. No change to HA_UBOX_bridge or C6 flash. Pinned
ESP-Hosted documentation distinguishes legacy station RPCs from newer metadata.
A P4-only probe got station start and scan ESP_OK despite the version RPC timeout;
first probe found 2 networks. Installed C6 version remains UNKNOWN.

Implemented PIN/NVS configuration workers, Wi-Fi selection/keyboard/password/DHCP/
reconnect/RSSI, original cached UBox HTTP/ETag/backoff, bounded hardware JPEG to
800x480 cover rendering, independent Warsaw SNTP clock, timestamped MQTT
Celsius input with stale/unavailable rules and an asynchronous test greeting.
Added offline synthetic JPEG and coordinate diagnostics, protected individual
settings and per-service system diagnostics. NVS is not encrypted at rest; secure
production storage remains incomplete. No biometric or simulated HA events.

Local Windows host CTest 3/3 passed (geometry, actual LVGL font decoder, dashboard
transfer/error/ETag/JPEG bounds/cover/time/actual cJSON temperature parser).
Python project checks 3/3 passed, clang-format check and actionlint passed.
Windows skips POSIX TZ transition assertions; Linux CI is required for those.
These tests do not emulate ESP-IDF sockets, hardware JPEG, UI buffer lifetime,
NVS power loss or a real broker/bridge. Those acceptance checks remain open.

Wifi firmware built without application warnings, size 0x17c8c0 bytes at first
iteration-2 flash, within the 6 MiB slot. Esptool 4.12 wrote P4 via COM5 and verified
hashes. Boot log reports LVGL ready, then scan ESP_OK / 8 records; no startup panic.
Internal free/largest/min 328135/245760/328135 and PSRAM free 31244760 at uptime 3 s.
The physical operator has been asked to verify JPEG/greeting/corners/brightness
and configure Wi-Fi on-device. Live DHCP/SNTP, UBox/ETag and MQTT acceptance are
pending. No production release or merge was performed. CI receipt follows when
iteration-2 jobs finish; baseline CI is not evidence for these new changes.


### Operator feedback and navigation/font correction

The operator tested the requested actions and explicitly reported missing keyboard/
text glyphs plus a restart after initial PIN creation. No separate JPEG/corners/
brightness success claim is inferred from that general response. Passive serial
log captured Load access fault followed by reboot; NVS retained the newly saved
PIN. Root cause: deleting the active LVGL page sets act_scr=NULL, then LVGL 8.3.11
screen loading sends its unload event through that old NULL screen. Switch to the
persistent saver before deleting a page. Added a regression using actual LVGL
screen/textarea/keyboard/theme code: 100 PIN/form/return cycles and stable pool
usage after the first cache allocation. It verifies the active-screen invariant.

Expanded the existing uncompressed Noto subset with typographic dashes, bullet,
ellipsis and minus. Use the already-enabled uncompressed Montserrat14 fallback
for LVGL keyboard arrows/backspace/OK/newline. Font tests resolve the actual fallback
font before checking its bitmap, matching LVGL's rendering behavior. CTest now
passes 4/4 locally. This addresses missing glyphs without enabling compression.

The operator explicitly changed the Wi-Fi requirement: direct scan/SSID/password,
no administrator PIN. Implemented a narrowly scoped Wi-Fi-only save preserving
UBox/MQTT settings and PIN. Protected UBox/MQTT remain as specified; Wi-Fi-before-
PIN saves use zero hash/salt and do not create a hidden default PIN on restart.
Wi-Fi dropdown now has a selection prompt, so the first real SSID can be selected,
and refreshes in place after scans. Form arrival resets idle time. Numeric and
UTF-8 byte limits reject truncation. Suppress the pinned upstream rpc_utils SSID
warning. System diagnostics now preserve separate service states/IP/RSSI and
accepted-image age; the synthetic frame has an explicit test marker. Serial capture
sets DTR/RTS before opening so passive reads do not request a reset.


### Physical acceptance and final operator override

The operator confirmed: Test JPEG OK, touch OK, greeting OK, brightness OK; Wi-Fi
connected and obtained an IP, and the clock showed correct local time. A passive
serial sample records JPEG1280x720 decode+cover156ms, internal free316471/largest
237568, PSRAM free28575716/largest28311552, aligned input51743, raw1843200 and
final768000 bytes. Subsequent idle PSRAM was30474708. These are synthetic-test
measurements, not UBox-download peak memory or a leak soak. The navigation
regression also fails against the former unsafe deletion (active-screen assertion)
and passes after restoring the correction.

CI run39770586133 passed host checks and all three firmware build/config/package
jobs at0754f36d7141b7ae66e5d1e349b62575d107b227, including Linux DST boundaries.
The operator then explicitly clarified that UBox/MQTT must have NO administrator
PIN either. This overrides the initial specification's PIN workflow. Removed every
PIN gate/hash/lockout and kept all credential fields masked. Existing NVS settings
stay compatible; the obsolete48-byte hash/salt area is ignored and zeroed on next
save. There is no default or hidden PIN. UI navigation regression remains relevant
to all transient settings screens. Production storage encryption is still open.

Added a20-second total HTTP transfer deadline in addition to8-second individual
calls, and reset the request validator while a synthetic frame is shown so the
next poll restores the real image. Set station-connected false immediately on
credential change rather than waiting for the remote disconnect event.
The final no-PIN build is0x17cbf0 bytes; its fresh CI/flash receipt follows.


##0.2.1-dev: operator-requested Open-Meteo, camera colors and visible token

The operator confirmed a live UBox image but reported severe wrong colors; asked
for Open-Meteo/manual locality instead of MQTT, and a visible long-token field.
Pinned IDF5.5.1 jpeg_decode.c configures RGB565 endian scrambling under rgb_order.
The previous RGB setting emitted big-endian565 while LVGL expects native little
endian. Use BGR for native565 and explicit BT.601. Flashed P4-only and the operator
confirmed corrected colors and visible token. General logs still omit secrets.
A passive log recorded UBox304 accepted=1, preserving the existing decoded frame.

Implemented Pogoda: typed locality -> up to5geocoded country/region matches ->
explicit selection -> persistent independent location record. Open-Meteo current
Celsius over certificate/hostname-verified HTTPS,10-minute polling,1-hour timestamp
freshness, bounded retry/response/depth/deadline and off-LVGL processing. No default
city or invented temperature. Attribution/model-data status appears in settings.
Removed MQTT client/dependency/UI/subscription and legacy payload parser entirely.
Old transport bytes remain opaque layout-compatible padding and are cleared on
save. Compile-time assertions preserve852-byte settings/900-byte old record.

Host CTest5/5passed, including new real-parser geocoding/weather/UTF8/unit/time/
depth cases; project checks3/3passed. Public Warsaw API probe downloaded1087-byte
geocoding and319-byte current responses, and the actual C parsers passed against
them. This public contract probe is not a configured default or hardware reading.
The weather-capable P4 firmware built at0x175130bytes and was flashed/hash-verified.
Device locality search/temperature acceptance was requested and remains pending.
Source cleanup after that flash removes the unused MQTT archive/dependency too.

Earlier no-PIN/static-font implementation210862dc749ce50fb45b45858b5138a7a798b28b
passed all CI jobs in run39771837517. It is not a CI receipt for0.2.1 weather work;
a fresh run is required. PR#1 remains draft/unmerged, C6 unchanged, no release.


### Device weather acceptance and SDIO mitigation

The operator explicitly confirmed locality search, correct selection and a
current temperature shown on the saver. No MQTT setup was needed. Camera colors
and the visible-token field were separately confirmed. Weather functionality is
now device-verified; stale/outage/reconnect/soak cases remain pending.

Later passive logs at40MHz captured sdmmc_send_cmd0x109/INVALID_CRC and a rejected
snapshotHTTP200, leaving the previous image. Selected20MHz in the Wi-Fi profile,
with an effective-config CI assertion; this is an attempted host-side signal/
transport mitigation, not a proven root cause or a C6 firmware update. Fresh
build/flash/boot evidence will follow. Private location and credentials are not
copied to these public logs/docs. No merge or production release.


###20MHz restart verification and JPEG layout regression

The final Wi-Fi build0x1751a0bytes passed effective-config validation and was
flashed/hash-verified on COM5. Esptool HardReset with USB handling captured a full
boot: actual SDIO20MHz, station retries then DHCP, certificate validated, Open-Meteo
HTTP200/complete and accepted=1/same_location=1. Saved weather settings restored
without re-entering locality. A subsequent sample decoded a real640x360 bridge
JPEG in125ms and accepted HTTP200 with expected/received101730 and complete=1.
Internal free273339/largest196608 and PSRAM free27800540/largest27787264 during
that decode; raw471040, aligned input101730, output768000 bytes. This is a short
restart sample, not long-term transport/reconnect/soak proof. Earlier transient
200rejections did not advance ETag or publish partial frames.

Added bounded per-stage HTTP diagnostics(expected/received/completion/decode error)
without URLs/tokens/image contents, and rejected-header dimension/envelope details.
Pinned jpeg_parse_sof_marker.c aligns process_h/process_v to the first component's
sampling factors:8x8 for444,16x8 for422,16x16 for420. The earlier unconditional16
padding could reject a valid non-16-aligned decoded size or use a wrong stride.
Use an actual SOF/MCU layout helper; regressions cover1920x1080 all three variants
and invalid sampling. Host tests5/5still pass. This fixes a verified source-code
layout bug; it does not assert that all earlier network rejections had that cause.

CI run39804200456 passed all jobs(host plus diagnostics/bringup/wifi) at17f7206.
The following layout/diagnostic change requires its own fresh CI receipt.


### Original-resolution image bound

A later short sample established a separate precise cause for repeated decode
rejection: the original cached bridge image changed from640x360 to2304x1296.
All679649bytes were received with HTTP200/complete=1 and a valid JPEG envelope;
the old per-dimension2048guard rejected it before hardware decode. This roughly
3MP source is within the existing4MP decoded-pixel budget. Increase only the
per-dimension bound to4096, retaining the2MiB JPEG and4MP pixel limits. Add a
2304x1296 layout regression and a4096x2048 rejection proving the pixel budget is
still enforced. Host tests5/5passed, firmware built0x1754d0bytes. Device acceptance
and final CI for this bound adjustment follow separately. No low-resolution UBox
endpoint or capture call was introduced.


## Final iteration-2 receipt:0.2.1-dev

Implementation dd3d4443d3f3df3f7a5b1b09172ad261f976ccde passed every job in
CI run39807653198: host tests/checks and diagnostics, bringup and wifi firmware
build/configuration/package jobs. This includes five CTest suites, three Python
project checks, formatting, whitespace, dependency lockfile and effective SDIO
20MHz assertions; Linux executes the Warsaw DST boundaries. The local five CTest
suites also passed with final sampling/full-source dimension regressions.
https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39807653198

Final Wi-Fi firmware0x1754d0bytes was flashed/hash-verified on COM5. It booted at
actual SDIO20MHz, retried station association and obtained DHCP, restored saved
locality, validated Open-Meteo TLS and accepted HTTP200/current data. It accepted
complete original640x360 UBox frames(expected/received104438 then later103160bytes),
with approximately126ms decode+cover and repeated conditional304 accepted. Idle
PSRAM samples remained30474708bytes between these operations. This is a short
sample, not a leak/fragmentation/stability soak. No CRC was logged in that final
short sample; absence there does not prove the mitigation eliminates the issue.

The2304x1296 source caused the earlier reject and is now covered by the expanded
side bound, unchanged4MP/2MiB budgets and host layout regressions. The final short
post-fix hardware sample happened to contain640x360 frames; a fresh2304x1296
hardware frame remains a separate acceptance item. Polling still uses only the
original cached snapshot route and never initiates an LTE capture to force a test.

Public-only local package:dist/iteration2-final, with SHA256SUMS; app SHA256
9e75f39c429fc24531fb9eae3e71aa4d36f28f4b9b8666d187995eed226ac002.
It contains app/boot/partition/OTA-initial/debug/license outputs, not NVS, private
backups, user photos or credentials. A documentation-only receipt follows this
verified implementation with[skip ci]. PR#1 stays draft and unmerged. No release,
C6 write, HA_UBOX_bridge modification, biometric inference or active MQTT.

Next acceptance work: a naturally available full-resolution frame, sustained
connection/error recovery and stale-weather tests, repeated replacement heap/
fragmentation checks, cold power-on and the planned72-hour physical soak. Secure
production storage/provisioning remains a later task; do not infer it from masked
Wi-Fi fields or direct setup. The operator-requested UBox token edit is visible.

## Iteration 3 in progress, 2026-10-22

Baseline/PR inspected: clean06f8a11, PR1 open/draft/unmerged. Original private P4
backup SHA256 rechecked unchanged. No C6 write or bridge modification.
Read-only SCCB identified0x5602 at0x36. GUITION schematic/source/per-file Apache
license audit recorded in camera/vendor/SOURCE.md; no ESPHome runtime imported.
Native CSI one400Mbps lane /1288x728 RAW10 -> ISP RGB565 measured29.9..30.1fps.
Three leased slots + discard coexist with retained UBox, weather TLS and DHCP.
Host six tests and Python three checks passed at the first UI implementation.
Operator reports green/pale and~1fps preview, so image quality is explicitly NOT
accepted. Correcting pinned-driver cache length, preview pace and preview-only
white balance/display gamma. Brightness controls and schema1 separate persistence
implemented; actual day/night/override/reboot acceptance pending. See LOCAL_CAMERA
and BRIGHTNESS for algorithms, relative-metric limitations and face-model research.

## Iteration 3 RAW8 and upright preview receipt, 2026-10-22

The initial RGB565 image path was rejected by the operator. Native RAW10->RAW8
GBRG, black-target subtraction and thumbnail-only white balance/gamma were then
operator-confirmed for usable colors and detail. Head initially pointed right;
CCW90 portrait preview now has operator confirmation of upright orientation with
the device in landscape. Synthetic four-quadrant regression verifies the mapping
and unchanged source Bayer storage. Frame API declares rotation_ccw=90 for future
AI preprocessing; no actual inference or recognition accuracy is claimed.

Pinned CSI source has two build-local DMA/cache corrections with exact source
hash admission. Native RAW10->RAW8 requires output-bit transfer count to avoid an
overrun. ISP constructor/uncertain cleanup failures quarantine bounded allocations
until reboot rather than repeatedly claiming/leaking driver resources.

Final local host checks: six CTest suites, three project checks, formatting and
whitespace passed. Raw linear light uses unquantized channels and rejects clipping
or near-total black; black-level-only data remains invalid. The connected short
sample still reported luma0/invalid at nominal32ms and8x gain. Thus actual local
illumination measurement, day/night calibration and AUTO behavior are NOT accepted
from the preview confirmation. Manual operation remains available; no false anchor
is written. Real calibration, NVS reboot restore, five-minute temporary expiry,
changing-light stability and fault injection remain on-device acceptance work.

Upright firmware built0x17db60bytes and was written/hash-verified on COM5. Capture
measured about30fps. At158..178seconds PSRAM stayed26,723,652bytes, largest22,544,384;
internal DMA265,431 then265,219bytes. These are short samples, not a leak/72-hour
soak. Earlier RGB preview measured7fps; the final portrait rate still needs a fresh
active-preview sample and is not equated to30fps capture. Boot temporarily logged
network host-unreachable; no uninterrupted connectivity claim is made.

No C6 firmware write, bridge modification, forced LTE snapshot, model download,
biometric enrollment, production release or PR merge occurred. Naturally available
2304x1296 UBox acceptance, cold-power boot and planned72-hour soak remain pending.
