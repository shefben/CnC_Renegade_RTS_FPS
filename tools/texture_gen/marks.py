"""The seven `ow_mark_*` decals -- roadmap Section 35, `docs/assets/SurfaceMarks.md`.

Every mark is a single non-tiling quad, `u`/`v` each running 0..1 once
across the whole image (`SurfaceMarks.md`: "The quad is the whole image").
`Mark_Shader` is alpha-blended (straight alpha, not premultiplied) and
per-vertex alpha already carries the fade/opacity, so:

- the texture's own alpha is only the *shape* of the mark and must reach
  fully transparent well inside the image edge -- a mark opaque to its
  border makes a visible hard-edged square, "the one thing a scorch must
  not be";
- RGB stays independent of alpha (straight, not premultiplied) and leans on
  darkening rather than saturated colour, because a mark is seen on every
  ground surface in the game and a strongly-coloured mark fights at least
  one of them.

Shapes are built from a radial field distorted by tileable noise (the noise
doesn't need to tile here -- nothing about a single decal repeats -- but
reusing `noise.fbm`/`noise.worley` is still the simplest way to get organic,
non-perfectly-circular silhouettes instead of a compass-drawn circle).
"""
from __future__ import annotations

import numpy as np

from . import noise as nz
from . import palette as pal
from .noise import clamp01
from .imgio import radial_field, to_rgba

WORK = 1024


def _ragged_alpha(shape, seed, radius, ragged_amp, edge_soft, cell_freq=10):
    r = radial_field(shape)
    f1, _ = nz.worley(shape, cell_freq, seed, warp_amp=0.3, warp_freq=3)
    bumpy = nz.fbm(shape, 6, 4, seed + 1, warp_amp=0.15, warp_freq=3)
    edge = r - radius - ragged_amp * (bumpy - 0.5) - ragged_amp * 0.4 * (f1 - f1.mean())
    alpha = clamp01(1.0 - nz.smoothstep(0.0, edge_soft, edge))
    return alpha


def _tint(rgb_shape, color):
    c = np.array(color, dtype=np.float64) / 255.0
    return np.ones(rgb_shape, dtype=np.float64) * c[None, None, :]


def _scorch(shape, seed):
    p = pal.MARKS["scorch"]
    r = radial_field(shape)
    alpha = _ragged_alpha(shape, seed, radius=0.32, ragged_amp=0.08, edge_soft=0.16)
    soot = clamp01(1.0 - r / 0.35)
    rgb = _tint(shape + (3,), p["ring"])
    soot_col = np.array(p["soot"], dtype=np.float64) / 255.0
    rgb = rgb * (1 - soot[..., None]) + soot_col[None, None, :] * soot[..., None]
    grain = nz.fbm(shape, 30, 3, seed + 5)
    rgb = clamp01(rgb * (0.85 + 0.25 * grain[..., None]))
    return to_rgba(rgb, alpha)


def _blast(shape, seed):
    p = pal.MARKS["blast"]
    h, w = shape
    r = radial_field(shape)
    y = (np.arange(h) + 0.5) / h - 0.5
    x = (np.arange(w) + 0.5) / w - 0.5
    Y, X = np.meshgrid(y, x, indexing="ij")
    theta = np.arctan2(Y, X)
    rays = clamp01(0.5 + 0.5 * np.sin(theta * 9 + nz.fbm(shape, 5, 2, seed + 9) * 4))
    alpha = _ragged_alpha(shape, seed, radius=0.4, ragged_amp=0.1, edge_soft=0.2, cell_freq=8)

    soot = clamp01(1.0 - r / 0.22)
    dirt_ring = clamp01(nz.smoothstep(0.18, 0.4, r)) * clamp01(1.0 - nz.smoothstep(0.35, 0.48, r))
    dirt_ring *= 0.6 + 0.4 * rays

    dirt_col = np.array(p["dirt"], dtype=np.float64) / 255.0
    mid_col = np.array(p["mid"], dtype=np.float64) / 255.0
    center_col = np.array(p["center"], dtype=np.float64) / 255.0
    rgb = mid_col[None, None, :] * np.ones(shape + (1,))
    rgb = rgb * (1 - dirt_ring[..., None]) + dirt_col[None, None, :] * dirt_ring[..., None]
    rgb = rgb * (1 - soot[..., None]) + center_col[None, None, :] * soot[..., None]
    grain = nz.fbm(shape, 26, 3, seed + 6)
    rgb = clamp01(rgb * (0.85 + 0.25 * grain[..., None]))
    return to_rgba(rgb, alpha)


def _impact(shape, seed):
    p = pal.MARKS["impact"]
    r = radial_field(shape)
    alpha = _ragged_alpha(shape, seed, radius=0.12, ragged_amp=0.03, edge_soft=0.1, cell_freq=14)
    hole = clamp01(1.0 - nz.smoothstep(0.04, 0.11, r))
    rim = clamp01(1.0 - nz.smoothstep(0.09, 0.16, r)) * clamp01(nz.smoothstep(0.03, 0.09, r))
    hole_col = np.array(p["hole"], dtype=np.float64) / 255.0
    rim_col = np.array(p["rim"], dtype=np.float64) / 255.0
    rgb = rim_col[None, None, :] * np.ones(shape + (1,))
    rgb = rgb * (1 - hole[..., None]) + hole_col[None, None, :] * hole[..., None]
    return to_rgba(clamp01(rgb), alpha)


def _dirt(shape, seed):
    p = pal.MARKS["dirt"]
    alpha = _ragged_alpha(shape, seed, radius=0.3, ragged_amp=0.12, edge_soft=0.2, cell_freq=7)
    loose = np.array(p["loose"], dtype=np.float64) / 255.0
    dark = np.array(p["dark"], dtype=np.float64) / 255.0
    w = clamp01(nz.fbm(shape, 18, 3, seed + 2))
    rgb = loose[None, None, :] * (1 - w[..., None] * 0.5) + dark[None, None, :] * (w[..., None] * 0.5)
    return to_rgba(clamp01(rgb), alpha)


def _stain(shape, seed):
    p = pal.MARKS["stain"]
    r = radial_field(shape)
    puddle = nz.fbm(shape, 5, 4, seed, warp_amp=0.25, warp_freq=3)
    edge = r - 0.28 - 0.14 * (puddle - 0.5)
    alpha_outer = clamp01(1.0 - nz.smoothstep(0.0, 0.12, edge))
    density = clamp01(0.55 + 0.45 * (puddle - 0.3) * 2.0)
    alpha = clamp01(alpha_outer * (0.55 + 0.45 * density))
    oil = np.array(p["oil"], dtype=np.float64) / 255.0
    rim = np.array(p["rim"], dtype=np.float64) / 255.0
    rim_band = clamp01(nz.smoothstep(0.75, 1.0, alpha_outer)) * clamp01(1.0 - nz.smoothstep(0.9, 1.0, alpha_outer))
    rgb = oil[None, None, :] * np.ones(shape + (1,))
    rgb = rgb * (1 - rim_band[..., None] * 0.5) + rim[None, None, :] * (rim_band[..., None] * 0.5)
    return to_rgba(clamp01(rgb), alpha)


def _world(shape, seed):
    p = pal.MARKS["world"]
    r = radial_field(shape)
    alpha = _ragged_alpha(shape, seed, radius=0.34, ragged_amp=0.1, edge_soft=0.22, cell_freq=9)
    mark = np.array(p["mark"], dtype=np.float64) / 255.0
    edge_c = np.array(p["edge"], dtype=np.float64) / 255.0
    w = clamp01(nz.smoothstep(0.15, 0.32, r))
    rgb = mark[None, None, :] * (1 - w[..., None]) + edge_c[None, None, :] * w[..., None]
    grain = nz.fbm(shape, 22, 3, seed + 3)
    rgb = clamp01(rgb * (0.85 + 0.25 * grain[..., None]))
    return to_rgba(rgb, alpha)


def _construction(shape, seed):
    h, w = shape
    p = pal.MARKS["construction"]
    r = radial_field(shape, aspect=1.0)
    # A wide, softly-rounded rectangular pad rather than a circle: max-norm
    # distance reads as a rounded square.
    y = np.abs((np.arange(h) + 0.5) / h - 0.5) * 2
    x = np.abs((np.arange(w) + 0.5) / w - 0.5) * 2
    Y, X = np.meshgrid(y, x, indexing="ij")
    box = np.maximum(Y, X)
    bumpy = nz.fbm(shape, 8, 3, seed, warp_amp=0.06, warp_freq=3)
    edge = box - 0.72 - 0.025 * (bumpy - 0.5)
    alpha = clamp01(1.0 - nz.smoothstep(0.0, 0.05, edge))

    pad = np.array(p["pad"], dtype=np.float64) / 255.0
    dark = np.array(p["dark"], dtype=np.float64) / 255.0
    mottle = nz.fbm(shape, 16, 4, seed + 1)
    rgb = pad[None, None, :] * (0.85 + 0.3 * mottle[..., None])
    rgb = clamp01(rgb)

    # Corner tick/survey marks (positioning marks, per SurfaceMarks.md).
    line_col = np.array(p["line"], dtype=np.float64) / 255.0
    tick = np.zeros(shape)
    inset, length, thick = 0.24, 0.13, 0.014
    for cy, cx, sy, sx in [(inset, inset, 1, 1), (inset, 1 - inset, 1, -1),
                           (1 - inset, inset, -1, 1), (1 - inset, 1 - inset, -1, -1)]:
        yy = (np.arange(h) + 0.5) / h
        xx = (np.arange(w) + 0.5) / w
        Yg, Xg = np.meshgrid(yy, xx, indexing="ij")
        vert = (np.abs(Xg - cx) < thick) & (((Yg - cy) * sy >= 0) & (np.abs(Yg - cy) < length))
        horiz = (np.abs(Yg - cy) < thick) & (((Xg - cx) * sx >= 0) & (np.abs(Xg - cx) < length))
        tick = np.maximum(tick, (vert | horiz).astype(np.float64))
    rgb = rgb * (1 - tick[..., None]) + line_col[None, None, :] * tick[..., None]

    dark_edge = clamp01(nz.smoothstep(0.5, 0.68, box))
    rgb = rgb * (1 - dark_edge[..., None] * 0.35) + dark[None, None, :] * (dark_edge[..., None] * 0.35)
    return to_rgba(clamp01(rgb), alpha)


GENERATORS = {
    "ow_mark_scorch": (_scorch,),
    "ow_mark_blast": (_blast,),
    "ow_mark_impact": (_impact,),
    "ow_mark_dirt": (_dirt,),
    "ow_mark_stain": (_stain,),
    "ow_mark_world": (_world,),
    "ow_mark_construction": (_construction,),
}

# Larger marks (blast, construction) get more resolution to hold their
# detail; small ones (impact) don't need it -- see SurfaceMarks.md's radius
# table (impact 0.25 m vs blast 3 m / construction 4 m).
EXPORT_SIZE = {
    "ow_mark_scorch": 512,
    "ow_mark_blast": 1024,
    "ow_mark_impact": 256,
    "ow_mark_dirt": 512,
    "ow_mark_stain": 512,
    "ow_mark_world": 512,
    "ow_mark_construction": 1024,
}


def generate(name: str, seed: int, size: int = WORK):
    (fn,) = GENERATORS[name]
    return fn((size, size), seed)
