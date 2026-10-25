"""Deterministic LVGL 8.3.11/font/layout regression; inputs contain no device data."""
import hashlib
import json
from pathlib import Path
import sys
root = Path(__file__).resolve().parents[1]
for name, expected in json.loads((root / 'tests/ui_render_sha256.json').read_text()).items():
    data = (Path(sys.argv[1]) / name).read_bytes()
    if not data.startswith(b'P6\n800 480\n255\n') or hashlib.sha256(data).hexdigest() != expected:
        raise SystemExit(f'UI render changed: {name}; inspect synthetic image before updating baseline')
    print(f'{name}: 800x480 visual regression passed')
