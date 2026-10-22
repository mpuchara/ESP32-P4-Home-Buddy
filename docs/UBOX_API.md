# UBox original cached snapshot client

Read-only contract assessed at mpuchara/HA_UBOX_bridge commit
7f1f1656046b7200eab05dacdecbd2107af6a704, ubox_bridge/bridge.py,
blob 36992a672b2ccd167ac995c2404d87372a7f16da. That repository was not modified.

Configure host/IP, port (default 8100), token and poll interval (default 30 seconds,
allowed 5..3600) in UBox settings. The client issues only:

```http
GET /snapshot.jpg HTTP/1.1
X-UBox-Token: <device-configured secret>
If-None-Match: <last accepted ETag, if present>
```

It reads the bridge's existing original cached JPEG, not the older 320x240 route.
It never requests a new LTE image or calls capture/live/PTZ. Token is a header,
never a query parameter. Redirects are disabled, so credentials do not follow them.
Current contract is LAN HTTP, not end-to-end encrypted; use only a trusted LAN.
There is no administrator PIN; development NVS is not encrypted at rest.

| Response | Implemented result |
|---|---|
| 200 | Bounded transfer, JPEG checks, hardware decode and cover, queued replacement |
| 304 | Retain accepted image and ETag; no body decode or accepted-image-age reset |
| 401 | Report token error; keep image; bounded backoff |
| 503 | Report absent cache; keep image; bounded backoff |
| Timeout, partial, oversize, invalid JPEG | Reject replacement, preserve image/ETag |
| Redirect | Reject; do not follow or forward token |

Two MiB JPEG cap, 8-second per-call timeout, 20-second total download deadline, 5/10/20/.../300-second error retry.
JPEG baseline SOF0 8-bit <=2048 each dimension and <=4 MP; progressive JPEG is
rejected. This is a documented bounded development limit, not a low-resolution
endpoint. A 200 without ETag clears the previous ETag on acceptance. Changing
host/port/token resets the cache validator. All downloaded data stays in RAM.
ETag commits only after decode and UI queue acceptance; a full UI queue rejects
and frees the new frame. The previous image stays visible until UI replacement.

System diagnostics report elapsed time since this device ACCEPTED an image.
This is not capture time or remote image age. The API's ETag is not interpreted as
an authoritative measurement timestamp. A repeated 304 does not refresh this age.

Host tests cover transfer/error policy, ETag rollback, bounds and cover. These
are not socket-level HTTP or actual HA integration tests. Live 200/304/401/503,
timeout/reconnect and repeated-image heap behavior still need network acceptance.

After an explicit synthetic image test, the next configured UBox poll requests
a full image so 304 cannot leave the diagnostic image displayed indefinitely.
