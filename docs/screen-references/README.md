# Flipper Dongle screen references

Approved visual references for the Pocket-Dongle-S3 display UI.

The ZIP archive in this folder contains the approved reference set:

- keyboard / HID mode
- joystick mode
- mouse mode
- evil / BadKB-style mode
- Flipper found
- Bluetooth pairing PIN screen
- three scan / echolocation frames (center, up, down)

These are **reference images**, not final firmware assets. Codex should adapt/crop/redraw them to the actual dongle display geometry and color depth while preserving the approved visual concept.

Important scan rule: the Flipper Zero target stays visually fixed across the three scan frames; the dolphin and sonar-wave angle move between frames.

The pairing image uses `123456` as a placeholder. Firmware should show the real runtime pairing code.
