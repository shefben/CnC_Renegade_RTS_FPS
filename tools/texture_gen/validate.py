#!/usr/bin/env python3
"""Validate the 42 generated `ow_*` textures against `MissingAssets.md`'s
exact file list and the per-texture technical requirements gathered in
`docs/assets/GeneratedTextureReport.md`.

Checks, per the task brief's own validation list:
- every required filename exists under `data/textures/`;
- the file opens as a valid image;
- dimensions and power-of-two status;
- colour mode and alpha presence;
- blank/suspiciously-uniform image detection (std close to 0);
- for `ow_ter_detail`: mean luminance, std, min, max, and a pass/fail against
  the ~230/255 near-white low-contrast requirement.

Also runs a numeric seam check on every texture this repo's docs require to
tile (the six terrain layers, six water kinds, and three road-bed... no --
roads do not tile in U, only V; ribbons tile only in V): compares opposite
edges directly (`edge_diff` = mean abs difference between a border row/
column and its opposite), rather than trusting a visual contact sheet alone.

Usage:
    python -m tools.texture_gen.validate [--json report.json]
"""
from __future__ import annotations

import argparse
import json
import os
import sys

import numpy as np
from PIL import Image

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME_ROOT = os.path.join(REPO_ROOT, "data", "textures")

REQUIRED = [
    "ow_ter_ground", "ow_ter_macro", "ow_ter_rock", "ow_ter_cliff", "ow_ter_shore",
    "ow_ter_road", "ow_ter_detail",
    "ow_road_highway", "ow_road_street", "ow_road_track",
    "ow_water_river", "ow_water_stream", "ow_water_lake", "ow_water_pond",
    "ow_water_coast", "ow_water_ocean",
    "ow_mark_construction", "ow_mark_stain", "ow_mark_scorch", "ow_mark_blast",
    "ow_mark_impact", "ow_mark_dirt", "ow_mark_world",
    "ow_ribbon_tank_track", "ow_ribbon_harvester_track", "ow_ribbon_tire_track",
    "ow_ribbon_scorch_trail", "ow_ribbon_drag_mark",
    "ow_weather_rain", "ow_weather_dust", "ow_weather_tiberium", "ow_weather_falling",
    "ow_tree_conifer", "ow_tree_conifer_impostor",
    "ow_bush_scrub", "ow_bush_scrub_impostor",
    "ow_tree_dead", "ow_tree_dead_impostor",
    "ow_grass_clump", "ow_grass_clump_impostor",
    "ow_rock_boulder", "ow_rock_boulder_impostor",
]

# Tiles in BOTH axes (must wrap seamlessly U and V).
TILE_BOTH = {
    "ow_ter_ground", "ow_ter_macro", "ow_ter_rock", "ow_ter_cliff", "ow_ter_shore",
    "ow_ter_road", "ow_ter_detail",
    "ow_water_river", "ow_water_stream", "ow_water_lake", "ow_water_pond",
    "ow_water_coast", "ow_water_ocean",
}
# Tiles along V only (ribbons: u never repeats; roads: u never repeats either,
# but they're excluded here since RoadSurfaces.md explicitly forbids relying
# on U tiling and the cross-section content is deliberately not U-periodic --
# the V seam is the only one that matters for either family).
TILE_V_ONLY = {
    "ow_road_highway", "ow_road_street", "ow_road_track",
    "ow_ribbon_tank_track", "ow_ribbon_harvester_track", "ow_ribbon_tire_track",
    "ow_ribbon_scorch_trail", "ow_ribbon_drag_mark",
}

UNIFORM_STD_THRESHOLD = 1.5  # 0-255 scale; below this the image is "suspiciously flat"


def is_pot(n: int) -> bool:
    return n > 0 and (n & (n - 1)) == 0


def edge_diff(arr: np.ndarray, axis: int) -> float:
    """Mean abs difference between the two edges that become adjacent when
    the texture is tiled. Compared against `interior_step` (below), not
    against a fixed absolute number: a high-frequency grain texture has
    large pixel-to-pixel jumps *everywhere*, so a boundary jump of the same
    order is normal, not a seam. A real seam is one that stands out from the
    texture's own normal local variation."""
    if axis == 0:
        a, b = arr[0].astype(np.float64), arr[-1].astype(np.float64)
    else:
        a, b = arr[:, 0].astype(np.float64), arr[:, -1].astype(np.float64)
    return float(np.abs(a - b).mean())


def interior_step(arr: np.ndarray, axis: int, sample: int = 200) -> float:
    """Typical adjacent-pixel difference within the texture's own interior,
    as a local baseline for `edge_diff`."""
    if axis == 0:
        region = arr[:min(arr.shape[0], sample)]
        d = np.diff(region.astype(np.float64), axis=0)
    else:
        region = arr[:, :min(arr.shape[1], sample)]
        d = np.diff(region.astype(np.float64), axis=1)
    return float(np.abs(d).mean())


def validate_one(name: str):
    path = os.path.join(GAME_ROOT, name + ".tga")
    result = {"name": name, "path": os.path.relpath(path, REPO_ROOT), "ok": True, "problems": []}

    if not os.path.isfile(path):
        result["ok"] = False
        result["problems"].append("file missing")
        return result

    try:
        img = Image.open(path)
        img.load()
    except Exception as e:  # noqa: BLE001
        result["ok"] = False
        result["problems"].append(f"failed to open: {e}")
        return result

    w, h = img.size
    result["size"] = [w, h]
    result["mode"] = img.mode
    result["pot"] = [is_pot(w), is_pot(h)]
    result["has_alpha"] = img.mode in ("RGBA", "LA")

    arr = np.array(img.convert("RGB"), dtype=np.float64)
    lum = np.array(img.convert("L"), dtype=np.float64)
    result["luminance"] = {
        "mean": round(float(lum.mean()), 2),
        "std": round(float(lum.std()), 2),
        "min": round(float(lum.min()), 2),
        "max": round(float(lum.max()), 2),
    }
    alpha_std = None
    if result["has_alpha"]:
        alpha_std = float(np.array(img.convert("RGBA"))[..., 3].astype(np.float64).std())
        result["alpha_std"] = round(alpha_std, 2)

    # A flat-colour sprite whose *shape* lives entirely in alpha (e.g. the
    # rain streak, greyscale RGB + varying alpha) is correct, not blank --
    # only flag when neither channel group varies.
    rgb_flat = lum.std() < UNIFORM_STD_THRESHOLD
    alpha_flat = alpha_std is None or alpha_std < UNIFORM_STD_THRESHOLD
    if rgb_flat and alpha_flat:
        result["ok"] = False
        result["problems"].append(f"suspiciously uniform (rgb std={lum.std():.2f}, alpha std={alpha_std})")

    def seam_bad(d, step):
        # A real seam is meaningfully larger than the texture's own normal
        # pixel-to-pixel variation, and also large in absolute terms (a flat
        # texture with near-zero interior variation shouldn't need a 10x
        # margin to fail on a genuine 3/255 seam).
        return d > 3.0 and d > 3.0 * max(step, 0.25)

    if name in TILE_BOTH:
        dv, dh = edge_diff(arr, axis=0), edge_diff(arr, axis=1)
        sv, sh = interior_step(arr, axis=0), interior_step(arr, axis=1)
        result["seam"] = {"vertical": round(dv, 3), "horizontal": round(dh, 3),
                           "interior_step": [round(sv, 3), round(sh, 3)]}
        if seam_bad(dv, sv) or seam_bad(dh, sh):
            result["ok"] = False
            result["problems"].append(f"visible seam (v={dv:.2f} vs step {sv:.2f}, h={dh:.2f} vs step {sh:.2f})")
    elif name in TILE_V_ONLY:
        dv = edge_diff(arr, axis=0)
        sv = interior_step(arr, axis=0)
        result["seam"] = {"vertical": round(dv, 3), "interior_step": round(sv, 3)}
        if seam_bad(dv, sv):
            result["ok"] = False
            result["problems"].append(f"visible seam along V (v={dv:.2f} vs step {sv:.2f})")

    if name == "ow_ter_detail":
        mean_255 = lum.mean()
        target = 230.0
        if abs(mean_255 - target) > 6.0:
            result["ok"] = False
            result["problems"].append(f"mean luminance {mean_255:.1f} not within 6 of target {target}")
        if lum.std() > 25.0:
            result["ok"] = False
            result["problems"].append(f"contrast too high for a near-white detail layer (std={lum.std():.1f})")

    return result


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", default=None)
    args = ap.parse_args(argv)

    results = [validate_one(n) for n in REQUIRED]
    present = sum(1 for r in results if os.path.isfile(os.path.join(GAME_ROOT, r["name"] + ".tga")))
    failing = [r for r in results if not r["ok"]]

    print(f"{present}/{len(REQUIRED)} required files present")
    for r in results:
        tag = "OK  " if r["ok"] else "FAIL"
        extra = ""
        if "size" in r:
            extra = f'{r["size"][0]}x{r["size"][1]} {r["mode"]}'
            if "seam" in r:
                extra += f' seam={r["seam"]}'
            if r["name"] == "ow_ter_detail":
                extra += f' lum_mean={r["luminance"]["mean"]} std={r["luminance"]["std"]}'
        print(f"  [{tag}] {r['name']:32s} {extra}  {'; '.join(r['problems'])}")

    if args.json:
        with open(args.json, "w") as fh:
            json.dump(results, fh, indent=2)

    print(f"\n{len(results) - len(failing)}/{len(results)} passed.")
    return 0 if not failing else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
