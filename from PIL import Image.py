from PIL import Image
import os
import glob

FRAMES_DIR = r"C:\Users\artur\Desktop\VERSIONES VGI\VERSION 9_12_25\EntornVGI\textures\hands\Animation15"
OUTPUT_DIR = os.path.join(FRAMES_DIR, "alpha_fixed")

THRESHOLD = 15   # negro < 15 → transparente

os.makedirs(OUTPUT_DIR, exist_ok=True)

for path in glob.glob(os.path.join(FRAMES_DIR, "*.png")):
    img = Image.open(path).convert("RGBA")
    pixels = img.load()
    w, h = img.size

    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if r < THRESHOLD and g < THRESHOLD and b < THRESHOLD:
                pixels[x, y] = (0, 0, 0, 0)
            else:
                pixels[x, y] = (r, g, b, 255)

    out_path = os.path.join(OUTPUT_DIR, os.path.basename(path))
    img.save(out_path)
    img.close()

print("✅ Alpha reconstruido. Frames en:", OUTPUT_DIR)
