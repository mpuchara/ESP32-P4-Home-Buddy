# Development firmware 0.3.0-dev user guide

Tap the saver to open settings. All pages open directly, without administrator
PIN, as requested. Thirty idle seconds return to the saver; pending saves are
exempt. Touch diagnostics remain open until Return. Brightness has a 26/255 floor
and has a separate persistent configuration; see BRIGHTNESS.md.

1. Wi-Fi: Scan, choose SSID, enter the network password and Save. The network
   password stays masked. Wi-Fi obtains DHCP IP; System shows IP/RSSI.
2. Kamera UBox: enter HA host/IP without URL/path, port (usually8100), token and
   poll seconds. The token is intentionally VISIBLE in this field to make long
   input checkable. It is not shown in System or serial logs. Save starts original
   cached-image polling. No new LTE capture is requested.
3. Pogoda: enter a locality name, tap Szukaj, choose the correct country/region
   result from the dropdown, then Zapisz. ASCII spelling is accepted by geocoding;
   you need not type Polish accents. No default location is assumed. Empty the
   city field and Save to disable weather. No MQTT/broker setup exists.
4. The device requests Open-Meteo current temperature at2m every10minutes over
   verified HTTPS. These are weather-model data, not a local sensor measurement.
   Data older than1hour become unavailable. Source attribution appears in Pogoda.
5. Internet SNTP establishes time before HTTPS weather searches/data display.
   The clock follows Warsaw CET/CEST. Missing/stale time/data are never fabricated.
6. Test JPEG is an explicit synthetic reference, Test greeting displays Test UI,
   and Test touch checks corner/center/drag coordinates. System reports per-service
   state, heap/largest blocks and time since an image was accepted, not capture time.

Wi-Fi, UBox image, corrected image colors, touch, greeting, brightness and local
clock were physically confirmed by the operator. Weather search/selection/temperature display was also confirmed. Reconnect/cold-boot/fault/soak acceptance is separate.
NVS is not encrypted at rest; private flash backups may contain credentials. No
factory-reset, enrollment, local capture, biometric inference or OTA is exposed.

7. Kamera lokalna: turn preview on/off explicitly. It uses only the onboard camera;
   The portrait thumbnail rotates the fixed module mounting90degrees left. Test
   kolorów shows a synthetic sensor pattern and suspends light measurement.
   Wygaszacz UBox returns to the saved remote image. Capture FPS and visible preview
   FPS are different. No camera images are saved or uploaded.
8. Wyświetlacz: AUTO/MANUAL/TYMCZASOWO, day/night calibration, fallback and response
   settings. [Detailed workflow](BRIGHTNESS.md). Missing calibration uses fallback;
   missing camera data after calibration holds the last safe level.
