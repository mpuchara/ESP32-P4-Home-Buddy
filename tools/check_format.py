"""Format application code only; preserve generated and upstream source files."""
from pathlib import Path
import subprocess
import clang_format

root = Path(__file__).resolve().parents[1]
paths = list((root / "main").glob("*.c")) + list((root / "tests").glob("*.c"))
for component in ("board", "ui", "core", "connectivity"):
    base = root / "components" / component
    paths.extend(p for p in base.glob("*.c") if not p.name.startswith("hb_font_"))
    paths.extend((base / "include").glob("*.h"))
binary = Path(clang_format.__file__).parent / "data" / "bin" / "clang-format"
if not binary.is_file():
    binary = binary.with_suffix(".exe")
subprocess.run([str(binary), "--dry-run", "--Werror", *map(str, sorted(paths))], check=True)
