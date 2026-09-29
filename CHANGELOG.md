# Changelog

## Unreleased — Pocket-Dongle bring-up
- operator accepted the Pocket HID dongle as working; preserved `v0.5.5-first-test` as rollback, integrated the tested Pocket branch into `main`, and replaced an incomplete reference ZIP with the full approved montage
- adapted the operator-approved dolphin montage into ten 160x80 RGB565 Pocket scenes (including derived idle art); three search frames keep the Flipper target stationary
- connected keyboard, mouse, BadUSB, found, other-device and BLE pairing-code scenes to Pocket status events; unknown HID reports remain unforwarded, and the USB HID transport is unchanged
- generated the pairing placard without fixed digits and draw the existing NimBLE display/numeric-comparison value at runtime
- Pocket and Waveshare builds passed; Pocket flash write hashes verified; operator confirmed the new art, scan animation, keyboard/mouse switching and BadUSB operation after USB reconnect. Pairing code still needs a hardware check
- recorded actual-board photos and esptool identification: ESP32-S3 rev. v0.2, detected 16 MB flash, reported embedded 8 MB PSRAM, USB-Serial/JTAG on COM23
- full 16 MB factory flash backed up outside Git; USB HID and peripheral pin verification remain pending; firmware unchanged
- recorded the operator's tentative identification of the red component as an antenna; LED presence remains unverified
- clearer photo confirms the `BOOT` button silkscreen and supports, without proving, an RF antenna identification
- added and flashed a separate ESP-IDF 6.1 first-boot diagnostic; 16 MB flash reported and a 64 KiB PSRAM write/read test passed on the actual board
- recorded that the Pocket-Dongle has no physical HID/DRIVE switch; BLE HID core remains unchanged
- added an isolated ST7735 status display for the Pocket build; diagnostic text appeared on the actual screen and BOOT press/release confirmed GPIO0
- built and flashed the experimental Pocket BLE bridge; Windows enumerated USB HID keyboard, mouse and CDC debug COM22; the operator confirmed keyboard and mouse input from Flipper Bluetooth Remote
- Pocket build uses a separate generated sdkconfig with the measured 16 MB flash size; Waveshare build still compiles
- added a Pocket-only dolphin scene with keyboard/mouse icon selected from the last queued HID report; both board builds pass and Pocket flashed with USB HID re-enumeration; drawing confirmation pending

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
