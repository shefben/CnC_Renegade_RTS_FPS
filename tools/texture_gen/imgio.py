"""Small image I/O / colour-compositing helpers shared by every generator.

Conventions used across this package:
- Working arrays are float64, shape (h, w) for scalar fields/masks or
  (h, w, 3) for colour, values nominally in [0, 1].
- `to_rgba(rgb01, alpha01)` is the last step before saving: it converts to
  uint8 and packs an (h, w, 4) array.
- Game-ready files are written as uncompressed 24/32-bit TGA (matches the
  canonical `.tga` names in `docs/assets/MissingAssets.md`; Pillow's TGA
  writer produces an uncompressed, straight-alpha file, not RLE/paletted).
- Source masters are written as lossless PNG at a higher working resolution
  than the game-ready export, then box-filtered down -- see `save_pair`.
"""
from __future__ import annotations

import os
import numpy as np
from PIL import Image


def to_uint8(a: np.ndarray) -> np.ndarray:
    return np.clip(np.round(a * 255.0), 0, 255).astype(np.uint8)


def to_rgb(rgb01: np.ndarray) -> Image.Image:
    return Image.fromarray(to_uint8(rgb01), mode="RGB")


def to_rgba(rgb01: np.ndarray, alpha01: np.ndarray) -> Image.Image:
    h, w = alpha01.shape
    out = np.empty((h, w, 4), dtype=np.uint8)
    out[..., :3] = to_uint8(rgb01)
    out[..., 3] = to_uint8(alpha01)
    return Image.fromarray(out, mode="RGBA")


def mix_colors(weights: np.ndarray, colors) -> np.ndarray:
    """weights: (h, w, k) blend weights (need not be pre-normalized).
    colors: sequence of k (r, g, b) tuples in 0-255. Returns (h, w, 3) in
    [0, 1]."""
    w = weights / np.clip(weights.sum(axis=-1, keepdims=True), 1e-9, None)
    pal = np.array(colors, dtype=np.float64) / 255.0  # (k, 3)
    return np.tensordot(w, pal, axes=([2], [0]))


def downsample_box(img: Image.Image, factor: int) -> Image.Image:
    if factor <= 1:
        return img
    w, h = img.size
    return img.resize((w // factor, h // factor), Image.BOX)


def save_pair(rgb_or_rgba: Image.Image, master_path: str, game_path: str, export_size: int):
    """Save the full-resolution working image as the PNG master, then a
    box-filtered, resized copy as the game-ready TGA."""
    os.makedirs(os.path.dirname(master_path), exist_ok=True)
    os.makedirs(os.path.dirname(game_path), exist_ok=True)
    rgb_or_rgba.save(master_path)
    game_img = rgb_or_rgba
    if game_img.size != (export_size, export_size) and game_img.size[0] == game_img.size[1]:
        factor = game_img.size[0] // export_size
        if factor > 1 and game_img.size[0] % export_size == 0:
            game_img = downsample_box(game_img, factor)
        else:
            game_img = game_img.resize((export_size, export_size), Image.LANCZOS)
    game_img.save(game_path)


def save_pair_wh(rgb_or_rgba: Image.Image, master_path: str, game_path: str, export_w: int, export_h: int):
    os.makedirs(os.path.dirname(master_path), exist_ok=True)
    os.makedirs(os.path.dirname(game_path), exist_ok=True)
    rgb_or_rgba.save(master_path)
    sw, sh = rgb_or_rgba.size
    if (sw, sh) != (export_w, export_h):
        if sw % export_w == 0 and sh % export_h == 0 and sw // export_w == sh // export_h:
            game_img = downsample_box(rgb_or_rgba, sw // export_w)
        else:
            game_img = rgb_or_rgba.resize((export_w, export_h), Image.LANCZOS)
    else:
        game_img = rgb_or_rgba
    game_img.save(game_path)


def radial_field(shape, cy: float = 0.5, cx: float = 0.5, aspect: float = 1.0) -> np.ndarray:
    """Normalized radial distance from (cy, cx) in UV space, 0 at center."""
    h, w = shape
    y = (np.arange(h) + 0.5) / h - cy
    x = ((np.arange(w) + 0.5) / w - cx) * aspect
    Y, X = np.meshgrid(y, x, indexing="ij")
    return np.sqrt(Y * Y + X * X)


def tile_preview(img: Image.Image, n: int = 4) -> Image.Image:
    w, h = img.size
    sheet = Image.new(img.mode, (w * n, h * n))
    for j in range(n):
        for i in range(n):
            sheet.paste(img, (i * w, j * h))
    return sheet


def luminance_stats(img: Image.Image):
    arr = np.array(img.convert("L"), dtype=np.float64)
    return {
        "mean": float(arr.mean()),
        "std": float(arr.std()),
        "min": float(arr.min()),
        "max": float(arr.max()),
    }
