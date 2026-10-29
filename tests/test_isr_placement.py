"""Link placement guard rejects a missing or Flash-resident ISR callee."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from check_isr_placement import REQUIRED, validate


class ISRPlacement(unittest.TestCase):
    def test_complete_iram_closure(self):
        validate({name: ['.iram0.text'] for name in REQUIRED})

    def test_flash_callee_rejected(self):
        for name in REQUIRED:
            sections = {item: ['.iram0.text'] for item in REQUIRED}
            sections[name] = ['.flash.text']
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, name):
                validate(sections)

    def test_missing_callee_rejected(self):
        with self.assertRaisesRegex(ValueError, 'Missing'):
            validate({})
