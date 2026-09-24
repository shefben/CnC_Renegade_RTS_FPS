#!/usr/bin/env python3
"""Build the QA contact sheets and tiled previews called for by the task
brief: one labeled contact sheet per category, plus a 4x4 tiled preview for
every texture that has to tile. These are QA artifacts only -- they do not
replace the `.tga` files in `data/textures/`.

Usage:
    python -m tools.texture_gen.contact_sheet
"""
from __future__ import annotations

import os
import sys

from PIL import Image, ImageDraw

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GAME_ROOT = os.path.join(REPO_ROOT, "data", "textures")
QA_ROOT = os.path.join(REPO_ROOT, "art", "procedural_textures", "qa")

CATEGORIES = {
    "terrain": ["ow_ter_ground", "ow_ter_macro", "ow_ter_rock", "ow_ter_cliff",
                "ow_ter_shore", "ow_ter_road", "ow_ter_detail"],
    "roads": ["ow_road_highway", "ow_road_street", "ow_road_track"],
    "water": ["ow_water_river", "ow_water_stream", "ow_water_lake",
              "ow_water_pond", "ow_water_coast", "ow_water_ocean"],
    "marks": ["ow_mark_construction", "ow_mark_stain", "ow_mark_scorch",
              "ow_mark_blast", "ow_mark_impact", "ow_mark_dirt", "ow_mark_world"],
    "ribbons": ["ow_ribbon_tank_track", "ow_ribbon_harvester_track",
                "ow_ribbon_tire_track", "ow_ribbon_scorch_trail", "ow_ribbon_drag_mark"],
    "weather": ["ow_weather_rain", "ow_weather_dust", "ow_weather_tiberium", "ow_weather_falling"],
    "foliage": ["ow_tree_conifer", "ow_tree_conifer_impostor", "ow_bush_scrub",
                "ow_bush_scrub_impostor", "ow_tree_dead", "ow_tree_dead_impostor",
                "ow_grass_clump", "ow_grass_clump_impostor", "ow_rock_boulder",
                "ow_rock_boulder_impostor"],
}

TILE_BOTH = {
    "ow_ter_ground", "ow_ter_macro", "ow_ter_rock", "ow_ter_cliff", "ow_ter_shore",
    "ow_ter_road", "ow_ter_detail",
    "ow_water_river", "ow_water_stream", "ow_water_lake", "ow_water_pond",
    "ow_water_coast", "ow_water_ocean",
}
TILE_V_ONLY = {
    "ow_road_highway", "ow_road_street", "ow_road_track",
    "ow_ribbon_tank_track", "ow_ribbon_harvester_track", "ow_ribbon_tire_track",
    "ow_ribbon_scorch_trail", "ow_ribbon_drag_mark",
}

CHECKER = None


def checkerboard(size, cell=16):
    img = Image.new("RGB", size)
    px = img.load()
    for y in range(size[1]):
        for x in range(size[0]):
            px[x, y] = (200, 200, 200) if (x // cell + y // cell) % 2 == 0 else (140, 140, 140)
    return img


def load_on_checker(path, size=None):
    img = Image.open(path).convert("RGBA")
    if size:
        img = img.resize(size, Image.LANCZOS)
    bg = checkerboard(img.size)
    bg.paste(img, (0, 0), img)
    return bg


def build_category_sheet(category, names):
    cell = 300
    cols = 4
    rows = (len(names) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cell, rows * (cell + 22)), (20, 20, 20))
    draw = ImageDraw.Draw(sheet)
    for i, name in enumerate(names):
        path = os.path.join(GAME_ROOT, name + ".tga")
        if not os.path.isfile(path):
            continue
        thumb = load_on_checker(path, (cell, cell))
        x = (i % cols) * cell
        y = (i // cols) * (cell + 22)
        sheet.paste(thumb, (x, y))
        draw.text((x + 4, y + cell + 4), name, fill=(255, 255, 0))
    out_path = os.path.join(QA_ROOT, f"contact_{category}.png")
    os.makedirs(QA_ROOT, exist_ok=True)
    sheet.save(out_path)
    return out_path


def build_tile_preview(name, n=4, cell=192):
    path = os.path.join(GAME_ROOT, name + ".tga")
    if not os.path.isfile(path):
        return None
    img = Image.open(path).convert("RGB").resize((cell, cell), Image.LANCZOS)
    if name in TILE_BOTH:
        sheet = Image.new("RGB", (cell * n, cell * n))
        for j in range(n):
            for i in range(n):
                sheet.paste(img, (i * cell, j * cell))
    else:  # TILE_V_ONLY
        sheet = Image.new("RGB", (cell, cell * n))
        for j in range(n):
            sheet.paste(img, (0, j * cell))
    out_path = os.path.join(QA_ROOT, f"tile_{name}.png")
    os.makedirs(QA_ROOT, exist_ok=True)
    sheet.save(out_path)
    return out_path


def main(argv):
    made = []
    for category, names in CATEGORIES.items():
        made.append(build_category_sheet(category, names))
    for name in sorted(TILE_BOTH | TILE_V_ONLY):
        p = build_tile_preview(name)
        if p:
            made.append(p)
    for p in made:
        print(p)
    print(f"\n{len(made)} QA images written to {QA_ROOT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
