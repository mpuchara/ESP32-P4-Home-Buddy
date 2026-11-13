# Home Buddy 0.14.0 experimental recognition

## Matching and enrollment
- Keep MBF_S8_V1 model partition and existing profile namespace/schema for rollback. ESP-DL 3.3.15 and detect 0.5.2 pinned; recognition 0.3.3 unchanged. CI compares downloaded registry-package hashes with lock; unchanged model image hash still checked. Runtime numeric equivalence of embeddings must be confirmed; re-enrollment recommended.
- Live recognition uses top-1 cosine 0.50 (reference Espressif wrapper default), household margin 0.10, two templates >= 0.45 and three chronological confirmations. The margin and support are our engineering choices, not Espressif defaults. Shadow tests .70/.65/.60 (old top-three) and unprotected reference .50 use same observations and temporal confirmations; never trigger events from shadows. Scores are not probabilities. Explicit evaluated known/guest trials count TA/FR/FA/confusion/unavailable separately; correlate frames only within a trial.
- Enrollment retains stronger 0.65 verification and independent fresh frame, three coherent frontal anchors then diverse samples supported by two anchors >=0.55, plus conflict check. Existing 8–15 template schema. Guided eight samples prompt pose/lighting diversity; manually add samples up to 15. Not biometric liveness or autonomous profile drift.
- Adaptive fusion: two adjacent unknown embeddings, overlapping ROI, same session, bounded time and cosine >=0.65, quality-weighted and normalized. No merging known conflicting or ambiguous identities, no reuse of fused pairs. Still requires three temporal decisions.

## Detection and preparation
- ESPDet224 for recent, large, high-confidence, overlapping faces; 416 fallback on small, uncertain, multiple, new/changed scenes; periodic 416 audits. Enrollment always 416. Both models only if spare RAM. Lightweight ROI association, not optical-flow tracking; no skipping the single-person check.
- Identity PPA client rotates/crops native RGB888, half-scales then CPU resamples detector input with existing geometry. CPU fallback on PPA error. Near faces use larger source ROI scaled into <=640x640 scratch; map landmarks back to native image. PPA rotation/color order, FPS, bus contention and memory require physical comparison.
- In diagnostics 112x112 / kamera switches to dequantized MBF input using the upstream alignment and normalization before inference; calibration counters visible. Local RAM only. No image, embedding or similarity exported via HA.

## ESP-WHO-inspired performance
- PERF compiler and L2 256KB/128B line. Cache line padding of RGB888 DMA backing storage and cache maintenance; exact payload unchanged. Camera/LVGL core 0, inference core 1, ESP-DL can schedule kernels on both cores. Keep DIO, validated 80MHz PSRAM, no XIP, bootloader/C6 untouched: QIO/XIP/higher PSRAM rates require board-specific hardware/boot validation. No partition changes.
- Test accuracy, latency, heap and stability against 0.13.0 including two people, unknowns, similar household members, day/night, glasses and distance. 0.50 is an experimental greeting policy, not access control. No physical benchmark implied by CI.
