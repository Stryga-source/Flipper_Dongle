# Pocket-Dongle-S3-0.96 Hardware Notes

Status: **board family identified; MCU/memory/native USB verified on the actual board; peripheral pinout still to be hardware-verified**

PCB silkscreen from the user's board:

`Pocket-Dongle-S3-0.96`

This must be treated as a **Pocket-Dongle-S3 target**, not as an original LILYGO T-Dongle-S3.

## Verified on the actual board

The following values were read non-destructively with `esptool v5.3.1` on COM23.

### MCU

- Chip: **ESP32-S3** — **VERIFIED**
- Package: **QFN56** — **VERIFIED**
- Silicon revision: **v0.2** — **VERIFIED**
- CPU/features reported by esptool: Wi-Fi, Bluetooth 5 LE, Dual Core + LP Core, 240 MHz — **VERIFIED**
- Crystal: **40 MHz** — **VERIFIED**
- Embedded PSRAM: **8 MB, AP_3v3** — **VERIFIED**

This is effectively an ESP32-S3 configuration with 8 MB embedded PSRAM. Do not infer the external Flash size from other Pocket-Dongle revisions; this board was measured separately below.

### Flash

- Detected Flash size: **16 MB** — **VERIFIED**
- JEDEC manufacturer byte: `0x20` — **VERIFIED RAW ID**
- JEDEC device ID: `0x4018` — **VERIFIED RAW ID**
- Flash bus mode from eFuse: **quad / 4 data lines** — **VERIFIED**
- Flash voltage from eFuse: **3.3 V** — **VERIFIED**

Important: the public `ronenkr/Pocket-Dongle-S3` README describes an 8 MB Flash revision, while this actual board reports **16 MB**. This confirms there are multiple memory revisions and validates the rule to trust hardware measurements over public listings.

### USB

- Connector: integrated **USB Type-A male** — **VERIFIED FROM PHOTO**
- esptool connection: **COM23** — **VERIFIED**
- ESP32-S3 reported USB mode: **USB-Serial/JTAG** — **VERIFIED**
- A separate USB-UART bridge is not required for programming this board; communication is through the ESP32-S3 native USB Serial/JTAG path — **VERIFIED BY ACTUAL CONNECTION**

For Flipper Dongle this is a strong positive result: the physical USB connector is routed to the ESP32-S3 native USB pins sufficiently for USB Serial/JTAG operation. The project still needs an explicit TinyUSB HID enumeration test before the board is declared fully supported.

## Evidence from the actual board photos

### Display

- Integrated display present — **VERIFIED FROM PHOTO**
- PCB marking includes `0.96` — **MARKING**
- Matching public Pocket-Dongle-S3 reference reports **0.96 inch, 80x160** LCD and TFT_eSPI/ST7735 examples — **REFERENCE, NOT YET VERIFIED ON THIS EXACT BOARD**

Candidate pinout from `ronenkr/Pocket-Dongle-S3`:

- TFT MOSI: GPIO11 — **REFERENCE**
- TFT SCLK: GPIO10 — **REFERENCE**
- TFT CS: GPIO12 — **REFERENCE**
- TFT DC: GPIO13 — **REFERENCE**
- TFT RST: GPIO14 — **REFERENCE**
- Backlight: **UNKNOWN on our exact board**

These pins must be confirmed with a minimal display test before being used in the production board profile.

### Buttons

- One tactile button is present near the USB connector — **VERIFIED FROM PHOTO**
- Function: likely BOOT/user button — **INFERRED**
- GPIO: **UNKNOWN**

Do not assume GPIO0 until tested.

### LEDs

- No dedicated RGB/status LED is clearly identifiable from the supplied photos — **UNKNOWN**
- GPIO: **UNKNOWN**

### microSD

- microSD socket present on the PCB back side — **VERIFIED FROM PHOTO**

Candidate SPI-style pinout from the matching public `ronenkr/Pocket-Dongle-S3` README:

- SD MISO: GPIO16 — **REFERENCE**
- SD MOSI: GPIO18 — **REFERENCE**
- SD SCK: GPIO17 — **REFERENCE**
- SD CS: GPIO47 — **REFERENCE**

The same public project also contains an `SD_MMC_Example`; therefore the exact usable card mode and wiring on our board should be verified in firmware rather than assumed from one example.

### Exposed GPIO

- Edge castellated/test pads are present on both sides — **VERIFIED FROM PHOTO**
- Individual labels still need to be mapped if they are required by the project.

### Power

- Powered from USB Type-A — **VERIFIED FROM PHOTO / TEST**
- Regulator: present but exact device/ratings not yet identified — **UNKNOWN**
- Battery connector: none visible — **INFERRED: likely none**

## Matching public reference

External reference for this board family:

- `ronenkr/Pocket-Dongle-S3`
- ESP32-S3 Pocket Dongle with 0.96 inch display and microSD
- reports 80x160 display
- reference TFT pins: 11/10/12/13/14
- reference SD pins: 16/18/17/47
- public README reports 8 MB Flash, which **does not match our measured 16 MB board**

Treat this repository as a **reference implementation**, not as proof that every revision has identical memory or wiring.

## Factory firmware preservation — next step

Before any erase/write operation, make a complete 16 MB dump of the actual factory firmware.

Current esptool syntax:

```powershell
esptool --chip esp32s3 --port COM23 read-flash 0x0 0x1000000 pocket-dongle-s3-factory-16MB.bin
```

Then calculate a hash:

```powershell
Get-FileHash .\pocket-dongle-s3-factory-16MB.bin -Algorithm SHA256
```

Recommended: read the flash a second time into another file and compare SHA256 hashes before considering the backup trusted.

```powershell
esptool --chip esp32s3 --port COM23 read-flash 0x0 0x1000000 pocket-dongle-s3-factory-16MB-verify.bin
Get-FileHash .\pocket-dongle-s3-factory-16MB*.bin -Algorithm SHA256
```

The two hashes should be identical.

Do **not** erase or write the Flash until a verified backup exists.

## Remaining hardware validation procedure

1. Preserve and verify the full 16 MB factory image.
2. Record Windows USB VID/PID if useful for documentation.
3. Determine the tactile button GPIO.
4. Test the candidate TFT pinout with a minimal 80x160 diagnostic.
5. Test the candidate microSD pinout with read-only initialization first.
6. Build a minimal ESP-IDF 6.1 target with correct 16 MB Flash + 8 MB PSRAM settings.
7. Confirm TinyUSB HID enumeration over the same Type-A connector.
8. Only then port the full Flipper Dongle BLE HID bridge.

## Evidence labels

- **VERIFIED** — measured, directly reported by the actual hardware/tool, schematic-confirmed for this exact PCB, or tested in firmware
- **MARKING** — directly read from this exact component/package/PCB marking
- **REFERENCE** — documented for a matching public Pocket-Dongle project but not yet tested on our PCB
- **INFERRED** — inferred from PCB layout/family similarity
- **UNKNOWN** — no reliable evidence yet

## Porting rule

Do not declare the Pocket-Dongle board supported until at minimum these are verified on the actual hardware:

- ESP32-S3 boots under ESP-IDF 6.1
- 16 MB Flash configuration
- 8 MB PSRAM configuration
- native USB TinyUSB HID enumeration
- BOOT/user button GPIO
- display pinout if display support is enabled
- microSD pinout if DRIVE mode is enabled

The Waveshare ESP32-S3-LCD-1.47 build must continue to build and remain supported while Pocket-Dongle support is added.
