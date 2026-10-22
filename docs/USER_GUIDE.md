# Development firmware 0.2.0-dev user guide

Use the wifi build on the verified GUITION profile. Tap the saver to open settings;
use Return to close. After 30 idle seconds it returns automatically. A pending
PIN hash/save cannot time out halfway through. Touch diagnostics stay open until
Return so corner/drag testing is possible. Brightness PWM has a 26/255 floor and
is temporary; reboot restores the default.

1. Test JPEG shows a synthetic 1280x720 reference through the real hardware JPEG
   decoder, cropped to fullscreen 800x480. It is explicitly marked TEST JPEG.
2. Test greeting displays Test UI for about 4.8 seconds including fades. It is a
   separate overlay, not an identity detection or modified JPEG.
3. Test touch: check all four targets and center, then drag and watch coordinates.
4. Open Wi-Fi directly, without an administrator PIN (operator-requested flow).
   Tap fields to show the keyboard. Select an SSID from the scan or type it,
   enter its password, then Save. Use Scan on the same screen to refresh the list.
5. Check connection/IP/RSSI in Wi-Fi or System. Disconnect is deliberate until
   credentials change or restart. It does not delete stored credentials.
6. Open UBox, create/repeat your own PIN on first protected use or unlock with
   the existing PIN, then enter HA host/IP (without URL/path), bridge port,
   token and poll seconds. Use the original cached snapshot bridge, not a capture
   automation. The previous good image remains during an outage.
7. Open MQTT, unlock, enter mqtt://host:1883 or mqtts://host:8883, separate username/
   password, exact temperature topic and maximum age. TLS requires a trusted CA
   from the IDF bundle; no insecure-certificate bypass is provided.
8. Internet SNTP must succeed before a clock or available temperature is shown.
   Time follows Polish CET/CEST rules. Missing/stale data shows unavailable.

Only UBox/MQTT are protected; Wi-Fi requires only the network password.
Each protected edit requires PIN again; unlocking lasts 120 seconds and a save
consumes the session. After five wrong PINs wait one minute (RAM-only limit).
Do not send PINs, Wi-Fi passwords, tokens or broker passwords in issues/logs/chat.
Only the touchscreen fields receive them; general diagnostics omit them.
Development NVS is NOT encrypted at rest. A flash backup can contain all secrets.
No factory reset or PIN recovery UI exists yet; preserve NVS before any recovery.

Physical visual/JPEG, corners, brightness, connection/reconnection and live HA
acceptance require operator confirmation. Compilation/serial init alone does not
prove them. No enrollment, camera/audio capture, face/speaker recognition,
discovery, OTA or access-control workflow is available in this iteration.
