# Pocket pairing-race test candidate — 2026-09-29

Status: **experimental Pocket build flashed; repeat connection and pairing-code placard lifetime confirmed; broader stress testing pending**.
Do not promote this branch to `main` or call it a tested release until the operator completes the checks below.

- Branch: `codex/pocket-pairing-056`
- Source commit: `eafba1103b794bf8c52e83ce2c59a81ec1a678ba`
- Change: port the `v0.5.6-test-candidate` pairing/reset race fix onto the working Pocket `main` source tree. The shared USB HID transport and Pocket screen assets were not changed.
- ESP-IDF: 6.1
- Pocket build: passed; app size `0xce7c0`, 45% free in the smallest app partition.
- Waveshare build: passed; app size `0x865c0`, 64% free in the smallest app partition. Waveshare hardware was not flashed.
- Pocket application SHA-256: `27161241715C03C9D12A827A0B9E7A5ACE346BB7F998EC1A305D0BEEB4FC896F`
- Final Pocket test application SHA-256 (pairing placard held until encryption result): `BECBB16459C81B36B77927AB5779EE727A7C3226DABF4831DA1B4E913091979E`
- The newly built Pocket partition table matches the hardware-tested full release image byte for byte.

## Flash performed

The operator's Pocket-Dongle-S3-0.96 was identified on `COM23` by ESP32-S3 MAC `90:70:69:f6:60:2c`, revision v0.2, 16 MB flash and reported 8 MB embedded PSRAM. With esptool v5.3.1, only `build-pocket-056/flipper_dongle_v041.bin` was written at `0x10000`; esptool reported `Hash of data verified`. Bootloader and partition table were not rewritten, so the existing NVS partition was not overwritten by this flash. Clearing bonds during a later test will still delete those bonds.

After the esptool reset, Windows initially remained in ROM USB-Serial/JTAG on `COM23` (`303A:1001`). A watchdog reset started the application, which enumerated USB HID keyboard/mouse and CDC `COM22`. The operator confirmed that long BOOT cleared bonds and a subsequent short BOOT reconnected the Flipper. CDC logs showed the asynchronous CLOSE, new scan generation and successful encrypted bond.

The Flipper initially displayed a numeric-comparison code, but the Pocket did not. The CDC log contained `NIMBLE_HIDH: PASSKEY NUMCMP auto-accept rc=0` but no `FD_STATUS: === PAIRING CODE ===`. The code was wired only to the scanner helper, while the HID Host owned the active passkey callback. The first follow-up source fix forwarded the real NUMCMP event value to the indicator; both board builds passed and the Pocket app-only image was flashed with a verified hash. The next CDC log showed `FD_STATUS: === PAIRING CODE ===`, and the operator confirmed the digits matched Flipper. The fixed four-second placard interval was still too short. The next follow-up removed that timer and retained the placard until an encryption result or disconnect. Both board builds passed; the Pocket app-only image was flashed with a verified hash. The operator waited before confirming on Flipper and confirmed that matching digits stayed visible until confirmation. The CDC log shows `FD_STATUS: === PAIRING CODE ===`, then `Pairing code display complete` and `ENC_CHANGE status=0 encrypted=1 authenticated=1 bonded=1` about ten seconds later.

## Hardware smoke test

1. Release BOOT and reconnect the dongle without pressing it. Confirm screen scenes, USB keyboard/mouse and the CDC COM port.
2. Capture CDC logs and confirm the startup line reports `v0.5.6: pairing race fix`.
3. Confirm Bluetooth Remote keyboard and mouse input still reaches the PC.
4. While connected, long-press BOOT to clear bonds; immediately short-press BOOT. Verify a pairing scan starts after BLE CLOSE and no stale automatic reconnection occurs.
5. Repeat step 4 with a 1–2 second delay, then with the Flipper already advertising.
6. Pair and test BadUSB/BadKB BLE identity separately. Repeat the reset and pairing sequence.
7. Record any error messages, serial log and exact button timing. Keep `v0.5.6` as a candidate until repeated runs pass.

## Rollback

The hardware-tested Pocket application is the `Pocket-Dongle-S3-0.96-HID-app-20260929.bin` asset in the `pocket-hid-2026-09-29` release. In bootloader mode, write it at `0x10000`, then reconnect without BOOT. This restores the earlier app while leaving the partition table and current NVS contents in place; it cannot recreate bonds erased during testing.
