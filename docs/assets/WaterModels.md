# Water surface textures

Roadmap Section 22 (reflective water / river system). Six texture names, one per kind of
water, and none of them exists.

`WaterSystem::Build_Area_Geometry` in `Code/WWPhys/watersystem.cpp` never builds a mesh for
an area whose kind names no texture -- see "What happens without them" below. The names
live in `WaterSystem::Define_Default_Water`, and today every one of them is empty.

## What happens without them

Nothing is drawn, and everything else works.

That is the same answer `FoliageModels.md` gives and for the same reason: a channel with no
water drawn over it is a dry channel, which is exactly what it looks like. The parts of the
system that are not art keep working without any:

- the ground is still carved to the declared surface height and bed depth
  (`WaterSystem::Shape_Terrain`);
- the terrain's shoreline mask is still stamped, so nearby ground still blends towards a wet
  texture layer the way Section 18 already knows how to blend one
  (`WaterSystem::Stamp_Mask`);
- `WaterSystem::Get_Water_Height`, `Get_Water_Depth`, `Get_Flow` and `Is_Navigable` all
  answer from the authored shape, not from the mesh, so a soldier's depth, a river's current
  and a naval unit's draft check all work with nothing drawn at all;
- `WaterSystem::Get_Missing_Texture_Count` reports how many kinds named nothing that loaded,
  and `water_test` at the console prints it.

## What the physical side does not need any art for at all

This is the part worth reading before painting anything. Section 22's acceptance is that
water works physically **without a competing collision system**, and the way that is true
here is that the physical water is not this mesh's own invention -- it is Renegade's
existing one. A soldier's feet already report `SURFACE_TYPE_UNDERWATER_DIRT` standing on a
lake bed, and a ray cast up from there already looks for `SURFACE_TYPE_WATER_PERMEABLE` to
find the surface; see `soldier.cpp` and `surfaceeffects.cpp`, both untouched by this system.
The mesh this file is about is what gets tagged `SURFACE_TYPE_WATER_PERMEABLE`, poly by
poly, so every one of those existing reactions -- wake, splash, surface damage -- already
fires over it once it exists, texture or no texture. See
[../zerohour/WaterSystem.md](../zerohour/WaterSystem.md) for the full argument.

## The one hard constraint: it is translucent, not cut out

Unlike a foliage batch, a water surface is not alpha-tested cut-out art -- there is no edge
to cut around, only a surface to blend over the lake bed beneath it. The shader
(`Water_Shader` in `Code/WWPhys/watersystem.cpp`) is alpha blended, depth-write disabled and
not culled, so the texture should be a plausible water surface at whatever opacity it is
painted at, not a sprite sheet with transparent holes in it.

Related consequences:

- **No per-area animation yet.** The mesh is built once, the same way a foliage batch is.
  Flow direction and scroll speed are authored and queryable
  (`WaterAreaClass::Get_Flow_Direction`, `WaterDefinitionClass::Get_Scroll_Speed`), but
  nothing yet drives a per-frame UV offset from them; see the "not done" list in
  `WaterSystem.md`.
- **One texture, one pass**, the same as a foliage batch and for the same reason: a mesh
  built once from `DynamicMeshBuilderClass` carries one material.
- **Tiling.** UVs are laid out at a fixed 0.1 world-units-per-texel scale (ten metres per
  tile), so the texture should tile seamlessly in both directions at that scale, and should
  read reasonably from directly above, which is the angle most of a still lake is seen from.

## The six kinds

| Name | Category | Flows | Carves a bed | Default depth |
|------|----------|-------|---------------|---------------|
| `ow_water_ocean` | `WATER_OCEAN` | no | no (no boundary to carve along) | -- |
| `ow_water_coast` | `WATER_COAST` | no | no | -- |
| `ow_water_lake` | `WATER_LAKE` | no | yes | 3.0 m |
| `ow_water_pond` | `WATER_POND` | no | yes | 1.5 m |
| `ow_water_river` | `WATER_RIVER` | yes | yes | 2.5 m |
| `ow_water_stream` | `WATER_STREAM` | yes | yes | 1.0 m |

One per category, chosen to cover both of the roadmap's still/flowing behaviours and both
of its carves/does-not-carve behaviours.

### Constraints on placement, for the level author rather than the artist

**Ocean and coast are not carved.** There is no boundary to a coastline for a channel to be
cut along, so those two categories place their surface plane at the declared height and
trust the level's own terrain to already sit below it. A coastline authored above the
terrain it is supposed to cover will show the terrain poking through.

**A closed body's surface is one flat height.** A lake cannot slope, so unlike a river's
per-station height there is exactly one number for the whole area
(`WaterAreaClass::Set_Height`). A closed boundary is fanned from its own centroid, correct
for the convex ponds and lakes this targets; an author drawing a strongly concave shoreline
will see the fan cross itself rather than the engine refusing it.

## The textures

```
ow_water_ocean.tga
ow_water_coast.tga
ow_water_lake.tga
ow_water_pond.tga
ow_water_river.tga
ow_water_stream.tga
```

Read by `WaterSystem::Build_Area_Geometry` through `WW3DAssetManager::Get_Texture`. Without
one, the area's kind is counted by `Get_Missing_Texture_Count` and nothing is built for it
at all -- unlike foliage's untextured-white-batch case, water has no shader that would draw
sensibly with no texture bound, so the choice here is nothing rather than something wrong.

## Trying it before the art exists

```
water_test <radius> <texture>
water_clear
```

`water_test` places a round pond around the player, carves the ground to it, stamps the
shoreline mask and reports what happened. The second argument replaces the pond kind's
texture, so pointing it at a texture that already exists in a stock Renegade mix file is how
the surface is seen drawn, tagged and blended before any of ours exists.

## Not listed here, and why

**Flow / normal maps for animated ripple detail.** `WaterDefinitionClass` already has a
`FlowTexture` field and `Set_Flow_Texture`/`Get_Flow_Texture` accessors for it, but
`Build_Area_Geometry` does not bind a second texture stage yet -- there is only the one
draw pass a foliage-style mesh has. Wiring a second stage is straightforward; driving it
with real flowing normals from a texture that does not exist yet is not, so it is not asked
for until there is a shader that would read it.

**Reflection.** Listed in Section 22's rendering requirements and not attempted here. A
planar reflection needs a render-to-texture pass the renderer does not have yet, which is a
renderer capability, not an asset -- nothing to name, nothing to paint, until that pass
exists.

**Foam, wakes and shore edge decals.** All three are the kind of thing `SurfaceEffectsManager`
already spawns for a surface tagged `SURFACE_TYPE_WATER_PERMEABLE` -- see the physical
section above -- so they are existing-system art requests, not new ones this file would add.
