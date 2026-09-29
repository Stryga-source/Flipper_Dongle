# Flipper Dongle

Flipper Dongle is a standalone ESP32-S3 USB adapter that turns Flipper Zero Bluetooth HID profiles into a standard USB keyboard and mouse for a PC.

The PC does not need a driver, companion application, or custom Flipper application. The dongle acts as a BLE HID host on the Flipper side and as a standard USB HID device on the computer side.

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

The non-original `Pocket-Dongle-S3-0.96` is a working USB HID target. esptool detected an ESP32-S3, 16 MB flash and embedded 8 MB PSRAM; its full factory flash was backed up outside Git. The operator confirmed Bluetooth Remote keyboard/mouse input, BadUSB operation and display scenes on the actual board. BOOT on GPIO0 and the 160×80 screen were also tested. Repeated pairing/reset tests and microSD remain pending; see `docs/hardware/T_DONGLE_CLONE.md`.

A separate diagnostic passed a 64 KiB PSRAM write/read test. The board has no physical HID/DRIVE switch. The Pocket build is selected at build time, leaving the Waveshare configuration intact.

An original LILYGO T-Dongle-S3 has its own **build-tested, hardware-untested**
profile. Its display pins differ from Pocket's. See the
[LILYGO board notes](docs/hardware/LILYGO_T_DONGLE_S3.md); the Pocket release
binary must not be used for this board.

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
application-only binary from the [Pocket HID release](https://github.com/stryginuv/Flipper_Dongle/releases/tag/pocket-hid-2026-09-29).
The [flashing guide](docs/releases/POCKET_HID_20260929.md) gives the exact
addresses and explains when an update preserves BLE bonds. These binaries are
for the `Pocket-Dongle-S3-0.96` with 16 MB flash, not the Waveshare board.

Windows PowerShell:

```powershell
$env:IDF_TOOLS_PATH="C:\Espressif\tools"
C:\esp\v6.1\esp-idf\export.ps1

idf.py set-target esp32s3
idf.py build
idf.py flash
```

Pocket-Dongle build, from the repository root:

```powershell
idf.py -B build-pocket -DPOCKET_DONGLE=ON build
idf.py -B build-pocket -DPOCKET_DONGLE=ON -p COM23 flash
```

The Pocket profile uses `sdkconfig.pocket.defaults` and an independent generated `sdkconfig.pocket` with the measured 16 MB flash size. After flashing, the debug CDC interface appeared as COM22 on the tested PC; COM numbering can change. The operator considers this HID target working. Pairing/reset stress tests, microSD and the PIN placard still need separate verification.

Original LILYGO T-Dongle-S3 build candidate:

```powershell
idf.py -B build-lilygo -DLILYGO_T_DONGLE_S3=ON build
```

It uses `sdkconfig.lilygo.defaults` and `sdkconfig.lilygo`. Pocket, Waveshare,
and LILYGO configurations compile; LILYGO has not been flashed or tested on a
physical board. Different LILYGO models/revisions need their own profile.

The Pocket screen uses full-screen 160x80 RGB565 dolphin scenes based on the operator-approved references. It cycles three search frames, keeps the Flipper target fixed during that animation, shows a found scene, and switches to keyboard or mouse art after the corresponding USB HID report is queued. BadUSB uses its own art based on the connected BLE identity. An unrecognized HID report shows joystick/other-device art but is **not** forwarded as a USB joystick.

For BLE display-passkey or numeric-comparison events, the six digits are drawn on the placard held by the dolphin. The image asset itself contains no fixed PIN. The current project-local NimBLE helper uses `123456` for its display-passkey action; numeric comparison uses the BLE event value. This UI change does not alter the pairing policy.

The new Pocket image was flashed with verified write hashes. After reconnecting without BOOT, the operator confirmed the new art, three-frame search animation, keyboard/mouse switching and BadUSB operation; Windows again exposed CDC COM22. The pairing-code screen has not yet been confirmed on hardware. Waveshare still builds from the same source tree.

## Version status

### v0.5.5 — First Test Release
This is the first version considered usable enough for wider hardware testing.

### v0.5.6 — Test Candidate
Pairing race-condition fix. Awaiting repeated verification before promotion.

## Project scope

Flipper Dongle is a standalone project. It is intentionally separate from Flipper Life / Flipper Chimera.

## AI-assisted development

The project is developed with the help of GPT, Codex and other AI tools. Project goals, product decisions and hardware validation come from a human; AI assists with code analysis, implementation, debugging and documentation.
