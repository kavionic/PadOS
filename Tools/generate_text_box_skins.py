#!/usr/bin/env python3

# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

"""Render the text-box PNG skins from their theme palettes using only stdlib."""

import argparse
import math
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
import zlib


IMAGE_SIZE = 32
CORNER_RADIUS = 5.0
SUBPIXEL_COUNT = 8
STATES = (("Normal", "normal", 0.42, 1.0), ("ReadOnly", "read_only", 0.24, 0.45), ("Disabled", "disabled", 0.20, 0.65))
LIGHT = (255, 255, 255)
SHADOW = (0, 0, 0)


def mix(first, second, weight):
    return tuple(start + (end - start) * weight for start, end in zip(first, second))


def parse_color(value):
    return tuple(int(value[offset:offset + 2], 16) for offset in (1, 3, 5))


def sample_frame(position_x, position_y, fill, border, depth, raised):
    # Rounded-box distance and its outward normal keep the straight slices flat.
    center = IMAGE_SIZE * 0.5
    delta_x = position_x - center
    delta_y = position_y - center
    corner_x = max(abs(delta_x) - (center - CORNER_RADIUS), 0.0)
    corner_y = max(abs(delta_y) - (center - CORNER_RADIUS), 0.0)
    distance_to_corner = math.hypot(corner_x, corner_y)
    distance = distance_to_corner - CORNER_RADIUS
    if distance >= 0.0:
        return None
    if distance <= -2.0:
        return fill
    normal_x = math.copysign(corner_x / distance_to_corner, delta_x)
    normal_y = math.copysign(corner_y / distance_to_corner, delta_y)
    lighting = -(normal_x + normal_y) * 0.5
    if not raised:
        lighting = -lighting
    base = border if distance > -1.0 else fill
    tint = LIGHT if lighting > 0.0 else SHADOW
    return mix(base, tint, abs(lighting) * depth * 0.50)


def render_frame(fill, foreground, outline_weight, depth, raised):
    border = mix(fill, foreground, outline_weight)
    pixels = bytearray()
    sample_count = SUBPIXEL_COUNT * SUBPIXEL_COUNT
    for pixel_y in range(IMAGE_SIZE):
        for pixel_x in range(IMAGE_SIZE):
            channels = [0.0, 0.0, 0.0]
            coverage = 0
            for sample_y in range(SUBPIXEL_COUNT):
                for sample_x in range(SUBPIXEL_COUNT):
                    position_x = pixel_x + (sample_x + 0.5) / SUBPIXEL_COUNT
                    position_y = pixel_y + (sample_y + 0.5) / SUBPIXEL_COUNT
                    color = sample_frame(position_x, position_y, fill, border, depth, raised)
                    if color is not None:
                        coverage += 1
                        for channel in range(3):
                            channels[channel] += color[channel]
            if coverage:
                pixels.extend(round(value / coverage) for value in channels)
                pixels.append(round(255 * coverage / sample_count))
            else:
                pixels.extend((0, 0, 0, 0))
    return pixels


def png_chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def write_png(path, pixels):
    stride = IMAGE_SIZE * 4
    rows = b"".join(b"\x00" + pixels[offset:offset + stride] for offset in range(0, len(pixels), stride))
    header = struct.pack(">IIBBBBB", IMAGE_SIZE, IMAGE_SIZE, 8, 6, 0, 0, 0)
    data = b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", header) + png_chunk(b"IDAT", zlib.compress(rows, 9)) + png_chunk(b"IEND", b"")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def main():
    default_directory = Path(__file__).resolve().parents[1] / "SDCard/GUI/Styles/Skinned"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--styles-dir", type=Path, default=default_directory)
    arguments = parser.parse_args()
    for theme in ("Normal", "Dark"):
        directory = arguments.styles_dir / theme
        palette = ET.parse(directory / "Style.xml").getroot()
        foreground = parse_color(palette.attrib["color_text_box_text_normal"])
        for state, attribute, outline_weight, depth in STATES:
            fill = parse_color(palette.attrib[f"color_text_box_background_{attribute}"])
            for raised in (False, True):
                pixels = render_frame(fill, foreground, outline_weight, depth, raised)
                prefix = "TextBoxRaised" if raised else "TextBox"
                write_png(directory / "TextBox" / f"{prefix}{state}.png", pixels)


if __name__ == "__main__":
    main()
