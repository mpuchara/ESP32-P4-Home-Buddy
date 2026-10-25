"""Stage one verified SV 3s image using the pinned ESP-DL packer. No serial access."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
from voice_model_image import validate_raw, validate_image, RAW_SHA, IMAGE_SHA, IMAGE_SIZE, MODEL


def prepare(raw, destination):
    root = Path(__file__).resolve().parents[1]
    validate_raw(raw.read_bytes())
    if raw.name != MODEL:
        raise ValueError("Use the pinned upstream filename")
    destination.mkdir(parents=True, exist_ok=False)
    spec = importlib.util.spec_from_file_location("espdl_pack", root / "managed_components/espressif__esp-dl/fbs_loader/pack_espdl_models.py")
    packer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(packer)
    target = destination / "sv_model_3s.espdl"
    packer.pack_models([raw], target, pack_format="PDL3", package_version="v1.0")
    validate_image(target.read_bytes())
    shutil.copyfile(root / "managed_components/espressif__esp-dl/LICENSE", destination / "ESP-DL-MIT.txt")
    (destination / "model-manifest.json").write_text(json.dumps(dict(
        schema=1, source_commit="eb72563e6a4e61d2bbc0d46e4c79f2c9f816c3c6", component="speaker_verification:0.3.1",
        esp_dl="3.3.14", partition="sv_model_3s", offset="0xc20000", partition_bytes=0x300000,
        raw_sha256=RAW_SHA, image_sha256=IMAGE_SHA, image_bytes=IMAGE_SIZE,
        headroom_bytes=0x300000-IMAGE_SIZE, model=MODEL), indent=2), encoding="utf-8")
    return target


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("raw", type=Path)
    p.add_argument("destination", type=Path)
    a = p.parse_args()
    print(prepare(a.raw, a.destination))
