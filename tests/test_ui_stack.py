"""Regression for the exact receipt-to-enrollment stack overflow seen on P4."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from check_ui_stack import parse, validate


class UIStackBudget(unittest.TestCase):
    def test_reject_captured_target_frames(self):
        # Disassembly of installed ELF 576394fb6; actual LVGL protection fault.
        with self.assertRaisesRegex(ValueError, "profile_page"):
            validate(dict(profile_page=3360, identity_update=4464,
                          identity_page=1824, update=1072, household_page=1800))

    def test_frame_and_nested_chain_limits(self):
        safe = dict(profile_page=128, identity_update=1200, identity_page=128,
                    update=1072, household_page=128)
        self.assertEqual(validate(safe), 2400)
        with self.assertRaisesRegex(ValueError, "refresh-to-page"):
            validate(dict(safe, identity_update=2048, update=1536, identity_page=512,
                          household_page=512))
        with self.assertRaisesRegex(ValueError, "Missing"):
            validate({})

    def test_target_stack_usage_parse(self):
        text = "C:/repo/ui.c:737:13:identity_update\t1200\tstatic\n"
        self.assertEqual(parse(text), {"identity_update": 1200})
        with self.assertRaisesRegex(ValueError, "Unbounded"):
            parse(text.replace("static", "dynamic"))


if __name__ == "__main__":
    unittest.main()
