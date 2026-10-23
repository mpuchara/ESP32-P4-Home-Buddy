# Brightness configuration

Open **Wyświetlacz** (page title **Jasność**) without PIN.

* MANUAL: move the main slider; PWM follows within the 100ms control interval.
* AUTO: use two calibrated local-light anchors. Missing/too-close anchors use
  configurable fallback. Once calibrated, missing camera readings hold last safe
  output. No Internet, UBox or Home Assistant dependency.
* TYMCZASOWO: use the slider for five minutes, then return to AUTO. Reboot never
  extends a persisted temporary override; persisted mode becomes AUTO.

Calibration: under normal daytime lighting, press **Kalibruj dzień**, adjust the
slider to a comfortable level and **Zapisz**. The button snapshots a fresh valid
local-camera metric; it refuses unavailable/stale/saturated measurements. Repeat
**Kalibruj noc** under normal nighttime lighting. Select AUTO and **Zapisz** when
both anchors are ready. Saving confirms after the off-LVGL NVS commit succeeds.
Anchors closer than 0.5 relative stops disable interpolation; do not create fake
day/night points under the same lighting just to enable AUTO.

The lower slider sets fallback PWM; controls select measurement interval1/5/10/
30/60s and response20..60s. Main slider switches AUTO to MANUAL; moving it in
temporary mode preserves the active override. Configuration changes are live in
RAM, and become persistent only on explicit **Zapisz**.

Separate `hb_display/brightness` blob, schema1, exact length and value validation.
Wi-Fi/UBox main schema1 and weather blob remain untouched. Invalid/corrupt records
are preserved and start with manual128. No automatic NVS erasure. PWM remains on
existing GPIO23, 5kHz, range26..255. It never requests an off value.

Auto interpolates either anchor direction, clamps endpoints, low-pass filters,
requires three consistent samples for changes >2 stops, uses a3-count hysteresis
and limits full-range travel to configured20..60s. Rejected/stale samples hold the
last output. No per-frame/per-update writes to flash. Partial occlusion and scene
changes cannot always be distinguished from room light; placement and real
day/night calibration remain necessary. This is not calibrated photometry.

Host checks cover interpolation/reversed direction, missing/equal/invalid anchors,
freshness, limits, rate, outliers, manual and override expiry, invalid schema;
synthetic meter exposure compensation and pool stress are separate from physical
color/brightness acceptance. Operator confirmed a valid metric responding to
increased camera illumination. Real calibration, reboot persistence, changing room
light and five-minute on-device expiry are pending operator acceptance.

## Override expiry and save admission (0.3.1-dev)

Only an explicit press of TYMCZASOWO starts/restarts its five-minute deadline.
Slider/options edits and saving never extend it. After expiry, the open form
follows the live AUTO mode; a later slider movement selects MANUAL. A stale
form/save snapshot containing OVERRIDE is normalized to AUTO by the service if
there is no active interval. Stored OVERRIDE records also load as AUTO.
Brightness controls ignore edits while a save is in flight so the persisted
snapshot cannot silently replace a newer live edit.

A host integration test compiles the real brightness service with deterministic
clock, task-yield and NVS boundaries. It covers restart loading, missing/corrupt/
short/oversized records without erasure, failed open/set/commit without live RAM
admission, no flash writes during control ticks, expiry and stale edits/saves.
This emulates storage/task boundaries; it does not prove real NVS rollback on
commit error, physical flash durability, concurrency or an actual board reboot.
After any reported save failure the user must retry intentionally and verify.
Operator confirmed physical return to AUTO after five minutes on2026-10-23.
Post-expiry option edits, real two-anchor calibration and physical restart
persistence remain pending.
