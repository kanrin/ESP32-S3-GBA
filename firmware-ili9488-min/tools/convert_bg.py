"""Convert Pokemon.jpg to raw RGB565 for ILI9488 display (480x320 landscape)."""
from PIL import Image
import numpy as np
import os

# Paths
src = os.path.join(os.path.dirname(__file__), '..', '..', 'pics', 'Pokemon.jpg')
dst = os.path.join(os.path.dirname(__file__), '..', 'roms', 'bg.raw')

print(f"Source: {src}")
print(f"Dest:   {dst}")

# Open and resize
img = Image.open(src).resize((480, 320), Image.LANCZOS)
print(f"Resized to: {img.size}")

# Convert to numpy array for fast processing
arr = np.array(img, dtype=np.uint8)  # shape: (320, 480, 3)

# RGB565 conversion using vectorized operations
r = arr[:, :, 0].astype(np.uint16)
g = arr[:, :, 1].astype(np.uint16)
b = arr[:, :, 2].astype(np.uint16)

rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

# Write as raw little-endian uint16
rgb565.astype('<u2').tofile(dst)

size = os.path.getsize(dst)
expected = 480 * 320 * 2
print(f"Written: {size} bytes (expected: {expected})")
print("Done!")
