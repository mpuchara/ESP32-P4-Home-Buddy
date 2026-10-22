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

## First installation

List ports with `python -m serial.tools.list_ports -v`. For this session, P4 is COM5.
Close monitors before opening the port from another program.

```powershell
New-Item -ItemType Directory -Force local
python -m esptool --chip esp32p4 --port COM5 --before usb_reset flash_id
python -m esptool --chip esp32p4 --port COM5 --before usb_reset read_flash 0 0x1000000 local/original-p4-flash.bin
Get-FileHash local/original-p4-flash.bin -Algorithm SHA256
idf.py -B build-bringup -p COM5 flash monitor
```

Keep the full backup private and copy it somewhere durable before cleaning local
build files. Verify backup length is 16,777,216 bytes. Do not publish it.
Flashing a new partition table can invalidate existing vendor configuration;
the backup allows whole-image recovery. Never use erase_flash during routine updates.
No operation above writes C6.

## Restore original P4 image

Use only your own verified full backup for the same device and correct flash size.
```powershell
python -m esptool --chip esp32p4 --port COM5 --before usb_reset write_flash 0 local/original-p4-flash.bin
```
If USB cannot connect, enter P4 ROM download mode with its BOOT/reset procedure
from the actual board manual, then retry. Do not confuse the P4 and C6 BOOT pins.

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
vendored files are excluded. CI validates all three firmware profiles.
