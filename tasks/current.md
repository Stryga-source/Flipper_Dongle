# Current Tasks — Flipper Dongle

## Priority 0 — Preserve the working baseline

Do not break `main` / `v0.5.5-first-test`.

The first working keyboard+mouse+BadKB bridge is already achieved.

Before experimental work, use a dedicated branch.

---

## Priority 1 — Bring up Pocket-Dongle-S3-0.96 safely

The new board has been visually identified from the actual PCB photos as `Pocket-Dongle-S3-0.96` with an ESP32-S3 MCU, integrated USB-A, 0.96-inch display, microSD socket, one tactile button, and exposed edge pads.

### First rule

**Do not erase or overwrite the factory firmware before identification and backup.**

### Phase A — non-destructive identification

1. Connect the board to Windows.
2. Record Device Manager / USB VID/PID / device names.
3. Enter ESP32-S3 ROM bootloader if required.
4. Run:

```powershell
esptool.py --chip esp32s3 chip_id
esptool.py --chip esp32s3 flash_id
```

5. Record actual flash manufacturer and size.
6. Determine whether PSRAM exists; do not assume an Internet-listed N16R8/N8 variant.
7. Back up the complete factory flash before project firmware is written.

### Phase B — peripheral verification

Using `docs/hardware/T_DONGLE_CLONE.md`:

- verify native USB routing / HID enumeration
- identify tactile button GPIO
- verify display controller and candidate TFT pinout
- verify microSD interface and candidate pinout
- determine backlight pin if needed
- determine any LED/status output if present

A matching public Pocket-Dongle-S3 project provides candidate pin assignments. They are **REFERENCE values only** until tested on this exact PCB.

### Deliverables

- update `docs/hardware/T_DONGLE_CLONE.md`
- replace REFERENCE/UNKNOWN entries with VERIFIED values as tests succeed
- add a dedicated Pocket-Dongle board profile
- keep Waveshare ESP32-S3-LCD-1.47 build working

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

After Pocket-Dongle verification, refactor board-specific pins/peripherals so both targets can coexist:

- Waveshare ESP32-S3-LCD-1.47
- Pocket-Dongle-S3-0.96

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

Only after pairing and Pocket-Dongle board support are stable.

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
