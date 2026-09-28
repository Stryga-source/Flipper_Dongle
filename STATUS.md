# Flipper Dongle — Current Status

Last updated: 2026-09-28

## Overall

The project has reached the first working BLE HID -> USB HID milestone.

Current known-good baseline:

- branch: `main`
- preserved branch: `v0.5.5-first-test`
- version: `v0.5.5`
- status: **first hardware-tested test release**

Current next candidate:

- branch: `v0.5.6-test-candidate`
- version: `v0.5.6`
- status: **awaiting hardware verification**

## Confirmed working on hardware

Using Waveshare ESP32-S3-LCD-1.47:

- Flipper Bluetooth Remote keyboard -> dongle -> USB keyboard
- Flipper Bluetooth Remote mouse -> dongle -> USB mouse
- BadUSB / BadKB BLE -> dongle -> USB HID
- keyboard press and release reports work without permanent stuck keys after USB TX synchronization fix
- mouse works after converting Flipper mouse payload to TinyUSB mouse report
- CDC debug console works alongside HID

## Important technical discoveries

### ESP-IDF 6.1 NimBLE HID Host

The stock HID host path required project-local fixes for Flipper HID:

- Report Map handling/subscription issue in NimBLE HID host
- scanner filtering tied to Espressif demo names

The repository contains local patch logic. Do not modify the globally installed ESP-IDF tree.

### USB HID transport

The keyboard stuck-key problem was caused by the next report being attempted before the previous TinyUSB HID IN transfer had completed.

Known-good solution:

- serialize keyboard and mouse reports through one HID IN endpoint
- release the TX gate from `tud_hid_report_complete_cb()`
- do not drop release reports

Do not regress this behavior.

### Flipper BLE identities

Normal Bluetooth Remote and BadUSB/BadKB may advertise as different BLE identities.

Observed/source-backed model:

- Bluetooth Remote: typically `Control <Flipper name>`
- BadUSB/BadKB BLE: typically `BadUSB <Flipper name>` and may use a different MAC

Therefore v0.5.5 supports multiple bonds (`CONFIG_BT_NIMBLE_MAX_BONDS=4`).

## v0.5.6 candidate

Purpose: fix intermittent pairing-button behavior after clearing bonds.

Symptom:

- long BOOT clears bonds
- a following short BOOT is visible in logs
- sometimes no pairing scan starts

Likely race handled in v0.5.6:

- `esp_hidh_dev_close()` is asynchronous
- late `ESP_HIDH_CLOSE_EVENT` can overwrite a new pairing request
- a blocking scan may return stale results after a reset

v0.5.6 adds:

- reset-in-progress state
- deferred short-BOOT pairing request
- scan generation counter
- stale scan invalidation

**Not yet confirmed on hardware.**

## USB product variants

Current development mode:

- HID Keyboard
- HID Mouse
- CDC COM debug interface

Planned final variants:

1. **Debug** — HID + CDC COM
2. **Release** — HID only, no visible COM port

Do not remove CDC from the only debug-capable build while active development continues.

## Previous proven subsystem to merge later

An earlier `v0.3` hardware-mode prototype was validated separately:

- HID mode
- DRIVE mode
- boot-time SPDT selector
- microSD USB MSC

This should be merged only after BLE HID/pairing is stable.

## New hardware target: Pocket-Dongle-S3-0.96

The newly arrived T-Dongle-style board is now identified as `Pocket-Dongle-S3-0.96`.

### Verified on the actual board with esptool v5.3.1

- MCU: **ESP32-S3 QFN56 rev v0.2**
- 240 MHz dual-core + LP core
- Wi-Fi + Bluetooth 5 LE
- crystal: **40 MHz**
- embedded PSRAM: **8 MB (AP_3v3)**
- Flash: **16 MB**
- Flash JEDEC raw ID: manufacturer `0x20`, device `0x4018`
- Flash bus: **quad / 4 data lines**
- Flash voltage: **3.3 V**
- USB mode reported by ESP32-S3: **USB-Serial/JTAG**
- enumerates on Windows as **COM23** through the ESP32-S3 native USB path

This is especially important because the matching public Pocket-Dongle-S3 reference documents an 8 MB Flash revision. Our actual board is therefore a different memory revision and must use its measured 16 MB configuration.

### Present on the PCB

- integrated USB Type-A male plug
- integrated 0.96-inch display
- microSD socket
- one tactile button near USB
- exposed edge GPIO/test pads

### Still not hardware-verified

- tactile button GPIO
- exact display controller/wiring on this revision
- exact microSD wiring/mode on this revision
- TinyUSB HID enumeration over the Type-A connector

A matching public Pocket-Dongle-S3 project with schematics/examples and candidate display/SD pins is documented in `docs/hardware/T_DONGLE_CLONE.md`. Treat those pins as **REFERENCE** until tested on this exact board.

**Do not erase or overwrite factory firmware until a complete 16 MB backup has been made and verified by SHA256.**

## Near-term order

1. Make and verify two matching full 16 MB factory Flash dumps from the Pocket-Dongle-S3-0.96.
2. Verify candidate display/button/microSD pinout using minimal diagnostics.
3. Build a dedicated Pocket-Dongle board profile for ESP-IDF 6.1 with 16 MB Flash + 8 MB PSRAM.
4. Confirm TinyUSB HID enumeration over the native Type-A USB path.
5. Verify v0.5.6 pairing behavior on the existing Waveshare board.
6. If v0.5.6 passes, promote pairing fix into the next baseline.
7. Add Debug vs Release USB configurations.
8. Merge HID/DRIVE + microSD MSC.
9. Add display/status UI.
