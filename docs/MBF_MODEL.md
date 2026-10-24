# MBF model integration

Pinned `espressif/human_face_recognition ==0.3.3` resolves with existing
`human_face_detect ==0.5.1` and `esp-dl ==3.3.14`. No existing dependency upgraded.
Default runtime backend is explicitly `HumanFaceFeat::MBF_S8_V1`, never MFN.

Model: generic P4 `human_face_feat_mbf_s8_v1.espdl`, 3,522,832 bytes. SHA256:
`0cd2629f7111b821f419531d0d3ff33d81018391fa57039acd1022a4531cfd48`.
Input is validated as NHWC 1x112x112x3. Upstream aligns from five real landmarks,
subtracts/divides by 127.5, runs the quantized model and returns 512 L2-normalized
float features. The wrapper checks type/dimension/finiteness/norm before matching.

The generic model is prepared off-device, but all personal embeddings are generated
on P4. `tools/prepare_identity_model.py` packages verified generic weights as PDL3 for
the onboard human_face_feat partition, plus the MIT notice. No external storage
is mounted. Version/size/full-package SHA256 are verified before ESP-DL. Model and
scratch are lazily loaded/unloaded under the existing compute mutex. Failed model
admission/read returns unavailable. Allocation assertions inside upstream code
remain a physical stress-test risk; no isolated-failure acceptance is claimed.

[Memory/flash assessment](IDENTITY_FLASH_LAYOUT.md), [full implementation](FACE_RECOGNITION.md).
Actual neural execution and latency on this board are still unverified.
