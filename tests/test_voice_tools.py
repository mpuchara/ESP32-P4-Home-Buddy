"""Speaker provisioning isolation and passive telemetry privacy, without serial writes."""
import hashlib
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch
import json

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import voice_model_image as image
from flash_usb import package_plan
from monitor_soak import record
from test_usb_package import table


class VoiceTools(unittest.TestCase):
    def test_generic_structure_and_corruption(self):
        raw = bytes(image.RAW_SIZE)
        data = bytearray(image.IMAGE_SIZE)
        data[:4] = b"PDL3"
        data[4:8] = b"v1.0"
        struct.pack_into("<I", data, 20, len(data))
        struct.pack_into("<4I", data, 56, 1, 96, 72, len(image.MODEL))
        data[72:72+len(image.MODEL)] = image.MODEL.encode()
        data[96:] = raw
        with patch.object(image, "RAW_SHA", hashlib.sha256(raw).hexdigest()), patch.object(image, "IMAGE_SHA", hashlib.sha256(data).hexdigest()):
            image.validate_image(data)
            for offset in (0, 4, 20, 56, 60, 64, 68, 72, 95, 96, len(data)-1):
                corrupt = bytearray(data)
                corrupt[offset] ^= 1
                with self.assertRaises(ValueError):
                    image.validate_image(corrupt)
            with self.assertRaises(ValueError):
                image.validate_image(data, len(data)-1)
        with self.assertRaises(ValueError):
            image.validate_raw(raw)

    def test_speaker_usb_selects_exact_partition(self):
        with tempfile.TemporaryDirectory() as temporary:
            package = Path(temporary)
            (package / "partition_table").mkdir()
            (package / "espdl_models").mkdir()
            voice = package / "espdl_models/sv_model_3s.espdl"
            voice.write_bytes(b"synthetic image; validator mocked")
            part = package / "partition_table/partition-table.bin"
            (package / "home_buddy.bin").write_bytes(b"synthetic app")
            (package / "usb-package.json").write_text(json.dumps(dict(layout="single-factory-v1",security_enabled=False,recognition=True)))
            def checksums():
                files = [p for p in package.rglob("*") if p.is_file() and p.name != "SHA256SUMS"]
                (package / "SHA256SUMS").write_text("".join(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(package).as_posix()}\n" for p in files))
            for label, offset, size in (("sv_model_3s",0xc20000,0x300000), ("wrong",0xc20000,0x300000), ("sv_model_3s",0xc30000,0x300000), ("sv_model_3s",0xc20000,0x200000)):
                part.write_bytes(table([("factory",0,0,0x20000,0x700000),(label,1,0x43,offset,size)]))
                checksums()
                with patch("flash_usb.validate_voice_image") as validate:
                    if (label,offset,size)==("sv_model_3s",0xc20000,0x300000):
                        files, _ = package_plan(package,"speaker-model")
                        self.assertEqual(files,[(0xc20000,voice)])
                        validate.assert_called_once()
                    else:
                        with self.assertRaises(ValueError):
                            package_plan(package,"speaker-model")
            self.assertEqual([o for o,_ in package_plan(package,"application")[0]],[0x20000])

    def test_telemetry_keeps_only_metrics(self):
        self.assertIsNone(record("recognized name=private HA_token=secret"))
        self.assertEqual(record("uptime=60s internal_free=100000 private_name=Alice token=123"),{"uptime":60,"internal_free":100000})
        self.assertEqual(record("PCM slot=1 rms=300 dc=-12 samples=500 name=private"),{"slot":1,"rms":300,"dc":-12,"samples":500})
        self.assertEqual(record("Task watchdog got triggered private details"),{"fault":"Task watchdog"})
        self.assertEqual(record("voice diagnostic only load_us=10 features_us=20 inference_us=30 voice_loads=2 voice_releases=2 person_id=99 embedding=100"),
                         {"load_us":10,"features_us":20,"inference_us":30,"voice_loads":2,"voice_releases":2})
