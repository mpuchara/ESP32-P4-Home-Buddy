# Landmark and alignment contract

The locked MNP output order is image-left eye, image-left mouth, nose,
image-right eye, image-right mouth. These are image coordinates, not anatomical
left/right. Never reorder into the common eyes-eyes-nose-mouth-mouth convention.
The recognizer's canonical 112-point values use this same unusual order.

Validate all ten finite in-image coordinates, ordered eye/mouth geometry,
positive signed areas, nondegenerate scale and a bounded similarity-fit residual.
Missing/edge-clipped/mirrored/degenerate results are rejected before embedding.
The residual gate is geometric and provisional, not a recognition-quality claim.

`hb_id_to_sensor` maps upright (x,y) to sensor (raw_width-1-y,x), matching the
90-degree CCW mounting. `hb_id_roi_map` follows the existing thumbnail Bayer-cell
sampling based on dimension-2 rather than assuming an exact width ratio.
MNP receives ROI-relative candidates; its crop offset and inverse resize produce
ROI-relative points, which are translated to upright coordinates for diagnostics.
The MBF preprocessor receives the native ROI and ROI-relative landmarks.

[Detailed investigation and limitations](FACE_RECOGNITION.md). Synthetic tests
cover bounds, rotation, missing/invalid geometry, degeneracy and multiple sizes;
real camera alignment/ESPDet versus MSR reliability remains pending.
