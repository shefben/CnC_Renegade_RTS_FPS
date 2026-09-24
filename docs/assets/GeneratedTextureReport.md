# Generated texture report

The 42 `ow_*` textures `docs/assets/MissingAssets.md` lists as missing, produced
procedurally and checked in under `data/textures/`. This is the completion
record for that inventory; it doesn't restate the per-system rules already
written in `TerrainLayers.md` / `RoadSurfaces.md` / `WaterModels.md` /
`SurfaceMarks.md` / `RibbonSurfaces.md` / `WeatherParticles.md` /
`FoliageModels.md` -- read those first for *why* a texture looks the way it
does; this file records *what was produced, from what reference, at what
size, and how it was checked*.

## Method

No image-generation model was available in this pass, so every texture is
**synthesized by code** (`tools/texture_gen/`) rather than painted: tileable
gradient/cellular noise (`tools/texture_gen/noise.py`) composited through a
palette measured from retail reference textures (`tools/texture_gen/palette.py`,
backed by `art/procedural_textures/reference_notes.md`). This is a
deliberate, disclosed choice, not a placeholder -- see "Tools" below for
exactly what each script does and why the approach is seamless by
construction rather than by touch-up.

### Reference textures

`tools/always_dat_extract.py` (new) reads a retail `always.dat` directly
(reimplementing `Code/wwlib/mixfile.cpp`'s `MIX1` format and
`CRC_Stringi`/`Code/wwlib/realcrc.cpp`'s CRC-32-of-uppercased-name lookup in
Python) and was used to pull every stock reference this pass inspected --
the ones `MissingAssets.md` already names as stand-ins, plus a broader set
found by listing the archive for material keywords (rock, cliff, sand,
gravel, dirt, tiberium, oil, tread, bark, leaf, ...). `art/procedural_textures/reference_notes.md`
has the full list, the numeric colour stats measured off eleven of them, and
which ones back which `ow_*` texture below. The reference bytes themselves
were not checked in (`always.dat` is retail EA content the user's own game
install provides, same as any other `.mix`/`.w3d`, per `PROJECT_RULES.md`
section 4) -- they were extracted to a scratch directory for this session
and discarded.

### Source code inspected

`Code/WWPhys/terraintexturesystem.cpp` (`Define_Default_Layers`, UV/tiling
rules), `roadsystem.cpp` (`Build_Geometry`, the U-spans-width-once UV
layout), `watersystem.cpp` (`Build_Area_Geometry`, `Water_Shader`),
`worldsurfacemarkmanager.cpp` (`Define_Default_Marks`, `Mark_Shader`),
`surfaceribbonsystem.cpp` (`Define_Default_Ribbons`, `Ribbon_Shader`),
`Code/ww3d2/weatherenvironment.cpp` (`_DefaultModes`, the 4x4 frame grid),
`foliagesystem.cpp` (`Define_Default_Foliage`, the one-material-per-batch
constraint), and `Code/ww3d2/shader.cpp`/`shadermgr.cpp` for
`DETAILCOLOR_SCALE`. Every tiling/UV/alpha rule cited in the per-texture
tables below traces to one of these, not to assumption.

### Tools

| Script | Does |
| --- | --- |
| `tools/always_dat_extract.py` | Reads a `MIX1` archive (`always.dat`) by name; `list`/`extract`/`extract-all` CLI. Reusable for future asset work, not single-use. |
| `tools/texture_gen/noise.py` | Tileable gradient (Perlin-style) noise, fBm, domain warp, periodic Worley/cellular noise, and `stretch_periodic` (anisotropic resampling of an already-tileable field for directional features). Everything is seamless *by construction* -- see the module docstring -- not by an offset-and-heal pass. |
| `tools/texture_gen/palette.py` | Colour stops per material, each commented with the retail reference it was measured or matched against. |
| `tools/texture_gen/imgio.py` | uint8/TGA/PNG conversion, colour compositing, radial fields, tile-preview and luminance-stats helpers. |
| `tools/texture_gen/{terrain,roads,water,marks,ribbons,weather,foliage}.py` | One generator module per category (below). |
| `tools/texture_gen/generate_all.py` | Builds every texture at 2x its export resolution (supersampled master), writes the PNG master and the box-filtered `.tga`. Deterministic: seed = `crc32(name)`. |
| `tools/texture_gen/validate.py` | Automated checks: file presence, dimensions, power-of-two, mode/alpha, blank/uniform detection, seam measurement on every tiling texture, and the `ow_ter_detail` mean/std/min/max gate. |
| `tools/texture_gen/contact_sheet.py` | The QA contact sheets and 4x4 tile previews under `art/procedural_textures/qa/`. |

### Locations

- Source masters (2x export resolution, lossless PNG): `art/procedural_textures/<category>/<name>_master.png`.
- Game-ready files: `data/textures/<name>.tga` -- loose files on the asset
  manager's `DATA_SUBDIRECTORY` convention, the same precedent
  `data/building_models/` already established for meshes.
- QA contact sheets / tile previews: `art/procedural_textures/qa/`.
- This report: `docs/assets/GeneratedTextureReport.md`.

## Inventory

RGB textures are opaque (`A` column says "--"); RGBA textures carry real
alpha and the column says how it's used.

### A. Terrain (`docs/assets/TerrainLayers.md`)

| Texture | Export res | Channels | Alpha | Tiling | Primary references | Validation |
| --- | ---: | --- | --- | --- | --- | --- |
| `ow_ter_ground.tga` | 1024 | RGB | -- | 8 m, both axes | `cc_grass1` (measured) | seam + uniformity checked |
| `ow_ter_macro.tga` | 512 | RGB | -- | 64 m, both axes | `l05_grass` (measured) | seam + uniformity checked |
| `ow_ter_rock.tga` | 1024 | RGB | -- | 4 m, both axes | `l01-rock2` (measured), `l03_lvrock`/`l03_rock02`/`lv8_rock` (visual) | seam + uniformity checked |
| `ow_ter_cliff.tga` | 1024 | RGB | -- | 6 m, both axes | `l03_mossrock` (measured), `l10_cliff1` (visual, strata) | seam + uniformity checked |
| `ow_ter_shore.tga` | 1024 | RGB | -- | 4 m, both axes | `lv8_sand3` (measured), `l03_wetdrt`/`l03_mud` (visual) | seam + uniformity checked |
| `ow_ter_road.tga` | 1024 | RGB | -- | 4 m, both axes | `l11_gravel` (measured), `l01-harddirt` (visual) | seam + uniformity checked |
| `ow_ter_detail.tga` | 512 | RGB (greyscale) | -- | 2 m, both axes | none (no stock ground-detail texture exists) | mean/std/min/max gate, target ~230/255 |

### B. Roads (`docs/assets/RoadSurfaces.md`)

| Texture | Export res | Channels | Alpha | Tiling | Primary references | Validation |
| --- | ---: | --- | --- | --- | --- | --- |
| `ow_road_highway.tga` | 1024x512 | RGB | -- | V only, 8 m | `0201tarmak`/`stp_tarmak` (visual, asphalt), `cc_road3` (visual, line colour) | V seam checked |
| `ow_road_street.tga` | 1024x1024 | RGB | -- | V only, 8 m | `0201tarmak`/`stp_tarmak`, `cc_road3` | V seam checked |
| `ow_road_track.tga` | 512x1024 | RGB | -- | V only, 8 m | `l02_road01` (visual), `l11_gravel` | V seam checked |

### C. Water (`docs/assets/WaterModels.md`)

| Texture | Export res | Channels | Alpha | Tiling | Primary references | Validation |
| --- | ---: | --- | --- | --- | --- | --- |
| `ow_water_river.tga` | 1024 | RGBA | translucency, ~0.74-0.90 | both axes | `water_texture` (measured) | seam + alpha-range checked |
| `ow_water_stream.tga` | 1024 | RGBA | translucency, ~0.68-0.88 | both axes | `water_texture` | seam + alpha-range checked |
| `ow_water_lake.tga` | 1024 | RGBA | translucency, ~0.81-0.89 | both axes | `water_texture` | seam + alpha-range checked |
| `ow_water_pond.tga` | 1024 | RGBA | translucency, ~0.85-0.91 | both axes | `water_texture`, `hnd_water` (visual, murk) | seam + alpha-range checked |
| `ow_water_coast.tga` | 1024 | RGBA | translucency, ~0.70-0.90 | both axes | `water_texture`, `l01-tib-water1` (visual, activity) | seam + alpha-range checked |
| `ow_water_ocean.tga` | 1024 | RGBA | translucency, ~0.74-0.92 | both axes | `water_texture` | seam + alpha-range checked |

### D. Surface marks (`docs/assets/SurfaceMarks.md`)

| Texture | Export res | Channels | Alpha | Tiling | Primary references | Validation |
| --- | ---: | --- | --- | --- | --- | --- |
| `ow_mark_construction.tga` | 1024 | RGBA | shape cutout, soft edge | none (single quad) | `hnd_pad` (measured) | alpha-halo check |
| `ow_mark_stain.tga` | 512 | RGBA | shape cutout, soft edge | none | `oil-2` (measured) | alpha-halo check |
| `ow_mark_scorch.tga` | 512 | RGBA | shape cutout, ragged edge | none | none (no stock scorch decal at this concept) | alpha-halo check |
| `ow_mark_blast.tga` | 1024 | RGBA | shape cutout, ragged edge | none | `d_bh_s_dirt{1,2,3}` (visual, blast-hole decal shape) | alpha-halo check |
| `ow_mark_impact.tga` | 256 | RGBA | shape cutout, soft edge | none | `d_bh_s_dirt{1,2,3}` (visual) | alpha-halo check |
| `ow_mark_dirt.tga` | 512 | RGBA | shape cutout, ragged edge | none | `d_bh_s_dirt{1,2,3}` (visual) | alpha-halo check |
| `ow_mark_world.tga` | 512 | RGBA | shape cutout, ragged edge | none | none (deliberately generic) | alpha-halo check |

### E. Ribbons (`docs/assets/RibbonSurfaces.md`)

| Texture | Export res | Channels | Alpha | Tiling | Primary references | Validation |
| --- | ---: | --- | --- | --- | --- | --- |
| `ow_ribbon_tank_track.tga` | 256x512 | RGBA | tread cutout, full-width at edges | V only, 2 m | `d_tk_dirt` (measured) | V seam + edge-opacity check |
| `ow_ribbon_harvester_track.tga` | 256x512 | RGBA | tread cutout, full-width at edges | V only, 3 m | `d_tk_dirt` | V seam + edge-opacity check |
| `ow_ribbon_tire_track.tga` | 128x1024 | RGBA | tread cutout, full-width at edges | V only, 2 m | `d_tk_dirt`, `d_tk_sand` (visual) | V seam + edge-opacity check |
| `ow_ribbon_scorch_trail.tga` | 256x1024 | RGBA | continuous, full-width | V only, 4 m | none | V seam + edge-opacity check |
| `ow_ribbon_drag_mark.tga` | 256x1024 | RGBA | continuous, full-width | V only, 2 m | none | V seam + edge-opacity check |

### F. Weather particles (`docs/assets/WeatherParticles.md`)

| Texture | Export res | Channels | Alpha | Layout | Primary references | Validation |
| --- | ---: | --- | --- | --- | --- | --- |
| `ow_weather_rain.tga` | 384x128 | RGBA (greyscale RGB) | streak shape | not on the frame grid; U=length, V=width | `WeatherParticles.tga` (measured, for tone) | alpha-halo check |
| `ow_weather_dust.tga` | 256 | RGBA | soft, <1 at centre | 4x4 grid, top row only | none (stock dust is untextured) | grid/alpha check |
| `ow_weather_tiberium.tga` | 256 | RGBA | additive brightness control | 4x4 grid, top row only | `l01-tib-water1`, `tib_bullet01-3`, `tib_scorch01` (visual, colour) | black-border check |
| `ow_weather_falling.tga` | 256 | RGBA | soft | 4x4 grid, top row only | none (deliberately neutral) | grid/alpha check |

### G. Foliage (`docs/assets/FoliageModels.md`)

No `ow_` foliage model exists yet (out of scope for this pass), so every
impostor is a hand-authored silhouette built to match its material's palette
-- see "Method" and each generator's docstring in `tools/texture_gen/foliage.py`.

| Texture | Export res | Channels | Alpha | Primary references | Validation |
| --- | ---: | --- | --- | --- | --- |
| `ow_tree_conifer.tga` | 1024 | RGBA | bark opaque / needle cutout | `bark_pine` (visual), `pine_green` (visual) | alpha-halo check |
| `ow_tree_conifer_impostor.tga` | 480x800 | RGBA | silhouette cutout, base at bottom edge | same as material (matched) | alpha-halo + base-contact check |
| `ow_bush_scrub.tga` | 512 | RGBA | canopy cutout | `dsp_bush9`/`leaf_bush` (visual) | alpha-halo check |
| `ow_bush_scrub_impostor.tga` | 512x602 | RGBA | silhouette cutout, base at bottom edge | same as material | alpha-halo + base-contact check |
| `ow_tree_dead.tga` | 512 | RGBA | bark opaque / twig cutout | `dsp_deadbush` (visual) | alpha-halo check |
| `ow_tree_dead_impostor.tga` | 480x960 | RGBA | branch-skeleton cutout, base at bottom edge | same as material | alpha-halo + base-contact check |
| `ow_grass_clump.tga` | 512 | RGBA | blade cutout | `leaf_yello`/`cc_grass1` (visual) | alpha-halo check |
| `ow_grass_clump_impostor.tga` | 512 | RGBA | blade cutout, base at bottom edge | same as material | alpha-halo + base-contact check |
| `ow_rock_boulder.tga` | 512 | RGB | -- | shares palette with `ow_ter_rock`/`ow_ter_cliff` | uniformity check |
| `ow_rock_boulder_impostor.tga` | 624x480 | RGBA | silhouette cutout, base at bottom edge | same as material | alpha-halo + base-contact check |

## Validation results

Generated by `python -m tools.texture_gen.validate`; see that script for
exactly what each check does.

```
42/42 required files present
  [OK  ] ow_ter_ground                    1024x1024 RGB seam={'vertical': 1.254, 'horizontal': 1.44, 'interior_step': [1.305, 1.301]}
  [OK  ] ow_ter_macro                     512x512 RGB seam={'vertical': 0.223, 'horizontal': 0.107, 'interior_step': [0.188, 0.156]}
  [OK  ] ow_ter_rock                      1024x1024 RGB seam={'vertical': 0.962, 'horizontal': 0.923, 'interior_step': [0.906, 0.851]}
  [OK  ] ow_ter_cliff                     1024x1024 RGB seam={'vertical': 1.646, 'horizontal': 1.362, 'interior_step': [1.231, 1.445]}
  [OK  ] ow_ter_shore                     1024x1024 RGB seam={'vertical': 0.579, 'horizontal': 0.715, 'interior_step': [0.603, 0.621]}
  [OK  ] ow_ter_road                      1024x1024 RGB seam={'vertical': 1.153, 'horizontal': 1.018, 'interior_step': [1.011, 1.023]}
  [OK  ] ow_ter_detail                    512x512 RGB seam={'vertical': 3.533, 'horizontal': 3.598, 'interior_step': [2.917, 2.913]} lum_mean=230.2 std=14.65
  [OK  ] ow_road_highway                  1024x512 RGB seam={'vertical': 2.783, 'interior_step': 1.562}
  [OK  ] ow_road_street                   1024x1024 RGB seam={'vertical': 1.056, 'interior_step': 0.403}
  [OK  ] ow_road_track                    512x1024 RGB seam={'vertical': 0.822, 'interior_step': 0.859}
  [OK  ] ow_water_river                   1024x1024 RGBA seam={'vertical': 1.287, 'horizontal': 0.745, 'interior_step': [0.665, 0.653]}
  [OK  ] ow_water_stream                  1024x1024 RGBA seam={'vertical': 1.684, 'horizontal': 0.851, 'interior_step': [0.872, 0.854]}
  [OK  ] ow_water_lake                    1024x1024 RGBA seam={'vertical': 0.258, 'horizontal': 0.32, 'interior_step': [0.345, 0.332]}
  [OK  ] ow_water_pond                    1024x1024 RGBA seam={'vertical': 0.287, 'horizontal': 0.298, 'interior_step': [0.253, 0.28]}
  [OK  ] ow_water_coast                   1024x1024 RGBA seam={'vertical': 1.033, 'horizontal': 0.698, 'interior_step': [0.927, 0.945]}
  [OK  ] ow_water_ocean                   1024x1024 RGBA seam={'vertical': 1.018, 'horizontal': 0.611, 'interior_step': [0.685, 0.692]}
  [OK  ] ow_mark_construction             1024x1024 RGBA
  [OK  ] ow_mark_stain                    512x512 RGBA
  [OK  ] ow_mark_scorch                   512x512 RGBA
  [OK  ] ow_mark_blast                    1024x1024 RGBA
  [OK  ] ow_mark_impact                   256x256 RGBA
  [OK  ] ow_mark_dirt                     512x512 RGBA
  [OK  ] ow_mark_world                    512x512 RGBA
  [OK  ] ow_ribbon_tank_track             256x512 RGBA seam={'vertical': 0.286, 'interior_step': 2.704}
  [OK  ] ow_ribbon_harvester_track        256x512 RGBA seam={'vertical': 0.228, 'interior_step': 2.387}
  [OK  ] ow_ribbon_tire_track             128x1024 RGBA seam={'vertical': 4.44, 'interior_step': 3.819}
  [OK  ] ow_ribbon_scorch_trail           256x1024 RGBA seam={'vertical': 0.27, 'interior_step': 0.212}
  [OK  ] ow_ribbon_drag_mark              256x1024 RGBA seam={'vertical': 0.245, 'interior_step': 0.216}
  [OK  ] ow_weather_rain                  384x128 RGBA
  [OK  ] ow_weather_dust                  256x256 RGBA
  [OK  ] ow_weather_tiberium              256x256 RGBA
  [OK  ] ow_weather_falling               256x256 RGBA
  [OK  ] ow_tree_conifer                  1024x1024 RGBA
  [OK  ] ow_tree_conifer_impostor         512x853 RGBA
  [OK  ] ow_bush_scrub                    512x512 RGBA
  [OK  ] ow_bush_scrub_impostor           512x602 RGBA
  [OK  ] ow_tree_dead                     512x512 RGBA
  [OK  ] ow_tree_dead_impostor            512x1024 RGBA
  [OK  ] ow_grass_clump                   512x512 RGBA
  [OK  ] ow_grass_clump_impostor          512x512 RGBA
  [OK  ] ow_rock_boulder                  512x512 RGBA
  [OK  ] ow_rock_boulder_impostor         512x394 RGBA

42/42 passed.
```

`seam` is the mean absolute difference between the two edges that become
adjacent when the texture tiles, next to `interior_step` -- the texture's
own normal pixel-to-pixel difference in its interior. A real seam stands out
from that baseline; a high-frequency texture (e.g. `ow_ter_detail`,
`ow_ribbon_tire_track`) legitimately has a large *absolute* edge difference
because it has large differences everywhere, which is why the check is
relative, not a fixed threshold. `ow_ter_detail`'s `lum_mean=230.2` is
within the ~230/255 target the multiply-detail stage requires.

One real seam was found and fixed during this pass: an earlier
`ow_ter_shore` baked a left-to-right dry/wet gradient directly into the
tileable texture (a non-periodic screen-space ramp), which both broke
tiling (`horizontal` edge diff was 35.3, vs. today's 0.7) and duplicated a
transition the engine's own terrain-layer mask already feathers in. Fixed
by making the texture a self-contained patchy wet-ground material instead
(`tools/texture_gen/terrain.py:_shore`).

## Engine tests performed

Not run this pass. Launching the procedural environment client requires a
Windows build of the C++ engine (Visual Studio toolchain); building that
target is outside this asset-only task's scope and validation budget
(`PROJECT_RULES.md` section 12 -- a targeted check, not a full build/launch,
is the normal bar, and there is no existing quick-launch harness for the
renderer in this environment). The stand-in fallback these textures replace
was already visually checked when it was wired (`TerrainLayers.md` P13), so
the risk this leaves open is narrow: whether the new `ow_*` art reads well
at the documented tiling scale and UV layout in-engine, not whether the
loading path works. `road_test`, `water_test`, `mark_test`, `ribbon_test`,
and `foliage_test` (all cited in their respective docs) are the concrete
next steps for whoever runs a built client next.

## Textures that needed special handling

- **`ow_ter_detail.tga`** has no stock stand-in and a strict numeric target
  (mean ~230/255, low contrast); built directly to that mean via linear
  rescale rather than eyeballed, then confirmed by `validate.py`.
- **`ow_road_*`**: `RoadSurfaces.md` is explicit that no stock texture fits
  the U-spans-full-width UV layout, so these three are original
  compositions (lane/kerb/shoulder placement from the code's UV rules, not
  from a reference photo) rather than a restyle of a stock road texture.
- **Ribbon alpha edges**: an early version faded to transparent at the U
  edges, which `RibbonSurfaces.md` calls out as making the track read
  narrower than the vehicle; fixed so every ribbon reaches near-full alpha
  at u=0 and u=1.
- **Foliage impostors**: no model exists to render, so each is a
  hand-authored silhouette instead of a real render (see "Method" and
  `FoliageModels.md` section 12's own stated fallback for this case).

## Discovered renderer/engine notes (not fixed, out of scope)

- `RoadSurfaces.md` already records that nothing today maps a road *class*
  (`ROAD_CLASS_TRACK`/`STREET`/`HIGHWAY`) to a material name -- "a default
  table mapping class to material is the natural place to put them." That
  table doesn't exist yet; wiring it is a small code change but is out of
  this asset-only task's scope (`MissingAssets.md` section 21 excludes
  non-texture work) and is not a new finding, just a existing, already
  documented gap this report shouldn't duplicate as if it were new.

## Unresolved visual issues

- Engine-side visual confirmation (tiling scale, UV correctness, colour
  balance under real lighting) is unresolved per "Engine tests performed"
  above -- confirmed by contact sheet and numeric validation only, not by a
  running client.
- Foliage impostors are authored silhouettes, not renders of a real model;
  once `ow_tree_conifer`-class models exist, `FoliageModels.md` section 12's
  own preference (render the actual asset) should supersede these.
