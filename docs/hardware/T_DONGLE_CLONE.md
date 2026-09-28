# T-Dongle Clone Hardware Notes

Status: **unidentified / not yet validated**

This board is a non-original T-Dongle-style device and must not be assumed to match an original LILYGO T-Dongle-S3.

## Identification checklist

### MCU

- Marking: TBD
- Family: TBD
- Native USB support: TBD
- Flash: TBD
- PSRAM: TBD

### USB

- Connector type: TBD
- Native USB D+/D- pins: TBD
- USB-UART bridge present: TBD
- Power-only / native / bridge routing: TBD

### Display

- Controller: TBD
- Resolution: TBD
- Interface: TBD
- SCLK: TBD
- MOSI: TBD
- MISO: TBD
- CS: TBD
- DC: TBD
- RST: TBD
- Backlight: TBD

### Buttons

- BOOT button GPIO: TBD
- Additional user button GPIO(s): TBD

### LEDs

- Type: TBD
- GPIO: TBD
- Active level: TBD

### microSD

- Present: TBD
- Interface: TBD
- CLK/SCK: TBD
- CMD/MOSI: TBD
- D0/MISO: TBD
- CS / other data pins: TBD

### Power

- Regulator: TBD
- USB 5 V path: TBD
- Battery support: TBD

## Evidence

Add photos and observations here.

For each pin/component, mark one of:

- **VERIFIED** — measured, schematic-confirmed, or tested in firmware
- **MARKING** — directly read from component/package marking
- **INFERRED** — inferred from PCB traces or similarity to another board
- **UNKNOWN** — no reliable evidence yet

## Porting rule

Do not add this board as a supported Flipper Dongle target until at minimum the following are verified:

- exact MCU
- USB path
- BOOT/user button GPIO
- display pinout if display support is enabled

The Waveshare target must continue to build while this clone is being added.
