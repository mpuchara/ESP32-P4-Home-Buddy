"""Verify update boundaries with synthetic generic build files, no serial access."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from flash_usb import package_plan, private_relocation
from flash_layout import parse_table


def table(entries):
    data = b"".join(struct.pack("<HBBII16sI", 0x50AA, kind, subtype, offset, size, name.encode(), 0)
                    for name, kind, subtype, offset, size in entries)
    return data + b"\xeb\xeb" + b"\xff" * 14 + hashlib.md5(data).digest()


class USBPackaging(unittest.TestCase):
    def test_application_only_and_full(self):
        with tempfile.TemporaryDirectory() as temporary:
            p = Path(temporary)
            (p / "partition_table").mkdir()
            (p / "partition_table/partition-table.bin").write_bytes(table([("factory", 0, 0, 0x20000, 0x700000)]))
            (p / "home_buddy.bin").write_bytes(b"synthetic-test-application")
            (p / "bootloader").mkdir()
            (p / "bootloader/bootloader.bin").write_bytes(b"synthetic-bootloader")
            (p / "usb-package.json").write_text(json.dumps(dict(layout="single-factory-v1",security_enabled=False,recognition=False)))
            def checksum():
                paths = [x for x in p.rglob("*") if x.is_file() and x.name != "SHA256SUMS"]
                (p / "SHA256SUMS").write_text("".join(f"{hashlib.sha256(x.read_bytes()).hexdigest()}  {x.relative_to(p).as_posix()}\n" for x in paths))
            checksum()
            plan, _ = package_plan(p,"application")
            self.assertEqual([o for o, _ in plan],[0x20000])
            plan, _ = package_plan(p,"full")
            self.assertEqual([o for o, _ in plan],[0x2000,0x10000,0x20000])
            self.assertNotIn(0xb20000,[o for o, _ in plan])
            (p / "home_buddy.bin").write_bytes(b"corrupt")
            with self.assertRaises(ValueError): package_plan(p,"application")
            (p / "home_buddy.bin").write_bytes(bytes(0x700000))
            checksum()
            with self.assertRaises(ValueError): package_plan(p,"application")

    def test_private_relocation_and_unknown_profiles(self):
        with tempfile.TemporaryDirectory() as temporary:
            data = bytearray(b"\xff" * 0x1000000)
            entries = [("nvs",1,2,0x9000,0x6000),("phy_init",1,1,0x11000,0x1000),
                       ("ota_0",0,16,0x20000,0x600000),("profiles",1,130,0xc20000,0x200000)]
            binary = table(entries)
            data[0x8000:0x8000+len(binary)] = binary
            data[0x9000:0xf000] = b"S" * 0x6000
            files = private_relocation(data,Path(temporary))
            self.assertEqual([o for o, _ in files],[0x11000,0x19000])
            self.assertEqual(files[0][1].read_bytes(),b"S" * 0x6000)
            data[0xc20000] = 1
            with self.assertRaises(ValueError): private_relocation(data,Path(temporary))
            bad = bytearray(binary); bad[10] ^= 1
            with self.assertRaises(ValueError): parse_table(bad)


if __name__ == "__main__":
    unittest.main()
