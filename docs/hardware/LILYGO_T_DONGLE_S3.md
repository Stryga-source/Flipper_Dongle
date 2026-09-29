# Original LILYGO T-Dongle-S3 board profile

Status: **build-tested candidate; no LILYGO hardware is available to the
operator, so USB, BLE, BOOT, and display behavior are unverified on this board.**

This profile targets the original ESP32-S3 `T-Dongle-S3`. It is separate from
the operator's `Pocket-Dongle-S3-0.96` clone and from LILYGO T-Dongle-S2,
T-Dongle-S3-Dual, and T-Dongle-S3-Plus. Never flash the Pocket binary to a
LILYGO board merely because both have a 160x80 screen.

## Source-backed configuration

LILYGO's [T-Dongle-S3 display setup](https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/lib/TFT_eSPI/User_Setups/Setup209_LilyGo_T_Dongle_S3.h)
lists ST7735, 80x160, green-tab 160x80 mode, BGR colors, and these SPI pins:

| Signal | Original LILYGO GPIO | Pocket GPIO |
| --- | ---: | ---: |
| SCLK | 5 | 10 |
| MOSI | 3 | 11 |
| CS | 4 | 12 |
| DC | 2 | 13 |
| RST | 1 | 14 |

LILYGO's [product documentation](https://github.com/Xinyuan-LilyGO/documentation/blob/master/en/products/t-dongle-series/t-dongle-s3/index.md)
lists ESP32-S3, 16 MB flash, BOOT on GPIO0, and native USB Type-A. Published
LILYGO pages disagree about PSRAM on T-Dongle-S3 variants. This candidate
does not enable PSRAM; identify the actual revision before changing memory
settings. No SD-card or LED GPIO is used by this firmware.

The firmware reuses the Pocket 160x80 scene assets and ST7735 initialization
with the LILYGO pin map. Their visual appearance, backlight behavior, and
controller compatibility require a real-board test. The shared BLE HID and USB
HID transport code is unchanged.

## Build

Use ESP-IDF 6.1 with a fresh build directory:

```powershell
idf.py -B build-lilygo -DLILYGO_T_DONGLE_S3=ON build
```

This uses `sdkconfig.lilygo.defaults` and a separate generated
`sdkconfig.lilygo`. Do not combine `LILYGO_T_DONGLE_S3` and `POCKET_DONGLE`.
The output is a **test candidate**, not a hardware-verified release. Preserve
the original board's factory flash before a first test.

## Required first hardware test

1. Identify exact board marking/revision, flash size, PSRAM, and USB port.
2. Back up factory flash and verify the backup before writing.
3. Flash the candidate, reconnect without BOOT, and check USB HID + CDC
   enumeration, screen orientation/colors, and BOOT scan/reset behavior.
4. Test Flipper Bluetooth Remote keyboard and mouse plus BadUSB/BadKB.
5. Repeat the pairing and reconnect sequence; report serial logs and photos.

Only after those checks should this profile be called hardware-supported.
