# OV02C10 register sequence provenance

Selected source: public mirror of GUITION JC4880P443C SDK,
[wegi1/ESP32P4-JC4880P443C-I-W](https://github.com/wegi1/ESP32P4-JC4880P443C-I-W/tree/534b6cffba51c87ff28076513b53bb389c22bfbe),
commit `534b6cffba51c87ff28076513b53bb389c22bfbe`.

Original file:
`1-Demo/idf_examples/ESP-IDF/esp_brookesia_phone/components/espressif__esp_cam_sensor/sensors/ov02c10/private_include/ov02c10_settings.h`.
The file declares copyright 2024 Espressif Systems and Apache-2.0. The component's
Apache-2.0 LICENSE is retained alongside this receipt. Only the named
`ov02c10_input_24M_MIPI_1lane_raw10_1288x728_30fps` register sequence is retained;
the sequence entries are unchanged, with a local type/name wrapper. Other modes
and the large original sensor driver are not imported.

Audit alternatives inspected on 2026-10-22:

* `sullb/esphome-p4-csi-camera` at `461aa9b178be63b40e1bb7c1f967c971640be600`:
  root MIT, sensor files Apache-2.0 headers, README explicitly references Linux
  GPL-2.0 register sequences during early development. Its tables, driver and
  ESPHome runtime are **not** imported under an assumed MIT relicensing.
* `profi-max/JC8012P4A1_BSP_ESP32P4` at
  `864d486942f45dc5c4858dcdb4acd5d36cdb49b1`: related-board Apache-labelled sensor
  files, useful comparison, not our source of redistributed entries.
* `espressif/esp-video-components` master at
  `3ae2756e21c5f85ed8d9809100ab02d079347a79`: no merged OV02C10 sensor. Open
  [PR90](https://github.com/espressif/esp-video-components/pull/90), head
  `1b6029f8faab1b8da468408b0ce310c07658eb92`, independently points to the GUITION
  SDK as its source; an open PR is not upstream acceptance or hardware proof.

Home Buddy's SCCB transport, capture owner, pool, light estimator and exposure
controller are original Apache-2.0 code using pinned IDF-native drivers. The SDK's
publicly mirrored OV02C10 datasheet was consulted for register semantics; the
proprietary PDF is not redistributed. Linux GPL driver code/tables are not
redistributed. These are recorded source declarations and scope of reuse, not
a claim that a mirror proves every historical upstream contributor's provenance.
