"""Read partition metadata only. No credentials or device writes."""
import hashlib
import struct


def parse_table(data):
    rows = []
    for offset in range(0, len(data) - 31, 32):
        row = data[offset:offset + 32]
        if row[:2] == b"\xeb\xeb":
            if hashlib.md5(data[:offset]).digest() != row[16:]:
                raise ValueError("Partition-table MD5 mismatch")
            return rows
        magic, kind, subtype, start, size, name, flags = struct.unpack("<HBBII16sI", row)
        if magic != 0x50AA:
            raise ValueError("Unsupported/encrypted partition table")
        rows.append(dict(name=name.split(b"\0")[0].decode("ascii"), type=kind,
                         subtype=subtype, offset=start, size=size, flags=flags))
    raise ValueError("Missing partition-table integrity record")


def installed_table(data):
    for offset in (0x10000, 0x8000):
        try:
            return offset, parse_table(data[offset:offset + 4096])
        except ValueError:
            continue
    raise ValueError("No supported verified partition table")
