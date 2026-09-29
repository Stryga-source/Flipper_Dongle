"""Generate Pocket-Dongle RGB565 scene data from the approved montage.

Requires Pillow. Run: python tools/generate_pocket_scenes.py
The generated binary is committed so ESP-IDF builds need no Python imaging dependency.
"""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "docs/screen-references/approved_montage.png"
OUTPUT = ROOT / "main/pocket_scenes.bin"
WIDTH, HEIGHT = 160, 80


def tile(source, col, row):
    x, y = 20 + 440 * col, 20 + 490 * row
    return source.crop((x, y, x + 420, y + 420))


def orange_background(image):
    image = image.copy()
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b = pixels[x, y]
            if r > 170 and 55 < g < 200 and b < 90:
                pixels[x, y] = (255, 132, 0)
    return image


def scene_bytes(image):
    if image.size != (WIDTH, HEIGHT):
        raise ValueError(f"Scene is {image.size}, expected 160x80")
    raw = bytearray()
    pixels = image.convert("RGB").load()
    for y in range(HEIGHT):
        for x in range(WIDTH):
            r, g, b = pixels[x, y]
            pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            raw.extend((pixel >> 8, pixel & 0xFF))
    return raw


def main():
    source = Image.open(SOURCE).convert("RGB")
    if source.size != (1340, 1490):
        raise ValueError(f"Unexpected approved montage size: {source.size}")
    images = [tile(source, col, row) for col, row in
              [(0, 0), (1, 0), (2, 0), (0, 1), (1, 1), (2, 1),
               (0, 2), (1, 2), (2, 2)]]
    scenes = []

    # Generic waiting state: a dolphin without waves or a discovered target.
    idle = Image.new("RGB", (WIDTH, HEIGHT), (255, 132, 0))
    swimmer = orange_background(images[6].crop((0, 100, 218, 310)))
    idle.paste(swimmer.resize((88, 80), Image.Resampling.LANCZOS), (36, 0))
    scenes.append(idle)

    # Approved keyboard, other-device joystick, mouse, BadUSB and found art.
    for image in images[:5]:
        scenes.append(image.resize((WIDTH, HEIGHT), Image.Resampling.LANCZOS))

    # The image carries a blank placard. Firmware draws the actual BLE code.
    pin = images[5].copy()
    ImageDraw.Draw(pin).rectangle((119, 225, 303, 280), fill="white")
    scenes.append(pin.crop((0, 85, 420, 295)).resize(
        (WIDTH, HEIGHT), Image.Resampling.LANCZOS))

    # Keep the Flipper target at fixed coordinates across all three frames.
    target = orange_background(images[6].crop((300, 145, 415, 290)))
    target = target.resize((46, 50), Image.Resampling.LANCZOS)
    crops = [(0, 100, 310, 310), (0, 100, 310, 325), (0, 65, 315, 330)]
    for image, box in zip(images[6:], crops):
        scene = Image.new("RGB", (WIDTH, HEIGHT), (255, 132, 0))
        moving = orange_background(image.crop(box)).resize(
            (113, 80), Image.Resampling.LANCZOS)
        scene.paste(moving, (0, 0))
        ImageDraw.Draw(scene).rectangle((103, 42, 113, 79), fill=(255, 132, 0))
        scene.paste(target, (114, 22))
        scenes.append(scene)

    if len(scenes) != 10:
        raise AssertionError("Expected 10 scenes")
    OUTPUT.write_bytes(b"".join(scene_bytes(image) for image in scenes))
    expected = 10 * WIDTH * HEIGHT * 2
    if OUTPUT.stat().st_size != expected:
        raise AssertionError(f"Expected {expected} RGB565 bytes")
    print(f"Wrote {OUTPUT} ({expected} bytes, 10 scenes)")


if __name__ == "__main__":
    main()
