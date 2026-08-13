#!/usr/bin/env python3
"""Whisperlet app icon: deep-navy rounded square, five rounded waveform bars.

Outputs assets/icon-<size>.png, assets/whisperlet.ico and the iconset dir
for iconutil (macOS):
    iconutil -c icns assets/whisperlet.iconset -o assets/whisperlet.icns
"""
from PIL import Image, ImageDraw
import os

ROOT = os.path.join(os.path.dirname(__file__), "..", "assets")
os.makedirs(ROOT, exist_ok=True)

TOP = (24, 26, 34)       # background gradient, top
BOTTOM = (12, 13, 18)    # background gradient, bottom
ACCENT = (10, 132, 255)  # Theme::Accent
WHITE = (242, 242, 245)


def draw_icon(size: int) -> Image.Image:
    # Draw at 4x and downscale — cheap, clean anti-aliasing at every size.
    S = size * 4
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # Vertical gradient background
    grad = Image.new("RGBA", (1, S))
    for y in range(S):
        t = y / (S - 1)
        grad.putpixel((0, y), tuple(int(TOP[i] + (BOTTOM[i] - TOP[i]) * t) for i in range(3)) + (255,))
    grad = grad.resize((S, S))

    mask = Image.new("L", (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, S - 1, S - 1], radius=int(S * 0.225), fill=255)
    img.paste(grad, (0, 0), mask)
    d = ImageDraw.Draw(img)

    # Waveform: five vertical bars, rounded caps, symmetric around center.
    # Center bar white and tallest; neighbours accent blue.
    cx, cy = S / 2, S / 2
    bar_w = S * 0.088
    gap = S * 0.075
    heights = [0.26, 0.46, 0.68, 0.46, 0.26]  # fraction of S
    colors = [ACCENT, ACCENT, WHITE, ACCENT, ACCENT]

    total = 5 * bar_w + 4 * gap
    x = cx - total / 2
    for h, col in zip(heights, colors):
        bh = S * h
        d.rounded_rectangle([x, cy - bh / 2, x + bar_w, cy + bh / 2],
                            radius=bar_w / 2, fill=col + (255,))
        x += bar_w + gap

    return img.resize((size, size), Image.LANCZOS)


sizes = [16, 32, 48, 64, 128, 256, 512, 1024]
imgs = {s: draw_icon(s) for s in sizes}

for s, im in imgs.items():
    im.save(os.path.join(ROOT, f"icon-{s}.png"))

imgs[256].save(os.path.join(ROOT, "whisperlet.ico"),
               sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])

iconset = os.path.join(ROOT, "whisperlet.iconset")
os.makedirs(iconset, exist_ok=True)
for pt in [16, 32, 128, 256, 512]:
    imgs[pt].save(os.path.join(iconset, f"icon_{pt}x{pt}.png"))
    imgs[pt * 2].save(os.path.join(iconset, f"icon_{pt}x{pt}@2x.png"))

print("done ->", ROOT)
