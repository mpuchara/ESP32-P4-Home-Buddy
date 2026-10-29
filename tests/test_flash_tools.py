"""Host checks of actual model assets and candidate layout; no hardware writes."""
import csv
import hashlib
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from identity_model_image import validate_image, validate_raw, IMAGE_SIZE


def layout(path, table_offset=0x8000):
    rows = []
    for row in csv.reader(path.read_text().splitlines()):
        if row and not row[0].startswith("#"):
            name, kind, subtype, offset, size, *flags = [c.strip() for c in row]
            rows.append((name, kind, subtype, int(offset, 0), int(size, 0), flags))
    end = table_offset + 4096
    for name, kind, _, offset, size, _ in rows:
        if offset < end or offset % (0x10000 if kind == "app" else 4096) or size % 4096:
            raise ValueError(f"Overlap/alignment: {name}")
        end = offset + size
        if end > 0x1000000:
            raise ValueError("Flash capacity exceeded")
    return rows


class FlashTools(unittest.TestCase):
    def test_layout(self):
        rows = layout(ROOT / "partitions.csv", 0x10000)
        p = {r[0]: r for r in rows}
        self.assertEqual(p["factory"][3:5], (0x20000, 0x700000))
        self.assertEqual([r[0] for r in rows if r[1]=="app"], ["factory"])
        self.assertNotIn("otadata",p)
        self.assertEqual(p["human_face_feat"][3:5], (0x720000, 0x400000))
        self.assertEqual(p["profiles"][3:5], (0xB20000, 0x100000))
        self.assertIn("encrypted", p["profiles"][5])
        self.assertGreater(p["human_face_feat"][4], IMAGE_SIZE)
        self.assertEqual(p["sv_model_3s"][3:5],(0xC20000,0x300000))
        self.assertIn("encrypted", p["nvs_keys"][5])

    def test_bad_layout(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "bad.csv"
            for body in ("bad,data,0x40,0x8000,0x1000,", "bad,app,ota_0,0x21000,0x1000,", "bad,data,0x40,0xfff000,0x2000,"):
                p.write_text(body)
                with self.assertRaises(ValueError):
                    layout(p)

    def test_real_pinned_package(self):
        component = ROOT / "managed_components/espressif__human_face_recognition/models/p4/human_face_feat_mbf_s8_v1.espdl"
        if not component.exists():
            self.skipTest("Run after dependency resolution for real generic weights")
        raw = component.read_bytes()
        validate_raw(raw)
        spec = importlib.util.spec_from_file_location("packer", ROOT / "managed_components/espressif__esp-dl/fbs_loader/pack_espdl_models.py")
        packer = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(packer)
        with tempfile.TemporaryDirectory() as d:
            image = Path(d) / "model.espdl"
            packer.pack_models([component], image, pack_format="PDL3", package_version="v1.0")
            data = image.read_bytes()
            validate_image(data)
            for offset in (0, 4, 20, 24, 56, 60, 64, 68, 72, 111, 112, len(data) - 1):
                corrupt = bytearray(data)
                corrupt[offset] ^= 1
                with self.assertRaises(ValueError):
                    validate_image(corrupt)
            with self.assertRaises(ValueError):
                validate_image(data, 0x1E0000)
            for wrong in (data[:-1], data + b"\0", b"", raw):
                with self.assertRaises(ValueError):
                    validate_image(wrong)


if __name__ == "__main__":
    unittest.main()
