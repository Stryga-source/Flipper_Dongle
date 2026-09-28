# Changelog

## Unreleased — Pocket-Dongle bring-up
- recorded actual-board photos and esptool identification: ESP32-S3 rev. v0.2, detected 16 MB flash, reported embedded 8 MB PSRAM, USB-Serial/JTAG on COM23
- full 16 MB factory flash backed up outside Git; USB HID and peripheral pin verification remain pending; firmware unchanged
- recorded the operator's tentative identification of the red component as an antenna; LED presence remains unverified
- clearer photo confirms the `BOOT` button silkscreen and supports, without proving, an RF antenna identification
- added and flashed a separate ESP-IDF 6.1 first-boot diagnostic; 16 MB flash reported and a 64 KiB PSRAM write/read test passed on the actual board
- recorded that the Pocket-Dongle has no physical HID/DRIVE switch; BLE HID core remains unchanged
- added an isolated ST7735 status display for the Pocket build; diagnostic text appeared on the actual screen and BOOT press/release confirmed GPIO0
- built and flashed the experimental Pocket BLE bridge; Windows enumerated USB HID keyboard, mouse and CDC debug COM22; the operator confirmed keyboard and mouse input from Flipper Bluetooth Remote
- Pocket build uses a separate generated sdkconfig with the measured 16 MB flash size; Waveshare build still compiles
- added a Pocket-only dolphin scene with keyboard/mouse icon selected from the last queued HID report; both board builds pass, hardware display test pending

## v0.5.6 — Test Candidate
- pairing/reset race-condition fix
- deferred pairing after asynchronous disconnect
- stale BLE scan invalidation
- awaiting hardware verification

## v0.5.5 — First Test Release
- first usable hardware-tested BLE HID -> USB HID bridge
- keyboard and mouse working
- BadUSB / BadKB BLE working
- synchronized TinyUSB HID transport
- multi-profile BLE bonding
