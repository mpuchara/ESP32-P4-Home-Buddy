"""Package only explicit public build outputs, never local/ or full-flash backups."""
import hashlib
from pathlib import Path
import shutil
import sys
import json
from identity_model_image import validate_image

build, output = map(Path, sys.argv[1:3])
root = Path(__file__).resolve().parents[1]
files = [
    "home_buddy.bin", "home_buddy.elf", "home_buddy.map",
    "bootloader/bootloader.bin", "partition_table/partition-table.bin",
    "flasher_args.json", "flash_args",
]
output.mkdir(parents=True, exist_ok=False)
hashes = []
config = json.loads((build / "config/sdkconfig.json").read_text())
if config.get("HB_IDENTITY_EXPERIMENTAL"):
    model = build / "espdl_models/human_face_feat.espdl"
    validate_image(model.read_bytes())
    files.append("espdl_models/human_face_feat.espdl")
for name in files:
    source = build / name
    if not source.is_file():
        raise SystemExit(f"Missing build artifact: {source}")
    target = output / name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
    hashes.append(f"{hashlib.sha256(target.read_bytes()).hexdigest()}  {name}")
for source_name, name in [
    ("LICENSE", "licenses/Apache-2.0.txt"),
    ("tools/flash_usb.py", "tools/flash_usb.py"),
    ("tools/flash_layout.py", "tools/flash_layout.py"),
    ("tools/identity_model_image.py", "tools/identity_model_image.py"),
    ("docs/USB_FLASHING.md", "docs/USB_FLASHING.md"),
    ("docs/FLASH_MIGRATION.md", "docs/FLASH_MIGRATION.md"),
    ("docs/SINGLE_IMAGE_FLASH_LAYOUT.md", "docs/SINGLE_IMAGE_FLASH_LAYOUT.md"),
    ("docs/HOME_ASSISTANT_HTTP.md", "docs/HOME_ASSISTANT_HTTP.md"),
    ("components/board/vendor/LICENSE", "licenses/ultramcu-MIT.txt"),
    ("components/camera/vendor/LICENSE", "licenses/GUITION-camera-Apache-2.0.txt"),
    ("components/camera/vendor/SOURCE.md", "licenses/GUITION-camera-source.md"),
    ("components/esp_driver_cam/SOURCE.md", "licenses/IDF-CSI-adapter-source.md"),
    ("assets/fonts/OFL-NotoSans.txt", "licenses/NotoSans-OFL.txt"),
    ("components/face/vendor/ESP-DL-MIT.txt", "licenses/ESP-DL-MIT.txt"),
    ("components/face/vendor/Human-Face-MIT.txt", "licenses/Human-Face-MIT.txt"),
    ("components/face/vendor/DL-FFT-MIT.txt", "licenses/DL-FFT-MIT.txt"),
    ("components/face/vendor/ESP-New-JPEG.txt", "licenses/ESP-New-JPEG.txt"),
    ("components/face/vendor/SOURCE.md", "licenses/Face-source.md"),
    ("components/identity/vendor/MBF-MIT.txt", "licenses/MBF-MIT.txt"),
    ("components/identity/vendor/SOURCE.md", "licenses/Identity-source.md"),
    ("dependencies.lock", "dependencies.lock"),
]:
    target = output / name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(root / source_name, target)
    hashes.append(f"{hashlib.sha256(target.read_bytes()).hexdigest()}  {name}")
if config.get("HB_IDENTITY_EXPERIMENTAL"):
    (output / "MIGRATION_REQUIRED.txt").write_text(
        "Development candidate only. Full flash_args writes a NEW partition table.\n"
        "Factory single-image layout. Application-only USB update preserves models/profiles.\n"
        "Requires explicit authorization and docs/FLASH_MIGRATION.md preflight.\n"
        "No encrypted profile provisioning or eFuse operation is authorized by this package.\n", encoding="utf-8")
    hashes.append(f"{hashlib.sha256((output / 'MIGRATION_REQUIRED.txt').read_bytes()).hexdigest()}  MIGRATION_REQUIRED.txt")
metadata = {"schema": 1, "layout": "single-factory-v1", "partition_table_offset": 0x10000,
            "application_offset": 0x20000, "application_size": 0x700000,
            "model_offset": 0x720000, "model_size": 0x400000,
            "security_enabled": bool(config.get("SECURE_FLASH_ENC_ENABLED") or config.get("SECURE_BOOT")),
            "recognition": bool(config.get("HB_IDENTITY_EXPERIMENTAL"))}
(output / "usb-package.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
hashes.append(f"{hashlib.sha256((output / 'usb-package.json').read_bytes()).hexdigest()}  usb-package.json")
(output / "SHA256SUMS").write_text("\n".join(hashes) + "\n", encoding="utf-8")
