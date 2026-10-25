"""Package only explicit public build outputs, never local/ or full-flash backups."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys
import json
from identity_model_image import validate_image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("build", type=Path)
parser.add_argument("output", type=Path)
parser.add_argument("--speaker-model", type=Path, help="Optional verified single 3s PDL3 image; never auto-flashed")
args = parser.parse_args()
build, output = args.build, args.output
if args.speaker_model:
    from voice_model_image import validate_image as validate_voice_image
    validate_voice_image(args.speaker_model.read_bytes())
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
if args.speaker_model:
    target = output / "espdl_models/sv_model_3s.espdl"
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(args.speaker_model, target)
    hashes.append(f"{hashlib.sha256(target.read_bytes()).hexdigest()}  espdl_models/sv_model_3s.espdl")
for source_name, name in [
    ("docs/ITERATION11_REPORT.md", "docs/ITERATION11_REPORT.md"),
    ("docs/ITERATION11_ACCEPTANCE.md", "docs/ITERATION11_ACCEPTANCE.md"),
    ("docs/PERSISTENCE_DECISION.md", "docs/PERSISTENCE_DECISION.md"),
    ("tools/voice_model_image.py", "tools/voice_model_image.py"),
    ("tools/monitor_soak.py", "tools/monitor_soak.py"),
    ("docs/VOICE_RECOGNITION.md", "docs/VOICE_RECOGNITION.md"),
    ("docs/ITERATION10_REPORT.md", "docs/ITERATION10_REPORT.md"),
    ("LICENSE", "licenses/Apache-2.0.txt"),
    ("tools/flash_usb.py", "tools/flash_usb.py"),
    ("tools/flash_layout.py", "tools/flash_layout.py"),
    ("tools/identity_model_image.py", "tools/identity_model_image.py"),
    ("docs/USB_FLASHING.md", "docs/USB_FLASHING.md"),
    ("docs/FLASH_MIGRATION.md", "docs/FLASH_MIGRATION.md"),
    ("docs/SINGLE_IMAGE_FLASH_LAYOUT.md", "docs/SINGLE_IMAGE_FLASH_LAYOUT.md"),
    ("docs/HOME_ASSISTANT_HTTP.md", "docs/HOME_ASSISTANT_HTTP.md"),
    ("docs/ITERATION9_REPORT.md", "docs/ITERATION9_REPORT.md"),
    ("docs/ITERATION9_VALIDATION.md", "docs/ITERATION9_VALIDATION.md"),
    ("docs/ITERATION8_REPORT.md", "docs/ITERATION8_REPORT.md"),
    ("docs/ITERATION8_ACCEPTANCE.md", "docs/ITERATION8_ACCEPTANCE.md"),
    ("docs/SECURITY_PROVISIONING.md", "docs/SECURITY_PROVISIONING.md"),
    ("docs/AUDIO_DIAGNOSTICS.md", "docs/AUDIO_DIAGNOSTICS.md"),
    ("docs/SPEAKER_FEASIBILITY.md", "docs/SPEAKER_FEASIBILITY.md"),
    ("docs/PROFILE_DATABASE.md", "docs/PROFILE_DATABASE.md"),
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
    ("managed_components/espressif__esp_codec_dev/LICENSE", "licenses/esp-codec-dev-Apache-2.0.txt"),
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
description = build / "project_description.json"
metadata["firmware_version"] = json.loads(description.read_text()).get("project_version") if description.is_file() else None
try:
    metadata["source_commit"] = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    metadata["source_dirty"] = bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=root, text=True).strip())
except (subprocess.CalledProcessError, FileNotFoundError):
    metadata["source_commit"] = None
    metadata["source_dirty"] = None
metadata["application_bytes"] = (output / "home_buddy.bin").stat().st_size
metadata["application_sha256"] = hashlib.sha256((output / "home_buddy.bin").read_bytes()).hexdigest()
metadata["application_headroom_bytes"] = 0x700000 - metadata["application_bytes"]
metadata["optional_speaker_image"] = bool(args.speaker_model)
if args.speaker_model:
    metadata["speaker_image_bytes"] = args.speaker_model.stat().st_size
    metadata["speaker_headroom_bytes"] = 0x300000 - metadata["speaker_image_bytes"]
(output / "usb-package.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
hashes.append(f"{hashlib.sha256((output / 'usb-package.json').read_bytes()).hexdigest()}  usb-package.json")
(output / "SHA256SUMS").write_text("\n".join(hashes) + "\n", encoding="utf-8")
