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

The newly arrived non-original T-Dongle-style device has now been visually identified from the actual PCB photos as:

- PCB silkscreen: `Pocket-Dongle-S3-0.96`
- MCU marking: **ESP32-S3**
- integrated USB Type-A male plug
- integrated 0.96-inch display
- microSD socket on the back
- one tactile button near USB
- exposed edge GPIO/test pads

This is **not** to be treated as an original LILYGO T-Dongle-S3.

A matching public Pocket-Dongle-S3 project was found with schematics/examples and candidate pin assignments. Those values are documented in `docs/hardware/T_DONGLE_CLONE.md` as **REFERENCE**, not yet verified on our exact PCB.

Important unknowns that still require measurement/test:

- actual flash size
- PSRAM presence/size
- native USB routing confirmation
- tactile button GPIO
- exact display wiring/controller confirmation on this revision
- exact microSD wiring on this revision

**Do not overwrite factory firmware before recording USB enumeration, reading flash ID and backing up the original flash image.**

## Near-term order

1. Connect the Pocket-Dongle-S3-0.96 and perform non-destructive identification / factory backup.
2. Verify actual flash/PSRAM and USB behavior.
3. Verify candidate display/button/microSD pinout and create a dedicated board profile.
4. Verify v0.5.6 pairing behavior on the existing Waveshare board.
5. If v0.5.6 passes, promote pairing fix into the next baseline.
6. Add Debug vs Release USB configurations.
7. Merge HID/DRIVE + microSD MSC.
8. Add display/status UI.
