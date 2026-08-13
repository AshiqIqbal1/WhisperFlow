#!/usr/bin/env python3
"""Generate the Whisperlet app icon: dark rounded square, accent-blue mic.

Outputs assets/icon-<size>.png, assets/whisperlet.ico (Windows) and an
iconset dir ready for iconutil (macOS .icns, run on a Mac):
    iconutil -c icns assets/whisperlet.iconset -o assets/whisperlet.icns
"""
from PIL import Image, ImageDraw
import os

ROOT = os.path.join(os.path.dirname(__file__), "..", "assets")
os.makedirs(ROOT, exist_ok=True)

BG = (20, 20, 22, 255)         # matches Theme::Window
ACCENT = (10, 132, 255, 255)   # Theme::Accent
FG = (245, 245, 247, 255)      # Theme::RecordIdle


def draw_icon(size: int) -> Image.Image:
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    s = size / 1024.0  # design in 1024-space

    # Rounded-square plate
    r = 232 * s
    d.rounded_rectangle([0, 0, size - 1, size - 1], radius=r, fill=BG)

    # Soft accent glow ring behind the mic
    cx, cy = size / 2, size / 2 - 40 * s
    glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow)
    for i, alpha in ((300, 26), (240, 40), (190, 26)):
        rad = i * s
        gd.ellipse([cx - rad, cy - rad, cx + rad, cy + rad],
                   fill=(ACCENT[0], ACCENT[1], ACCENT[2], alpha))
    img = Image.alpha_composite(img, glow)
    d = ImageDraw.Draw(img)

    # Mic capsule
    w, h = 150 * s, 320 * s
    d.rounded_rectangle([cx - w, cy - h, cx + w, cy + h], radius=w, fill=FG)

    # Mic cradle (open arc) + stem + base, in accent
    lw = max(1, int(56 * s))
    arc_r = 260 * s
    d.arc([cx - arc_r, cy - arc_r, cx + arc_r, cy + arc_r], start=25, end=155,
          fill=ACCENT, width=lw)
    d.line([cx, cy + arc_r, cx, cy + arc_r + 120 * s], fill=ACCENT, width=lw)
    d.line([cx - 130 * s, cy + arc_r + 150 * s, cx + 130 * s, cy + arc_r + 150 * s],
           fill=ACCENT, width=lw)

    return img


sizes = [16, 32, 48, 64, 128, 256, 512, 1024]
imgs = {s: draw_icon(s) for s in sizes}

for s, im in imgs.items():
    im.save(os.path.join(ROOT, f"icon-{s}.png"))

# Windows .ico
imgs[256].save(os.path.join(ROOT, "whisperlet.ico"),
               sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])

# macOS iconset (iconutil turns this into .icns)
iconset = os.path.join(ROOT, "whisperlet.iconset")
os.makedirs(iconset, exist_ok=True)
for pt in [16, 32, 128, 256, 512]:
    imgs[pt].save(os.path.join(iconset, f"icon_{pt}x{pt}.png"))
    imgs[pt * 2].save(os.path.join(iconset, f"icon_{pt}x{pt}@2x.png"))

print("done ->", ROOT)
