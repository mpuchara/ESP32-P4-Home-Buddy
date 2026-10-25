"""Passive bounded telemetry for a future hardware soak. Never resets or records raw logs."""
import argparse
import json
from pathlib import Path
import re
import time

ALLOWED = frozenset("uptime internal_free internal_largest internal_min psram_free psram_largest dma_free dma_largest samples rms peak dc clip repeat other_rms other_dc other_clip other_repeat errors dropped internal largest psram largest_psram stack processing_us internal_low psram_low slot".split())


def record(line):
    """Keep numeric resource/PCM metrics only; never identity names or payloads."""
    line = re.sub(r"\x1b\[[0-9;]*m", "", line)
    fault = next((x for x in ("Guru Meditation", "Task watchdog", "Interrupt wdt", "Brownout", "ESP-ROM:") if x in line), None)
    if fault:
        return {"fault": fault}
    if not any(x in line for x in ("uptime=", "PCM slot=", "diagnostic only", "MODEL_LOAD", "MODEL_RELEASE")):
        return None
    values = {key: int(value) for key, value in re.findall(r"\b([a-z_]+)=(-?\d+)(?:s)?\b", line) if key in ALLOWED}
    if "MODEL_LOAD begin" in line:
        values["model_load_begin"] = 1
    if "MODEL_RELEASE done" in line:
        values["model_release_done"] = 1
    return values or None


def monitor(port, hours, output, max_bytes=64 * 1024 * 1024):
    import serial
    deadline = time.monotonic() + hours * 3600
    connection = None
    previous_uptime = None
    with output.open("x", encoding="utf-8") as sink:
        def emit(data):
            if sink.tell() >= max_bytes:
                raise RuntimeError("Telemetry size limit reached")
            data["host_monotonic_s"] = round(time.monotonic(), 3)
            sink.write(json.dumps(data) + "\n")
            sink.flush()
        emit({"event": "started", "requested_hours": hours})
        try:
            while time.monotonic() < deadline:
                if connection is None:
                    candidate = serial.Serial(port=None, baudrate=115200, timeout=1)
                    candidate.dtr = candidate.rts = False
                    candidate.port = port
                    try:
                        candidate.open()
                    except (serial.SerialException, OSError):
                        candidate.close()
                        time.sleep(1)
                        continue
                    connection = candidate
                    emit({"event": "serial_connected"})
                try:
                    raw = connection.read_until(b"\n", size=4096)
                except (serial.SerialException, OSError):
                    connection.close()
                    connection = None
                    emit({"event": "serial_gap"})
                    continue
                data = record(raw.decode("utf-8", errors="replace"))
                if data:
                    uptime = data.get("uptime")
                    if uptime is not None:
                        if previous_uptime is not None and uptime < previous_uptime:
                            emit({"event": "device_uptime_regressed"})
                        previous_uptime = uptime
                    emit(data)
        finally:
            if connection is not None:
                connection.close()
        emit({"event": "capture_interval_ended", "hardware_acceptance": False})


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--port", required=True)
    p.add_argument("--hours", type=float, default=72)
    p.add_argument("--output", type=Path, required=True)
    a = p.parse_args()
    if not 0 < a.hours <= 72:
        p.error("hours must be in (0, 72]")
    monitor(a.port, a.hours, a.output)
