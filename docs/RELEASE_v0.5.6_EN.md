# Flipper Dongle v0.5.6 — Test Candidate

**Status: awaiting verification.**

v0.5.6 is based on v0.5.5 and changes pairing/reset state handling only. The known-good keyboard, mouse and USB HID transport are intentionally left unchanged.

## Problem addressed
After clearing BLE bonds with long BOOT, a following short BOOT pairing request could sometimes be visible in the serial log but fail to start a scan.

The behavior was intermittent because:
- BLE disconnect is asynchronous
- a late `ESP_HIDH_CLOSE_EVENT` could erase a new pairing request
- a blocking BLE scan could return stale results after bonds had already been cleared

## Changes
- explicit pairing/reset synchronization state
- deferred pairing request if short BOOT is pressed while disconnect is still finishing
- scan generation counter
- stale scan results are discarded after reset
- an old scan cannot reconnect after bond deletion
- working HID transport from v0.5.5 is unchanged

## Verification required
Repeat several times:
1. connect Bluetooth Remote or BadUSB
2. long BOOT to clear bonds
3. immediately short BOOT
4. repeat with a 1–2 second delay
5. repeat while the HID profile is already advertising

Expected result: every short BOOT either starts a scan immediately or is deferred until disconnect completes.

## Status
Test candidate — do not treat as the baseline until hardware verification is complete.
