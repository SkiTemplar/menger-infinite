"""Diferencia media absoluta por canal (0-255) entre dos imágenes del mismo tamaño.

Uso: uv run --with pillow --with numpy python Tools/imgdiff.py a.png b.png
"""

import sys

import numpy as np
from PIL import Image


def mean_abs_diff(path_a: str, path_b: str) -> float:
    a = np.asarray(Image.open(path_a).convert("RGB"), dtype=np.float64)
    b = np.asarray(Image.open(path_b).convert("RGB"), dtype=np.float64)
    if a.shape != b.shape:
        raise ValueError(f"Tamaños distintos: {a.shape} frente a {b.shape}")
    return float(np.mean(np.abs(a - b)))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("Uso: imgdiff.py a.png b.png")
    print(f"{mean_abs_diff(sys.argv[1], sys.argv[2]):.4f}")
