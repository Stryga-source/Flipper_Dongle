# AGENTS.md — Flipper Dongle

This repository is the source of truth for the standalone **Flipper Dongle** project.

## Read first

Before changing code, read in this order:

1. `STATUS.md`
2. `tasks/current.md`
3. `README.md` / `README_RU.md`
4. `CHANGELOG.md`
5. relevant release notes in `docs/`
6. `docs/hardware/T_DONGLE_CLONE.md` before any Pocket-Dongle hardware work

If chat history and repository documentation disagree, prefer the repository state unless explicitly overridden.

## Project goal

Build a standalone ESP32-S3 USB dongle that bridges Flipper Zero BLE HID to a normal USB HID device:

`Flipper Zero BLE HID -> ESP32-S3 BLE HID Host -> USB HID Keyboard/Mouse -> PC`

The PC should require no custom driver or companion application.

## Version baselines

- `v0.5.5-first-test` = preserved first hardware-tested baseline.
- `main` = working HID core plus hardware-tested Pocket-Dongle keyboard,
  mouse, BadUSB and display scenes. Keep the original baseline available.
- `v0.5.6-test-candidate` = pairing/reset race-fix candidate awaiting hardware verification.
- Never rewrite or destructively modify the known-good baseline.
- Make incremental, versioned changes.
- Do not promote a candidate until hardware verification is complete.

## Known-good behavior

Confirmed on hardware in v0.5.5 lineage:

- Flipper Bluetooth Remote keyboard works through the dongle.
- Flipper Bluetooth Remote mouse works through the dongle.
- BadUSB / BadKB over BLE works through the dongle.
- USB HID keyboard release no longer sticks.
- TinyUSB HID transfers must be serialized by transfer-completion callback.
- Multiple BLE HID identities/bonds are intentional because Bluetooth Remote and BadUSB/BadKB can appear as different BLE devices.
- CDC COM is currently retained for debug builds.

## ESP-IDF / BLE constraints

Development stack:

- ESP-IDF 6.1
- NimBLE
- TinyUSB

Important fixes already discovered:

- ESP-IDF 6.1 NimBLE HID Host has a Report Map handling problem for this use case. Keep the project-local patched `esp_hid` component under `components/esp_hid/`.
- Do not patch the global ESP-IDF installation.
- The scan helper is patched locally to accept Flipper HID advertisements instead of Espressif demo names only.
- USB HID keyboard/mouse reports share one HID IN endpoint and must be strictly serialized using TinyUSB transfer completion. Do not restore fire-and-forget report sending.

## Pairing model

Target behavior:

- Short BOOT: one explicit pairing scan.
- Saved bond: automatic reconnect to a trusted HID identity.
- Multiple bonds are supported because `Control <name>` and `BadUSB <name>` may use different BLE addresses.
- Long BOOT: clear all stored HID bonds and remain idle.
- v0.5.6 specifically addresses races between asynchronous disconnect, bond clearing, stale BLE scans, and a new short-BOOT pairing request.

## Hardware targets

### Known development board

Primary validated board so far:

- Waveshare ESP32-S3-LCD-1.47

### New hardware target — Pocket-Dongle-S3-0.96

The newly arrived T-Dongle-like board has been visually identified from the actual PCB photos as:

- silkscreen: `Pocket-Dongle-S3-0.96`
- MCU: ESP32-S3
- integrated USB Type-A male plug
- integrated 0.96-inch display
- microSD socket
- one tactile button near USB
- exposed edge pads

This is **not an original LILYGO T-Dongle-S3**. Do not use LILYGO pin assignments by assumption.

A matching public Pocket-Dongle-S3 reference exists and candidate display/microSD pins are recorded in `docs/hardware/T_DONGLE_CLONE.md`. Those values remain **REFERENCE** until tested on the target board.

Before porting firmware to this target:

1. Preserve the factory firmware; do not erase it first.
2. Record Windows USB enumeration / VID/PID.
3. Run non-destructive ESP32-S3 identification (`chip_id`, `flash_id`).
4. Determine actual flash size.
5. Determine whether PSRAM exists and its size.
6. Back up the complete factory flash before overwriting it.
7. Verify native USB routing.
8. Verify the tactile button GPIO; do not assume GPIO0.
9. Verify display controller/pinout with a minimal diagnostic.
10. Verify microSD pinout with a minimal/read-only diagnostic.
11. Only then add a dedicated Pocket-Dongle board profile/config.

Do not break the Waveshare build while adding Pocket-Dongle support.

## USB product plan

During development:

- Debug build = HID Keyboard + HID Mouse + CDC COM logs.

Final target:

- Release build = HID only, no visible COM port.
- Keep Debug and Release as separate build configurations from the same source tree.

Later, after BLE HID and pairing are stable:

- merge the already-proven HID/DRIVE hardware switch work;
- restore microSD USB MSC DRIVE mode;
- add LCD status UI/animations.

## Working style

- Prefer exact code changes over vague suggestions.
- Preserve working versions before experiments.
- Update `STATUS.md` and `tasks/current.md` when work materially changes project state.
- Keep README EN/RU aligned for user-facing changes.
- Hardware behavior must be labeled `tested` only after hardware verification is complete.
