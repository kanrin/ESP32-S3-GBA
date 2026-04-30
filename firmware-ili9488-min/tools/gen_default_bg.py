"""Generate default_bg.h from Pokemon.jpg - embedded default background."""
from PIL import Image
import numpy as np
import os

# Paths
script_dir = os.path.dirname(os.path.abspath(__file__))
project_dir = os.path.join(script_dir, '..')
src = os.path.join(project_dir, '..', 'pics', 'Pokemon.jpg')
dst = os.path.join(project_dir, 'src', 'drivers', 'default_bg.h')

print(f"Source: {src}")
print(f"Dest:   {dst}")

# Open and resize
img = Image.open(src).resize((480, 320), Image.LANCZOS)
print(f"Resized to: {img.size}")

# Convert to numpy array
arr = np.array(img, dtype=np.uint8)
r = arr[:, :, 0].astype(np.uint16)
g = arr[:, :, 1].astype(np.uint16)
b = arr[:, :, 2].astype(np.uint16)
rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
flat = rgb565.flatten()

print(f"Total pixels: {len(flat)}")

# Write header file
with open(dst, 'w') as f:
    f.write('// Auto-generated default background image (480x320 RGB565)\n')
    f.write('// Source: pics/Pokemon.jpg\n')
    f.write('// Regenerate with: python tools/gen_default_bg.py\n')
    f.write('#pragma once\n\n')
    f.write('#include <Arduino.h>\n\n')
    f.write('namespace drivers {\n\n')
    f.write('constexpr int kDefaultBgWidth = 480;\n')
    f.write('constexpr int kDefaultBgHeight = 320;\n\n')
    f.write('// 480 * 320 = 153,600 pixels x 2 bytes = 307,200 bytes\n')
    f.write('alignas(4) static const uint16_t kDefaultBg[] = {\n')

    # Write 16 values per line
    for i in range(0, len(flat), 16):
        chunk = flat[i:i+16]
        line = ', '.join(f'0x{v:04X}' for v in chunk)
        f.write(f'  {line},\n')

    f.write('};\n\n')
    f.write('}  // namespace drivers\n')

size = os.path.getsize(dst)
print(f"Header file size: {size:,} bytes")
print("Done!")
