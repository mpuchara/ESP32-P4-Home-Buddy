"""Capture a bounded USB diagnostic log; does not change firmware."""
import argparse
from pathlib import Path
import time
import serial

parser = argparse.ArgumentParser()
parser.add_argument("--port", required=True)
parser.add_argument("--seconds", type=float, default=10)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--reset", action="store_true")
args = parser.parse_args()
if not 0 < args.seconds <= 60:
    parser.error("seconds must be in (0, 60]")
port = serial.Serial(port=None, baudrate=115200, timeout=0.1)
# Set modem lines before opening; passive observation must not reset provisioning.
port.dtr = False
port.rts = False
port.port = args.port
with port:
    if args.reset:
        port.dtr = False
        port.rts = True
        time.sleep(0.1)
        port.rts = False
    deadline = time.monotonic() + args.seconds
    chunks = []
    while time.monotonic() < deadline:
        chunks.append(port.read(max(1, port.in_waiting)))
args.output.write_bytes(b"".join(chunks))
print(f"Saved {sum(map(len, chunks))} bytes to {args.output}")
