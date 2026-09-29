# Flipper connection through the dongle as if by cable — planned

## User goal

Plug the Pocket-Dongle into a PC and connect a Flipper Zero wirelessly, with
the PC recognizing and using the Flipper in the same everyday workflow as a
direct USB connection. The initial interpretation is qFlipper access (device
recognition, management, files, applications, and supported maintenance
operations). Confirm the exact wanted functions with the operator before
implementation.

This is a **future feature**, not part of the `pocket-hid-2026-09-29` release.
The current bridge carries BLE HID keyboard/mouse reports to USB HID. Its CDC
interface is a dongle debug console, not a Flipper connection.

## Feasibility study before code changes

1. Capture what qFlipper and the operating system see with a Flipper connected
   directly by USB: device/interface descriptors, discovery behavior, and the
   functions the operator actually uses.
2. Identify the matching Flipper-side transport/protocol and whether the stock
   Flipper firmware exposes it over Bluetooth. Record which parts can work
   through the dongle and which would need Flipper firmware/app or PC software
   changes. Do not assume that forwarding a COM port alone is sufficient.
3. Check whether the existing HID identities and a management connection can
   coexist, including reconnection, authentication, throughput, and recovery
   after either device is unplugged.
4. Choose a safe USB interface design. Keep the current HID path and its
   synchronized TinyUSB transfers intact; do not repurpose the debug CDC port
   without providing a separate way to diagnose failures.
5. Define a small proof of concept and test it on a separate branch. Begin with
   device recognition and a read-only operation, then test file transfer and
   other requested functions one at a time. Build both Pocket and Waveshare
   after each coherent change.

## Acceptance criteria

- The operator can connect, disconnect, and reconnect a Flipper through the
  dongle without connecting a USB cable to the Flipper.
- The agreed PC functions work through the dongle in the operator's normal
  application, with results checked on the actual Pocket hardware.
- Existing Bluetooth Remote keyboard/mouse and BadUSB behavior still works.
- The Waveshare build still passes.
- Unsupported direct-cable functions, if any, are documented explicitly.

Do not call the connection cable-equivalent until the operator confirms the
agreed functions on hardware.
