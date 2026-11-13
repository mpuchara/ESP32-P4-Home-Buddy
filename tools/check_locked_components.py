"""Validate pinned registry packages even if IDF reclassifies them as local.

The second Kconfig resolution pass can turn already downloaded components
into local-source records and expand direct_dependencies. This is not a
dependency change. Verify the actual installed managed packages against the
original registry hashes before compiling, using IDF's own integrity checker.
"""
import sys
from pathlib import Path
import yaml
from idf_component_tools.hash_tools.validate import (
    validate_hash_eq_hashfile,
    validate_hashfile_eq_hashdir,
)


def load(path):
    return yaml.safe_load(Path(path).read_text(encoding="utf-8"))


def check(original, resolved, root):
    if original["target"] != resolved["target"]:
        raise ValueError("Target changed")
    before, after = original["dependencies"], resolved["dependencies"]
    if set(before) != set(after):
        raise ValueError("Dependency set changed")
    if not set(original["direct_dependencies"]) <= set(resolved["direct_dependencies"]) <= set(before):
        raise ValueError("Direct dependency set incompatible")
    for name, entry in before.items():
        actual = after[name]
        if entry["version"] != actual["version"]:
            raise ValueError(f"{name}: version changed")
        source = entry["source"]
        if source["type"] == "idf":
            if actual["source"] != source:
                raise ValueError(f"{name}: IDF source changed")
            continue
        if source["type"] != "service" or not entry.get("component_hash"):
            raise ValueError(f"{name}: unsupported original lock entry")
        directory = root / name.replace("/", "__")
        current = actual["source"]
        if current["type"] == "local":
            if Path(current["path"]).resolve() != directory.resolve():
                raise ValueError(f"{name}: unexpected local source")
        elif current["type"] == "service":
            if current["registry_url"].rstrip("/") != source["registry_url"].rstrip("/"):
                raise ValueError(f"{name}: registry changed")
            if actual.get("component_hash") != entry["component_hash"]:
                raise ValueError(f"{name}: registry hash changed")
        else:
            raise ValueError(f"{name}: unexpected resolved source")
        validate_hash_eq_hashfile(directory, entry["component_hash"])
        validate_hashfile_eq_hashdir(directory)
        print(f"Verified {name} {entry['version']} against original package hash")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: check_locked_components.py original.lock generated.lock")
    check(load(sys.argv[1]), load(sys.argv[2]), Path("managed_components").resolve())
    print("All pinned component versions and downloaded package hashes verified.")
