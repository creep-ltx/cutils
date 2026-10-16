import sys
from PIL import Image
# crop.py IN OUT [MINH]: cut empty rows (background colour) off the bottom, keep 1 cell row margin
im = Image.open(sys.argv[1]).convert("RGB"); w, h = im.size
bg = im.getpixel((w - 2, h - 2)); px = im.load()
last = 0
for y in range(h):
    if any(px[x, y] != bg for x in range(0, w, 1)): last = y
cut = min(h, last + 16)
minh = int(sys.argv[3]) if len(sys.argv) > 3 else 0
im.crop((0, 0, w, max(cut, minh))).save(sys.argv[2], optimize=True)
