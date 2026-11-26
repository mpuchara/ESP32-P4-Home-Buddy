"""Build the app-only Web Serial installer from a verified recognition package."""
import argparse
import base64
import hashlib
import html as html_escape
import json
from pathlib import Path
import shutil

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--package", type=Path, default=Path("dist/recognition"))
parser.add_argument("--source-commit")
args = parser.parse_args()
package = args.package
app = (package / "home_buddy.bin").read_bytes()
table = (package / "partition_table/partition-table.bin").read_bytes()
metadata = json.loads((package / "usb-package.json").read_text())
assert metadata["layout"] == "single-factory-v1"
assert metadata["application_offset"] == 0x20000
assert metadata["application_size"] == 0x700000
assert metadata["recognition"] and not metadata["security_enabled"]
assert metadata["profile_storage_mode"] == "LOCAL_UNENCRYPTED"
if args.source_commit:
    assert metadata["source_commit"] == args.source_commit
assert 0 < len(app) <= 0x700000 and app[0] == 0xe9
assert 0 < len(table) <= 0x1000
digest = hashlib.sha256(app).hexdigest()
assert digest == metadata["application_sha256"]
version = metadata["firmware_version"]
assert isinstance(version, str) and version
out = Path("web-dist")
out.mkdir(exist_ok=True)
html = Path("web/installer.html").read_text(encoding="utf-8")
html = html.replace("__APP_BASE64__", base64.b64encode(app).decode())
html = html.replace("__APP_SHA256__", digest)
html = html.replace("__TABLE_BASE64__", base64.b64encode(table).decode())
html = html.replace("__FIRMWARE_VERSION__", html_escape.escape(version))
(out / "index.html").write_text(html, encoding="utf-8")
(out / "home_buddy.bin").write_bytes(app)
shutil.copyfile("docs/VOICE_HARDWARE_TEST.md", out / "HARDWARE_TEST.md")
vendor = Path("components/voice/vendor/libfvad")
(out / "THIRD_PARTY_NOTICES.txt").write_text(
    "Home Buddy / libfvad (WebRTC)\n\n"
    + "\n\n".join((vendor / name).read_text(encoding="utf-8")
                    for name in ("LICENSE", "PATENTS", "AUTHORS", "UPSTREAM.md")),
    encoding="utf-8")
names = ("home_buddy.bin", "index.html", "HARDWARE_TEST.md", "THIRD_PARTY_NOTICES.txt")
(out / "SHA256SUMS").write_text(
    "".join(f"{hashlib.sha256((out / name).read_bytes()).hexdigest()}  {name}\n"
            for name in names), encoding="utf-8")
