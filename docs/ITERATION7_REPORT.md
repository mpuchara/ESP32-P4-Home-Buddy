# 0.6.1-dev single-image / HA HTTP continuation

This report supersedes iteration 6 deployment decisions. Builds/tests below are
not evidence of physical recognition accuracy or a working HA automation.

1. **Source revision:** firmware implementation d938878958886ab78ecb24784a1cbf270450d75f.
   Use `git rev-parse HEAD` for the latest receipt/tooling commit. PR #2 stays
   draft on codex/local-face-recognition, based on codex/initial-hardware-foundation.
2. **Implemented:** preserves onboard ESPDet416/MNP/alignment/MBF, enrollment,
   transactions and bounded event bus; replaces MQTT with separate Webhook/REST
   HTTP task and masked touchscreen settings. Preserves fullscreen UBox, clock,
   weather, brightness and independent greeting. No OTA, SD, external inference,
   voice implementation, broker or HA custom integration.
3. **Flash map:** table 0x10000; NVS 0x11000/24 KiB; PHY 0x19000/4 KiB;
   encrypted NVS-key reserve 0x1a000/4 KiB; factory 0x20000/7 MiB;
   MBF 0x720000/4 MiB; encrypted-flag profiles 0xb20000/1 MiB;
   future sv_model_3s 0xc20000/3 MiB; reserve 0xf20000/896 KiB.
   Exact ranges and gaps: SINGLE_IMAGE_FLASH_LAYOUT.md. No OTA metadata.
4. **Firmware:** locally built recognition image 4,382,064 bytes, SHA256
   4704c52a0f255a11d39eafda5f60aeb7b3b5edacdb04e1cb6374daf18a0d78e8.
   App growth headroom 2,957,968 bytes, above enforced 256 KiB floor.
5. **MBF:** raw 3,522,832 bytes; locked PDL3 package 3,522,944 bytes, SHA256
   f8aa1d21c47814bf324b78ec3a050f993dfa38b1f6a582b6afb5ce0264daf294.
   Standard Flash-partition loader follows pinned-image/hash validation.
6. **Voice:** 3,145,728-byte partition. Actual upstream 3s asset 2,844,528 bytes;
   locked packer output 2,844,624, leaving 301,104 bytes. One asset fits, both
   3s/6s do not. Runtime/operator compatibility is unverified. Future integration
   must explicitly select 3s and replace upstream dual-model flash defaults.
7. **Free space:** app 2,957,968; MBF 671,360; separately reserved 917,504 bytes.
   Future voice region remains unprovisioned. Profile first 512 KiB holds two
   256 KiB face banks, each with 246,640-byte snapshot +128-byte header and
   15,376-byte headroom; second 512 KiB is future voice storage, no format yet.
   Reserved partitions are not interchangeable general free heap/filesystem.
8. **USB:** USB_FLASHING.md gives build/package, application-only/model/full
   commands and ROM recovery. Package validates hashes and image bounds; normal
   updates check installed table and exclude models/profiles/settings. Full mode
   requires new private full backup, security query and explicit layout flag.
9. **Webhook:** HOME_ASSISTANT_HTTP.md gives graphical automation setup, local
   POST-only trigger, secret entry on Home Buddy, diagnostic branch and actual
   recognition test. GET reachability and POST 2xx do not establish HA action success.
10. **REST:** same document covers separate HA Long-Lived Access Token, Bearer
    header, GET /api/ test, actual POST /api/events/home_buddy_identity and event
    automation trigger. UBox credentials are never reused.
11. **Helper:** graphical Text helper and recognized-only input_text.set_value
    action. This is not an auto-registered device sensor. Optional inbound status
    endpoint evaluated and deferred; /api/status is not implemented.
12. **Automated validation:** local recognition firmware builds; effective config,
    budget enforcement and three USB dry-run modes pass. Host C suite 15/15;
    project checks 3/3; Python model/USB tests 5/5; ASan/UBSan storage/event and
    HTTP policy/payload executables pass. Fault injection preserves
    transaction recovery and event overflow/session semantics. HTTP tests cover
    payload escaping, URLs, credentials, status classification, cooldown and
    freshness; they do not substitute for real ESP TLS/network fault injection.
13. **CI:** see PR checks for this implementation revision. Historical iteration-6
    success is not counted as current CI. Current run result will be recorded in
    the physical/CI receipt below.
14. **Physical:** migration and USB/boot/MBF/HA results are recorded below when
    actually observed. Operator must enter the webhook on the screen and perform
    real enrollment/recognition. A connection_test is not biometric acceptance.
15. **Security/migration:** necessary layout migration authorized by owner; no
    eFuses, secure boot provisioning, C6 flash or production release authorized.
    Unsecured hardware remains explicitly RAM-only. Secure persistent activation,
    signed USB recovery, encrypted credentials, real power-loss/deletion tests,
    multi-person open-set accuracy, long stress/leak/watchdog and harmless real HA
    recognition automation acceptance remain separate requirements.

## Physical and CI receipt

Later HA form readability fix: explicit dark panel/contrast, shorter labels,
readable checkbox/transport text and bounded status wrapping. Verified using
before/after 800x480 LVGL renders of the actual form construction. This updated
recognition app is 4,382,192 bytes, SHA256
8b45c8004f67c56600cda54963fddf2ee7d1b2c25bc03359f26730053b66f2c5;
app headroom 2,957,840 bytes. Earlier image sizes above remain baseline receipts.

2026-10-23: [implementation CI run](https://github.com/mpuchara/ESP32-P4-Home-Buddy/actions/runs/39988036946)
passed host and all four firmware profiles on d938878. Subsequent tooling/document
changes retain the same firmware source; their latest checks remain visible in PR #2.

P4 COM5 full migration completed with exit 0. Before any write, a fresh private
16,777,216-byte backup was read and SHA256-verified:
f1fbe5ecf3c24ec4a9941592bd3e18dae101ea4200a03aeefe5ee2f64eaf04bf.
It matches the original snapshot. Current ROM query confirmed Secure Boot and
Flash encryption disabled. NVS/PHY were relocated byte-for-byte from that copy.
Bootloader/table/application/MBF/settings writes all passed verify_flash. No chip
erase, profile/voice writes, eFuses or C6 flash. Backup/credentials remain private.

The migrated application boots and captures OV02C10 frames at approximately
30 fps. Stored Wi-Fi reconnected and obtained DHCP; UBox reports accepted=1,
HTTP 304 and successful decode after initial network recovery. Short serial
observation to about 108 seconds uptime showed no panic/watchdog or reboot;
PSRAM settled near 26.3 MB free. This is not a soak/leak test. Visual touchscreen,
brightness/weather regression, MBF model loading/inference, real enrollment,
multi-person accuracy, application-only physical upgrade and actual recognized
HA automation remain unaccepted until directly observed. Webhook is entered by
the operator on the touchscreen; no secrets are requested in chat.
