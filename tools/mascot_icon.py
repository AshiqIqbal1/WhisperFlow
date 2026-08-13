#!/usr/bin/env python3
"""Turn assets/mascot-source.png (pasted mascot art on a grid background)
into the app icon set: background removed via edge flood-fill, tight crop,
square pad, then every size + .ico + iconset for iconutil.
"""
from PIL import Image
from collections import deque
import os

ROOT = os.path.join(os.path.dirname(__file__), "..", "assets")
SRC = os.path.join(ROOT, "mascot-source.png")

img = Image.open(SRC).convert("RGBA")
w, h = img.size
px = img.load()


def is_background(p):
    r, g, b, a = p
    # The paper background: light gray/white, including grid lines and their
    # darker crossing dots. Mascot whites are pure enough to stay above this.
    return r > 175 and g > 175 and b > 175 and abs(r - g) < 22 and abs(g - b) < 22


# Flood fill from every edge pixel: only background CONNECTED to the border
# becomes transparent — the whites inside the mascot stay opaque.
visited = bytearray(w * h)
q = deque()
for x in range(w):
    q.append((x, 0)); q.append((x, h - 1))
for y in range(h):
    q.append((0, y)); q.append((w - 1, y))

while q:
    x, y = q.popleft()
    if x < 0 or y < 0 or x >= w or y >= h:
        continue
    i = y * w + x
    if visited[i]:
        continue
    visited[i] = 1
    if not is_background(px[x, y]):
        continue
    px[x, y] = (0, 0, 0, 0)
    q.extend(((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)))

# Tight crop to remaining content, then pad to square with a small margin.
bbox = img.getbbox()
img = img.crop(bbox)
side = int(max(img.size) * 1.10)
canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
canvas.paste(img, ((side - img.width) // 2, (side - img.height) // 2), img)

sizes = [16, 32, 48, 64, 128, 256, 512, 1024]
imgs = {s: canvas.resize((s, s), Image.LANCZOS) for s in sizes}

for s, im in imgs.items():
    im.save(os.path.join(ROOT, f"icon-{s}.png"))

imgs[256].save(os.path.join(ROOT, "whisperlet.ico"),
               sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])

iconset = os.path.join(ROOT, "whisperlet.iconset")
os.makedirs(iconset, exist_ok=True)
for pt in [16, 32, 128, 256, 512]:
    imgs[pt].save(os.path.join(iconset, f"icon_{pt}x{pt}.png"))
    imgs[pt * 2].save(os.path.join(iconset, f"icon_{pt}x{pt}@2x.png"))

print("done")
