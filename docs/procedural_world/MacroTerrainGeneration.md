# Macro Terrain Generation (V2 Phase 4)

Roadmap Section 34 Phase 4 -- "generate the bounded playable landform inside the immutable
perimeter" Phase 3 built. See `WorldBoundsAndBoundaryCliffs.md` for the perimeter itself;
this document covers what stands inside it.

## Ownership

`WorldTerrainSystem` (`Code/wwphys/worldterrainsystem.h`) is the canonical terrain owner and
was already complete before this phase started: creation, height storage, sampling, patches,
collision, LOD and the far/background layer are all v1. Phase 4 adds exactly one new thing
that did not already exist -- a generator that fills a heightfield with a deterministic macro
landform -- plus the chunk registry Phase 4 step 14 asks for. Both are additive extensions of
the existing owner, not a second terrain authority:

- `MacroTerrainGenerator` (`Code/wwphys/macroterraingenerator.h/.cpp`) fills a `Heights[]`
  array and hands it to `WorldTerrainSystem::Create_Terrain`/`Set_Heights`, the same entry
  point every other caller uses. There is no second way into the heightfield.
- `WorldTerrainSystem::Build_Chunk_Registry` (new this phase) computes per-chunk metadata --
  height/slope extent, a coarse buildability flag, a cliff-band tag, a revision and a checksum
  -- from the heightfield's own patch data and `WorldBoundsService`. It lives on
  `WorldTerrainSystem` itself rather than on the generator, because "chunk" is not a second
  unit alongside the render/collision patch grid that class already owns; it is that grid
  under Phase 4's name for it. A hand-authored level with its own heightfield gets the same
  registry for free.

Both do-not-proceed conditions ("generated terrain bypasses WorldTerrainSystem", "procedural
terrain has a separate collision truth") are kept structurally: there is one heightfield, one
collision builder, and the chunk registry is read off the same patch data collision already
is.

## Coexist, not replace

Phase 3's own planning note carried an open design question forward: does generated terrain
replace the host level's ground or coexist with it? Phase 4 answers it, and the answer is
neither in the usual sense -- **a fitted world's terrain generator never runs at all.**

`HeightfieldClass` is one rectangular grid with no holes in it. A world laid over a
hand-authored level (`ProceduralWorldDimensions::FitToLevel`, the only kind the "Procedural
Skirmish" menu button has ever produced) already has ground there: the level's own W3D
geometry, the one a player has always stood on. Generating terrain anywhere in a fitted
world -- even a flat plane nobody would notice -- would stand a second, collidable ground
plane under or through the level's real one. That is precisely the "separate collision truth"
fault Phase 4 refuses to proceed past, so `ProceduralWorldManager::Build_Macro_Terrain` checks
`profile.World.FitToLevel` before calling the generator at all, and returns success having
done nothing when it is true. Not a failure: it is the correct behaviour for every world this
integration can currently produce.

The consequence, stated plainly: **the "Procedural Skirmish" button generates no macro
terrain today**, because `data/procedural_world/default.json` sets `fit_to_level: true`.
Nobody will see a generated hill from that button, the same honest gap Phase 3 recorded for
its own cliffs before this session, one level further removed -- Phase 3's cliffs are built
but unseen; Phase 4's terrain is built, proven by a device-less self check, and structurally
wired to the one caller that exists, but that caller's own default profile never asks for it.
Reaching the generation path today needs a profile with `fit_to_level: false` and something
that requests one -- the console command `ProceduralWorldManager`'s own header comment
mentions was never actually built (checked; only the menu button exists), so this is
currently exercised only by `-terrainselfcheck macro`. That is a real gap, not a design flaw:
generating a world with no host level at all is Phase 7+ territory (candidate bases, initial
layout selection), and Phase 4's job was to make sure the generator is correct and safe to
call the day something does.

## Generation

One elevation sample is four independent noise channels, each its own fractal (multi-octave)
sum of `TerrainMaskClass::Hash_Value` -- the deterministic seeded lattice hash
`terraintexturesystem.cpp` already uses for layer variation, reused rather than duplicated;
only the fractal-summing loop and the per-channel frequency/seed offsets are new:

- **macro** -- broad masses, the base relief, weighted by `terrain.macro_relief`;
- **ridge** -- `1 - |noise|`, sharp along its own zero crossing, weighted by
  `terrain.mountain_density` and sharpened further by `terrain.ridge_sharpness`;
- **hill** -- finer detail at roughly three times the macro frequency, weighted by
  `terrain.hill_density`;
- **valley** -- broad, `|noise|`-shaped basins subtracted from the result, weighted by
  `terrain.valley_density`.

`terrain.plateau_chance` optionally terraces the combined result into five bands where a
separate mask channel says a rise may become a mesa, blended rather than snapped so the
terrace meets a slope without a discontinuity the relaxation pass below would immediately
have to fight. This is a heightfield approximation of "plateaus/mesas where profile permits"
(Phase 4 step 5), not Phase 5's vertical-faced mesa -- Section 8.1's real cliff-classification
and mesh replacement needs interior cliffs to exist first.

Every vertex is classified against `WorldBoundsService::Classify_Point` before anything else.
Anything at or past `WORLD_ZONE_CLIFF_BAND` is written flat, at sea level, and marked
`locked` -- neither the generation formula above nor the slope relaxation pass below can ever
write to a locked vertex again, so the immutable face stays immutable by construction, not by
a check that a future edit could defeat. A vertex in `WORLD_ZONE_BOUNDARY_APPROACH` is blended
toward that same flat value as `WorldBoundsService::Get_Inward_Distance` shrinks toward zero,
scaled by `terrain.boundary_approach_flattening` -- Phase 4 step 6's "bias terrain toward
boundary approach targets without modifying the immutable cliff face", kept the same way: the
cliff face is never a term in the blend, only its neighbour is pulled toward matching it.

## Slope enforcement, and one number doing two jobs

`terrain.max_buildable_slope_degrees` is both the generation-time cap Phase 4 step 9 asks for
("enforce maximum ordinary terrain slopes") and the threshold the chunk registry's
`Buildable` flag is computed from. One number was a deliberate choice: two separate
definitions of "too steep" that could silently disagree with each other is a worse fault than
one number that is occasionally slightly conservative for one of its two uses.

Enforcement is a bounded, deterministic local relaxation (`SLOPE_RELAXATION_PASSES = 4` --
generator tuning, not a profile field, same precedent `ProceduralWorldManager`'s own
`CLIFF_STATION_SPACING` already sets): each pass walks the grid row-major and pulls a vertex
and its right/down neighbour together whenever they exceed the slope limit, skipping any pair
touching a locked (cliff-band) vertex entirely. This is **not a hard mathematical guarantee**
for an arbitrarily aggressive profile -- a bounded number of passes reduces slope toward the
limit rather than proving it eliminated everywhere. `MacroTerrainGenerator::Check_Ordinary_
Slope_Limit` reports what is actually left rather than asserting there is nothing, which is
the honest version of this contract; it passes clean for the profile the self check ships
with.

## The chunk registry

`WorldTerrainSystem::Build_Chunk_Registry` computes, per patch (one chunk = one patch =
`ProceduralWorldDimensions::ChunkSize` world units, by construction -- `cell_size =
ChunkSize / HeightfieldClass::PATCH_CELLS`, so the seam guarantee step 15 asks for is the
same seam guarantee the render/collision grid already has):

- height and slope extent, read straight off the patch (`HeightfieldClass::TerrainPatchClass`
  gained `MinSlopeRadians`/`MaxSlopeRadians` this phase, computed the same place
  `MinHeight`/`MaxHeight` already were, from `Compute_Vertex_Normal`);
- `CliffBand` -- true the moment any of five sampled points (centre plus four corners) of the
  chunk classifies at or past `WORLD_ZONE_CLIFF_BAND`. Half a chunk inside the immutable band
  is still not a chunk anything may build on;
- `Buildable` -- coarse and slope-only. **This is not Section 18C's canonical
  `EvaluateBuildability` query.** That contract also needs road/corridor/tunnel/bridge/
  resource exclusions that do not exist yet (Phases 7-10). `Buildable` here exists so a chunk
  can be ruled out cheaply and early; nothing may read it as the final word once 18C's real
  query exists;
- `Revision`/`Checksum` -- a chunk is recomputed only when its `Revision` is still 0 (never
  computed) or `HeightfieldClass::Is_Patch_Dirty` says the ground under it moved, read
  non-destructively so collision's own dirty-flag consumer still sees the edit afterward.
  `Checksum` is a Section-3B.4-quantized CRC over the fields above; `Get_World_Chunk_Checksum`
  folds every chunk's checksum together in row-major order into the one number Phase 4 step 17
  and `WorldGenerationManifest::TerrainChecksum` both mean by "the terrain".

Building the registry needs no physics scene -- every input is the heightfield's own patch
data plus `WorldBoundsService`, neither of which needs one -- so it is provable the same
device-less way Phase 3's `Plan_Ring` is.

## Debug drawing

`debug_layer terrain_chunks 1` draws one box per chunk at the chunk's own top, coloured green
(buildable), yellow (ordinary but too steep) or red (cliff band), read entirely off the
registry rather than off the visible ground -- a chunk whose colour disagrees with its own
terrain is the tagging/seam bug this layer exists to catch. `DEBUG_LAYER_TERRAIN_CHUNKS`
(`Code/ww3d2/debugrendersystem.h`) was already enumerated by Section 33's original eighteen
names, with no renderer behind it; this phase is its first consumer.

## What is proven, and what is not

`terrain_macro`/`fds_terrain_macro` (`Code/Commando/terrainselfcheck.cpp`,
`-terrainselfcheck macro`) prove, device-less: parameter refusal leaves nothing behind;
vertex/chunk counts match the arithmetic a chunk size and cell size imply; height range and
ordinary slope hold for the shipped defaults; a cliff-band chunk is flat at sea level and an
interior chunk is not, once a boundary is configured; the same seed and profile reproduce the
same terrain checksum, and a different seed does not.

What is not proven, because nothing here runs with a display: whether the generated relief
looks like terrain rather than noise; whether the plateau terracing reads as a mesa or as a
staircase; whether a vehicle can actually drive the slopes the check calls legal; and the
whole reachability gap above -- the one caller that exists never asks for generation at all.
Next exact action for a display-bearing check: a profile with `fit_to_level: false`, driven
through `Request_World`/`Generate_World` once a caller exists to ask for one, or a
purpose-built manual harness in the meantime.
