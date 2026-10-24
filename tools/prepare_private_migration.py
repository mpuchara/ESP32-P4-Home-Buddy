"""Deprecated read-only helper for historical backups.

New single-image installation uses flash_usb.py --mode full, which takes a fresh
private backup, verifies the installed layout and relocates settings itself.
This helper never writes a device and is retained for old recovery receipts.
"""
import argparse
import hashlib
import json
from pathlib import Path
from audit_flash_backup import inspect


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("backup", type=Path)
    parser.add_argument("security_info", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    destination = args.destination.resolve()
    if destination == root or root in destination.parents:
        raise SystemExit("Private settings must be staged outside the repository")
    security = args.security_info.read_text(errors="replace")
    if "Secure Boot: Disabled" not in security or "Flash Encryption: Disabled" not in security:
        raise SystemExit("Encrypted/secured/unknown devices require a separate provisioning recovery plan")
    data = args.backup.read_bytes()
    audit = inspect(data)
    partitions = {p["name"]: p for p in audit["partitions"]}
    expected = {"nvs": (0x9000, 0x6000), "otadata": (0xF000, 0x2000), "phy_init": (0x11000, 0x1000)}
    for name, (offset, size) in expected.items():
        if (partitions[name]["offset"], partitions[name]["size"]) != (offset, size):
            raise SystemExit("Unexpected installed layout; re-review migration")
    destination.mkdir(parents=True, exist_ok=False)
    files = {}
    for name, target in (("nvs", 0x11000), ("phy_init", 0x19000)):
        p = partitions[name]
        payload = data[p["offset"]:p["offset"] + p["size"]]
        (destination / f"{name}.bin").write_bytes(payload)
        files[f"{target:#x}"] = dict(file=f"{name}.bin", bytes=len(payload), sha256=hashlib.sha256(payload).hexdigest())
    obsolete = {}
    for name in ("profiles", "models"):
        p = partitions[name]
        obsolete[name] = dict(blank=all(x == 255 for x in data[p["offset"]:p["offset"] + p["size"]]))
    receipt = dict(schema=1, backup_sha256=audit["sha256"], private_flash_files=files,
                   obsolete_regions=obsolete, requires_explicit_authorization=True,
                   warning="No device writes. Recheck unchanged backup/settings before applying. Nonblank obsolete data requires explicit assessment.")
    (destination / "private-migration.json").write_text(json.dumps(receipt, indent=2), encoding="utf-8")
    print("Private relocation images and SHA256 receipts prepared outside repository. No device writes.")


if __name__ == "__main__":
    main()
