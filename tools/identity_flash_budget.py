"""Report measured binary/model sizes without modifying any partition or device."""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    paths = {
        "application": args.build / "home_buddy.bin",
        "mbf": root / "managed_components/espressif__human_face_recognition/models/p4/human_face_feat_mbf_s8_v1.espdl",
        "mnp": root / "managed_components/espressif__human_face_detect/models/p4/human_face_detect_mnp_s8_v1.espdl",
        "msr": root / "managed_components/espressif__human_face_detect/models/p4/human_face_detect_msr_s8_v1.espdl",
        "espdet416": root / "managed_components/espressif__human_face_detect/models/p4/espdet_pico_416_416_face.espdl",
    }
    measured = {}
    for key, path in paths.items():
        data = path.read_bytes()
        measured[key] = {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
    print(json.dumps({"measured": measured, "current_ota_bytes": 0x600000,
                      "current_models_bytes": 0x1e0000,
                      "mbf_fits_current_models": measured["mbf"]["bytes"] <= 0x1e0000,
                      "ota_headroom_bytes": 0x600000 - measured["application"]["bytes"],
                      "runtime_memory": "Requires hardware measurement; not inferred from file sizes"}, indent=2))


if __name__ == "__main__":
    main()
