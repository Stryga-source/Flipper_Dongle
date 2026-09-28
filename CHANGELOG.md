# Changelog

## Unreleased — Pocket-Dongle identification (documentation only)
- recorded actual-board photos and esptool identification: ESP32-S3 rev. v0.2, detected 16 MB flash, reported embedded 8 MB PSRAM, USB-Serial/JTAG on COM23
- full 16 MB factory flash backed up outside Git; USB HID and peripheral pin verification remain pending; firmware unchanged
- recorded the operator's tentative identification of the red component as an antenna; LED presence remains unverified

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
