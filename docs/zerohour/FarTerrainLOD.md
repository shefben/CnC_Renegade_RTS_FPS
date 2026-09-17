# Far / background terrain LOD

Roadmap Section 34 (Zero Hour / SAGE feature 21).

Donor: the TheSuperHackers Zero Hour equivalent of `W3DTerrainBackground`, which the Zero
Hour implementation describes as backup terrain rendered at lower resolution.

Acceptance:

- no visible cracks at near/far boundaries under normal camera ranges;
- far terrain uses substantially fewer vertices/draw submissions than near terrain;
- terrain edits invalidate the correct background regions;
- one canonical terrain query/collision representation remains authoritative.

## What is here

| File | Holds |
|------|-------|
| `Code/WWPhys/worldterrainsystem.h/.cpp` | the block table, the drop, the blocks, the per-frame LOD decision |
| `Code/WWPhys/terraintexturesystem.h/.cpp` | `Build_Sampled_Materials`, one dresser read at a stride; `Build_Far_Block_Materials` |
| `Code/WWPhys/renegadeterrainpatch.cpp` | `Render` reads the hidden flag, the way every other render object in `ww3d2` does |
| `Code/WWPhys/pscene.cpp` | `Update_Terrain_LOD` once per frame from `Pre_Render_Processing`, where the camera is |
| `Code/Commando/terrainselfcheck.cpp` | `Check_Far_Terrain`, run as `terrain_far` and `fds_terrain_far` |
| `Code/Commando/consolefunction.cpp` | `far_terrain`; `terrain_test` builds the layer as well as the field |

## Not a second terrain database

Section 34 says to integrate this into Feature 4's single `WorldTerrainSystem` and not to
create a second independent terrain database. So a far block is not a copy of the ground,
it is a *view* of the one `HeightfieldClass` this class already owns: every `FAR_STRIDE`-th
grid point, over `FAR_BLOCK_PATCHES` near patches each way, filled into one
`RenegadeTerrainPatchClass` — the same render object the near patches use, so nothing in
the renderer learns a second way to read terrain.

The block calls `Set_Collision_Type(0)`. That is not a note in a comment; the patch class's
`Cast_Ray` begins by testing the collision type and refuses outright, so the far layer is
*unable* to answer a question about the ground. Sampling, placement, footsteps, rays,
physics and every future consumer keep going to the near representation because there is
no other one that will speak to them. That is the fourth acceptance line, kept by
construction rather than by convention, and the check proves it by dropping a ray on a far
block and requiring a miss.

## Why blocks meet without cracks

Two seams exist and they are different problems.

**Far to far.** `FAR_STRIDE` divides `HeightfieldClass::PATCH_CELLS`, and a block spans a
whole number of patches, so a block boundary is always a sampled grid line and two
neighbouring blocks keep *the same* points on the line they share. Their edges are
identical, vertex for vertex. The check compares block (0,0)'s right column against block
(1,0)'s left column and requires exact agreement.

**Near to far.** Here the two surfaces agree only at the sampled points; between them the
far chord can ride above the true ground, and where it does, far terrain would poke
through near terrain. So the layer measures how much: `Compute_Far_Terrain_Drop` walks
every grid point, works out how high the coarse surface sits over that point, and takes the
largest answer over the whole field. Bilinear and *both* diagonal triangulations are
tested, so the bound holds whichever way a quad is split. The whole far layer is then
lowered by that one number.

The result is the invariant the boundary rests on: **the far surface is nowhere above the
near surface.** A boundary can therefore show a small step down; it can never show a hole
through to the sky, and far ground can never appear on top of near ground. The check
verifies it at every grid point of a 129×129 field using arithmetic written independently
of the service's own — a check that reused `Compute_Far_Terrain_Drop` would only be
comparing it with itself.

One drop for the whole layer rather than one per block, deliberately. A per-block drop
would move two neighbours' shared edge by different amounts and tear the far layer open
along every block seam, which is the crack this phase exists not to have. The price is that
a change in the drop refills every block, which is why `Build_Far_Terrain_Representation`
and `Update_Far_Terrain` both check it once before they start.

## Which representation draws which ground

`Update_Terrain_LOD` runs once per frame from `PhysicsSceneClass::Pre_Render_Processing`,
which is where the camera is and which is before the visible set is collected. It costs
nothing on a level with no heightfield, which is every stock Renegade level.

The decision is **per block, not per patch**. If a block and its own near patches could
disagree, two surfaces would be drawn over the same ground and fight; one decision per
block means every square metre is drawn by exactly one representation, always. A block
draws near if any patch inside it selects better than `LOD_FAR`, and a block with no
geometry built yet always draws near, because it has nothing to hand the ground to.

Hiding is how the hand-over happens, which is why `RenegadeTerrainPatchClass::Render` now
reads `Is_Not_Hidden_At_All` like everything else in `ww3d2`. Terrain never needed the flag
while every patch was always drawn.

## The cost, as arithmetic

With `PATCH_CELLS` 16, `FAR_BLOCK_PATCHES` 4 and `FAR_STRIDE` 4:

| | near | far |
|---|---|---|
| submissions over one block's ground | 16 patches | 1 block |
| vertices over one block's ground | 16 × 289 = 4624 | 289 |

Sixteen to one both ways. The check builds one far block and the sixteen near patches under
it and compares the counts; `far_terrain` prints the same numbers for a running field.

## Edits

The block table belongs to the *field*, not to the renderer: `Create_Terrain` makes it, so
a system that changes the ground before anything has been drawn is still recorded. Blocks
start clean, because dirty means "the block that is built is out of date" and there is no
block yet.

Three routes reach it, and they are three because a caller should not have to know which
one applies:

- `Invalidate_Far_Terrain_Region(AABox)` — what a system says when it has changed the
  ground itself. Resolved in grid indices rather than by testing block bounding boxes,
  because a block's box follows the ground and the ground is exactly what just moved; a
  valley that was deepened is outside the box it had a moment ago. A box off the end of the
  field names no block, rather than being clamped onto the nearest one.
- `Modify_Height_Region`, `Apply_Road_Grade` and `Apply_River_Cut` call it themselves, from
  the footprint they already know. A road or a river never has to have heard of far terrain.
- `Fold_Dirty_Patches_Into_Far_Blocks`, run by `Update_Far_Terrain`, picks up anything that
  went around all of the above and touched the heightfield directly.

`Update_Far_Terrain` refills dirty blocks and creates none — the same contract
`Update_Collision` has, and for the same reason: something shaping the ground every frame
wants what it moved refilled, not a world created underneath it because it called too
early.

## Art

The far layer names no asset of its own. It reads the same `TerrainTextureSystem` layer
table the near patches read, at the points it sampled, so a slope cannot change colour as
it crosses the LOD boundary. That is Section 34's shared material/texture selection, and it
is one code path rather than two agreeing: `Build_Sampled_Materials` takes the stride as a
parameter and the near patches pass 1.

The consequence for the art is recorded in `docs/assets/TerrainLayers.md`: a terrain layer
texture is tiled at four times the spacing when a far block draws it, so a pattern that
only reads correctly at near spacing will read as noise at distance.

## What is not done

- **Nothing has been seen.** The checks run without a device. Whether the boundary reads as
  a boundary, and whether the drop is invisible at the distance it happens at, wants one
  run with a display.
- **The mid-distance tier is enumerated and unused.** `Select_Render_LOD` returns
  `LOD_MEDIUM` and this layer treats it as near. Section 34 calls mid-distance tiled
  terrain "as useful"; it is not useful until there is a field big enough for two
  reductions to be worth switching between.
- **Nothing streams.** `SECTOR` asset residency (P08) and this phase are the two halves of
  loading part of a level, and neither has the other yet.
- **The drop is measured over the whole field.** An edit recomputes it, which is a pass over
  every grid point. That is affordable per edit and would not be affordable per frame; a
  system that shapes ground continuously should batch its edits.
