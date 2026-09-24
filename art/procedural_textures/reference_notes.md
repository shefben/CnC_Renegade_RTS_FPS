# Reference material used for `ow_*` procedural textures

These are notes about the retail Renegade textures used as art-direction
references while writing `tools/texture_gen/`. The reference bytes
themselves are **not** checked into this repository -- `always.dat` is
copyrighted retail content and, per `PROJECT_RULES.md` section 4, remains an
external input the user's own game install provides, the same way the repo
already treats `.w3d` / `.mix` assets. They were extracted to a scratch
directory for this session only, analysed, and discarded.

## Extraction method

`tools/always_dat_extract.py` (new, checked in) reimplements the read side
of `Code/wwlib/mixfile.cpp` in Python: `MIX1` header at offset 0, a
crc-sorted file table at `header_offset`, a name table at `names_offset`,
and `CRC_Stringi` (`Code/wwlib/realcrc.cpp`) reduces to a plain
`zlib.crc32(name.upper())`. Every name below was resolved against a retail
`always.dat` (15,161 files) and opened; nothing here is a guess.

```
python tools/always_dat_extract.py list <always.dat> <substring>
python tools/always_dat_extract.py extract <always.dat> <out_dir> <name...>
```

All textures inside `always.dat` are stored as `.dds` even when the engine
asks for them by a `.tga` name (`MissingAssets.md` already documents this).
Pillow (`>=9.2`) decodes them directly.

## Measured references (numeric)

Mean/std are over RGB channels, 0-255, computed on the decoded RGBA image.

| File | Size | Mean RGB | Std RGB | Mean alpha |
| --- | --- | --- | --- | --- |
| `cc_grass1.dds` | 256x256 | (91.6, 104.4, 50.8) | (26.9, 25.7, 19.2) | 255 (opaque) |
| `l05_grass.dds` | 256x256 | (60.7, 69.1, 33.5) | (17.8, 17.1, 12.8) | 255 |
| `l01-rock2.dds` | 256x256 | (67.1, 59.5, 49.0) | (14.2, 13.5, 13.2) | 255 |
| `l03_mossrock.dds` | 256x256 | (108.1, 99.6, 81.3) | (35.0, 31.0, 33.9) | 255 |
| `lv8_sand3.dds` | 256x256 | (192.3, 157.1, 110.1) | (10.6, 13.9, 15.5) | 255 |
| `l11_gravel.dds` | 256x256 | (130.5, 102.9, 77.5) | (31.2, 27.7, 26.8) | 255 |
| `water_texture.dds` | 256x256 | (13.7, 44.5, 51.0) | (3.9, 5.9, 7.3) | 255 |
| `hnd_pad.dds` | 64x64 | (197.3, 192.5, 185.3) | (25.6, 25.2, 24.3) | 255 |
| `oil-2.dds` | 64x64 | (5.8, 5.0, 7.1) | (15.6, 13.3, 18.4) | 16.9 (soft alpha blob) |
| `d_tk_dirt.dds` | 32x32 | (113.6, 83.1, 59.0) | (36.2, 26.4, 20.8) | 73.9 (tread cutout) |
| `WeatherParticles.dds` | 128x128 | (209.0, 208.9, 209.0) | (80.0, 80.2, 80.0) | 6.4 (mostly transparent) |

## Visually inspected, not numerically tabulated

Looked at (contact sheet, this session) and used to steer hue/detail-scale
choices in `tools/texture_gen/palette.py`, but not reduced to stats because
the generator uses them for shape/style cues rather than exact colour:

- Rock/cliff family: `l03_lvrock`, `l03_rock02`, `lv8_rock`, `l10_botcliff`,
  `l10_cliff1` (tan sedimentary strata -- the strongest cliff-banding
  reference of the set).
- Sand/dirt/mud: `l10_sandruff`, `l10_sandsmth`, `l01-harddirt`, `lv8_dirt2`,
  `d_smdirt2`, `l03_wetdrt`, `l03_mud`, `d_bh_s_dirt{1,2,3}`,
  `d_bh_s_mud{1,2,3}`, `d_bh_s_sand{1,2,3}` (the `d_bh_s_*` set are small
  round blast-hole surface decals -- soft-alpha burn/debris circles, used as
  the shape reference for `ow_mark_blast` / `ow_mark_impact` / `ow_mark_dirt`).
- Road colour (not UV layout -- `RoadSurfaces.md` is explicit that no stock
  texture fits the procedural road UVs): `0201tarmak`, `stp_tarmak`,
  `cc_road3` (painted line colour), `l02_road01`.
- Water variants: `water_foam`, `water_reflect`, `hnd_water`,
  `l01-tib-water1` (bright glowing green cellular pattern -- the Tiberium
  colour reference used for `ow_weather_tiberium`).
- Marks: `alpha_crack1` (hard jagged alpha cutout style), `tib_scorch01`
  (green radial burst), `tib_bullet01-3` (small glowing-crack impact decals).
- Ribbons: `d_tk_sand` alongside `d_tk_dirt`.
- Foliage: `bark_pine`, `bark_pinesno`, `pine_green` (blue-green needle
  cluster -- primary conifer reference), `dsp_bush13`, `dsp_bush9`,
  `dsp_deadbush` (bare branching grey-white -- primary dead-tree reference),
  `bush_dead`, `bark_leaf`, `leaf_bush`, `leaf_grn`, `leaf_yello`.

Two names that surfaced in the `always.dat` listing but whose actual
decoded content did not match what the filename suggests -- `deadtree1.dds`
and `l05_pine1.dds` both decode as a light-grey coiled rope/cable, not a
tree -- were **not** used as a reference for that reason; noted here so a
later pass does not repeat the lookup expecting foliage art.
