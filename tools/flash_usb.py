"""Explicit USB P4 update modes; default is an offline dry run.

Application/model modes verify the installed partition table before any write.
Full mode requires explicit layout authorization and a NEW private full backup.
No erase-chip, eFuse or C6 operation is implemented.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from flash_layout import installed_table, parse_table
from identity_model_image import validate_image
from voice_model_image import validate_image as validate_voice_image


def package_plan(package, mode):
    package = Path(package).resolve()
    for line in (package / "SHA256SUMS").read_text().splitlines():
        digest, name = line.split("  ", 1)
        path = (package / name).resolve()
        if package not in path.parents or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise ValueError("Package integrity/path failure")
    metadata = json.loads((package / "usb-package.json").read_text())
    if metadata["layout"] != "single-factory-v1" or metadata["security_enabled"]:
        raise ValueError("Unsupported layout or security provisioning image")
    table = (package / "partition_table/partition-table.bin").read_bytes()
    rows = parse_table(table)
    apps = [r for r in rows if r["type"] == 0]
    if len(apps) != 1 or apps[0]["subtype"] != 0 or apps[0]["offset"] != 0x20000 or apps[0]["size"] != 0x700000:
        raise ValueError("Expected one 7 MiB factory application")
    if any(r["name"] == "otadata" for r in rows):
        raise ValueError("OTA metadata is not part of this architecture")
    app = package / "home_buddy.bin"
    if app.stat().st_size > 0x700000 - 0x40000:
        raise ValueError("Insufficient application growth margin")
    model = package / "espdl_models/human_face_feat.espdl"
    if mode == "speaker-model":
        row = next((r for r in rows if r["name"] == "sv_model_3s"), {})
        if (row.get("type"), row.get("subtype"), row.get("offset"), row.get("size"), row.get("flags")) != (1, 0x43, 0xc20000, 0x300000, 0):
            raise ValueError("Wrong speaker partition")
        voice = package / "espdl_models/sv_model_3s.espdl"
        validate_voice_image(voice.read_bytes(), row["size"])
        return [(row["offset"], voice)], table
    if mode == "application":
        return [(0x20000, app)], table
    if mode == "model" or metadata["recognition"]:
        validate_image(model.read_bytes())
    if mode == "model":
        return [(0x720000, model)], table
    files = [(0x2000, package / "bootloader/bootloader.bin"),
             (0x10000, package / "partition_table/partition-table.bin"), (0x20000, app)]
    if metadata["recognition"]:
        files.append((0x720000, model))
    return files, table


def private_relocation(data, parent):
    if len(data) != 0x1000000:
        raise ValueError("Invalid full backup size")
    _, rows = installed_table(data)
    entries = {r["name"]: r for r in rows}
    if any(r["type"] == 0 and r["subtype"] == 0 for r in rows):
        raise ValueError("Different factory layout requires a reviewed data migration")
    nvs, phy = entries.get("nvs", {}), entries.get("phy_init", {})
    original = (nvs.get("offset"), phy.get("offset")) == (0x9000, 0x11000)
    previous = (nvs.get("offset"), phy.get("offset")) == (0x11000, 0x19000)
    app = entries.get("ota_0", {})
    expected_app_size = 0x600000 if original else 0x500000
    if (not (original or previous) or nvs.get("size") != 0x6000 or
            phy.get("size") != 0x1000 or nvs.get("type") != 1 or
            nvs.get("subtype") != 2 or phy.get("type") != 1 or phy.get("subtype") != 1 or
            app.get("type") != 0 or app.get("subtype") != 16 or
            app.get("offset") != 0x20000 or app.get("size") != expected_app_size):
        raise ValueError("Unknown settings/application map requires a reviewed migration")
    old_profiles = entries.get("profiles")
    if old_profiles and any(x != 255 for x in data[old_profiles["offset"]:old_profiles["offset"] + old_profiles["size"]]):
        raise ValueError("Existing nonblank profiles need a reviewed logical/security migration")
    files = []
    for name, destination in (("nvs", 0x11000), ("phy_init", 0x19000)):
        row = entries[name]
        if row["offset"] != destination:
            payload = data[row["offset"]:row["offset"] + row["size"]]
            path = parent / f"relocated-{name}.bin"
            with path.open("xb") as out:
                out.write(payload)
            files.append((destination, path))
    return files


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("package", type=Path)
    p.add_argument("--port", required=True, help="Explicit P4 port; never auto-select C6")
    p.add_argument("--mode", choices=("application", "model", "speaker-model", "full"), required=True)
    p.add_argument("--execute", action="store_true")
    p.add_argument("--authorize-layout-change", action="store_true")
    p.add_argument("--backup", type=Path, help="New private full-backup path, outside repository")
    a = p.parse_args()
    files, table = package_plan(a.package, a.mode)
    print("USB plan:", a.mode, [(hex(o), f.name) for o, f in files])
    print("Full mode replaces bootloader/table/app/model; layout migration relocates settings. No chip erase.")
    if not a.execute:
        print("Dry run only. No serial port opened.")
        return
    base = [sys.executable, "-m", "esptool", "--chip", "esp32p4", "--port", a.port,
            "--before", "usb_reset", "--after", "hard_reset"]
    def run(args):
        r = subprocess.run(base + args, text=True, capture_output=True)
        if r.returncode:
            raise RuntimeError("P4 serial operation failed; device preserved for USB recovery")
        return r.stdout
    security = run(["get_security_info"])
    if "Secure Boot: Disabled" not in security or "Flash Encryption: Disabled" not in security:
        raise ValueError("Secured/unknown device requires a separate signed recovery procedure")
    with tempfile.TemporaryDirectory() as temporary:
        current = Path(temporary) / "table.bin"
        run(["read_flash", "0x10000", str(len(table)), str(current)])
        unchanged = current.read_bytes() == table
    if a.mode != "full" and not unchanged:
        raise ValueError("Installed layout differs: application/model-only update refused")
    if a.mode == "full":
        if not a.authorize_layout_change or not a.backup:
            raise ValueError("Full installation requires authorization and a NEW private backup")
        backup = a.backup.resolve()
        root = Path(__file__).resolve().parents[1]
        if backup.exists() or root in backup.parents:
            raise ValueError("Backup must be new and outside repository")
        backup.parent.mkdir(parents=True, exist_ok=True)
        run(["read_flash", "0", "0x1000000", str(backup)])
        data = backup.read_bytes()
        if len(data) != 0x1000000:
            raise ValueError("Incomplete backup")
        digest = hashlib.sha256(data).hexdigest()
        backup.with_suffix(".sha256").write_text(digest + "\n")
        (backup.parent / "security-info.txt").write_text(security)
        if not unchanged:
            files.extend(private_relocation(data, backup.parent))
        # Firmware packages never contain household templates; private backups stay local.
        print("Verified private 16 MiB backup SHA256:", digest)
    args = ["write_flash", "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", "16MB"]
    for offset, file in sorted(files):
        args.extend([hex(offset), str(file)])
    run(args)
    # Verify writes against flash bytes (device is unencrypted; no personal content printed).
    verify = ["verify_flash"]
    for offset, file in files:
        verify.extend([hex(offset), str(file)])
    run(verify)
    print("P4 USB writes verified. Only the selected ranges were written; profile partition/C6/eFuses untouched.")


if __name__ == "__main__":
    main()
