# Ground mark textures

Roadmap Section 23. Read by `SurfaceRibbonSystem::Build_Definition_Geometry`
(`Code/WWPhys/surfaceribbonsystem.cpp`), through
`WW3DAssetManager::Get_Instance()->Get_Texture(def.Get_Texture())`.

Five textures. Every vehicle in the game is already laying marks — the system runs from the
moment `WWPhys::Init` does — and every one of them draws nothing, because no definition names
a texture yet.

## The textures

| Name | What it is |
|------|------------|
| `ow_ribbon_tank_track.tga` | One tread of a tank. Cleated bar pattern pressed into soft ground, running along the strip. |
| `ow_ribbon_tire_track.tga` | One tire of a wheeled vehicle. Narrower, finer tread pattern. |
| `ow_ribbon_harvester_track.tga` | The wide, deep track of a harvester. Coarser than a tank track and roughly half again as wide. |
| `ow_ribbon_scorch_trail.tga` | A burned line dragged along the ground — a beam weapon sweeping, a burning wreck sliding. Soot, not tread. |
| `ow_ribbon_drag_mark.tga` | A smooth gouge with no tread at all: something heavy pulled across dirt. |

## The kinds that read them

Set by `SurfaceRibbonSystem::Define_Default_Ribbons`. Width is metres across the strip;
segment is how far the emitter travels before another cross edge is laid; edges is the length
of that kind's ring buffer; life is how long a mark lasts; tile is how many metres of ribbon
one repeat of the texture covers.

| Kind | Width | Segment | Edges | Life | Tile |
|------|-------|---------|-------|------|------|
| `ow_ribbon_tank_track` | 0.90 m | 0.50 m | 48 | 25 s | 2 m |
| `ow_ribbon_tire_track` | 0.30 m | 0.60 m | 40 | 18 s | 2 m |
| `ow_ribbon_harvester_track` | 1.40 m | 0.70 m | 48 | 30 s | 3 m |
| `ow_ribbon_scorch_trail` | 1.20 m | 1.00 m | 24 | 12 s | 4 m |
| `ow_ribbon_drag_mark` | 0.50 m | 0.40 m | 32 | 15 s | 2 m |

Edges × segment is the longest a mark of that kind can ever be: a tank leaves twenty-four
metres of track behind it, and driving further pushes the far end off rather than making the
strip longer.

## Without them

Nothing is drawn, and everything else works. Marks are laid, aged, faded and dropped; the pool
fills and empties; `ribbon_status` reports real numbers; `Get_Missing_Texture_Count()` says how
many kinds are being laid with nothing to draw them with. No mesh is created at all until a
texture resolves, so the cost of the missing art is exactly zero rather than a white strip.

This is deliberately the foliage policy rather than the bridge one. A bridge that draws nothing
is a hole a soldier falls through, so it gets a plain slab to stand in. A track that draws
nothing is ground with no track on it, which is the truth.

## Constraints

**Tiling along the strip, clamped across it.** The `u` coordinate runs 0 at one edge of the
strip to 1 at the other and never repeats; `v` runs along the direction of travel and repeats
every "tile" metres from the table above. So the image must tile seamlessly **top to bottom**
and must not rely on tiling left to right. The tread pattern runs up the image.

**The strip is the whole width.** `u = 0` and `u = 1` are the outer corners of the track, so
the pattern must reach both edges of the image. Anything transparent at the left or right edge
becomes a track narrower than the vehicle that made it.

**Alpha is the shape of the mark, and it is multiplied by the fade.** The shader is
alpha-blended, not alpha-tested (`Ribbon_Shader` in `surfaceribbonsystem.cpp`): source alpha
against one-minus-source-alpha, no depth write, no culling. Per-vertex alpha carries the fade,
and the texture's own alpha carries the shape, and the two multiply. A texture that is fully
opaque everywhere makes a solid painted stripe; the shape of an actual tread has to be in the
alpha channel.

**Assume it will be seen on dirt, sand, mud, grass and snow.** `Ribbon_Surface_Takes_Marks`
(`Code/WWPhys/ribbontype.cpp`) allows exactly those, plus tiberium field and the two underwater
dirt types. A tread that reads only against one ground colour will look wrong on the others, so
darkening rather than colouring is the safer choice.

**No culling, so both faces show.** A strip laid while reversing has the opposite winding to
one laid driving forward. The texture is seen from above either way; nothing needs to be
mirrored.

**It sits 6 cm above the ground it marks.** `LiftHeight` defaults to 0.06 m, measured along the
ground normal, which is what clears both the terrain and a road drawn on top of it. The mark
follows slopes exactly, because each corner is placed from the wheel contact and the ground
normal there.

## How to see it

In a running client, at the console:

```text
ribbon_texture <a texture name from this level>
```

then get in a vehicle and drive over dirt. `ribbon_status` prints how many marks exist against
how many objects are drawing them. `ribbon_texture` with no argument puts it back.

## Not listed here, and why

- **No separate texture per surface type.** A tank track on snow and a tank track on mud are
  the same texture today. If they should differ, that is another definition and another row
  here, not a change to the system.
- **No wet or dust variant.** Nothing modulates the mark by how wet the ground is, because
  nothing in the engine reports that.
- **Nothing for water.** A wake is `SurfaceEffectsManager`'s job and already exists; see
  [WaterModels.md](WaterModels.md).
