"""Copy only the pinned generic MBF model and MIT notice to a local SD staging folder.

This prepares generic pretrained weights; it never reads camera images or personal templates.
"""
import argparse
import hashlib
from pathlib import Path
import shutil

EXPECTED = "0cd2629f7111b821f419531d0d3ff33d81018391fa57039acd1022a4531cfd48"
MODEL = "human_face_feat_mbf_s8_v1.espdl"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path, help="New SD staging directory; never overwritten")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    component = root / "managed_components/espressif__human_face_recognition"
    data = (component / "models/p4" / MODEL).read_bytes()
    if len(data) != 3522832 or hashlib.sha256(data).hexdigest() != EXPECTED:
        raise SystemExit("Unexpected generic MBF weights; refusing to stage")
    args.destination.mkdir(parents=True, exist_ok=False)
    target = args.destination / "models/p4"
    target.mkdir(parents=True)
    (target / MODEL).write_bytes(data)
    shutil.copyfile(component / "LICENSE", target / "MBF-MIT.txt")
    (args.destination / "SHA256SUMS").write_text(f"{EXPECTED}  models/p4/{MODEL}\n", encoding="ascii")
    print("Generic model staged. Copy models/ to a FAT microSD. No household data is included.")


if __name__ == "__main__":
    main()
