# Flipper Dongle v0.5.5 — First Test Release

This is the first public/test milestone of Flipper Dongle.

## Working
- Flipper Bluetooth Remote keyboard
- Flipper Bluetooth Remote mouse
- BadUSB / BadKB over BLE
- BLE HID -> ESP32-S3 -> USB HID bridge
- keyboard press/release without stuck keys
- TinyUSB completion-synchronized USB HID transport
- multiple bonded HID profiles, up to 4 bonds
- CDC debug console

## Important
Bluetooth Remote and BadUSB/BadKB may appear as different BLE identities and may need to be paired separately.

## Known limitations
- Pair/reset button behavior still needs more repeated edge-case testing.
- CDC COM port is still present in this development/test build.
- HID/DRIVE switch and microSD MSC are not yet merged into this branch.
- LCD user interface is not yet implemented.

## Hardware tested
Waveshare ESP32-S3-LCD-1.47

## Status
First test release / hardware-tested baseline.
