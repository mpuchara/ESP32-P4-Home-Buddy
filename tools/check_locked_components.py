"""Reject any dependency version/hash/source drift; tolerate lock serialization metadata."""
import sys
from pathlib import Path
import yaml


def pinned(path):
    doc = yaml.safe_load(Path(path).read_text(encoding="utf-8"))
    return (
        doc["target"],
        {
            name: (entry["version"], entry.get("component_hash"), entry["source"])
            for name, entry in doc["dependencies"].items()
        },
        sorted(doc["direct_dependencies"]),
    )


if len(sys.argv) != 3:
    raise SystemExit("usage: check_locked_components.py original.lock generated.lock")
before, after = map(pinned, sys.argv[1:])
if before != after:
    print("Original pins:", before)
    print("Resolved pins:", after)
    raise SystemExit("Dependency version, component hash, source or set changed")
print("All pinned component versions, hashes and sources unchanged.")
