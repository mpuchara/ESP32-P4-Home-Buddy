# Build, flash and recovery on Windows

Use an ESP-IDF **5.5.1** environment; keep any existing 5.1 environment intact.
This session installed IDF/tooling separately under the local Codex temporary
directory. That path is not a portable build dependency.

## Clean environment

Using Git and Python in PowerShell, in a directory outside this repository:
```powershell
git clone --branch v5.5.1 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
.\install.ps1 esp32p4
.\export.ps1
$env:PYTHONIOENCODING = "utf-8"
```
On subsequent shells activate the same export.ps1 again, then enter Home Buddy.
Use IDF's Python/tools, not an unactivated system Python without esptool.

```powershell
idf.py set-target esp32p4
idf.py build
idf.py -B build-bringup -D SDKCONFIG=build-bringup/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;config/sdkconfig.bringup" build
```

Diagnostics is the safe default. The bringup profile enables pins from the
experimental BSP. The wifi profile additionally initializes C6.
For reproducibility use a fresh build directory and its own sdkconfig. Changing
SDKCONFIG_DEFAULTS does not override values in an existing sdkconfig.

## USB installation and recovery

Use [USB_FLASHING.md](USB_FLASHING.md) for the current single-factory build,
verified package, application-only/model/full modes and private backups.
Never use an old build directory containing a dual-OTA sdkconfig. Full installation
is a layout migration, not an ordinary application update. Do not erase the chip.

## Host tests

With GCC/Clang, or Visual Studio x64 Native Tools prompt:
```powershell
cmake -S tests -B build-host -G Ninja
cmake --build build-host
ctest --test-dir build-host --output-on-failure
python -m pip install -r tools/requirements-dev.txt
python tools/check_project.py
```

The UI font is committed, so Node/font generation is not needed for normal builds.
Host font tests require the resolved managed LVGL component; run a firmware
configure/build first, or pass -D LVGL_SOURCE_DIR=/path/to/lvgl-8.3.11 to CMake.
CI fetches LVGL commit 396f70508480f79725fc1354cd290cc83f69b2ef for these tests.
Development formatting uses clang-format 19.1.7 and .clang-format; generated and
vendored files are excluded. CI validates all four firmware profiles.

Dashboard tests also require cJSON 1.7.18. With an installed IDF use:
`cmake -S tests -B build-host -D CJSON_SOURCE_DIR=/path/to/esp-idf/components/json/cJSON`.
Linux CI fetches cJSON commit 8f2beb57ddad1f94bed899790b00f46df893ccac and runs
POSIX CET/CEST boundary tests. Those boundaries are explicitly skipped by Windows
CRT, whose TZ parser does not support the embedded/newlib POSIX rule.
