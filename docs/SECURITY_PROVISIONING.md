# Secure profiles and USB recovery, 2026-10-24

Engineering plan only. Installed P4 revision 1.0 remains unprovisioned, with
RAM-only enrollment. No keys, security bootloader or eFuses are created here.

The audited two-bank, header-last transaction store retains integrity, namespace,
rollback and deletion scrub. The adapter requires explicit persistence, runtime
Secure Boot and Flash encryption, encrypted profiles subtype 0x41 and now exact
1 MiB capacity. There is no plaintext fallback or automatic format/rebuild.
The current USB helper rejects secured devices/packages; it is not a secure updater.

## Recovery decisions

Authoritative version-specific references:
[Flash Encryption](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32p4/security/flash-encryption.html),
[Secure Boot v2](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32p4/security/secure-boot-v2.html).
Generic P4 support does not establish revision-1.0 provisioning acceptance.

The current [P4 errata table](https://docs.espressif.com/projects/esp-chip-errata/en/latest/esp32p4/02-errata-summary/index.html)
lists ECDSA_DS-837 for v1.0 (also APM-560, RMT-176 and I2C-308).
ROM-764/770/816 describe other revisions and must not be attributed to this unit.
[Espressif advisory AR2026-006](https://documentation.espressif.com/AR2026-006_Security_Advisory_Concerning_ECDSA_Secure_Boot_Issue_in_ESP32-H2_ESP32-C5_ESP32-C61_ESP32-P4_ESP32-S31_EN.html)
recommends RSA instead of affected ROM ECDSA verification. The proposed new-unit
policy is therefore RSA-3072 Secure Boot plus Release XTS encryption, retaining a
proved secure USB service route and private per-device encryption key. This is a
design decision, not an instruction to provision the owner's unit. Confirm it on
a matching spare; do not force insecure ECDSA options in IDF 5.5.1.

| Decision | Consequence |
|---|---|
| Secure Boot | Updated bootloader and factory app need an authorized signature. An invalid sole app cannot boot; there is no spare OTA slot. |
| Release encryption | ROM cannot encrypt plaintext updates. USB-only servicing requires retained private per-device encryption keys and signed images encrypted at exact offsets, with a permitted ROM write path. |
| ROM USB OTG/DFU | Security enablement disables this ROM stack. Do not equate OTG with USB Serial/JTAG or a USB-UART bridge. |
| Current COM5 | Native Serial/JTAG recovery and its eFuse policy need proof on matching silicon/configuration. Its current availability is not a post-security guarantee. |
| Alternative USB-UART | Prove UART0, BOOT/reset wiring and secure download on a matching spare board first. A second USB connector is not proof of a UART bridge. |
| Lost signing key | Existing firmware may boot, but replacements need another retained, enrolled and unrevoked key. |
| Lost encryption key | Device-generated unreadable keys cannot be retrieved for host pre-encryption. Retained ciphertext is device/offset-specific and still needs an available write path. |
| Invalid image | Recover through the already-proved ROM service path with a known-good signed, correctly encrypted app. Download disabled plus no bootable app can mean lost recovery. |
| Full backup | A pre-security plaintext dump cannot undo eFuses. Secure download limits readback. Retain private logical images, ciphertext recovery packages, layout, keys and manifests offline. |

Irreversible policy covers key blocks/purposes/read/write protection, Secure Boot
enable/digests/revocations, SPI_BOOT_CRYPT_CNT, DIS_DOWNLOAD_MANUAL_ENCRYPT,
cache/JTAG/download restrictions and ROM-download policy. Review the exact generated
write-set against the IDF 5.5.1 P4 eFuse table and actual unit summary; other ESP
families' commands are not a recipe. First boot of a security-enabled bootloader
can itself provision eFuses, so flashing that bootloader is not a reversible test.

## Approval sequence

1. Preserve installed RAM enrollment; schedule its loss/re-enrollment explicitly.
   Secure an audited private backup without publishing personal data.
2. Read revision/security/non-secret eFuse summary. Review exact revision errata,
   signing scheme and ROM transport. Stop if USB recovery cannot be demonstrated.
3. Use a matching spare board. Establish unique private signing/encryption keys,
   two verified offline backups, fingerprints and custody. Keep an unrevoked recovery
   signing key. Never put keys or biometric exports in Git/CI.
4. Build offline with the single factory layout. Verify signed-image padding,
   bootloader size versus table 0x10000, exact encrypted ranges and mmap/model
   hashing. App encryption is automatic; profiles follows its encrypted flag;
   generic MBF retains its existing flag policy. Do not move partitions or write SV.
5. Implement a separate signed/encrypted USB package validator. Preserve the current
   helper's refusal. On the spare unit prove valid update, intentionally invalid-app
   recovery, interrupted update and preservation of settings/models/profiles.
6. Present exact irreversible operations, key custody and recovery evidence for
   explicit owner approval before provisioning the actual unit.
7. After security verification and enrollment consent: enroll/recognize/reboot/
   restore/recognize, rename/reboot, append/reboot, delete/scrub, and application
   update with stable ID checks. Test interrupted transactions with synthetic data
   first. Never erase active RAM profiles merely for testing.

## Reversible alternatives

| Mode | Protection and decision |
|---|---|
| Current RAM-only | No durable embeddings; unchanged default. Reboot loses profiles. |
| Plaintext Flash | Physically readable biometrics. Not enabled; explicit owner acceptance required before real embeddings. |
| Software AEAD, key in same Flash | Extractable key/replaceable firmware defeats physical protection. Not equivalent to hardware security; not enabled. |
| Software AEAD, owner-supplied key | Requires reviewed entry/unlock/recovery and consent, without a co-stored automatic-unlock secret. Not implemented. |
| Hardware encryption plus authenticated firmware | Device-bound protection with irreversible provisioning and reduced recovery choices; gated by the sequence above. |

No weaker mode bypasses the gate. Synthetic transaction tests remain available.
