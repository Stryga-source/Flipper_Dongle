# Current Tasks — Flipper Dongle

## Priority 0 — Preserve the working baseline

Do not break `main` / `v0.5.5-first-test`.

The first working keyboard+mouse+BadKB bridge is already achieved.

Before experimental work, use a dedicated branch.

---

## Priority 1 — Identify the new T-Dongle clone

A non-original T-Dongle-style board has arrived.

### Goal

Create a verified hardware profile for the clone before attempting to port the Flipper Dongle firmware.

### Required inputs from the user / photos

When images or markings are available, determine:

- MCU exact marking
- flash/PSRAM package or module marking
- USB connector routing
- display controller marking
- display resolution
- display interface and GPIOs
- BOOT/user button GPIO
- RGB/status LED type and GPIO
- microSD presence and wiring
- power/regulator details if relevant

### Deliverables

Create/update:

- `docs/hardware/T_DONGLE_CLONE.md`
- a pinout table
- confidence level for every inferred pin
- notes on which details are measured/verified vs inferred

Do not assume LILYGO T-Dongle-S3 compatibility just because the board looks similar.

---

## Priority 2 — Hardware-test v0.5.6 pairing fix

Branch:

`v0.5.6-test-candidate`

Test sequence several times:

1. Pair Bluetooth Remote.
2. Long BOOT to clear all bonds.
3. Immediately short BOOT.
4. Repeat with 1–2 s delay before short BOOT.
5. Repeat while the Flipper HID profile is already advertising.
6. Repeat with BadUSB/BadKB BLE identity.

Expected behavior:

- every short BOOT either starts pairing immediately or is explicitly deferred until disconnect completes
- no stale scan reconnects after bond deletion
- existing keyboard and mouse functionality remains unchanged

If the test passes, prepare the next baseline/release from the candidate.

If it fails, preserve full serial logs and fix only the pairing state machine; do not touch the known-good USB HID transport unless evidence requires it.

---

## Priority 3 — Add board abstraction

After clone identification, refactor board-specific pins/peripherals so both targets can coexist:

- Waveshare ESP32-S3-LCD-1.47
- T-Dongle clone

Keep BLE HID core shared.

Prefer build-time board selection instead of scattered `#ifdef` pin definitions.

---

## Priority 4 — Debug and Release USB variants

Create two reproducible configurations:

### Debug

- Keyboard HID
- Mouse HID
- CDC COM logs

### Release

- Keyboard HID
- Mouse HID
- no CDC interface
- no visible COM port

Do not remove debugging capability from the Debug build.

---

## Priority 5 — Merge HID/DRIVE mode

Only after pairing and clone board support are stable.

Bring forward the previously tested v0.3 behavior:

- boot-time HID/DRIVE selector
- HID mode = BLE HID bridge
- DRIVE mode = microSD USB MSC

Do not runtime-hot-switch USB descriptors unless deliberately redesigned and tested.

---

## Priority 6 — UI

After core behavior is reliable:

- boot state
- idle
- scanning/pairing
- HID found
- connecting
- connected
- disconnected
- pairing reset
- DRIVE mode
- error

Add animations/status graphics only after board/display support is verified.
