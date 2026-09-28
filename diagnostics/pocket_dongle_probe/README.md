# Pocket-Dongle first boot diagnostic

This is a separate ESP-IDF 6.1 project for the actual `Pocket-Dongle-S3-0.96` board. It checks ESP-IDF startup, flash size, PSRAM initialization, a 64 KiB PSRAM write/read pattern, and USB-Serial/JTAG logging. It does not use any candidate display, button, LED, or microSD GPIO assignments. It does not implement USB HID.

The factory 16 MB image was saved and verified before this experiment. Preserve that backup outside Git.

From this directory with ESP-IDF 6.1 activated:

```powershell
idf.py set-target esp32s3
idf.py build
idf.py -p COM23 flash
python "$env:IDF_PATH/tools/idf_monitor.py" -p COM23 -b 115200 --no-reset build/pocket_dongle_probe.elf
```

`COM23` was observed on the operator's Windows PC on 2026-09-28 and may change after reconnecting. A successful build is not a successful hardware test; record the serial output and any reset loop before treating flash, PSRAM, or USB logging as verified.

## First result on the actual board (2026-09-28)

- ESP-IDF 6.1 build completed; app binary `0x2a8d0` bytes.
- esptool wrote and verified bootloader, partition table, and app on `COM23`.
- The app reported 16,777,216 bytes of flash and four consecutive `PSRAM 64 KiB test=PASS` heartbeats through USB-Serial/JTAG.
- Only a 64 KiB PSRAM allocation was exercised; full 8 MB memory integrity and USB HID remain untested.
- `idf.py monitor` reset this USB-Serial/JTAG connection into ROM download mode. After a watchdog reset, `idf_monitor.py --no-reset` received the periodic heartbeat without another reset.
- The Pocket-Dongle has no physical HID/DRIVE mode switch. This diagnostic does not use a switch or drive peripheral GPIOs.
