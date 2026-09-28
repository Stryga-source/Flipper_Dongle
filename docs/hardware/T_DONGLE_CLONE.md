# Pocket-Dongle-S3-0.96 Hardware Notes

Status: **board family identified from photos; MCU and memory identified by esptool; exact peripheral pinout not yet hardware-verified**

This board was initially described as a non-original T-Dongle-style device. User photos now identify the PCB silkscreen as:

`Pocket-Dongle-S3-0.96`

It should be treated as a **Pocket-Dongle-S3 target**, not as an original LILYGO T-Dongle-S3.

## Non-destructive identification supplied by the operator (2026-09-28)

The operator ran esptool v5.3.1 against the actual board on `COM23`. These are observations from `chip_id` and `flash_id`, not a firmware runtime or peripheral test:

| Property | Observed result | Evidence status |
| --- | --- | --- |
| MCU | ESP32-S3 QFN56, revision v0.2 | DETECTED BY ESPTOOL |
| Crystal | 40 MHz | DETECTED BY ESPTOOL |
| PSRAM | Embedded 8 MB (`AP_3v3`) | DETECTED BY ESPTOOL; ESP-IDF boot and a 64 KiB PSRAM write/read test passed |
| External flash | Manufacturer ID `0x20`, device ID `0x4018`, detected size 16 MB | DETECTED BY ESPTOOL; complete 16 MB read succeeded |
| Flash electrical mode | Quad, 3.3 V per eFuse | REPORTED BY ESPTOOL |
| Current USB connection | USB-Serial/JTAG on `COM23` | OBSERVED IN ESPTOOL SESSION |
| Windows USB enumeration | `USB\\VID_303A&PID_1001&MI_00`; localized name `Устройство с последовательным интерфейсом USB (COM23)` | OBSERVED IN WINDOWS DEVICE ENUMERATION |

The complete factory flash was backed up outside Git as `Pocket-Dongle-S3-0.96_factory_20260928.bin` (16,777,216 bytes). SHA-256: `7eb9b4211c550ddffd57e013513d0c548faf8ade2e0ef4e4ec1a23761228ffdc`. The first 1 MiB matches a separate ROM read byte for byte. The first stub-loader read stopped at about 2.4%; the successful complete read used `--no-stub`. This preserves a recovery image, but it does not verify that the factory application runs correctly after a restore.

## Evidence from the actual board photos

### MCU

- Marking: **ESP32-S3** — **MARKING / VERIFIED FROM PHOTO**
- Family: ESP32-S3 — **VERIFIED FROM PHOTO**
- Native USB capability: ESP32-S3 supports native USB; actual PCB routing still needs confirmation — **INFERRED**
- Flash size: **16 MB DETECTED BY ESPTOOL** (`0x20:0x4018`); complete 16 MB read succeeded
- PSRAM presence/size: **embedded 8 MB DETECTED BY ESPTOOL**; ESP-IDF initialized PSRAM and a 64 KiB write/read pattern passed. Full 8 MB integrity is not tested.

Do not assume N8/N16 or PSRAM configuration from other Pocket-Dongle revisions. Read it from the actual board before selecting memory configuration.

### USB

- Connector type: integrated **USB Type-A male** — **VERIFIED FROM PHOTO**
- USB D+/D- routing: the board connected to ESP32-S3 USB-Serial/JTAG on `COM23` — **OBSERVED**; USB OTG/HID operation is **TO VERIFY**
- Windows VID/PID: `303A:1001`; interface `MI_00`, reported as a USB serial device on `COM23` — **OBSERVED**
- USB-UART bridge present: no dedicated bridge IC is obvious in the supplied photos, but do not rely on this alone — **INFERRED**
- Intended project role: native USB HID if the PCB routes ESP32-S3 USB directly — **TO VERIFY**

### Display

- Integrated display present — **VERIFIED FROM PHOTO**
- PCB marking includes `0.96` — **MARKING**
- Matching public Pocket-Dongle-S3 reference reports **0.96 inch, 80x160, ST7735-family TFT** — **REFERENCE, NOT YET VERIFIED ON THIS EXACT BOARD**
- The supplied front photo shows the dark display face, but no readable controller marking or powered image — **CONTROLLER UNKNOWN**

Candidate pinout from the matching public `ronenkr/Pocket-Dongle-S3` project:

- TFT MOSI: GPIO11 — **REFERENCE**
- TFT SCLK: GPIO10 — **REFERENCE**
- TFT CS: GPIO12 — **REFERENCE**
- TFT DC: GPIO13 — **REFERENCE**
- TFT RST: GPIO14 — **REFERENCE**
- Backlight: **UNKNOWN on our board**

These pins must be confirmed with a minimal display test before being used in the Flipper Dongle board profile.

### Buttons

- One tactile button is present near the USB connector — **VERIFIED FROM PHOTO**
- The clearer rear photo shows `BOOT` printed next to the tactile button — **MARKING VERIFIED FROM PHOTO**; its electrical behavior is not yet tested
- GPIO: **UNKNOWN**

Do not assume GPIO0 until tested.

### Mode selector

- The operator confirms this Pocket-Dongle has **no physical HID/DRIVE switch**. Do not reuse the older v0.3 SPDT switch behavior as a board assumption.

### Red component / LEDs

- A red component at the end opposite the USB plug is visible more clearly in the new rear photo. Its edge placement is consistent with a small RF chip antenna; the operator suggests a capacitive antenna — **VISUAL INFERENCE, NOT ELECTRICALLY VERIFIED**. Its exact type is unknown.
- Do not assign this component an LED or GPIO role without a marking, schematic, or electrical check.
- No separate RGB/status LED has been identified — **UNKNOWN whether one exists**.
- GPIO: **UNKNOWN**

### microSD

- microSD socket present on the PCB back side — **VERIFIED FROM PHOTO**
- Interface: likely SPI on this Pocket-Dongle family — **REFERENCE / TO VERIFY**

Candidate pinout from the matching public `ronenkr/Pocket-Dongle-S3` project:

- SD MISO: GPIO16 — **REFERENCE**
- SD MOSI: GPIO18 — **REFERENCE**
- SD SCK: GPIO17 — **REFERENCE**
- SD CS: GPIO47 — **REFERENCE**

Do not merge these values into the production board profile until the card is successfully initialized and read on this exact board.

### Exposed GPIO

- Edge castellated/test pads are present on both sides — **VERIFIED FROM PHOTO**
- Individual silkscreen labels are not yet fully decoded from the current photos.

### Power

- Powered from USB Type-A — **VERIFIED FROM PHOTO**
- Regulator: present but exact device/ratings not yet identified — **UNKNOWN**
- Battery support: no battery connector is visible in the supplied photos — **INFERRED: likely none**

## Matching public reference

Useful external reference discovered for this board family:

- `ronenkr/Pocket-Dongle-S3`
- describes ESP32-S3 Pocket Dongle with 0.96 inch LCD and microSD
- includes Arduino, ESP-IDF, firmware examples and schematics
- reports 80x160 display and the candidate TFT/SD pins listed above

Treat that repository as a **reference implementation**, not as proof that every clone/revision has identical wiring.

## Important revision warning

Public projects describe more than one Pocket-Dongle-S3 memory/revision combination (including boards advertised with larger flash/PSRAM). Therefore:

- do not hard-code flash size from Internet listings;
- do not enable PSRAM until the actual board confirms it;
- do not assume original LILYGO T-Dongle-S3 display/SD pins;
- do not assume a different Pocket-Dongle-S3 revision uses the same pinout.

## Next hardware identification procedure

Before flashing project firmware, preserve the factory image if possible.

1. Windows enumerated the board as `USB\\VID_303A&PID_1001&MI_00` on `COM23`; record any changes after bootloader or firmware transitions.
2. The operator has already run non-destructive identification; equivalent current esptool syntax is:

```powershell
python -m esptool --chip esp32s3 -p COM23 chip-id
python -m esptool --chip esp32s3 -p COM23 flash-id
```

3. Complete: the full 16 MB factory image is stored outside Git; its byte count and SHA-256 were checked, and its first 1 MiB matches an independent read. Preserve this file before any erase or write.
4. Completed in the separate `diagnostics/pocket_dongle_probe` project: ESP-IDF 6.1 booted, reported 16,777,216 flash bytes, initialized PSRAM, and passed a 64 KiB PSRAM write/read test over USB-Serial/JTAG logs. A full-memory test and USB HID remain pending.
5. Test the tactile button GPIO.
6. Test the candidate TFT pinout with a minimal display diagnostic; confirm its controller and resolution.
7. Test the candidate microSD SPI pinout with read-only card initialization first.
8. Confirm native USB HID enumeration.

## Evidence labels

For each pin/component use one of:

- **VERIFIED** — measured, schematic-confirmed for this exact PCB, or tested in firmware
- **MARKING** — directly read from this exact component/package/PCB marking
- **REFERENCE** — documented for a matching public Pocket-Dongle project but not yet tested on our PCB
- **INFERRED** — inferred from PCB layout/family similarity
- **UNKNOWN** — no reliable evidence yet

## Porting rule

Do not declare the Pocket-Dongle board supported until at minimum these are verified on the actual hardware:

- ESP32-S3 boots under ESP-IDF 6.1
- actual flash size
- native USB path / USB HID enumeration
- BOOT/user button GPIO
- display pinout if display support is enabled
- microSD pinout if DRIVE mode is enabled

The Waveshare ESP32-S3-LCD-1.47 build must continue to build and remain supported while Pocket-Dongle support is added.
