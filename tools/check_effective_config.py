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
    "FLASH_HUMAN_FACE_DETECT_MSRMNP_S8_V1": profile == "recognition",
    "FLASH_ESPDET_PICO_224_224_FACE": False,
    "FLASH_ESPDET_PICO_416_416_FACE": True,
    "HUMAN_FACE_DETECT_MODEL_IN_FLASH_RODATA": True,
    "FREERTOS_SUPPORT_STATIC_ALLOCATION": True,
    "LV_FONT_MONTSERRAT_48": True,
    "ESP_HOSTED_ENABLED": profile in ("wifi", "recognition"),
    "HB_BOARD_PROFILE_CONFIRMED": profile != "diagnostics",
    "HB_WIFI_DIAGNOSTICS": profile in ("wifi", "recognition"),
    "HB_IDENTITY_EXPERIMENTAL": profile == "recognition",
    "HUMAN_FACE_FEAT_MODEL_IN_SDCARD": False,
    "HUMAN_FACE_FEAT_MODEL_IN_FLASH_PARTITION": profile == "recognition",
    "PARTITION_TABLE_CUSTOM_FILENAME": "partitions.identity.csv" if profile == "recognition" else "partitions.csv",
    "PARTITION_TABLE_OFFSET": 0x10000 if profile == "recognition" else 0x8000,
    "MQTT_PROTOCOL_5": True,
    "HUMAN_FACE_FEAT_MBF_S8_V1": True,
    "FLASH_HUMAN_FACE_FEAT_MFN_S8_V1": False,
    "FLASH_HUMAN_FACE_FEAT_MBF_S8_V1": True,
    "HB_IDENTITY_THRESHOLD_MILLI": 700,
    "HB_IDENTITY_MARGIN_MILLI": 120,
    "HB_IDENTITY_AGREEMENT_MILLI": 600,
}
if profile in ("wifi", "recognition"):
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
