"""The three `ow_road_*` carriageway textures -- roadmap Section 19,
`docs/assets/RoadSurfaces.md`.

Unlike every terrain layer, a road texture does **not** tile across U: `u`
runs 0 to 1 exactly once over the whole carriageway width
(`RoadSplineClass`/`RoadSystem::Build_Geometry`), so the image is a fixed
cross-section -- kerbs, lanes, centre line, all baked into specific U
positions -- and only needs to repeat along `v` (`metresPerTile`, default
8 m). Every U-positioned feature below (kerb bands, lane lines, dashes) is
therefore authored directly as a function of X (never wrapped), while the
asphalt/dirt grain uses `noise.fbm`/`noise.worley`, which are periodic in Y
by construction and so tile along the strip with no seam.
"""
from __future__ import annotations

import numpy as np

from . import noise as nz
from . import palette as pal
from .noise import clamp01
from .imgio import mix_colors, to_rgb

WORK_W = 1024


def _dash(Y, cycles, duty=0.5, phase=0.0):
    """A duty-cycle square wave in Y, periodic over the [0, 1) tile -- an
    integer `cycles` guarantees the dash pattern lines up across the wrap."""
    t = (Y * cycles + phase) % 1.0
    return (t < duty).astype(np.float64)


def _line_mask(X, center, half_width):
    return clamp01(1.0 - np.abs(X - center) / half_width)


def _highway(shape, seed):
    h, w = shape
    p = pal.ROADS["highway"]
    Y, X = nz.uv_grid(shape)

    grain = nz.fbm(shape, 30, 4, seed, warp_amp=0.02, warp_freq=6)
    patch = nz.fbm(shape, 6, 3, seed + 1, warp_amp=0.04, warp_freq=3)
    crack_f1, crack_f2 = nz.worley(shape, 22, seed + 2, warp_amp=0.25, warp_freq=5)
    crack = clamp01(1.0 - nz.remap(crack_f2 - crack_f1, 0, 1) / 0.08) ** 4

    w_light = clamp01(nz.smoothstep(0.5, 0.6, patch))
    asphalt = np.array(p["asphalt"], dtype=np.float64) / 255.0
    asphalt_light = np.array(p["asphalt_light"], dtype=np.float64) / 255.0
    rgb = asphalt[None, None, :] * (1 - w_light[..., None]) + asphalt_light[None, None, :] * w_light[..., None]
    shade = 0.82 + 0.3 * grain
    rgb = clamp01(rgb * shade[..., None])
    wear_col = np.array(p["wear"], dtype=np.float64) / 255.0
    rgb = rgb * (1 - crack[..., None] * 0.7) + wear_col[None, None, :] * (crack[..., None] * 0.7)

    # Shoulders at both outer edges (no kerb -- a highway per RoadSurfaces.md).
    shoulder = clamp01(_line_mask(X, 0.03, 0.03) + _line_mask(X, 0.97, 0.03))
    wear_amt = shoulder * 0.5
    rgb = rgb * (1 - wear_amt[..., None]) + wear_col[None, None, :] * wear_amt[..., None]

    # Edge lines, two lanes each way, dashed centre pair (divided highway).
    line_col = np.array(p["line"], dtype=np.float64) / 255.0
    edge = clamp01(_line_mask(X, 0.06, 0.006) + _line_mask(X, 0.94, 0.006))
    lane_div_l = _line_mask(X, 0.35, 0.005) * _dash(Y, 6, 0.5)
    lane_div_r = _line_mask(X, 0.65, 0.005) * _dash(Y, 6, 0.5)
    centre = clamp01(_line_mask(X, 0.485, 0.004) + _line_mask(X, 0.515, 0.004))
    paint = clamp01(edge + lane_div_l + lane_div_r + centre)
    rgb = rgb * (1 - paint[..., None]) + line_col[None, None, :] * paint[..., None]
    return to_rgb(clamp01(rgb))


def _street(shape, seed):
    h, w = shape
    p = pal.ROADS["street"]
    Y, X = nz.uv_grid(shape)

    grain = nz.fbm(shape, 26, 4, seed, warp_amp=0.02, warp_freq=6)
    patch = nz.fbm(shape, 5, 3, seed + 1, warp_amp=0.05, warp_freq=3)
    oil_f1, _ = nz.worley(shape, 8, seed + 2, warp_amp=0.3, warp_freq=4)
    oil = clamp01(1.0 - nz.remap(oil_f1, 0, 1) / 0.3) ** 5

    base_col = np.array(p["asphalt"], dtype=np.float64) / 255.0
    patch_col = np.array(p["patch"], dtype=np.float64) / 255.0
    w_patch = clamp01(nz.smoothstep(0.6, 0.85, patch))
    rgb = base_col[None, None, :] * (1 - w_patch[..., None]) + patch_col[None, None, :] * w_patch[..., None]
    shade = 0.8 + 0.32 * grain
    rgb = clamp01(rgb * shade[..., None])
    oil_col = np.array(p["oil"], dtype=np.float64) / 255.0
    rgb = rgb * (1 - oil[..., None] * 0.8) + oil_col[None, None, :] * (oil[..., None] * 0.8)

    # Kerbs at both edges (street has them; highway/track don't).
    kerb = clamp01(_line_mask(X, 0.0, 0.05) + _line_mask(X, 1.0, 0.05))
    kerb_col = np.array(p["patch"], dtype=np.float64) / 255.0 * 1.15
    rgb = rgb * (1 - kerb[..., None] * 0.5) + clamp01(kerb_col)[None, None, :] * (kerb[..., None] * 0.5)

    line_col = np.array(p["line"], dtype=np.float64) / 255.0
    centre = _line_mask(X, 0.5, 0.006) * _dash(Y, 4, 0.5)
    rgb = rgb * (1 - centre[..., None]) + line_col[None, None, :] * centre[..., None]
    return to_rgb(clamp01(rgb))


def _track(shape, seed):
    h, w = shape
    p = pal.ROADS["track"]
    Y, X = nz.uv_grid(shape)

    grain = nz.fbm(shape, 16, 3, seed, warp_amp=0.015, warp_freq=5)
    fine = nz.fbm(shape, 90, 3, seed + 3)
    stones_f1, _ = nz.worley(shape, 30, seed + 1, warp_amp=0.06, warp_freq=6)
    stones = clamp01(1.0 - nz.remap(stones_f1, 0, 1) / 0.3) ** 4

    dirt = np.array(p["dirt"], dtype=np.float64) / 255.0
    light = np.array(p["dirt_light"], dtype=np.float64) / 255.0
    rgb = dirt[None, None, :] * (1 - stones[..., None]) + light[None, None, :] * stones[..., None]
    shade = 0.72 + 0.3 * grain + 0.16 * (fine - 0.5)
    rgb = clamp01(rgb * shade[..., None])

    # Two wheel ruts (a soft dark band, not a hard line -- pressed earth, not
    # a painted mark), no other markings; edges dissolve toward the ambient
    # dirt tone rather than a hard kerb.
    rut_wobble = (nz.fbm(shape, 8, 2, seed + 5, warp_amp=0.02, warp_freq=4) - 0.5) * 0.03
    rut = clamp01(_line_mask(X + rut_wobble, 0.27, 0.055) + _line_mask(X + rut_wobble, 0.73, 0.055))
    rut = rut ** 1.5
    rut_col = np.array(p["rut"], dtype=np.float64) / 255.0
    rgb = rgb * (1 - rut[..., None] * 0.65) + rut_col[None, None, :] * (rut[..., None] * 0.65)

    edge_fade = clamp01(1.0 - np.minimum(X, 1.0 - X) / 0.1)
    rgb = clamp01(rgb * (1.0 - 0.15 * edge_fade[..., None]))
    return to_rgb(clamp01(rgb))


# name -> (generator, world width metres, tile length metres)
GENERATORS = {
    "ow_road_highway": (_highway, 16.0, 8.0),
    "ow_road_street": (_street, 8.0, 8.0),
    "ow_road_track": (_track, 4.0, 8.0),
}

EXPORT = {
    # (width, height) chosen so pixel aspect matches width:tile_length --
    # see the module docstring: U never tiles, so only the V (length) axis
    # needs to be seamless, but the aspect still has to match world
    # proportions or the cross-section reads as visibly stretched.
    "ow_road_highway": (1024, 512),   # 16 m : 8 m = 2:1
    "ow_road_street": (1024, 1024),   # 8 m : 8 m = 1:1
    "ow_road_track": (512, 1024),     # 4 m : 8 m = 1:2
}


def generate(name: str, seed: int, size: int = WORK_W):
    fn, width_m, tile_m = GENERATORS[name]
    ew, eh = EXPORT[name]
    aspect = eh / ew
    shape = (int(round(size * aspect)), size)
    return fn(shape, seed)
