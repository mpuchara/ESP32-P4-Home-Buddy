"""Read-only audit of a private, unencrypted 16 MiB P4 Flash backup.

Only layout, hashes, OTA metadata and application version are emitted. Never
prints NVS contents/credentials. Keep the backup and generated receipt private.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib


def inspect(data):
    if len(data) != 0x1000000:
        raise ValueError("Expected exactly 16,777,216 bytes")
    from flash_layout import installed_table
    table_offset, partitions = installed_table(data)
    factory = next((p for p in partitions if p["type"] == 0 and p["subtype"] == 0), None)
    entries = []
    slot = None
    if factory:
        app = factory
    else:
        ota = next(p for p in partitions if p["name"] == "otadata")
        for bank in range(2):
            seq, _, state, crc = struct.unpack_from("<I20sII", data, ota["offset"] + bank * 4096)
            valid = seq != 0xFFFFFFFF and crc == zlib.crc32(struct.pack("<I", seq), 0xFFFFFFFF) and state not in (3, 4)
            entries.append(dict(bank=bank, sequence=seq, state=state, crc_valid=valid))
        valid = [e for e in entries if e["crc_valid"]]
        slot = (max(valid, key=lambda e: e["sequence"])["sequence"] - 1) % 2 if valid else 0
        app = next(p for p in partitions if p["type"] == 0 and p["subtype"] == 0x10 + slot)
    description = data[app["offset"] + 32:app["offset"] + 176]
    version = description[16:48].split(b"\0")[0].decode("ascii", errors="replace") if description[:4] == b"2T\xcd\xab" else "unknown"
    return dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest(), partitions=partitions,
                partition_table_offset=table_offset, selected_app=app["name"], ota_metadata=entries, selected_ota_slot=slot, application_version=version,
                bootloader_header_valid=data[0x2000] == 0xE9,
                note="Boot selection derived from backup; confirm against running boot log. Security state is a separate ROM query.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("backup", type=Path)
    parser.add_argument("--receipt", type=Path, required=True)
    args = parser.parse_args()
    result = inspect(args.backup.read_bytes())
    with args.receipt.open("x", encoding="utf-8") as f:
        json.dump(result, f, indent=2)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
