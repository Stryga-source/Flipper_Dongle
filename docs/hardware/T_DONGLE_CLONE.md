# Pocket-Dongle-S3-0.96 Hardware Notes

Status: **board family identified from photos; exact electrical pinout not yet hardware-verified**

This board was initially described as a non-original T-Dongle-style device. User photos now identify the PCB silkscreen as:

`Pocket-Dongle-S3-0.96`

It should be treated as a **Pocket-Dongle-S3 target**, not as an original LILYGO T-Dongle-S3.

## Evidence from the actual board photos

### MCU

- Marking: **ESP32-S3** — **MARKING / VERIFIED FROM PHOTO**
- Family: ESP32-S3 — **VERIFIED FROM PHOTO**
- Native USB capability: ESP32-S3 supports native USB; actual PCB routing still needs confirmation — **INFERRED**
- Flash size: **UNKNOWN**
- PSRAM presence/size: **UNKNOWN**

Do not assume N8/N16 or PSRAM configuration from other Pocket-Dongle revisions. Read it from the actual board before selecting memory configuration.

### USB

- Connector type: integrated **USB Type-A male** — **VERIFIED FROM PHOTO**
- USB D+/D- routing: **UNKNOWN / TO VERIFY**
- USB-UART bridge present: no dedicated bridge IC is obvious in the supplied photos, but do not rely on this alone — **INFERRED**
- Intended project role: native USB HID if the PCB routes ESP32-S3 USB directly — **TO VERIFY**

### Display

- Integrated display present — **VERIFIED FROM PHOTO**
- PCB marking includes `0.96` — **MARKING**
- Matching public Pocket-Dongle-S3 reference reports **0.96 inch, 80x160, ST7735-family TFT** — **REFERENCE, NOT YET VERIFIED ON THIS EXACT BOARD**

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
- Function: likely BOOT/user button — **INFERRED**
- GPIO: **UNKNOWN**

Do not assume GPIO0 until tested.

### LEDs

- No dedicated RGB/status LED is clearly identifiable from the supplied photos — **UNKNOWN**
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

1. Connect the Pocket-Dongle to the development PC.
2. Record the USB VID/PID and Windows device name(s).
3. Enter ESP32-S3 ROM bootloader if necessary.
4. Run non-destructive identification first:

```powershell
esptool.py --chip esp32s3 chip_id
esptool.py --chip esp32s3 flash_id
```

5. Record detected flash manufacturer and size.
6. Determine the full flash size and make a backup **before erasing or overwriting the factory firmware**.
7. Check whether PSRAM is present using a minimal ESP-IDF diagnostic build before selecting a PSRAM-enabled target.
8. Test the tactile button GPIO.
9. Test the candidate TFT pinout with a minimal ST7735 80x160 diagnostic.
10. Test the candidate microSD SPI pinout with read-only card initialization first.
11. Confirm native USB HID enumeration.

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
