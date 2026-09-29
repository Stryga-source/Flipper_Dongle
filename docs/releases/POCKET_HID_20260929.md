# Pocket-Dongle-S3-0.96 HID — 2026-09-29

Hardware-tested Pocket-Dongle build from the source tree tagged
`pocket-hid-2026-09-29`. This is the ESP32-S3 board marked `Pocket-Dongle-S3-0.96` with 16 MB flash and the 160x80
screen. It is not an original LILYGO T-Dongle. The `v0.5.6-test-candidate`
pairing change is not included.

## Files

- `Pocket-Dongle-S3-0.96-HID-full-20260929.bin`: bootloader, partition table,
  and application in one file. Flash at `0x0` for a first installation. This
  writes `0xFF` through the NVS partition at `0x9000` and clears BLE bonds.
- `Pocket-Dongle-S3-0.96-HID-app-20260929.bin`: application only. Flash at
  `0x10000` when upgrading an existing build with this same partition layout;
  it preserves NVS and BLE bonds. Do not use it to install over unrelated
  firmware.

SHA-256:

```text
A67AAA65FC314A33195F231919E37E54B6A561A71643F4B2968EADDBEB1CF8A4  Pocket-Dongle-S3-0.96-HID-full-20260929.bin
BD5619EF5759B29A734F3115BAA7449218D1E5805AA84429A9C3E6E73BE3ABC7  Pocket-Dongle-S3-0.96-HID-app-20260929.bin
```

## Flashing on Windows

Install `esptool` in your Python environment. Hold BOOT while connecting the
dongle to USB, then release it. Find the bootloader COM port in Device Manager
(`COM23` in the tested session). From the folder containing the downloaded file:

```powershell
python -m esptool --chip esp32s3 -p COM23 write-flash 0x0 .\Pocket-Dongle-S3-0.96-HID-full-20260929.bin
```

For an application-only upgrade of this project's Pocket build:

```powershell
python -m esptool --chip esp32s3 -p COM23 write-flash 0x10000 .\Pocket-Dongle-S3-0.96-HID-app-20260929.bin
```

After `Hash of data verified`, unplug the dongle and reconnect without holding
BOOT. The development USB configuration exposes a keyboard, mouse, and CDC COM
port. Short BOOT starts a BLE scan; long BOOT clears saved bonds.

## Scope and verification

The operator confirmed the Pocket keyboard, mouse, BadUSB, and screen-scene
transitions on hardware. Both Pocket and Waveshare builds passed. The pairing
code placard, repeated bond-reset tests, microSD/DRIVE, and any LED remain
unverified. This is a debug USB build with CDC, not the planned HID-only
release configuration. The board has no HID/DRIVE switch.

The original full 16 MB factory-flash backup is retained outside Git. Neither
release image contains that factory backup or user-specific flash contents.
