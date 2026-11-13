"""Build an app-only self-contained Web Serial installer after IDF checks."""
import base64
import hashlib
import json
from pathlib import Path

build = Path("build")
app = (build / "home_buddy.bin").read_bytes()
table = (build / "partition_table/partition-table.bin").read_bytes()
metadata = json.loads(Path("dist/recognition/usb-package.json").read_text())
assert metadata["layout"] == "single-factory-v1"
assert metadata["application_offset"] == 0x20000
assert metadata["application_size"] == 0x700000
assert metadata["recognition"] and not metadata["security_enabled"]
assert metadata["profile_storage_mode"] == "LOCAL_UNENCRYPTED"
assert 0 < len(app) <= 0x700000 and app[0] == 0xe9
assert 0 < len(table) <= 0x1000
digest = hashlib.sha256(app).hexdigest()
assert digest == metadata["application_sha256"]
out = Path("web-dist")
out.mkdir(exist_ok=True)
html = Path("web/installer.html").read_text(encoding="utf-8")
html = html.replace("__APP_BASE64__", base64.b64encode(app).decode())
html = html.replace("__APP_SHA256__", digest)
html = html.replace("__TABLE_BASE64__", base64.b64encode(table).decode())
(out / "index.html").write_text(html, encoding="utf-8")
(out / "home_buddy.bin").write_bytes(app)
(out / "SHA256SUMS").write_text(
    f"{digest}  home_buddy.bin\n"
    f"{hashlib.sha256((out / 'index.html').read_bytes()).hexdigest()}  index.html\n"
)
