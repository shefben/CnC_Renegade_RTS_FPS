# Surface mark textures

Roadmap Section 35. Read by `WorldSurfaceMarkManager::Build_Definition_Geometry`
(`Code/WWPhys/worldsurfacemarkmanager.cpp`), through
`WW3DAssetManager::Get_Instance()->Get_Texture(def.Get_Texture())`.

Seven textures, and none of them is urgent — because unlike every other art gap in this tree,
this one is already covered by shipped Renegade content. Every explosion definition and every
surface effect in `always.dat` carries a decal texture name of its own, and those names intern
their own groups. So the game already draws scorches and bullet holes through this manager
today, with stock art, on stock levels.

What is missing is the seven **named** kinds: the ones code can ask for by concept rather than
by asset name, when nothing in the content pipeline has an opinion.

## The textures

| Name | What it is |
|------|------------|
| `ow_mark_scorch.tga` | A round burn. Soot at the middle, ragged at the edge, darkening rather than colouring. |
| `ow_mark_blast.tga` | A bigger crater smudge — the ground thrown outward from a centre, with radial streaks. |
| `ow_mark_dirt.tga` | Loose earth kicked over the surface. Lighter than a scorch; sits on top rather than burns in. |
| `ow_mark_stain.tga` | Oil or fluid pooled and soaked in. Dark, wet-looking, a definite edge. |
| `ow_mark_construction.tga` | The scuffed, tracked-up ground where a building was put up. Survey lines, churn, tyre confusion. |
| `ow_mark_impact.tga` | A single small hit — a bullet strike on soft ground. Little more than a dark dot with a rim. |
| `ow_mark_world.tga` | A decorative mark a level designer places deliberately. Whatever the level needs; deliberately generic. |

## The kinds that read them

Set by `WorldSurfaceMarkManager::Define_Default_Marks`. Radius is half the width of the quad in
metres, so the mark is twice that across. Life is how long before it is gone; a life of zero
means it has no clock at all and leaves only when the pool needs its slot.

| Kind | Type | Radius | Life | Opacity |
|------|------|--------|------|---------|
| `ow_mark_scorch` | `SCORCH` | 1.50 m | 45 s | 1.00 |
| `ow_mark_blast` | `BLAST` | 3.00 m | 60 s | 0.90 |
| `ow_mark_dirt` | `DIRT` | 1.00 m | 30 s | 0.70 |
| `ow_mark_stain` | `OIL_OR_STAIN` | 0.80 m | 90 s | 0.85 |
| `ow_mark_construction` | `CONSTRUCTION` | 4.00 m | persistent | 1.00 |
| `ow_mark_impact` | `IMPACT` | 0.25 m | 25 s | 1.00 |
| `ow_mark_world` | `DECORATIVE_WORLD_MARK` | 2.00 m | persistent | 1.00 |

A radius that arrives from content — an explosion's `DecalSize`, a surface effect's
`DecalSize` plus its random spread — overrides the table for that mark. The table is what a
caller gets when it asks for a type and says nothing else.

## Without them

Nothing of those seven kinds is drawn, and everything else works. Marks of those kinds are
still placed, aged, evicted and counted; `mark_status` reports real numbers, and
`Get_Missing_Texture_Count()` says how many kinds hold marks with nothing to draw them with. No
mesh is created at all until a texture resolves, so a missing mark texture costs zero rather
than a white square.

Marks whose texture came from content are unaffected and draw normally.

## Constraints

**The quad is the whole image, and `u` and `v` both run 0 to 1 across it.** A mark does not
tile in either direction. The image is the mark: what is at the corners is at the corners of
the quad.

**Alpha is the shape of the mark, and it is multiplied by the fade.** The shader
(`Mark_Shader` in `worldsurfacemarkmanager.cpp`) is alpha-blended, not alpha-tested: source
alpha against one-minus-source-alpha, no depth write, no culling. Per-vertex alpha carries the
fade and the kind's opacity; the texture's own alpha carries the shape, and the two multiply. A
texture opaque out to its edges makes a hard-edged square, which is the one thing a scorch must
not be — the edge has to be in the alpha channel.

**It will be seen on every ground surface in the game.** Unlike a track, a mark is not gated by
surface type: a grenade scorches concrete as readily as dirt. Darkening reads correctly on all
of them; a mark with its own colour will fight at least one ground it lands on.

**No culling, so both faces show.** A mark's winding depends on which way the thing that made
it was facing. It is seen from above either way and nothing needs mirroring.

**It sits 5 cm above the ground, and its corners are individually on the ground.** `LiftHeight`
defaults to 0.05 m. The four corners are conformed one at a time
(`WorldSurfaceMarkManager::Drape_Mark`), so a mark on a slope follows the slope rather than
burying one edge. A mark whose corners disagree about the ground height by more than one and a
half times its own radius is not drawn as a quad at all — it goes to the decal projector, which
clips it into the geometry properly.

**Bigger marks drape worse.** The corner test scales with the radius, so a 3 m blast will
accept far more ground variation across itself than a 0.25 m impact will. That is deliberate —
a big mark spans real terrain and has to tolerate it — but it means a large mark is the one
most likely to look stretched. If a blast texture ever looks wrong on rough ground, the fix is
`SURFACE_MARK_MAX_DRAPE`, not the art.

## How to see it

In a running client, at the console:

```text
mark_texture <a texture name from this level>
mark_test 500
mark_status
```

`mark_test` scatters marks on the ground around the camera in a spiral and reports how many it
placed and how many the ground refused. `mark_status` prints the marks against the objects
drawing them, which is the acceptance in two numbers. `mark_clear` erases them;
`mark_texture` with no argument puts the named kinds back to drawing nothing.

Blowing something up or shooting a wall goes through the same manager, using whatever texture
the content named.

## Not listed here, and why

- **No separate texture per surface type.** A scorch on snow and a scorch on tarmac are the
  same texture. If they should differ that is another definition and another row here, not a
  change to the system.
- **No decay stages.** A mark fades in alpha and then goes; it does not become a different,
  fainter texture partway through.
- **Nothing for tracks.** A continuous strip behind something that moved is the ribbons'
  job, not this one; see [RibbonSurfaces.md](RibbonSurfaces.md).
