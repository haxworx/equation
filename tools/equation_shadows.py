#!/usr/bin/env python3
"""Regenerate the shared, nine-sliced window shadow (requires Pillow)."""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

# Render at 2x so corners stay smooth at fractional desktop scales. The panel
# footprint matches the frame's 18/12/18/26 logical-pixel transparent margins.
scale = 2
size = 192
mask = Image.new('L', (size * scale, size * scale))
ImageDraw.Draw(mask).rounded_rectangle(
    (18 * scale, 17 * scale, (size - 19) * scale, (size - 22) * scale),
    radius=14 * scale,
    fill=125,
)
mask = mask.filter(ImageFilter.GaussianBlur(8 * scale))
image = Image.new('RGBA', mask.size)
image.putalpha(mask)
output = (
    Path(__file__).resolve().parents[1]
    / 'data/themes/common/images/equation_window_shadow.png'
)
image.save(output)
