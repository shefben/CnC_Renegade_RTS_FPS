#!/usr/bin/env python3
"""Generate all 42 `ow_*` textures listed in `docs/assets/MissingAssets.md`.

For every texture: build a working image at 2x the game-ready export
resolution (supersampled, then box-filtered down on save -- see
`imgio.save_pair`/`save_pair_wh`), write the full-resolution result as the
lossless PNG *master* under `art/procedural_textures/<category>/`, and write
the box-filtered, game-ready result as the `.tga` `data/textures/` expects.

Each texture uses a fixed seed derived from its own name
(`zlib.crc32(name.encode())`), so a re-run reproduces byte-identical output
-- there is no hidden randomness to lose track of.

Usage:
    python -m tools.texture_gen.generate_all [--only NAME [NAME ...]]
"""
from __future__ import annotations

import argparse
import os
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from texture_gen import terrain, roads, water, marks, ribbons, weather, foliage
from texture_gen.imgio import save_pair, save_pair_wh

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MASTER_ROOT = os.path.join(REPO_ROOT, "art", "procedural_textures")
GAME_ROOT = os.path.join(REPO_ROOT, "data", "textures")


def seed_for(name: str) -> int:
    return zlib.crc32(name.encode("ascii")) & 0x7FFFFFFF


def _square_jobs(module, category):
    for name, size in module.EXPORT_SIZE.items():
        if isinstance(size, tuple):
            continue
        yield category, name, module, ("square", size)


def _rectlike_jobs(module, category, size_attr):
    table = getattr(module, size_attr)
    for name, size in table.items():
        yield category, name, module, ("rect", size)


def all_jobs():
    jobs = []
    jobs += list(_square_jobs(terrain, "terrain"))
    jobs += list(_rectlike_jobs(roads, "roads", "EXPORT"))
    jobs += list(_square_jobs(water, "water"))
    jobs += list(_square_jobs(marks, "marks"))
    jobs += list(_rectlike_jobs(ribbons, "ribbons", "EXPORT"))
    jobs += list(_rectlike_jobs(weather, "weather", "EXPORT_SIZE"))
    for name, export_w in foliage.EXPORT_W.items():
        _, aspect_wh = foliage.GENERATORS[name]
        export_h = export_w if aspect_wh == 1.0 else int(round(export_w / aspect_wh))
        jobs.append(("foliage", name, foliage, ("rect", (export_w, export_h))))
    return jobs


def build_one(category, name, module, spec, verbose=True):
    kind, size = spec
    seed = seed_for(name)
    master_path = os.path.join(MASTER_ROOT, category, name + "_master.png")
    game_path = os.path.join(GAME_ROOT, name + ".tga")

    if kind == "square":
        img = module.generate(name, seed, size=size * 2)
        save_pair(img, master_path, game_path, size)
        if verbose:
            print(f"{name:32s} square  export={size}x{size}  master={img.size}")
    else:
        export_w, export_h = size
        master_w = export_w * 2
        img = module.generate(name, seed, size=master_w)
        save_pair_wh(img, master_path, game_path, export_w, export_h)
        if verbose:
            print(f"{name:32s} rect    export={export_w}x{export_h}  master={img.size}")


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", nargs="*", default=None, help="generate only these texture names")
    args = ap.parse_args(argv)

    jobs = all_jobs()
    if args.only:
        wanted = set(args.only)
        jobs = [j for j in jobs if j[1] in wanted]
        missing = wanted - {j[1] for j in jobs}
        if missing:
            print("unknown names:", missing)
            return 1

    os.makedirs(GAME_ROOT, exist_ok=True)
    for category, name, module, spec in jobs:
        build_one(category, name, module, spec)
    print(f"\n{len(jobs)} textures generated.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
