"""The six `ow_water_*` surfaces -- roadmap Section 22, `docs/assets/WaterModels.md`.

All six are square, tile seamlessly in both axes (UVs are laid out at a
fixed ~10 m-per-tile scale), and are alpha-**blended** translucency over the
lake bed rather than cutout alpha -- `WaterModels.md`: "the texture should
be a plausible water surface at whatever opacity it is painted at, not a
sprite sheet with transparent holes in it." So alpha here stays in a
moderate, smoothly-varying band; it is never near 0 or a hard shape.

`river`/`stream` are the two flowing kinds (`WaterModels.md`'s table): their
ripple field is built anisotropically (`noise.stretch_periodic`, elongating
a coarse-in-V field back up) so ripples read as streaks along the flow
direction (V) rather than isotropic chop.
"""
from __future__ import annotations

import numpy as np

from . import noise as nz
from . import palette as pal
from .noise import clamp01
from .imgio import mix_colors, to_rgba

WORK = 1024


def _base_fields(shape, seed, flow: bool, flow_strength: float = 5.0):
    h, w = shape
    # Broad, low-contrast depth tint -- real water's colour variation is
    # subtle and large-scale, not blobby camouflage-style patches.
    depth = nz.fbm(shape, 3, 3, seed, warp_amp=0.06, warp_freq=2)

    # Cellular ripple: Worley cell-boundary lines read as an interlocking
    # ripple/caustic network much closer to the stock `water_texture`
    # reference than a plain fbm blob field does. Flowing kinds compute the
    # cells on a V-compressed grid and stretch back up (periodic-safe --
    # `noise.stretch_periodic`), which elongates each cell into a streak
    # along the flow direction.
    cell_freq = 16
    if flow:
        coarse_h = max(8, int(round(h / flow_strength)))
        f1, f2 = nz.worley((coarse_h, w), cell_freq, seed + 1, warp_amp=0.15, warp_freq=4)
        f1 = nz.stretch_periodic(f1, h / coarse_h, 1.0)[:h, :w]
        f2 = nz.stretch_periodic(f2, h / coarse_h, 1.0)[:h, :w]
    else:
        f1, f2 = nz.worley(shape, cell_freq, seed + 1, warp_amp=0.2, warp_freq=4)

    ripple_lines = clamp01(1.0 - nz.remap(f2 - f1, 0, 1) / 0.35)
    ripple_lines = ripple_lines ** 2

    fine_f1, _ = nz.worley(shape, 48, seed + 2, warp_amp=0.1, warp_freq=6)
    sparkle = clamp01(1.0 - nz.remap(fine_f1, 0, 1) / 0.18) ** 6
    return depth, ripple_lines, sparkle


def _compose(shape, seed, colors, flow, flow_strength, foam_amount, alpha_base, alpha_var):
    depth, ripple, sparkle = _base_fields(shape, seed, flow, flow_strength)

    deep = np.array(colors["deep"], dtype=np.float64) / 255.0
    mid = np.array(colors["mid"], dtype=np.float64) / 255.0
    foam = np.array(colors.get("foam", colors["mid"]), dtype=np.float64) / 255.0

    w_mid = clamp01(0.5 + (depth - 0.5) * 0.5)
    rgb = deep[None, None, :] * (1 - w_mid[..., None]) + mid[None, None, :] * w_mid[..., None]

    rgb = rgb * (1 - ripple[..., None] * 0.28) + mid[None, None, :] * 1.2 * (ripple[..., None] * 0.28)
    rgb = clamp01(rgb)

    foam_mask = clamp01(sparkle * foam_amount)
    rgb = rgb * (1 - foam_mask[..., None]) + foam[None, None, :] * foam_mask[..., None]
    rgb = clamp01(rgb)

    alpha = clamp01(alpha_base + alpha_var * (ripple - 0.5) + 0.5 * foam_mask * (1 - alpha_base))
    return to_rgba(rgb, alpha)


def _river(shape, seed):
    return _compose(shape, seed, pal.WATER["river"], flow=True, flow_strength=6.0,
                     foam_amount=0.5, alpha_base=0.82, alpha_var=0.08)


def _stream(shape, seed):
    return _compose(shape, seed, pal.WATER["stream"], flow=True, flow_strength=9.0,
                     foam_amount=0.65, alpha_base=0.78, alpha_var=0.1)


def _lake(shape, seed):
    return _compose(shape, seed, pal.WATER["lake"], flow=False, flow_strength=1.0,
                     foam_amount=0.15, alpha_base=0.85, alpha_var=0.04)


def _pond(shape, seed):
    return _compose(shape, seed, pal.WATER["pond"], flow=False, flow_strength=1.0,
                     foam_amount=0.08, alpha_base=0.88, alpha_var=0.03)


def _coast(shape, seed):
    return _compose(shape, seed, pal.WATER["coast"], flow=False, flow_strength=1.0,
                     foam_amount=0.7, alpha_base=0.8, alpha_var=0.1)


def _ocean(shape, seed):
    return _compose(shape, seed, pal.WATER["ocean"], flow=False, flow_strength=1.0,
                     foam_amount=0.45, alpha_base=0.83, alpha_var=0.09)


GENERATORS = {
    "ow_water_river": (_river,),
    "ow_water_stream": (_stream,),
    "ow_water_lake": (_lake,),
    "ow_water_pond": (_pond,),
    "ow_water_coast": (_coast,),
    "ow_water_ocean": (_ocean,),
}

EXPORT_SIZE = {name: 1024 for name in GENERATORS}


def generate(name: str, seed: int, size: int = WORK):
    (fn,) = GENERATORS[name]
    return fn((size, size), seed)
