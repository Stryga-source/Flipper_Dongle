# Flipper Dongle

[Русская версия](README_RU.md) · [Pocket firmware release](https://github.com/Stryga-source/Flipper_Dongle/releases/tag/pocket-hid-2026-09-29) · [Flashing guide](docs/releases/POCKET_HID_20260929.md)

Licensed under [MIT](LICENSE). Keep the license notice, including the link to
the [original repository](https://github.com/Stryga-source/Flipper_Dongle), when
redistributing the project.

Flipper Dongle is a standalone ESP32-S3 USB adapter that turns Flipper Zero Bluetooth HID profiles into a standard USB keyboard and mouse for a PC.

The PC does not need a driver, companion application, or custom Flipper application. The dongle acts as a BLE HID host on the Flipper side and as a standard USB HID device on the computer side.

This is a BLE HID bridge. qFlipper, USB file access, and Flipper firmware updates
through the dongle are not implemented; the [cable-like connection](docs/FLIPPER_CABLE_LIKE_CONNECTION.md)
is a research plan.

## Current status

**First test release: v0.5.5**

Working and tested:
- Flipper Zero `Bluetooth Remote -> Keyboard`
- keyboard input over BLE -> ESP32-S3 -> USB HID
- mouse input over BLE -> ESP32-S3 -> USB HID
- BadUSB / BadKB in BLE mode
- USB keyboard press/release without stuck keys
- USB mouse reports
- TinyUSB synchronized HID transmission
- support for multiple bonded BLE HID identities
- CDC serial debug interface for development

**Next test candidate: v0.5.6 — awaiting verification**

v0.5.6 adds a pairing-state race-condition fix. It is not yet promoted to the first stable/test baseline until repeated hardware testing is complete.

## Hardware

Primary development board:
- Waveshare ESP32-S3-LCD-1.47

The non-original `Pocket-Dongle-S3-0.96` is a working USB HID target. esptool detected an ESP32-S3, 16 MB flash and embedded 8 MB PSRAM; its full factory flash was backed up outside Git. Bluetooth Remote keyboard/mouse input, BadUSB operation and display scenes were tested on the actual board. BOOT on GPIO0 and the 160×80 screen were also tested. Repeated pairing/reset tests and microSD remain pending; see `docs/hardware/T_DONGLE_CLONE.md`.

A separate diagnostic passed a 64 KiB PSRAM write/read test. The board has no physical HID/DRIVE switch. The Pocket build is selected at build time, leaving the Waveshare configuration intact.

An original LILYGO T-Dongle-S3 has a separate [hardware-untested pre-release](https://github.com/Stryga-source/Flipper_Dongle/releases/tag/lilygo-t-dongle-s3-test-2026-09-29)
from the `codex/lilygo-board-profile` branch. Its profile is not in `main` and
its image must not be flashed to the Pocket or Waveshare board.

Development stack:
- ESP-IDF 6.1
- NimBLE
- TinyUSB

## Architecture

```text
Flipper Zero
   |
   | BLE HID
   v
ESP32-S3 Flipper Dongle
   |
   | USB HID Keyboard + Mouse
   v
PC
```

Flipper can keep its normal Bluetooth connection to a phone while the HID profile is used by the dongle when needed.

## BLE profiles

The project supports more than one Flipper HID identity.

Typical examples:

```text
Control <Flipper name>   -> Bluetooth Remote
BadUSB <Flipper name>    -> BadUSB / BadKB BLE
```

These profiles can use different BLE addresses and therefore may require separate bonds.

v0.5.5 supports up to 4 stored BLE HID bonds.

## Pairing controls

Current design target:
- short BOOT press: start one explicit pairing scan
- saved bond: automatic reconnect to a trusted HID profile
- long BOOT press: clear all stored HID bonds
- after clearing bonds: remain idle until a new short BOOT pairing request

The v0.5.6 candidate specifically improves race handling around bond reset, asynchronous disconnect, and a new pairing request.

## USB modes

Current development build exposes:
- HID Keyboard
- HID Mouse
- CDC COM port for logs/debugging

Planned final builds:
- **Debug**: HID + CDC COM
- **Release**: HID only, no visible COM port

A later milestone will merge the working BLE HID bridge with the previously tested HID/DRIVE hardware mode switch and microSD USB MSC mode.

## Build

For the hardware-tested Pocket-Dongle, download the ready-to-flash full or
application-only binary from the [Pocket HID release](https://github.com/Stryga-source/Flipper_Dongle/releases/tag/pocket-hid-2026-09-29).
The [flashing guide](docs/releases/POCKET_HID_20260929.md) gives the exact
addresses and explains when an update preserves BLE bonds. These binaries are
for the `Pocket-Dongle-S3-0.96` with 16 MB flash, not the Waveshare board.

Install ESP-IDF 6.1 and activate its environment using the `export.ps1` from
your own ESP-IDF installation. From the repository root, the default Waveshare
build is:

```powershell
idf.py set-target esp32s3
idf.py build
```

Pocket-Dongle build, from the repository root:

```powershell
idf.py -B build-pocket -DPOCKET_DONGLE=ON build
idf.py -B build-pocket -DPOCKET_DONGLE=ON -p COM_PORT flash
```

The Pocket profile uses `sdkconfig.pocket.defaults` and an independent generated `sdkconfig.pocket` with the measured 16 MB flash size. After flashing, the debug CDC interface appeared as COM22 on the tested PC; COM numbering can change. This HID target is tested and working. Pairing/reset stress tests, microSD and the PIN placard still need separate verification.

Replace `COM_PORT` with the bootloader port shown on your computer. Preserve a
factory flash backup before installing a full image on a new board.

The Pocket screen uses full-screen 160x80 RGB565 dolphin scenes based on the approved references. It cycles three search frames, keeps the Flipper target fixed during that animation, shows a found scene, and switches to keyboard or mouse art after the corresponding USB HID report is queued. BadUSB uses its own art based on the connected BLE identity. An unrecognized HID report shows joystick/other-device art but is **not** forwarded as a USB joystick.

For BLE display-passkey or numeric-comparison events, the six digits are drawn on the placard held by the dolphin. The image asset itself contains no fixed PIN. The current project-local NimBLE helper uses `123456` for its display-passkey action; numeric comparison uses the BLE event value. This UI change does not alter the pairing policy.

The new Pocket image was flashed with verified write hashes. After reconnecting without BOOT, the new art, three-frame search animation, keyboard/mouse switching and BadUSB operation were verified; Windows again exposed CDC COM22. The pairing-code screen has not yet been confirmed on hardware. Waveshare still builds from the same source tree.

## Version status

### v0.5.5 — First Test Release
This is the first version considered usable enough for wider hardware testing.

### v0.5.6 — Test Candidate
Pairing race-condition fix. Awaiting repeated verification before promotion.

## AI-assisted development

The project is developed with the help of GPT, Codex and other AI tools. Project goals, product decisions and hardware validation come from a human; AI assists with code analysis, implementation, debugging and documentation.
