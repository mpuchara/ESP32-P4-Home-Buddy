"""A USB reset must preserve the partial crash log and permit passive reconnect."""
import importlib.util
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import patch


class Detached(OSError):
    pass


class CaptureSerial(unittest.TestCase):
    def test_preserves_dump_and_passively_reconnects(self):
        self.exercise(True)

    def test_preserves_dump_when_reconnect_not_requested(self):
        self.exercise(False)

    def exercise(self, reconnect):
        clock = types.SimpleNamespace(now=0)
        ports = []

        class Port:
            def __init__(self, **kwargs):
                self.index = len(ports)
                self.reads = 0
                self.closed = False
                ports.append(self)

            def open(self):
                assert self.dtr is False and self.rts is False

            def close(self):
                self.closed = True

            @property
            def in_waiting(self):
                return 128

            def read(self, size):
                clock.now += 0.1
                self.reads += 1
                if self.index == 0:
                    if self.reads == 1:
                        return b"panic before USB disconnect\n"
                    raise Detached()
                return b"new boot\n" if self.reads == 1 else b""

        serial = types.SimpleNamespace(Serial=Port, SerialException=Detached)
        spec = importlib.util.spec_from_file_location(
            "capture_serial_under_test", Path(__file__).resolve().parents[1] / "tools/capture_serial.py")
        module = importlib.util.module_from_spec(spec)
        with patch.dict(sys.modules, {"serial": serial}):
            spec.loader.exec_module(module)

        def sleep(seconds):
            clock.now += seconds

        with tempfile.TemporaryDirectory() as directory, \
                patch.object(module.time, "monotonic", lambda: clock.now), \
                patch.object(module.time, "sleep", sleep):
            output = Path(directory) / "capture.log"
            if reconnect:
                size, disconnects = module.capture("TEST", 0.8, output, reconnect=True)
                self.assertEqual(disconnects, 1)
                self.assertEqual(output.read_bytes(), b"panic before USB disconnect\nnew boot\n")
                self.assertEqual(size, output.stat().st_size)
                self.assertEqual(len(ports), 2)
            else:
                with self.assertRaises(Detached):
                    module.capture("TEST", 0.8, output)
                self.assertEqual(output.read_bytes(), b"panic before USB disconnect\n")
            self.assertTrue(all(port.closed for port in ports))


if __name__ == "__main__":
    unittest.main()
