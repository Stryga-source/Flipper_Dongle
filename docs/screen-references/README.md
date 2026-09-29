# Pocket-Dongle screen scenes

`approved_montage.png` is the operator-approved visual reference. It contains
keyboard, joystick/other device, mouse, BadUSB, Flipper found, pairing PIN,
and three scan poses. The earlier ZIP on `origin/main` was incomplete; this
full montage is the source used by this branch.

Run `python tools/generate_pocket_scenes.py` with Pillow installed to recreate
`main/pocket_scenes.bin`. The binary contains ten 160x80 RGB565 big-endian
frames in this order: idle (derived from scan art), keyboard, other device,
mouse, BadUSB, found, blank pairing placard, scan 1, scan 2, scan 3. The
committed binary keeps ESP-IDF builds independent of Pillow.

The scan target stays at the same screen coordinates across all three frames.
The pairing placard in the binary is blank: firmware draws the six digits from
the BLE display or numeric-comparison event. The reference's `123456` is not
hard-coded into that image. The existing NimBLE HID helper currently uses
`123456` for its display-passkey action; numeric comparison uses its event
value. This change only forwards those existing values to the screen.

The joystick picture marks an unrecognized HID report. It does not mean the
dongle forwards a joystick report to USB. This branch leaves the USB HID
keyboard/mouse transport unchanged.

The art was adapted from square references to the board's 160x80 LCD. Colors
and legibility still need operator confirmation on the physical display.
