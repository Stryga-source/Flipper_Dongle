# AGENTS.md — Flipper Dongle

This repository is the source of truth for the standalone **Flipper Dongle** project.

## Read first

Before changing code, read in this order:

1. `STATUS.md`
2. `tasks/current.md`
3. `README.md` / `README_RU.md`
4. `CHANGELOG.md`
5. relevant release notes in `docs/`

If chat history and repository documentation disagree, prefer the repository state unless the user explicitly overrides it.

## Project goal

Build a standalone ESP32-S3 USB dongle that bridges Flipper Zero BLE HID to a normal USB HID device:

`Flipper Zero BLE HID -> ESP32-S3 BLE HID Host -> USB HID Keyboard/Mouse -> PC`

The PC should require no custom driver or companion application.

## Version baselines

- `main` and `v0.5.5-first-test` = first hardware-tested baseline.
- `v0.5.6-test-candidate` = pairing/reset race-fix candidate awaiting hardware verification.
- Never rewrite or destructively modify the known-good baseline.
- Make incremental, versioned changes.
- Do not promote a candidate until the user confirms hardware testing.

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

### New hardware target — T-Dongle clone

A non-original T-Dongle-style board has arrived and is the next hardware target.

Do **not** assume it matches an original LILYGO T-Dongle-S3 pinout or peripherals.

Before porting firmware:

1. Identify the exact MCU marking.
2. Identify USB wiring / whether native USB is exposed.
3. Identify display controller and pins, if present.
4. Identify microSD wiring, if present.
5. Identify BOOT/user button GPIOs.
6. Identify any RGB/status LED pins.
7. Record photos/markings/pinout in `docs/hardware/`.
8. Only then add a dedicated board profile/config.

Do not break the Waveshare build while adding clone support.

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
- Hardware behavior must be labeled `tested` only after explicit user confirmation.
