"""Format application code only; preserve generated and upstream source files."""
from pathlib import Path
import subprocess
import clang_format

root = Path(__file__).resolve().parents[1]
paths = list((root / "main").glob("*.c")) + list((root / "tests").glob("*.c"))
for component in ("board", "ui", "core", "connectivity", "configuration", "screensaver", "ubox_client", "services", "camera", "brightness", "face", "compute_budget", "identity", "home_assistant", "audio", "voice"):
    base = root / "components" / component
    paths.extend(p for p in base.glob("*.c") if not p.name.startswith("hb_font_"))
    paths.extend(base.glob("*.cpp"))
    paths.extend(base.glob("*.h"))
    paths.extend(base.glob("*.hpp"))
    paths.extend((base / "include").glob("*.h"))
paths.extend((root / "tests" / "brightness_platform").rglob("*.h"))
binary = Path(clang_format.__file__).parent / "data" / "bin" / "clang-format"
if not binary.is_file():
    binary = binary.with_suffix(".exe")
result = subprocess.run([str(binary), "--dry-run", "--Werror", *map(str, sorted(paths))])
if result.returncode:
    import difflib
    for path in sorted(paths):
        original = path.read_text(encoding="utf-8")
        formatted = subprocess.check_output([str(binary), str(path)]).decode("utf-8")
        if original != formatted:
            relative = str(path.relative_to(root))
            print("".join(difflib.unified_diff(original.splitlines(True), formatted.splitlines(True),
                                                fromfile="a/" + relative, tofile="b/" + relative)))
    raise SystemExit(result.returncode)
