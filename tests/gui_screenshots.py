#!/usr/bin/env python3
"""Render repeatable SDL screens and compare the UI regions to approved captures."""
import argparse
import os
import subprocess
import tempfile
from pathlib import Path

from PIL import Image, ImageChops

RESOLUTIONS = ((1280, 720), (1920, 1080))
REFERENCE = Path(__file__).with_name("ui-baselines")


def regions(width, height):
    x, y = width // 2, height // 2
    return {
        "new-world": (0, 0, width, height),
        "food-drawer": (0, 0, 432, 386),
        "climate-goals": (x - 400, y - 330, x + 400, y + 330),
        "port-trade": (x - 250, y - 335, x + 250, y + 335),
    }


def compare(expected, actual):
    if expected.size != actual.size:
        return False
    diff = ImageChops.difference(expected, actual)
    red, green, blue = diff.split()
    changed = ImageChops.lighter(red, ImageChops.lighter(green, blue)).point(
        lambda value: 255 if value > 45 else 0
    )
    width, height = changed.size
    # Ignore harmless rasterization differences. Check each local block so
    # clipping in one row cannot be hidden by a large area of unchanged UI.
    for y in range(0, height, 16):
        for x in range(0, width, 16):
            block = changed.crop((x, y, min(x + 16, width), min(y + 16, height)))
            if block.histogram()[255] > block.width * block.height * 0.12:
                return False
    return changed.histogram()[255] <= width * height * 0.015


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("data", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--update-baselines", action="store_true")
    args = parser.parse_args()
    failures = []
    with tempfile.TemporaryDirectory(prefix="city-evolution-ui-") as tmp:
        user = Path(tmp)
        for width, height in RESOLUTIONS:
            dimensions = f"{width}x{height}"
            output = args.output.resolve() / dimensions
            output.mkdir(parents=True, exist_ok=True)
            env = dict(os.environ, SDL_VIDEO_DRIVER="offscreen", SDL_RENDER_DRIVER="software",
                       SDL_AUDIO_DRIVER="dummy", CE_UI_SCREENSHOT_DIR=str(output))
            with (output / "launch.log").open("w") as log:
                subprocess.run([
                    str(args.executable.resolve()), "--sdl", "--window", "--mute",
                    "--size", dimensions, "--app-data", str(args.data.resolve()),
                    "--user-data", str(user), "--config", str(user / "config.xml"),
                ], env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=60)
            for name, bounds in regions(width, height).items():
                path = output / f"{name}.png"
                with Image.open(path) as screen:
                    assert screen.size == (width, height), f"Wrong capture size for {path}"
                    actual = screen.convert("RGB").crop(bounds)
                reference = REFERENCE / dimensions / path.name
                if args.update_baselines:
                    reference.parent.mkdir(parents=True, exist_ok=True)
                    actual.save(reference, optimize=True)
                else:
                    with Image.open(reference) as baseline:
                        expected = baseline.convert("RGB")
                    if compare(expected, actual):
                        print(f"PASS {dimensions} {name}")
                    else:
                        if expected.size == actual.size:
                            ImageChops.difference(expected, actual).save(output / f"{name}-diff.png")
                        failures.append(f"{dimensions} {name}")
    if failures:
        raise SystemExit("UI changed: " + ", ".join(failures))


if __name__ == "__main__":
    main()
