"""Static checks independent of the ESP-IDF installation."""
from pathlib import Path
import csv
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ProjectChecks(unittest.TestCase):
    def test_partition_budget(self):
        end = 0x9000
        apps = []
        for row in csv.reader((ROOT / "partitions.csv").read_text(encoding="utf-8").splitlines()):
            if not row or row[0].startswith("#"):
                continue
            name, kind, subtype, offset, size, *_ = [x.strip() for x in row]
            offset, size = int(offset, 0), int(size, 0)
            self.assertGreaterEqual(offset, end, name)
            self.assertEqual(offset % 0x1000, 0, name)
            if kind == "app":
                self.assertEqual(offset % 0x10000, 0)
                apps.append((subtype, size))
            end = offset + size
        self.assertEqual(end, 16 * 1024 * 1024)
        self.assertEqual(apps, [("ota_0", 6 * 1024 * 1024), ("ota_1", 6 * 1024 * 1024)])

    def test_dependency_pins(self):
        import yaml
        manifest = yaml.safe_load((ROOT / "main/idf_component.yml").read_text(encoding="utf-8"))
        lock = yaml.safe_load((ROOT / "dependencies.lock").read_text(encoding="utf-8"))
        for name, version in manifest["dependencies"].items():
            self.assertTrue(version.startswith("=="), name)
            self.assertEqual(version[2:], lock["dependencies"][name]["version"], name)
        self.assertEqual(lock["target"], "esp32p4")

    def test_polish_font(self):
        # All Polish letters must be present in the committed LVGL codepoint maps.
        import re
        font = (ROOT / "components/ui/hb_font_20.c").read_text(encoding="utf-8")
        coverage = set()
        for block in re.findall(r"\{\s*\.range_start =.*?\}", font, re.S):
            start, length = map(int, re.search(r"range_start = (\d+), .range_length = (\d+)", block).groups())
            sparse = re.search(r"unicode_list = (\w+)", block).group(1)
            if sparse == "NULL":
                coverage.update(range(start, start + length))
            else:
                body = re.search(rf"{sparse}\[\] = \{{(.*?)\}}", font, re.S).group(1)
                coverage.update(start + int(v, 16) for v in re.findall(r"0x[0-9a-fA-F]+", body))
        for letter in "ĄąĆćĘęŁłŃńÓóŚśŹźŻż°":
            self.assertIn(ord(letter), coverage, letter)
        ui = (ROOT / "components/ui/ui.c").read_text(encoding="utf-8")
        for text in re.findall(r'"(?:[^"\\]|\\.)*"', ui):
            for letter in text:
                if ord(letter) > 127:
                    self.assertIn(ord(letter), coverage, f"UI glyph {letter!r}")


if __name__ == "__main__":
    unittest.main(verbosity=2)
