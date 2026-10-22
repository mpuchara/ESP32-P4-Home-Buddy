"""Reject silently ignored GPIO/profile defaults in the resolved ESP-IDF config."""
import json
from pathlib import Path
import sys

config = json.loads(Path(sys.argv[1]).read_text())
profile = sys.argv[2]
expected = {
    "IDF_TARGET": "esp32p4",
    "ESPTOOLPY_FLASHSIZE": "16MB",
    "LV_COLOR_DEPTH": 16,
    "ESP_HOSTED_ENABLED": profile == "wifi",
    "HB_BOARD_PROFILE_CONFIRMED": profile != "diagnostics",
    "HB_WIFI_DIAGNOSTICS": profile == "wifi",
}
if profile == "wifi":
    expected.update({
        "ESP_HOSTED_SDIO_SLOT": 1,
        "ESP_HOSTED_SDIO_CLOCK_FREQ_KHZ": 20000,
        "ESP_HOSTED_SDIO_PIN_CLK": 18,
        "ESP_HOSTED_SDIO_PIN_CMD": 19,
        "ESP_HOSTED_SDIO_PIN_D0": 14,
        "ESP_HOSTED_SDIO_PIN_D1": 15,
        "ESP_HOSTED_SDIO_PIN_D2": 16,
        "ESP_HOSTED_SDIO_PIN_D3": 17,
        "ESP_HOSTED_GPIO_SLAVE_RESET_SLAVE": 54,
    })
for key, value in expected.items():
    actual = config.get(key, False)
    if actual != value:
        raise SystemExit(f"{key}: expected {value!r}, got {actual!r}")
print(f"Effective {profile} configuration verified")
