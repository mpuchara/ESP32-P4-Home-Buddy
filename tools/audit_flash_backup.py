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
    partitions = []
    for offset in range(0x8000, 0x9000, 32):
        row = data[offset:offset + 32]
        if row[:2] == b"\xeb\xeb":
            if hashlib.md5(data[0x8000:offset]).digest() != row[16:32]:
                raise ValueError("Partition table MD5 mismatch")
            break
        magic, kind, subtype, start, size, label, flags = struct.unpack("<HBBII16sI", row)
        if magic != 0x50AA:
            raise ValueError("Unreadable partition table: encrypted/unsupported backup")
        partitions.append(dict(name=label.split(b"\0")[0].decode("ascii"), type=kind,
                               subtype=subtype, offset=start, size=size, flags=flags))
    ota = next(p for p in partitions if p["name"] == "otadata")
    entries = []
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
                ota_metadata=entries, selected_ota_slot=slot, application_version=version,
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
