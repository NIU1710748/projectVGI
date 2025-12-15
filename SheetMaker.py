from PIL import Image
import math
import glob
import os
import re

# === CONFIG ===
FRAMES_DIR   = r"C:\Users\artur\Desktop\VERSIONES VGI\VERSION 9_12_25\EntornVGI\textures\hands\Animation18" 
PATTERN      = "Animation18_*.png"
OUTPUT_NAME  = "Animation18_sheet.png"

COLS         = 22
TARGET_W     = 256
TARGET_H     = 256
# =============

def frame_index_from_name(path: str) -> int:
    """
    Intenta extraer el último número del nombre de archivo.
    Da igual si se llama Animation3_1.png o Animation3_001.png.
    """
    base = os.path.basename(path)
    nums = re.findall(r'\d+', base)
    if not nums:
        return 0
    return int(nums[-1])   # cogemos el último bloque numérico

def main():
    pattern_path = os.path.join(FRAMES_DIR, PATTERN)
    files = glob.glob(pattern_path)

    if not files:
        print("No he encontrado PNGs con el patrón:", pattern_path)
        return

    # 🔥 ORDEN NUMÉRICO, no lexicográfico
    files = sorted(files, key=frame_index_from_name)

    frame_count = len(files)
    print(f"Encontrados {frame_count} frames")

    fw, fh = TARGET_W, TARGET_H
    print(f"Tamaño de cada frame en la sheet: {fw}x{fh}")

    rows = math.ceil(frame_count / COLS)
    print(f"Sheet tendrá {COLS} columnas x {rows} filas")

    sheet_w = COLS * fw
    sheet_h = rows * fh
    sheet = Image.new("RGBA", (sheet_w, sheet_h), (0, 0, 0, 0))

    for idx, filepath in enumerate(files):
        img = Image.open(filepath).convert("RGBA")
        img = img.resize((TARGET_W, TARGET_H), Image.LANCZOS)

        col = idx % COLS
        row = idx // COLS
        x = col * fw
        y = row * fh
        sheet.paste(img, (x, y))
        img.close()

    output_path = os.path.join(FRAMES_DIR, OUTPUT_NAME)
    sheet.save(output_path)
    print("Guardado sheet en:", output_path)

if __name__ == "__main__":
    main()
