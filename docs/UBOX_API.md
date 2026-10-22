# UBox snapshot contract

Read-only assessment of mpuchara/HA_UBOX_bridge at commit
7f1f1656046b7200eab05dacdecbd2107af6a704, ubox_bridge/bridge.py,
blob 36992a672b2ccd167ac995c2404d87372a7f16da.
The bridge repository was not modified.

The existing LAN interface uses port 8100 by default. GET /snapshot.jpg requires
X-UBox-Token and returns the original cached JPEG. The handler reads the existing
file; this route does not request a new LTE capture. Do not call capture, live or
PTZ actions as part of polling.

ETag is a quoted mtime_ns/size value. Send If-None-Match after accepting an image.
Matching ETag returns 304 with no JPEG body. A missing/empty cached image returns
503. Authentication failure is an error; do not retry in a tight loop.
Do not use snapshot-cyd variants, URL query tokens or redirect credentials to
another host.

## Planned client (not implemented)

Default polling 30 seconds, configurable with bounds, timeouts and backoff.
Accept bounded Content-Length or bounded streaming; reject oversized, partial
or invalid JPEGs. Stage the new image without clearing the old one.
Decode, preserve aspect ratio and center-crop using hb_cover before atomically
publishing to LVGL. Commit ETag only after successful decoding/acceptance.
On 304 keep image, ETag and original image-age timestamp. A successful poll is
not the image capture time; use documented metadata when available.
Keep the last good image on HTTP, Wi-Fi or decode failure.
