"""The five `ow_ribbon_*` ground-track strips -- roadmap Section 23,
`docs/assets/RibbonSurfaces.md`.

`u` runs 0 (one edge of the strip) to 1 (the other) and never repeats; `v`
runs along the direction of travel and repeats every "tile" metres
(`RibbonSurfaces.md`'s table). So: seamless top-to-bottom only (built from
`noise.fbm`, periodic in Y by construction), and the pattern must reach both
U edges at full strength -- "anything transparent at the left or right edge
becomes a track narrower than the vehicle that made it" -- so none of the
alpha fields below fade U all the way to 0 at u=0/u=1.
`Ribbon_Shader` is straight alpha-blended, so RGB stays independent of alpha.
"""
from __future__ import annotations

import numpy as np

from . import noise as nz
from . import palette as pal
from .noise import clamp01
from .imgio import to_rgba

WORK_W = 512


def _cleated(shape, seed, colors, cleats_per_tile, cleat_duty, rail_frac, depth):
    h, w = shape
    Y, X = nz.uv_grid(shape)
    dark = np.array(colors["dark"], dtype=np.float64) / 255.0
    light = np.array(colors["light"], dtype=np.float64) / 255.0

    grain = nz.fbm(shape, 24, 3, seed, warp_amp=0.03, warp_freq=4)
    rgb = dark[None, None, :] * (1 - grain[..., None]) + light[None, None, :] * grain[..., None]
    rgb = clamp01(rgb)

    cleat_phase = (Y * cleats_per_tile) % 1.0
    cleat = clamp01(1.0 - np.abs(cleat_phase - 0.5) / (cleat_duty * 0.5))
    cleat = nz.quintic(clamp01(cleat))

    rail = clamp01(1.0 - np.abs(X - rail_frac) / 0.03) + clamp01(1.0 - np.abs(X - (1 - rail_frac)) / 0.03)
    rail = clamp01(rail)

    edge_taper = clamp01(np.minimum(X, 1.0 - X) / 0.04)  # 0 exactly at u=0/1, 1 a hair inside
    shape_mask = clamp01(cleat * depth + rail)
    alpha = clamp01(shape_mask * (0.35 + 0.65 * edge_taper) + (1 - edge_taper) * 0.9)
    return to_rgba(rgb, alpha)


def _tank_track(shape, seed):
    return _cleated(shape, seed, pal.RIBBONS["tank_track"], cleats_per_tile=5,
                     cleat_duty=0.55, rail_frac=0.08, depth=1.0)


def _harvester_track(shape, seed):
    return _cleated(shape, seed, pal.RIBBONS["harvester_track"], cleats_per_tile=4,
                     cleat_duty=0.65, rail_frac=0.06, depth=1.0)


def _tire_track(shape, seed):
    h, w = shape
    p = pal.RIBBONS["tire_track"]
    Y, X = nz.uv_grid(shape)
    dark = np.array(p["dark"], dtype=np.float64) / 255.0
    light = np.array(p["light"], dtype=np.float64) / 255.0
    grain = nz.fbm(shape, 30, 3, seed, warp_amp=0.02, warp_freq=4)
    rgb = clamp01(dark[None, None, :] * (1 - grain[..., None]) + light[None, None, :] * grain[..., None])

    # Fine zig-zag block tread, full width -- one tire's contact patch.
    zigzag = np.abs(((X * 3.0 + Y * 8.0) % 1.0) - 0.5)
    tread = clamp01(1.0 - zigzag / 0.3)
    tread = nz.quintic(clamp01(tread))
    edge_taper = clamp01(np.minimum(X, 1.0 - X) / 0.05)
    alpha = clamp01(tread * 0.8 * (0.3 + 0.7 * edge_taper) + (1 - edge_taper) * 0.85)
    return to_rgba(rgb, alpha)


def _scorch_trail(shape, seed):
    p = pal.RIBBONS["scorch_trail"]
    Y, X = nz.uv_grid(shape)
    soot = np.array(p["soot"], dtype=np.float64) / 255.0
    ring = np.array(p["ring"], dtype=np.float64) / 255.0
    mottle = nz.fbm(shape, 20, 4, seed, warp_amp=0.04, warp_freq=5)
    rgb = clamp01(soot[None, None, :] * (1 - mottle[..., None] * 0.5) + ring[None, None, :] * (mottle[..., None] * 0.5))
    # Continuous burnt trail: no cleats, alpha stays high and full-width;
    # only mild noise variation so it still reads as scorched, not painted.
    alpha = clamp01(0.75 + 0.2 * (mottle - 0.5))
    return to_rgba(rgb, alpha)


def _drag_mark(shape, seed):
    p = pal.RIBBONS["drag_mark"]
    Y, X = nz.uv_grid(shape)
    dark = np.array(p["dark"], dtype=np.float64) / 255.0
    light = np.array(p["light"], dtype=np.float64) / 255.0
    scrape = nz.fbm(shape, 16, 3, seed, warp_amp=0.03, warp_freq=4)
    center = clamp01(1.0 - np.abs(X - 0.5) / 0.45)
    rgb = clamp01(dark[None, None, :] * center[..., None] * (0.8 + 0.3 * scrape[..., None])
                  + light[None, None, :] * (1 - center[..., None]))
    alpha = clamp01(0.35 + 0.55 * center + 0.1 * (scrape - 0.5))
    return to_rgba(rgb, alpha)


GENERATORS = {
    "ow_ribbon_tank_track": (_tank_track, 0.90, 2.0),
    "ow_ribbon_harvester_track": (_harvester_track, 1.40, 3.0),
    "ow_ribbon_tire_track": (_tire_track, 0.30, 2.0),
    "ow_ribbon_scorch_trail": (_scorch_trail, 1.20, 4.0),
    "ow_ribbon_drag_mark": (_drag_mark, 0.50, 2.0),
}

EXPORT = {
    # (width, height): pixel aspect matches width : tile-length so the
    # tread pattern isn't stretched (RibbonSurfaces.md's table).
    "ow_ribbon_tank_track": (256, 512),        # 0.9 : 2 ~= 1:2.2
    "ow_ribbon_harvester_track": (256, 512),   # 1.4 : 3 ~= 1:2.1
    "ow_ribbon_tire_track": (128, 1024),       # 0.3 : 2 = 1:6.7 (approx, 128:1024=1:8)
    "ow_ribbon_scorch_trail": (256, 1024),     # 1.2 : 4 = 1:3.3 (approx, 256:1024=1:4)
    "ow_ribbon_drag_mark": (256, 1024),        # 0.5 : 2 = 1:4
}


def generate(name: str, seed: int, size: int = WORK_W):
    fn, _width_m, _tile_m = GENERATORS[name]
    ew, eh = EXPORT[name]
    aspect = eh / ew
    shape = (int(round(size * aspect)), size)
    return fn(shape, seed)
