# Road surface textures

Roadmap Section 19 (runtime road system). Three textures.

`RoadSystem::Build_Geometry` in `Code/WWPhys/roadsystem.cpp` builds one mesh per material
name across all the roads that use it, and fetches the texture with
`WW3DAssetManager::Get_Texture(name)`. The name comes from the road:
`RoadSplineClass::Set_Material`. A road that names no texture is still built and still
draws — as untextured geometry — because a generator that has not chosen its art and a
device-less check are both that state, and the geometry is what the section is about.

Nothing in the engine maps a road *class* to a material today: `ROAD_CLASS_TRACK`,
`ROAD_CLASS_STREET` and `ROAD_CLASS_HIGHWAY` are navigation metadata, and whoever adds a
road picks its material. The three names below are what those three classes should be given
when the art exists, and a default table mapping class to material is the natural place to
put them.

## The UV layout, which decides what these images have to be

This is the important part and it is unusual, so it is worth being exact.

**Across the road, `u` runs 0 to 1 exactly once, over the carriageway width.** The ribbon's
two edges are `centre ± side * (width / 2)`, and they carry `u = 0` and `u = 1`. So:

- the **left and right edges of the image are the left and right edges of the road**;
- the image is a **full cross-section** — both kerbs, all lanes, any centre line, in one
  texture;
- it must **not** tile horizontally, and any horizontal wrap seam never appears, because
  `u` never leaves 0..1;
- the image is stretched to whatever `width` the road was given. A 4 m track and a 16 m
  highway sample the same image across their whole width, so the lane markings in the
  image are the lane markings on the road at any width. Draw for the width the road class
  is meant to be built at.

**Along the road, `v` is distance in metres divided by `metresPerTile`** (default 8 m). So
the image tiles top-to-bottom, once every 8 m of road, and must wrap cleanly in that
direction only. Dashes in a centre line have to divide into that distance or they will jump
at every tile boundary.

**The shoulder is not in this image.** `shoulderWidth` is spent on the terrain, not on more
ribbon: the road system stamps the road mask at carriageway width and feathers it across
the shoulder, so the verge is `ow_ter_road.tga` from
[TerrainLayers.md](TerrainLayers.md). These three textures stop at the road edge, and how
good the join looks is a question about this pair of images together.

**Junctions sample the same texture, projected flat.** The polygon that fills a crossing
takes its UVs from the frame of one arm, so at a junction the texture runs with one road
and across the other. Markings that read as lanes will therefore read as lanes crossing the
junction sideways. This is why the middle of a real intersection is usually blank, and the
same answer works here: keep the strong directional markings away from being the only thing
in the image.

## The three

### `ow_road_track.tga`

- **For:** `ROAD_CLASS_TRACK`. Dirt, one lane, wheel ruts, no markings, edges that dissolve
  into the ground rather than ending at a kerb.
- **Drawn at:** roads around 4 m wide.
- **Tiling along:** 8 m. Ruts must line up across the wrap.
- **Best case for junctions**, since it has no directional markings to conflict.

### `ow_road_street.tga`

- **For:** `ROAD_CLASS_STREET`. Surfaced, built up, two lanes, kerbs at both edges, a
  centre line.
- **Drawn at:** roads around 8 m wide.
- **Tiling along:** 8 m, so a dashed centre line wants a whole number of dashes in the
  image.
- Kerbs sit in the outermost few percent of `u`, and are the part the terrain's road layer
  has to meet.

### `ow_road_highway.tga`

- **For:** `ROAD_CLASS_HIGHWAY`. Surfaced, fast, multiple lanes each way, edge lines,
  lane dividers, no kerb — a shoulder painted into the image at each side.
- **Drawn at:** roads around 16 m wide.
- **Tiling along:** 8 m.

## Trying it before the art exists

`road_test` at the console takes a texture name as its third argument, so any texture already
in the game can be put on a generated road without a rebuild:

```
road_test 80 8 <some_texture.tga>
```

Nothing stock is drawn for this UV layout -- `u` spanning a whole carriageway once is not how
a Renegade road texture was authored -- so expect it to be wrong in an informative way. It
answers whether the geometry, the junctions and the tiling along the road are right, which is
what wants looking at.

## Not listed here, and why

**Junction tiles do not exist and are not wanted.** The donor picks intersections from an
atlas of authored corner pieces — tee, Y, four-way, two flavours of H — which is a modelled
mesh per junction kind and exactly what this section's acceptance rules out. Junctions here
are generated polygons that reuse an arm's texture. If the flat projection at a junction
ever looks bad enough to matter, the fix is a blend or a decal, not an atlas.

**Bridge decks are Section 20's**, and get their own list when that code names them. The
seam already exists: a road ending in `ROAD_ENDPOINT_BRIDGE` records where its surface
stopped and which way it pointed, so a bridge's own deck material meets the road there.

**Tunnel portals** likewise: a connection type with no geometry behind it yet.
