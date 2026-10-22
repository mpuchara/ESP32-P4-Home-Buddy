"""Package only explicit public build outputs, never local/ or full-flash backups."""
import hashlib
from pathlib import Path
import shutil
import sys

build, output = map(Path, sys.argv[1:3])
root = Path(__file__).resolve().parents[1]
files = [
    "home_buddy.bin", "home_buddy.elf", "home_buddy.map",
    "bootloader/bootloader.bin", "partition_table/partition-table.bin",
    "ota_data_initial.bin", "flasher_args.json", "flash_args",
]
output.mkdir(parents=True, exist_ok=False)
hashes = []
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
    ("components/board/vendor/LICENSE", "licenses/ultramcu-MIT.txt"),
    ("assets/fonts/OFL-NotoSans.txt", "licenses/NotoSans-OFL.txt"),
    ("dependencies.lock", "dependencies.lock"),
]:
    target = output / name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(root / source_name, target)
    hashes.append(f"{hashlib.sha256(target.read_bytes()).hexdigest()}  {name}")
(output / "SHA256SUMS").write_text("\n".join(hashes) + "\n", encoding="utf-8")
