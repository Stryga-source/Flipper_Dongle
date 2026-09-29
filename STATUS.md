# Flipper Dongle — Current Status

Last updated: 2026-09-29

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

A matching public Pocket-Dongle-S3 project supplied candidate display pins. A separate diagnostic rendered readable text on this exact screen, and BOOT press/release changed the on-screen GPIO0 message. See `docs/hardware/T_DONGLE_CLONE.md` for the remaining hardware limits.

Identification results and remaining checks:

- esptool v5.3.1 on the actual board detected ESP32-S3 QFN56 rev. v0.2, 16 MB flash (`0x20:0x4018`) and embedded 8 MB PSRAM; a complete 16 MB flash read succeeded
- a separate ESP-IDF 6.1 diagnostic was built, flashed, and observed running on the Pocket-Dongle: 16,777,216 flash bytes reported and four consecutive 64 KiB PSRAM write/read PASS heartbeats; full 8 MB PSRAM integrity is untested
- Windows enumerated `USB\\VID_303A&PID_1001&MI_00` on `COM23`; esptool reported USB-Serial/JTAG mode
- experimental Pocket bridge with display built and flashed; Windows enumerated HID keyboard, HID mouse, and CDC COM22 (VID:PID 303A:4005). The operator confirmed that Flipper Bluetooth Remote connected and both keyboard and mouse input worked on the PC.
- the Pocket display revision with a dolphin and keyboard/mouse icon was flashed after the operator entered ROM BOOT mode. The write hash verified, and the app restarted with HID keyboard, HID mouse, and CDC COM22 enumerated. Both Pocket and Waveshare compile; physical confirmation of the two drawings is pending.
- on `codex/pocket-screen-scenes`, the operator-approved nine reference pictures were adapted to 160x80 RGB565, with a tenth idle frame. The Pocket and Waveshare configurations both compile. Pocket flashing on COM23 verified all written hashes on 2026-09-29. After a BOOT-free USB reconnect, the operator confirmed the new pictures, all three scan frames, and keyboard/mouse scene switching; Windows exposed CDC COM22 (`303A:4005`).
- Pocket UI also has BadUSB art by BLE identity, other-device art for unrecognized HID reports, and a BLE pairing-code placard. The other-device scene does not add joystick USB forwarding. The operator confirmed BadUSB works on this build: CDC logs show a `BadUSB` HID connection and USB keyboard reports queued. The pairing-code screen has not yet been tested by the operator.
- A short BOOT press during an already-running automatic scan is serviced after that 5-second scan completes; this explains the initial apparent BadUSB delay in the 2026-09-29 test. No BLE core change was made for it.
- `BOOT` button GPIO0 verified through diagnostic press/release; short/long pairing behavior still needs repeated testing
- display SPI configuration produced readable `LCD TEST` on this board; controller package marking and backlight control remain unknown
- exact microSD wiring on this revision
- the visible red component is consistent with a small RF antenna in the clearer photo; exact type/function is unverified, and no separate LED/status GPIO has been identified
- the operator confirms no physical HID/DRIVE switch on this board; the older SPDT selector design is not applicable to this target

The full 16 MB factory flash was backed up outside Git on 2026-09-28 using esptool `--no-stub`. The file size and SHA-256 were checked, and its first 1 MB matches an independent read. The experimental Pocket build uses a separate 16 MB sdkconfig; the original Waveshare build was rebuilt successfully. Preserve the factory backup for recovery.

## Near-term order

1. Completed: non-destructive MCU/flash identification, Windows USB enumeration, and complete factory backup.
2. Completed: ESP-IDF boot, flash size report, a 64 KiB PSRAM test, native USB HID keyboard/mouse enumeration, and operator-confirmed keyboard/mouse input through the Pocket bridge.
3. Completed: display text and BOOT GPIO0 verification. Next: repeated pairing/reset tests, microSD pinout, and final board profile.
4. Verify v0.5.6 pairing behavior on the existing Waveshare board.
5. If v0.5.6 passes, promote pairing fix into the next baseline.
6. Add Debug vs Release USB configurations.
7. Merge HID/DRIVE + microSD MSC.
8. Experimental Pocket status screen is present; refine after connection testing.
