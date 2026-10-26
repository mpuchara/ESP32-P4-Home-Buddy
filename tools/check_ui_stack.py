"""Bound target-compiled UI frames; this is not a measurement of the full LVGL stack."""
import argparse
from pathlib import Path

# Leave space in the unchanged 8192-byte task for LVGL, logging and libc.
LIMITS = {"profile_page": 512, "identity_page": 512, "identity_update": 2048,
          "household_page": 512, "update": 1536}
CHAIN_LIMIT = 3584


def parse(text):
    frames = {}
    for line in text.splitlines():
        columns = line.split("\t")
        if len(columns) != 3:
            continue
        name = columns[0].rsplit(":", 1)[-1]
        if name in LIMITS:
            if columns[2] not in ("static", "dynamic,bounded"):
                raise ValueError(f"Unbounded UI frame: {name}")
            frames[name] = int(columns[1])
    return frames


def validate(frames):
    for name, limit in LIMITS.items():
        if name not in frames:
            raise ValueError(f"Missing target stack frame: {name}")
        if frames[name] > limit:
            raise ValueError(f"UI frame {name}: {frames[name]} > {limit} bytes")
    chain = frames["update"] + frames["identity_update"] + max(
        frames["identity_page"], frames["household_page"])
    if chain > CHAIN_LIMIT:
        raise ValueError(f"UI refresh-to-page frames: {chain} > {CHAIN_LIMIT} bytes")
    return chain


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    args = parser.parse_args()
    files = list(args.build.rglob("ui.c.su"))
    if len(files) != 1:
        raise SystemExit("Expected exactly one ui.c.su from -fstack-usage")
    frames = parse(files[0].read_text(encoding="utf-8"))
    chain = validate(frames)
    print(f"Target UI frame budget passed: {frames}; refresh/page subtotal={chain} B")


if __name__ == "__main__":
    main()
