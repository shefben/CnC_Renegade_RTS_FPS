"""The seven `ow_ter_*` textures -- roadmap Section 18, `docs/assets/TerrainLayers.md`.

All six tiling layers (everything except `ow_ter_detail`) are square,
power-of-two, and must wrap seamlessly in both U and V (`TerrainLayers.md`:
"Square, power of two, wrapping cleanly") because
`RenegadeTerrainPatchClass::Update_UVs` derives UVs straight from world X/Y
divided by `meters_per_tile` with no per-tile offset. Every field in this
module is built from `noise.fbm`/`noise.worley`, which are periodic by
construction (see `noise.py`'s module docstring), so no seam-hiding pass is
needed.
"""
from __future__ import annotations

import numpy as np

from . import noise as nz
from . import palette as pal
from .noise import clamp01
from .imgio import mix_colors, to_rgb

WORK = 2048   # master working resolution (downsampled to EXPORT on save)


def _ground(shape, seed):
    p = pal.TERRAIN["ground"]
    soil = nz.fbm(shape, 6, 4, seed, warp_amp=0.02, warp_freq=3)
    patch = nz.fbm(shape, 5, 3, seed + 1, warp_amp=0.06, warp_freq=3)
    mottle = nz.fbm(shape, 24, 4, seed + 2)          # fist-sized soil mottling
    fine = nz.fbm(shape, 160, 3, seed + 4)           # blade/soil-crumb grain
    stone_f1, _ = nz.worley(shape, 34, seed + 3, warp_amp=0.35, warp_freq=5)
    stone_mask = clamp01(1.0 - nz.remap(stone_f1, 0, 1) / 0.35) ** 3
    cluster = nz.fbm(shape, 7, 3, seed + 5, warp_amp=0.04, warp_freq=3)
    stone_mask *= nz.smoothstep(0.42, 0.75, cluster)

    w_soil = clamp01(1.0 - patch) * (0.35 + 0.65 * soil)
    w_dry = clamp01(patch) * (0.25 + 0.75 * (1.0 - soil))
    w_green = clamp01(0.42 + 0.5 * (soil - 0.5) + 0.32 * (patch - 0.5))
    weights = np.stack([w_soil, w_dry, w_green, stone_mask], axis=-1)
    colors = [p["soil"], p["dry_grass"], p["green_grass"], p["stone"]]
    rgb = mix_colors(weights, colors)

    shade = 0.74 + 0.3 * mottle + 0.18 * (fine - 0.5)
    rgb = clamp01(rgb * shade[..., None])
    return to_rgb(rgb)


def _macro(shape, seed):
    """Section-18-mandated: broad soft variation only, no readable features,
    hash noise around a 30 m feature scale at half strength -- see
    TerrainLayers.md. Kept to two low-frequency octaves on purpose."""
    p = pal.TERRAIN["macro"]
    n = nz.fbm(shape, 3, 2, seed, persistence=0.5, warp_amp=0.03, warp_freq=2)
    n = clamp01(0.5 + (n - 0.5) * 0.9)
    rgb = mix_colors(np.stack([1.0 - n, n], axis=-1), [p["dark"], p["light"]])
    return to_rgb(rgb)


def _rock(shape, seed):
    p = pal.TERRAIN["rock"]
    base = nz.fbm(shape, 8, 5, seed, warp_amp=0.03, warp_freq=4)
    f1, f2 = nz.worley(shape, 14, seed + 1)
    crack = clamp01(1.0 - nz.remap(f2 - f1, 0, 1) / 0.12)
    crack = crack ** 3
    plate_shade = nz.remap(f1, 0, 1)

    w_dark = clamp01(0.5 - (base - 0.5) - plate_shade * 0.4 + crack * 0.8)
    w_light = clamp01(0.5 + (base - 0.5) * 1.2 - plate_shade * 0.3)
    w_mid = clamp01(1.0 - w_dark - w_light + 0.4)
    weights = np.stack([w_dark, w_mid, w_light], axis=-1)
    rgb = mix_colors(weights, [p["dark"], p["mid"], p["light"]])
    grain = nz.fbm(shape, 64, 2, seed + 2)
    rgb = clamp01(rgb * (0.9 + 0.2 * grain[..., None]))
    return to_rgb(rgb)


def _cliff(shape, seed):
    """Strata rather than a vertical rhythm -- the projection note in
    TerrainLayers.md warns this layer gets stretched into vertical streaks
    on near-vertical geometry, so the dominant pattern here is horizontal
    banding, which survives that stretch."""
    p = pal.TERRAIN["cliff"]
    Y, _ = nz.uv_grid(shape)
    band_warp = nz.fbm(shape, 4, 4, seed, warp_amp=0.08, warp_freq=3)
    bands_a = np.sin((Y + band_warp * 0.35) * 2 * np.pi * 7)
    bands_b = np.sin((Y + band_warp * 0.5) * 2 * np.pi * 12)
    # Blend two strata frequencies with a periodic envelope so band spacing
    # reads as irregular rock layers rather than one perfect sine -- the mix
    # weight itself is fbm, so it stays exactly tileable in both axes (see
    # TerrainLayers.md: all six layers must wrap cleanly on both U and V).
    mix_w = nz.fbm(shape, 3, 2, seed + 11)
    bands = bands_a * (1.0 - mix_w) + bands_b * mix_w
    bands = clamp01(0.5 + 0.5 * bands)
    fracture_f1, fracture_f2 = nz.worley(shape, 10, seed + 1)
    fracture = clamp01(1.0 - nz.remap(fracture_f2 - fracture_f1, 0, 1) / 0.1) ** 4
    moss_n = nz.fbm(shape, 6, 3, seed + 2, warp_amp=0.05, warp_freq=3)
    moss_mask = clamp01(nz.smoothstep(0.62, 0.85, moss_n) * (1.0 - bands) * 0.6)

    w_dark = clamp01((1.0 - bands) * 0.8 + fracture * 0.9)
    w_light = clamp01(bands * 0.9)
    w_mid = clamp01(1.0 - w_dark - w_light + 0.3)
    weights = np.stack([w_dark, w_mid, w_light, moss_mask], axis=-1)
    rgb = mix_colors(weights, [p["dark"], p["mid"], p["light"], p["moss"]])
    grain = nz.fbm(shape, 48, 2, seed + 3)
    rgb = clamp01(rgb * (0.9 + 0.2 * grain[..., None]))
    return to_rgb(rgb)


def _shore(shape, seed):
    """Wet shoreline ground. The actual dry-land -> water transition is the
    *engine's* job -- the terrain layer's own mask feathers this texture in
    over `ow_ter_ground` within 4 m of water (TerrainLayers.md) -- so this
    texture only needs to be a convincing, self-contained wet-ground
    material, not a baked one-direction gradient. (An earlier version baked
    a left-to-right dry->wet ramp into the texture itself; that doesn't
    tile -- the left and right edges are then two different wetness levels
    -- and it duplicated a transition the engine already blends. Patchy
    noise-driven wetness instead of a screen-space ramp fixes both.)"""
    p = pal.TERRAIN["shore"]
    wet_patch = nz.fbm(shape, 5, 4, seed, warp_amp=0.06, warp_freq=3)
    dark_patch = nz.fbm(shape, 7, 3, seed + 4, warp_amp=0.05, warp_freq=3)
    pebble_f1, _ = nz.worley(shape, 30, seed + 1, warp_amp=0.15, warp_freq=5)
    pebble = clamp01(1.0 - nz.remap(pebble_f1, 0, 1)) ** 8

    w_dry = clamp01(1.0 - wet_patch * 1.1)
    w_wet = clamp01(wet_patch * (1.0 - dark_patch * 0.5))
    w_dark = clamp01(wet_patch * dark_patch * 1.3)
    weights = np.stack([w_dry, w_wet, w_dark, pebble * 0.4], axis=-1)
    rgb = mix_colors(weights, [p["dry"], p["wet"], p["dark_wet"], p["dry"]])
    grain = nz.fbm(shape, 50, 2, seed + 2)
    rgb = clamp01(rgb * (0.9 + 0.2 * grain[..., None]))
    return to_rgb(rgb)


def _road_bed(shape, seed):
    p = pal.TERRAIN["road_bed"]
    base = nz.fbm(shape, 10, 4, seed, warp_amp=0.03, warp_freq=4)
    f1, f2 = nz.worley(shape, 40, seed + 1)
    aggregate = clamp01(1.0 - nz.remap(f1, 0, 1)) ** 5

    w_dirt = clamp01(1.0 - base * 0.6)
    w_light = clamp01(base * 0.8 + aggregate * 0.5)
    w_dark = clamp01(aggregate * 0.3)
    weights = np.stack([w_dirt, w_light, w_dark], axis=-1)
    rgb = mix_colors(weights, [p["dirt"], p["gravel_light"], p["gravel_dark"]])
    grain = nz.fbm(shape, 70, 2, seed + 2)
    rgb = clamp01(rgb * (0.88 + 0.24 * grain[..., None]))
    return to_rgb(rgb)


def _detail(shape, seed):
    """Multiplied over every ground layer (`DETAILCOLOR_SCALE`, not 2x), so
    it must stay near white with low contrast -- TerrainLayers.md's own
    number is mean ~230/255, greyscale. Built, then linearly rescaled to hit
    that mean exactly rather than eyeballed, per the task's own validation
    requirement."""
    n = nz.fbm(shape, 24, 4, seed, persistence=0.55)
    n = clamp01(n)
    target_mean = 230.0 / 255.0
    contrast = 0.06  # deliberately low -- "low contrast" requirement
    field = target_mean + (n - n.mean()) * contrast / max(n.std(), 1e-6)
    field = clamp01(field)
    rgb = np.stack([field, field, field], axis=-1)
    return to_rgb(rgb)


GENERATORS = {
    "ow_ter_ground": (_ground, 8),
    "ow_ter_macro": (_macro, 64),
    "ow_ter_rock": (_rock, 4),
    "ow_ter_cliff": (_cliff, 6),
    "ow_ter_shore": (_shore, 4),
    "ow_ter_road": (_road_bed, 4),
    "ow_ter_detail": (_detail, 2),
}

# (working resolution, game-ready export resolution) per texture. Ground/
# rock/cliff/shore/road-bed are seen at close range (a few metres of tiling)
# so they export at 1024; macro is deliberately low-detail (Section 18: "no
# readable features") so 512 is plenty; detail is a fine multiplicative
# grain layer, 512 keeps its grain frequency sane at a 2 m tile.
EXPORT_SIZE = {
    "ow_ter_ground": 1024,
    "ow_ter_macro": 512,
    "ow_ter_rock": 1024,
    "ow_ter_cliff": 1024,
    "ow_ter_shore": 1024,
    "ow_ter_road": 1024,
    "ow_ter_detail": 512,
}


def generate(name: str, seed: int, size: int = WORK):
    fn, _tile_m = GENERATORS[name]
    return fn((size, size), seed)
