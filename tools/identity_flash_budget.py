"""Report measured binary/model sizes without modifying any partition or device."""
import argparse
import hashlib
import json
from pathlib import Path
from identity_model_image import validate_image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--enforce", action="store_true", help="Reject candidate image with <256 KiB app growth margin")
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
    config = json.loads((args.build / "config/sdkconfig.json").read_text())
    candidate = config.get("PARTITION_TABLE_CUSTOM_FILENAME") == "partitions.identity.csv"
    slot = 0x500000 if candidate else 0x600000
    if candidate:
        image = (args.build / "espdl_models/human_face_feat.espdl").read_bytes()
        validate_image(image)
        measured["model_package"] = {"bytes": len(image), "sha256": hashlib.sha256(image).hexdigest()}
    margin = slot - measured["application"]["bytes"]
    print(json.dumps({"measured": measured, "candidate_layout": candidate, "ota_bytes": slot,
                      "current_models_bytes": 0x1e0000,
                      "mbf_fits_current_models": measured["mbf"]["bytes"] <= 0x1e0000,
                      "ota_headroom_bytes": margin,
                      "model_partition_headroom_bytes": 0x400000 - measured.get("model_package", {}).get("bytes", 0),
                      "reserved_bytes": 0x160000,
                      "growth_floor_bytes": 0x40000,
                      "runtime_memory": "Requires hardware measurement; not inferred from file sizes"}, indent=2))
    if args.enforce and (not candidate or margin < 0x40000):
        raise SystemExit("Insufficient candidate OTA growth margin or wrong partition table")


if __name__ == "__main__":
    main()
