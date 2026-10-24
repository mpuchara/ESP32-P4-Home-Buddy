# Iteration 8: 0.7.0-dev candidate

Continues 6dab65cf5798d216d8f783ae8e749825d91cbffd on
codex/local-face-recognition, draft PR #2 based on PR #1. Baseline CI
[40118442005](https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/40118442005)
completed successfully. Installed source remains 00272f7; existing RAM enrollment
and physical recognition/HA/encounter receipts are preserved. This candidate was
not flashed. No merge or production release.

## Implemented

- Separate AUTO mode, explicit NVS enablement with commit/readback checks,
  unchanged ten-minute TEST and bounded command queue. Startup needs usable
  templates; RAM-only reboot waits instead of claiming restored enrollment.
- Passive recognition settings, mode/profile/MBF/last-recognition/error status.
  AUTO is independent of ordinary navigation. Models remain resident with existing
  admission, compute mutex, frame ownership, watchdog and cancellation guards.
- Stop bypasses a full queue. Rejected admission does not change the active epoch;
  accepted commands still invalidate in-flight results. Post-inference deadline
  checks and event-consumer deadline checks suppress expired TEST results.
- Local explicit evaluation requests three new observations per trial, preserves
  expected identity/guest, separates ambiguity/unavailability from correct guest
  rejection and retains correct known/unknown denominators. Counts and latency
  accumulators remain RAM-only, with no image/embedding export.
- Existing FACE Webhook/REST transport, encounter deduplication, independent
  cooldowns and stale-event dropping retained. No MQTT or durable replay queue.
- Manual 60-second ES8311 ADC diagnostic with chip-ID admission, shared I2C0,
  bounded I2S DMA/PCM, RMS/peak/clipping/errors/overflow and cleanup. Audio is not
  enabled on boot or page open. Driver dependency pinned to esp_codec_dev 1.6.2;
  prior dependency versions unchanged. Physical capture remains unverified.
- Secure profile adapter adds exact 1 MiB capacity validation; two-bank storage,
  schema, persistent IDs and strict security gate retained. No weaker persistence.
- Health logs add model cycles, worker stack, admission rejections and event
  overflow/staleness. Existing main/camera diagnostics retain memory/frame metrics.
  Fusion contracts add explicit CONFLICT; no executable voice/fusion provider.

No threshold relaxation, adaptive motion shortcut, partition change, model write,
eFuse provisioning, C6 change, HA bridge edit, audio upload or personal data commit.
Conservative existing inference cadence is retained pending actual resource data.

## Validation

Local host suite: 20/20, including AUTO configuration default/reboot/commit/readback
failure, navigation/lifecycle helpers, PCM bounds/format/DC/clipping, existing
identity/transaction/event/HA/camera/UI-font tests. Python suite 7/7; project checks
3/3; formatting and whitespace pass. These are software checks, not FreeRTOS
interleaving, physical microphone or quantified recognition acceptance.

Local recognition firmware builds with IDF 5.5.1. Final binary/package, effective
configuration, enforced Flash budget and candidate CI receipts follow below.
All four clean firmware profiles are required in CI; baseline CI is not reused as
candidate evidence. The signed/encrypted provisioning path is a reviewed plan,
not implemented or accepted deployment tooling.

## Outstanding physical gates

Follow [ITERATION8_ACCEPTANCE.md](ITERATION8_ACCEPTANCE.md): immediate/1s/20s
navigation, repeated menus, Stop during inference, queue stress, exact TEST expiry,
AUTO beyond ten minutes, RAM-only reboot and model release. USB install requires
scheduled loss of current RAM enrollment; no automatic reset is performed.

Consenting multi-person/unknown/similar-person and lighting/distance/occlusion
trials, real outage/reconnect tests, microphone signal/clock/cleanup/coexistence,
memory/CPU profiling and uninterrupted 72 h soak remain unperformed. No accuracy
percentage, new physical latency/heap figure or speaker identity claim.

[SECURITY_PROVISIONING.md](SECURITY_PROVISIONING.md) records P4 v1.0 ECDSA errata,
RSA proposal, irreversible steps, key custody and USB recovery gates. Real persistent
activation requires separate explicit approval. [AUDIO_DIAGNOSTICS.md](AUDIO_DIAGNOSTICS.md)
and [SPEAKER_FEASIBILITY.md](SPEAKER_FEASIBILITY.md) separate schematic/source
feasibility from measured hardware behavior; reserved SV partition stays unwritten.

## Local package receipt

Recognition app: 4,457,648 bytes; SHA256
bb3bc0897240de020c6d2df3144e72b2d317fada540bbc0541086ce5f23e0916.
Factory growth margin: 2,882,384 bytes, above the 262,144-byte enforced floor.
Effective recognition configuration confirms persistence, Secure Boot and Flash
encryption disabled, ES8311 support available, existing thresholds and Flash map.
Package: dist/0.7.0-dev-iteration8-final with hashes and codec license. Application-only
USB dry run selects only 0x20000/home_buddy.bin and opens no serial port.
Candidate CI is pending at this implementation commit; record actual results below.

Final timing bookkeeping excludes unavailable measurements rather than inserting zero-latency samples. Host inspection confirmed SV input int16 [1,300,80], output int8 [1,512] and a fitting locked package, without a device write.
