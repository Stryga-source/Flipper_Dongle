# Current Tasks — Flipper Dongle

The public repository and both firmware releases are online. Publication did
not change firmware behavior or complete the remaining hardware checks.

The tested Pocket HID build has a ready-to-flash package documented in
`docs/releases/POCKET_HID_20260929.md`: full image at `0x0`, application-only
update at `0x10000`. Packaging does not close the remaining hardware checks.

## Priority 0 — Preserve the working baseline

Keep `main` working and preserve `v0.5.5-first-test` as the first-test rollback point.

The first working keyboard+mouse+BadKB bridge is already achieved.

Before experimental work, use a dedicated branch.

---

## Priority 1 — Finish Pocket-Dongle-S3-0.96 verification

The new board has been visually identified from the actual PCB photos as `Pocket-Dongle-S3-0.96` with an ESP32-S3 MCU, integrated USB-A, 0.96-inch display, microSD socket, one tactile button, and exposed edge pads.

### First rule

**Do not erase or overwrite the factory firmware before identification and backup.**

Progress on 2026-09-28: photos confirm the `Pocket-Dongle-S3-0.96` marking, ESP32-S3 package, USB-A plug, display assembly, tactile button and microSD socket. Operator-provided esptool v5.3.1 output on `COM23` identifies ESP32-S3 QFN56 rev. v0.2, 16 MB flash and reported embedded 8 MB PSRAM. Windows enumerated `USB\\VID_303A&PID_1001&MI_00` on `COM23`; esptool reported USB-Serial/JTAG mode. A full 16 MB factory flash backup was saved outside Git with checked size and SHA-256; its first 1 MB matches a separate read. The operator suspects the visible red component is a capacitive antenna, but this is unverified. Later tests verified display SPI, BOOT GPIO0, USB HID enumeration, and keyboard/mouse input; microSD and any LED/status output remain unverified.

A clearer rear photo shows `BOOT` silkscreen beside the tactile button. GPIO0 was later verified by a physical press/release test. The red edge component looks consistent with a small RF chip antenna, but its exact type is not confirmed; any LED/status output remains unknown.

### Phase A — non-destructive identification

1. Completed: connect the board to Windows (`COM23` for this session).
2. Completed: record Windows device name and `USB\\VID_303A&PID_1001&MI_00`.
3. Completed: esptool reached the ESP32-S3 ROM loader.
4. Completed: run chip and flash identification; equivalent current syntax:

```powershell
python -m esptool --chip esp32s3 -p COM23 chip-id
python -m esptool --chip esp32s3 -p COM23 flash-id
```

5. Completed: flash manufacturer/device IDs `0x20:0x4018` and detected size 16 MB recorded from esptool; a complete 16 MB read succeeded.
6. Partially completed: esptool reports embedded 8 MB PSRAM, and an ESP-IDF diagnostic passed a 64 KiB write/read test. Full 8 MB integrity remains untested.
7. Completed: full 16 MB factory flash backed up outside Git; size and SHA-256 checked, first 1 MB matched a separate read. Preserve the file before project firmware is written.

First firmware experiment completed: `diagnostics/pocket_dongle_probe` built under ESP-IDF 6.1, flashed to the actual board, and emitted four consecutive 64 KiB PSRAM test PASS heartbeats with a 16,777,216-byte flash report. This verifies a small PSRAM region, not all 8 MB or USB HID.

### Phase B — peripheral verification

Using `docs/hardware/T_DONGLE_CLONE.md`:

- verify native USB routing / HID enumeration
- identify tactile button GPIO
- verify display controller and candidate TFT pinout
- verify microSD interface and candidate pinout
- determine backlight pin if needed
- determine any LED/status output if present
- account for the operator-confirmed absence of a physical HID/DRIVE selector; do not assign an SPDT switch GPIO to this board

A matching public Pocket-Dongle-S3 project provides candidate pin assignments. They are **REFERENCE values only** until tested on this exact PCB.

Progress: the separate diagnostic now renders `LCD TEST` on the actual display using the reference ST7735R 160x80 configuration and GPIO10–14. The operator confirmed readable text. The same diagnostic sampled GPIO0; pressing/releasing the physical BOOT button changed the on-screen message to `BOOT DOWN`/`RELEASED`. Those functions are now verified for this board. USB HID enumeration and input were subsequently verified; microSD is still pending.

The Pocket bridge is working on the actual board. Windows enumerated USB HID keyboard, mouse and CDC COM22. The operator confirmed Bluetooth Remote keyboard/mouse input, BadUSB operation, and the full-screen scan/keyboard/mouse scenes. Pocket and Waveshare builds pass. The 2026-09-29 Pocket write hashes verified and the app started after a BOOT-free USB reconnect.

Remaining Pocket checks: broader repeated pairing/reset behavior, the other-device scene, microSD and any LED/status output. The joystick picture marks an unrecognized HID report; joystick USB forwarding is not implemented. During the BadUSB test, short BOOT landed during an automatic 5-second scan, so the explicit pairing scan started after that scan completed.

### Deliverables

- update `docs/hardware/T_DONGLE_CLONE.md`
- replace REFERENCE/UNKNOWN entries with VERIFIED values as tests succeed
- add a dedicated Pocket-Dongle board profile
- keep Waveshare ESP32-S3-LCD-1.47 build working

---

Experimental Pocket integration on `codex/pocket-pairing-056` built for Pocket
and Waveshare on 2026-09-29. The Pocket app-only image was flashed on
COM23 with a verified write hash. The operator confirmed a repeat connection
after bond reset and a new scan. The missing on-screen pairing code was traced
to the HID Host passkey callback. The first project-local fix was flashed and
the operator confirmed matching digits, but reported that the placard vanished
too quickly. The final fix holds it until the BLE pairing result. Both board
builds pass, and the operator confirmed the Pocket code stayed visible until
Flipper confirmation. Broader repeated pairing/reset and BadUSB tests remain.
See `docs/releases/POCKET_PAIRING_056_TEST_20260929.md`.

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

The Pocket-Dongle has no physical HID/DRIVE switch. The earlier v0.3 SPDT selector cannot be carried over as a Pocket-Dongle board setting; any later DRIVE selection needs a separate design.

Bring forward the previously tested v0.3 behavior:

- boot-time HID/DRIVE selector
- HID mode = BLE HID bridge
- DRIVE mode = microSD USB MSC

Do not runtime-hot-switch USB descriptors unless deliberately redesigned and tested.

---

## Priority 6 — UI

Implemented for Pocket: idle, three-frame scan, Flipper found, keyboard,
mouse and BadUSB art. Still to verify or design:

- actual pairing code on the placard — confirmed on Pocket; held until Flipper confirmation
- other-device art on an unrecognized HID report
- DRIVE mode after microSD verification
- dedicated error art (current error text remains as a diagnostic fallback)

---

## Planned — Flipper connection through the dongle as if by USB cable

The operator wants the Flipper-to-PC connection through the Pocket-Dongle to
feel like connecting the Flipper directly by cable. Initial interpretation:
qFlipper device access, not just the existing Bluetooth Remote/BadUSB HID
input. Confirm the desired PC functions before implementation.

Start with the feasibility and acceptance plan in
`docs/FLIPPER_CABLE_LIKE_CONNECTION.md`. Keep this separate from the tested HID
release and the unverified v0.5.6 pairing candidate. Do not claim transparent
USB behavior from the existing HID + debug CDC interfaces.
