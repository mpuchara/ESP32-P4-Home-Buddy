"""Stage verified generic MBF as an onboard-Flash PDL3 image. Never flashes a device."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
from identity_model_image import MODEL, RAW_SHA, IMAGE_SHA, IMAGE_SIZE, validate_raw, validate_image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path, help="New staging directory; never overwritten")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    component = root / "managed_components/espressif__human_face_recognition"
    raw = component / "models/p4" / MODEL
    validate_raw(raw.read_bytes())
    spec = importlib.util.spec_from_file_location("espdl_pack", root / "managed_components/espressif__esp-dl/fbs_loader/pack_espdl_models.py")
    packer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(packer)
    args.destination.mkdir(parents=True, exist_ok=False)
    target = args.destination / "human_face_feat.espdl"
    packer.pack_models([raw], target, pack_format="PDL3", package_version="v1.0")
    validate_image(target.read_bytes())
    shutil.copyfile(component / "LICENSE", args.destination / "MBF-MIT.txt")
    (args.destination / "model-manifest.json").write_text(json.dumps(dict(
        schema=1, partition="human_face_feat", offset="0x720000", image_bytes=IMAGE_SIZE,
        package_version="v1.0", image_sha256=IMAGE_SHA, raw_sha256=RAW_SHA,
        requires_authorized_partition_migration=True), indent=2), encoding="utf-8")
    print("Verified onboard Flash image staged. No device writes performed.")


if __name__ == "__main__":
    main()
