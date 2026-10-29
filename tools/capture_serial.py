"""Capture a bounded USB diagnostic log; does not change firmware."""
import argparse
from pathlib import Path
import time
import serial

def capture(port_name, seconds, output, reset=False, reconnect=False):
    deadline = time.monotonic() + seconds
    port = None
    reset_done = False
    total = disconnects = 0
    # Persist incrementally: a reset/disconnection must not discard the crash dump.
    with output.open("wb") as log:
        try:
            while time.monotonic() < deadline:
                if port is None:
                    candidate = serial.Serial(port=None, baudrate=115200, timeout=0.1)
                    candidate.dtr = False
                    candidate.rts = False
                    candidate.port = port_name
                    try:
                        candidate.open()
                    except (serial.SerialException, OSError):
                        candidate.close()
                        if not reconnect:
                            raise
                        time.sleep(0.2)
                        continue
                    port = candidate
                    if reset and not reset_done:
                        port.rts = True
                        time.sleep(0.1)
                        port.rts = False
                        reset_done = True
                try:
                    chunk = port.read(max(1, port.in_waiting))
                except (serial.SerialException, OSError):
                    port.close()
                    port = None
                    disconnects += 1
                    if not reconnect:
                        raise
                    time.sleep(0.1)
                    continue
                if chunk:
                    log.write(chunk)
                    log.flush()
                    total += len(chunk)
        finally:
            if port is not None:
                port.close()
    return total, disconnects


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--seconds", type=float, default=10)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--reset", action="store_true")
    parser.add_argument("--reconnect", action="store_true",
                        help="Passively reopen USB after a device restart, within the same time bound")
    args = parser.parse_args()
    if not 0 < args.seconds <= 60:
        parser.error("seconds must be in (0, 60]")
    total, disconnects = capture(args.port, args.seconds, args.output, args.reset, args.reconnect)
    print(f"Saved {total} bytes to {args.output}; serial disconnects={disconnects}")


if __name__ == "__main__":
    main()
