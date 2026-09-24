"""The ten `ow_tree_*`/`ow_bush_*`/`ow_grass_*`/`ow_rock_*` foliage textures
-- roadmap Section 21, `docs/assets/FoliageModels.md`.

No `ow_` foliage *model* exists yet in this repository (models are out of
scope for this pass -- `MissingAssets.md` section 21 lists them separately),
so per `FoliageModels.md` section 12's own fallback ("if the renderer
supports only one viewing direction, choose the most useful canonical
view"), each impostor is hand-authored as a side-view silhouette rather than
rendered from a model, using the same palette and shape language as its
near-distance material sheet so the two visibly match -- the one hard
requirement `FoliageModels.md` states ("the two MUST visually match. Do not
create an impostor that looks like a completely different plant.").

Material sheets follow one convention throughout: the left half is a
tileable bark/stem/rock swatch (opaque), the right half is a foliage cutout
cluster on transparent background (`FoliageModels.md`: "alpha-tested
cut-out ... drawn two-sided", "one material" -- a model authored later
should UV its leaf cards into the right half and any bark-wrapped geometry
into the left half). `ow_grass_clump` and `ow_rock_boulder` are the two
exceptions the doc's own near-model table implies: grass has no bark, so its
material is cutout blades throughout; the boulder has no foliage at all, so
its material is a plain opaque rock swatch.

Impostor UV convention (`FoliageModels.md`): v=0 is the top of the plant,
v=1 (the bottom image edge) is where it meets the ground -- so every
silhouette below is drawn touching the bottom edge with no blank margin
under it.
"""
from __future__ import annotations

import numpy as np
from PIL import Image, ImageDraw

from . import noise as nz
from . import palette as pal
from .noise import clamp01
from .imgio import radial_field, to_rgba

WORK = 1024


def _col(c):
    return np.array(c, dtype=np.float64) / 255.0


def _blob_alpha(shape, seed, cy, cx, ry, rx, ragged, octaves=4, freq=5):
    h, w = shape
    y = (np.arange(h) + 0.5) / h
    x = (np.arange(w) + 0.5) / w
    Y, X = np.meshgrid(y, x, indexing="ij")
    r = np.sqrt(((Y - cy) / ry) ** 2 + ((X - cx) / rx) ** 2)
    bump = nz.fbm(shape, freq, octaves, seed, warp_amp=0.2, warp_freq=3)
    edge = r - 1.0 - ragged * (bump - 0.5)
    return clamp01(1.0 - nz.smoothstep(0.0, 0.12, edge))


def _canopy_alpha(shape, seed, base_v, top_v, width, ragged=0.22, lobes=3):
    """Several overlapping ragged blobs stacked from `base_v` up to `top_v`
    -- a generic irregular bush/canopy silhouette, reused for scrub and for
    the conifer's needle mass."""
    alpha = np.zeros(shape)
    for i in range(lobes):
        t = i / max(lobes - 1, 1)
        cy = base_v + (top_v - base_v) * (0.15 + 0.7 * t)
        ry = (base_v - top_v) * (0.55 - 0.15 * t) + 0.06
        rx = width * (0.95 - 0.35 * t)
        a = _blob_alpha(shape, seed + i * 13, cy, 0.5, ry, rx, ragged, freq=5 + i)
        alpha = np.maximum(alpha, a)
    return alpha


def _bark_swatch(shape, seed, dark, light):
    h, w = shape
    fissure = nz.fbm(shape, 5, 4, seed, warp_amp=0.02, warp_freq=6)
    grain = nz.fbm(shape, 40, 3, seed + 2)
    w_light = clamp01(nz.smoothstep(0.4, 0.7, fissure))
    rgb = dark[None, None, :] * (1 - w_light[..., None]) + light[None, None, :] * w_light[..., None]
    rgb = clamp01(rgb * (0.85 + 0.25 * grain[..., None]))
    return rgb


def _rock_swatch(shape, seed, dark, light):
    base = nz.fbm(shape, 8, 5, seed, warp_amp=0.04, warp_freq=4)
    f1, f2 = nz.worley(shape, 12, seed + 1)
    crack = clamp01(1.0 - nz.remap(f2 - f1, 0, 1) / 0.12) ** 3
    w = clamp01(base * 1.1)
    rgb = dark[None, None, :] * (1 - w[..., None]) + light[None, None, :] * w[..., None]
    rgb = rgb * (1 - crack[..., None] * 0.6) + dark[None, None, :] * (crack[..., None] * 0.6)
    return clamp01(rgb)


def _draw_branches(size, seed, n_start=3, depth=6, trunk_frac=0.06):
    """Recursive branch skeleton for the dead tree, rasterized with PIL.
    Returns an alpha mask [0, 1] and a per-pixel "thickness" field used to
    darken thicker limbs slightly relative to twigs."""
    w, h = size
    img = Image.new("L", size, 0)
    draw = ImageDraw.Draw(img)
    rng = np.random.default_rng(seed)

    def branch(x, y, angle, length, width, gen):
        if gen <= 0 or length < h * 0.02:
            return
        x2 = x + length * np.sin(angle)
        y2 = y - length * np.cos(angle)
        draw.line([(x, y), (x2, y2)], fill=255, width=max(1, int(width)))
        n_children = 2 if gen > 1 else rng.integers(1, 3)
        for _ in range(n_children):
            da = rng.uniform(0.35, 0.75) * rng.choice([-1, 1])
            branch(x2, y2, angle + da, length * rng.uniform(0.68, 0.8),
                   width * 0.68, gen - 1)

    trunk_h = h * rng.uniform(0.32, 0.4)
    trunk_w_px = max(2.0, trunk_frac * min(w, h) * 6.0)
    branch(w * 0.5, h, 0.0, trunk_h, trunk_w_px, depth)
    arr = np.array(img, dtype=np.float64) / 255.0
    return arr


def _conifer_silhouette(shape, seed, dark, light, needle_dark, needle_light):
    h, w = shape
    trunk_w = 0.045
    trunk = clamp01(1.0 - np.abs((np.arange(w) + 0.5) / w - 0.5) / trunk_w)
    trunk_mask = np.tile(trunk[None, :], (h, 1))
    y_frac = (np.arange(h) + 0.5) / h
    trunk_mask = trunk_mask * (y_frac[:, None] > 0.55)

    canopy = _canopy_alpha(shape, seed, base_v=0.55, top_v=0.04, width=0.42, ragged=0.16, lobes=5)
    alpha = clamp01(np.maximum(canopy, trunk_mask))

    bark = _bark_swatch(shape, seed + 50, dark, light)
    needle_field = nz.fbm(shape, 20, 3, seed + 60)
    needle_col = needle_dark[None, None, :] * (1 - needle_field[..., None]) + needle_light[None, None, :] * needle_field[..., None]
    rgb = np.where(trunk_mask[..., None] > canopy[..., None], bark, needle_col)
    return clamp01(rgb), alpha


def _tree_conifer_material(shape, seed):
    p_bark = pal.FOLIAGE["conifer_bark"]
    p_needle = pal.FOLIAGE["conifer_needle"]
    h, w = shape
    half = w // 2
    bark = _bark_swatch((h, half), seed, _col(p_bark["dark"]), _col(p_bark["light"]))
    bark_rgba = np.dstack([bark, np.ones((h, half))])

    needle_shape = (h, w - half)
    alpha = _canopy_alpha(needle_shape, seed + 3, base_v=0.85, top_v=0.1, width=0.85, ragged=0.2, lobes=6)
    field = nz.fbm(needle_shape, 24, 3, seed + 7)
    dark = _col(p_needle["dark"])
    light = _col(p_needle["light"])
    rgb = dark[None, None, :] * (1 - field[..., None]) + light[None, None, :] * field[..., None]
    needle_rgba = np.dstack([clamp01(rgb), alpha])

    canvas = np.zeros((h, w, 4))
    canvas[:, :half, :] = bark_rgba
    canvas[:, half:, :] = needle_rgba
    return to_rgba(canvas[..., :3], canvas[..., 3])


def _tree_conifer_impostor(shape, seed):
    p_bark = pal.FOLIAGE["conifer_bark"]
    p_needle = pal.FOLIAGE["conifer_needle"]
    rgb, alpha = _conifer_silhouette(shape, seed, _col(p_bark["dark"]), _col(p_bark["light"]),
                                      _col(p_needle["dark"]), _col(p_needle["light"]))
    return to_rgba(rgb, alpha)


def _bush_scrub_material(shape, seed):
    p = pal.FOLIAGE["scrub"]
    alpha = _canopy_alpha(shape, seed, base_v=0.92, top_v=0.06, width=0.9, ragged=0.24, lobes=6)
    field = nz.fbm(shape, 22, 3, seed + 4)
    dark, light = _col(p["dark"]), _col(p["light"])
    rgb = dark[None, None, :] * (1 - field[..., None]) + light[None, None, :] * field[..., None]
    return to_rgba(clamp01(rgb), alpha)


def _bush_scrub_impostor(shape, seed):
    p = pal.FOLIAGE["scrub"]
    h, w = shape
    canopy = _canopy_alpha(shape, seed, base_v=0.85, top_v=0.1, width=0.44, ragged=0.22, lobes=5)
    stem_w = 0.02
    x = (np.arange(w) + 0.5) / w
    y = (np.arange(h) + 0.5) / h
    Y, X = np.meshgrid(y, x, indexing="ij")
    stem = clamp01(1.0 - np.abs(X - 0.5) / stem_w) * (Y > 0.75)
    alpha = clamp01(np.maximum(canopy, stem))
    field = nz.fbm(shape, 20, 3, seed + 9)
    dark, light, stem_c = _col(p["dark"]), _col(p["light"]), _col(p["stem"])
    rgb = dark[None, None, :] * (1 - field[..., None]) + light[None, None, :] * field[..., None]
    rgb = np.where(stem[..., None] > canopy[..., None], stem_c[None, None, :], rgb)
    return to_rgba(clamp01(rgb), alpha)


def _tree_dead_material(shape, seed):
    p = pal.FOLIAGE["dead_tree"]
    h, w = shape
    half = w // 2
    bark = _bark_swatch((h, half), seed, _col(p["dark"]), _col(p["bark"]))
    bark_rgba = np.dstack([bark, np.ones((h, half))])

    twig_shape = (h, w - half)
    twigs = _draw_branches((w - half, h), seed + 8, depth=5, trunk_frac=0.05)
    field = nz.fbm(twig_shape, 30, 2, seed + 12)
    rgb = _col(p["dark"])[None, None, :] * (1 - field[..., None]) + _col(p["bark"])[None, None, :] * field[..., None]
    twig_rgba = np.dstack([clamp01(rgb), twigs])

    canvas = np.zeros((h, w, 4))
    canvas[:, :half, :] = bark_rgba
    canvas[:, half:, :] = twig_rgba
    return to_rgba(canvas[..., :3], canvas[..., 3])


def _tree_dead_impostor(shape, seed):
    p = pal.FOLIAGE["dead_tree"]
    w, h = shape[1], shape[0]
    alpha = _draw_branches((w, h), seed + 21, depth=7, trunk_frac=0.055)
    field = nz.fbm(shape, 26, 3, seed + 25)
    rgb = _col(p["dark"])[None, None, :] * (1 - field[..., None]) + _col(p["bark"])[None, None, :] * field[..., None]
    return to_rgba(clamp01(rgb), alpha)


def _grass_blades_alpha(shape, seed, n_blades, base_v, height, width):
    h, w = shape
    canvas = Image.new("L", (w, h), 0)
    draw = ImageDraw.Draw(canvas)
    rng = np.random.default_rng(seed)
    base_y = int(base_v * h)
    for i in range(n_blades):
        bx = w * rng.uniform(0.08, 0.92)
        lean = rng.uniform(-0.35, 0.35)
        blade_h = height * h * rng.uniform(0.7, 1.15)
        tip_x = bx + lean * blade_h
        tip_y = base_y - blade_h
        bw = max(1, int(width * w * rng.uniform(0.7, 1.3)))
        draw.line([(bx, base_y), (tip_x, tip_y)], fill=255, width=bw)
    return np.array(canvas, dtype=np.float64) / 255.0


def _grass_clump_material(shape, seed):
    p = pal.FOLIAGE["grass_clump"]
    alpha = _grass_blades_alpha(shape, seed, n_blades=36, base_v=0.98, height=0.9, width=0.018)
    field = nz.fbm(shape, 16, 3, seed + 5)
    dry, green = _col(p["dry"]), _col(p["green"])
    rgb = green[None, None, :] * (1 - field[..., None]) + dry[None, None, :] * field[..., None]
    return to_rgba(clamp01(rgb), alpha)


def _grass_clump_impostor(shape, seed):
    p = pal.FOLIAGE["grass_clump"]
    alpha = _grass_blades_alpha(shape, seed + 40, n_blades=22, base_v=0.99, height=0.85, width=0.022)
    field = nz.fbm(shape, 14, 3, seed + 44)
    dry, green = _col(p["dry"]), _col(p["green"])
    rgb = green[None, None, :] * (1 - field[..., None]) + dry[None, None, :] * field[..., None]
    return to_rgba(clamp01(rgb), alpha)


def _rock_boulder_material(shape, seed):
    p = pal.FOLIAGE["boulder"]
    rgb = _rock_swatch(shape, seed, _col(p["dark"]), _col(p["light"]))
    alpha = np.ones(shape)
    return to_rgba(rgb, alpha)


def _rock_boulder_impostor(shape, seed):
    p = pal.FOLIAGE["boulder"]
    alpha = _blob_alpha(shape, seed, cy=0.72, cx=0.5, ry=0.26, rx=0.46, ragged=0.16, freq=6)
    surf = _rock_swatch(shape, seed + 3, _col(p["dark"]), _col(p["light"]))
    # Ambient-only shading (no fixed directional highlight): darker near the
    # ground contact, per VisualFidelity.md's "no baked directional light".
    y = (np.arange(shape[0]) + 0.5) / shape[0]
    ao = clamp01(1.0 - 0.25 * nz.smoothstep(0.6, 0.95, y))[:, None] * np.ones((1, shape[1]))
    rgb = clamp01(surf * ao[..., None])
    return to_rgba(rgb, alpha)


GENERATORS = {
    "ow_tree_conifer": (_tree_conifer_material, 1.0),
    "ow_tree_conifer_impostor": (_tree_conifer_impostor, 6.0 / 10.0),
    "ow_bush_scrub": (_bush_scrub_material, 1.0),
    "ow_bush_scrub_impostor": (_bush_scrub_impostor, 0.85),
    "ow_tree_dead": (_tree_dead_material, 1.0),
    "ow_tree_dead_impostor": (_tree_dead_impostor, 0.5),
    "ow_grass_clump": (_grass_clump_material, 1.0),
    "ow_grass_clump_impostor": (_grass_clump_impostor, 1.0),
    "ow_rock_boulder": (_rock_boulder_material, 1.0),
    "ow_rock_boulder_impostor": (_rock_boulder_impostor, 1.3),
}

EXPORT_W = {
    "ow_tree_conifer": 1024, "ow_tree_conifer_impostor": 512,
    "ow_bush_scrub": 512, "ow_bush_scrub_impostor": 512,
    "ow_tree_dead": 512, "ow_tree_dead_impostor": 512,
    "ow_grass_clump": 512, "ow_grass_clump_impostor": 512,
    "ow_rock_boulder": 512, "ow_rock_boulder_impostor": 512,
}


def generate(name: str, seed: int, size: int = WORK):
    fn, aspect_wh = GENERATORS[name]
    w = size
    h = size if aspect_wh == 1.0 else int(round(size / aspect_wh))
    return fn((h, w), seed)
