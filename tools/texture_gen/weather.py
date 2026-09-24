"""The four `ow_weather_*` particle textures -- roadmap Section 36,
`docs/assets/WeatherParticles.md`.

`ow_weather_dust`/`ow_weather_tiberium`/`ow_weather_falling` are read as a
4x4 `PointGroupClass` frame grid, **top row only** (frames 0-3): each is a
256x256 square (64x64 px cells) with the bottom three rows left fully
transparent/black, per the doc ("must still be a square power-of-two image
so the grid divides it evenly ... ample" and "only the top row is used").
`ow_weather_rain` is the odd one out -- not on the frame grid at all, drawn
by the streak (`LineGroupClass`) renderer, which maps the texture's U along
the streak's length and V across its width; see `_rain` for the resulting
(wide, short) canvas orientation.
"""
from __future__ import annotations

import numpy as np

from . import noise as nz
from . import palette as pal
from .noise import clamp01
from .imgio import radial_field, to_rgba

GRID = 256   # 4x4 grid of 64x64 cells
CELL = GRID // 4


def _cell_origin(index):
    return 0, index * CELL  # row 0 (top row), column = index


def _blot_alpha(cell_shape, seed, radius, ragged, seed_offset=0):
    r = radial_field(cell_shape)
    bump = nz.fbm(cell_shape, 5, 3, seed + seed_offset, warp_amp=0.2, warp_freq=3)
    edge = r - radius - ragged * (bump - 0.5)
    return clamp01(1.0 - nz.smoothstep(0.0, 0.14, edge))


def _paste(canvas_rgba, cell_rgba, row, col):
    y0, x0 = row * CELL, col * CELL
    canvas_rgba[y0:y0 + CELL, x0:x0 + CELL, :] = cell_rgba


def _dust(shape, seed):
    h, w = GRID, GRID
    canvas = np.zeros((h, w, 4), dtype=np.float64)
    tan = np.array(pal.WEATHER["dust"]["tan"], dtype=np.float64) / 255.0
    for i in range(4):
        cshape = (CELL, CELL)
        radius = 0.22 + 0.05 * (i % 2)
        alpha = _blot_alpha(cshape, seed + i * 17, radius, ragged=0.12, seed_offset=i)
        alpha = clamp01(alpha * 0.4)  # "well under 1 at the centre"
        rgb = np.ones(cshape + (3,)) * tan[None, None, :]
        cell = np.dstack([rgb, alpha])
        _paste(canvas, cell, 0, i)
    return canvas


def _tiberium(shape, seed):
    h, w = GRID, GRID
    canvas = np.zeros((h, w, 4), dtype=np.float64)
    green = np.array(pal.WEATHER["tiberium"]["green"], dtype=np.float64) / 255.0
    bright = np.array(pal.WEATHER["tiberium"]["bright"], dtype=np.float64) / 255.0
    for i in range(4):
        cshape = (CELL, CELL)
        r = radial_field(cshape)
        bump = nz.fbm(cshape, 5, 3, seed + i * 23, warp_amp=0.25, warp_freq=3)
        radius = 0.2 + 0.04 * (i % 3)
        edge = r - radius - 0.1 * (bump - 0.5)
        falloff = clamp01(1.0 - nz.smoothstep(-0.05, 0.16, edge))
        core = clamp01(1.0 - nz.smoothstep(0.0, 0.12, r))
        rgb = green[None, None, :] * falloff[..., None] + bright[None, None, :] * core[..., None]
        rgb = clamp01(rgb)
        alpha = clamp01(falloff)  # black border (RGB->0 with falloff->0): additive-safe.
        cell = np.dstack([rgb, alpha])
        _paste(canvas, cell, 0, i)
    return canvas


def _falling(shape, seed):
    h, w = GRID, GRID
    canvas = np.zeros((h, w, 4), dtype=np.float64)
    grey = np.array(pal.WEATHER["falling"]["grey"], dtype=np.float64) / 255.0
    for i in range(4):
        cshape = (CELL, CELL)
        alpha = _blot_alpha(cshape, seed + i * 31, radius=0.16 + 0.03 * (i % 2), ragged=0.05, seed_offset=i)
        alpha = clamp01(alpha * 0.85)
        rgb = np.ones(cshape + (3,)) * grey[None, None, :]
        cell = np.dstack([rgb, alpha])
        _paste(canvas, cell, 0, i)
    return canvas


def _rain(shape, seed):
    """Not on the frame grid. `LineGroupClass` maps U (horizontal pixels,
    the wide axis below) along the streak's length and V (vertical pixels)
    across its width -- ParticleWidth 0.15 / ParticleHeight 0.45 is a 1:3
    width:length ratio, so the canvas is authored wide (length) and short
    (width): u=0 (left) is the falling head, u=1 (right) is the fading tail.
    """
    w, h = 384, 128
    core = np.array(pal.WEATHER["rain"]["core"], dtype=np.float64) / 255.0
    x = (np.arange(w) + 0.5) / w
    y = (np.arange(h) + 0.5) / h
    Y, X = np.meshgrid(y, x, indexing="ij")

    along = clamp01(np.sin(np.pi * X) ** 0.6)          # fades to 0 at both U ends
    head_bias = clamp01(1.0 - X) ** 0.4                # brighter/sharper near the head (u=0)
    across = np.exp(-((Y - 0.5) ** 2) / (2 * 0.16 ** 2))  # soft width falloff
    alpha = clamp01(along * (0.5 + 0.5 * head_bias) * across)

    rgb = np.ones((h, w, 3)) * core[None, None, :]
    return np.dstack([rgb, alpha])


def generate(name: str, seed: int, size: int = GRID):
    fn = {
        "ow_weather_dust": _dust,
        "ow_weather_tiberium": _tiberium,
        "ow_weather_falling": _falling,
        "ow_weather_rain": _rain,
    }[name]
    arr = fn(None, seed)
    return to_rgba(clamp01(arr[..., :3]), clamp01(arr[..., 3]))


GENERATORS = {
    "ow_weather_rain": None,
    "ow_weather_dust": None,
    "ow_weather_tiberium": None,
    "ow_weather_falling": None,
}

EXPORT_SIZE = {
    "ow_weather_rain": (384, 128),
    "ow_weather_dust": (256, 256),
    "ow_weather_tiberium": (256, 256),
    "ow_weather_falling": (256, 256),
}
