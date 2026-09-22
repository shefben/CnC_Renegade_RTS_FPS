# Interior Cliffs (V2 Phase 5)

Roadmap Section 34 Phase 5 -- "interior cliffs, ravines, canyon walls, and cliff reuse" (Section
8). See `MacroTerrainGeneration.md` for the heightfield relief this phase reads candidates
from and `WorldBoundsAndBoundaryCliffs.md` for the boundary ring this phase must never touch.

## Ownership -- one cliff renderer, two planners

Phase 3's own header comment turned out to answer Phase 5's own "locate in source" instruction
before Phase 5 was written: `BoundaryCliffGenerator` is a thin planner over
`EscarpmentClass`/`EscarpmentSystem` (`Code/wwphys/escarpment.h/.cpp`), which already is the
general "polyline with an upper and a lower bench height" cliff-face primitive Phase 5 step 4
asks to reuse. An open, non-ring polyline builds through `EscarpmentSystem` exactly as the
closed boundary ring does, so `InteriorCliffGenerator` (`Code/wwphys/interiorcliffgenerator.h/
.cpp`) is the same shape as `BoundaryCliffGenerator`: a planner that emits `EscarpmentClass`
chunks and hands them to the one shared system, tracking only the IDs it created so `Clear`
removes exactly its own work. The two generators' owned-ID lists partition the same
`EscarpmentSystem` collection -- neither ever iterates or clears the whole thing, which is what
lets a hand-placed escarpment, or the other generator's own escarpments, survive either one
being rebuilt.

There is no second cliff mesh builder, no second collision path, and no second material
projector. Phase 5's do-not-proceed condition "a second unrelated cliff renderer exists" is
kept the same way Phase 3 kept it: by there being exactly one.

## Where a candidate comes from

Phase 5 step 1 asks to "classify terrain regions whose requested slope/relief exceeds ordinary
heightfield limits." `MacroTerrainGenerator`'s own bounded relaxation pass (`Relax_Slopes`,
`SLOPE_RELAXATION_PASSES = 4`) already spends its whole effort pushing every ordinary chunk's
slope *down toward* the profile's buildable-slope cap -- and, by its own documented honesty,
"reduces toward the limit," not a hard guarantee it always reaches it. A chunk still sitting at
or near that cap after relaxation, in the chunk registry Phase 4 built, is exactly the terrain
that wanted a vertical face and was capped into a steep ramp instead. So `InteriorCliffGenerator`
reads candidates from `WorldTerrainSystem::Get_Chunk_Info`'s own `MaxSlopeRadians`, gated at
`InteriorCliffParams::SlopeThresholdFraction` of the shared buildable-slope cap -- data the
terrain generator already computed, never a second noise field that could disagree with it.

A chunk already tagged `CliffBand` is never a candidate, checked directly against the registry
rather than re-derived, so a chunk the boundary ring already owns can never also become an
interior cliff.

## The walk, and what it deliberately is not

`ProceduralWorldTerrainSettings::InteriorCliffDensity` (reserved by Phase 1, unconsumed until
this phase) gates which qualifying chunks actually become candidates, one deterministic
`TerrainMaskClass::Hash_Value` draw per chunk against the profile's own density weight.

Turning a scatter of candidate chunks into a wall needs an order, and a full connectivity flood
fill over an arbitrary blob of candidates has no single ordered polyline to build from. This
generator does not attempt ridge-line extraction (non-maximum suppression along a gradient,
watershed segmentation) -- that is a real image-processing problem and a mathematically
different task. Instead it walks: starting from the first unclaimed candidate chunk found in
row-major order, it extends chunk to chunk through other unclaimed candidates, preferring to
continue in whatever direction it is already heading and falling back to a fixed neighbour
order (east, north, west, south) only when it must turn, capped at `MaxRunLength` chunks. The
result is a real, deterministic, collision-backed wall through real relief -- not a claim that
the walk found the *best* wall the terrain could have offered.

A walk that finds no second chunk to extend into is discarded rather than built as a one-chunk
feature. Phase 5's own do-not-proceed condition forbids "interior cliffs... implemented as
extreme heightfield spikes," and a lone candidate chunk with no unclaimed steep neighbour is
exactly that spike.

## Terrain conforms cliffs; cliffs do not conform terrain

`EscarpmentSystem::Conform_To_Terrain` resamples the bench heights of *every* escarpment
currently registered against the heightfield -- useful when an escarpment is meant to sit on
ground that already exists, wrong when it would also resample the boundary ring's own authored
rim/toe against the deliberately flat cliff-band terrain and collapse Phase 3's cliffs back to
sea level. `InteriorCliffGenerator` never calls it. Instead, for each station it is building,
it samples `WorldTerrainSystem::Sample_Height` itself, once to either side of the local path
direction (`BENCH_SAMPLE_INSET`), and takes the higher sample as the upper bench and the lower
as the lower -- the same idea `Conform_To_Terrain` uses, scoped so it can only ever touch a
station this generator is itself creating.

## Type, traversability, and what is deliberately not built yet

Section 8.1's cliff kinds (`InteriorCliffType`) and Section 8.3's traversability kinds
(`InteriorCliffTraversabilityType`) are both a single deterministic hash draw off the run's own
starting chunk, on their own offsets so neither draw, nor the density gate above, correlates
with either of the others.

`COASTAL_CLIFF` is not in `InteriorCliffType` at all -- left out of the enum rather than a value
this generator promises never to pick, because Phase 5 runs before Phase 6 hydrology exists to
say where a coast is. A future hydrology pass may retag a feature that turns out to border
generated water; that retagging is Phase 6's job.

A feature whose traversability names a bypass kind (infantry ramp, vehicle ramp, tunnel bypass,
bridge crossing) records one representative point -- the midpoint of its own run -- and nothing
else. Phase 5 step 10 explicitly forbids building the actual ramp, tunnel or bridge yet; that is
Phases 8, 9 and 10's job. `InteriorCliffFeature::BypassPoint` is a candidate, not a placement.

## Non-buildable, and navigation for free

`WorldTerrainSystem::Mark_Chunk_Interior_Cliff` is the one door this generator uses to record
that a chunk is occupied -- never a direct field write, per Section 3D.1's ownership rule that
no subsystem mutates another owner's authoritative state through a private backdoor. It flips
`TerrainChunkInfoClass::InteriorCliff` and `Buildable` and recomputes the chunk's own checksum,
so a chunk an interior cliff run occupies reads exactly as non-buildable as a cliff-band chunk
does to anything reading the registry.

Navigation blocking costs nothing extra: `WorldTraversabilityGraph` (roadmap Section 52B Phase
65, already complete) already consults `EscarpmentSystem::Is_Segment_Blocked` for every
registered escarpment when it decides whether an edge exists between two cells. An interior
cliff, once added to `EscarpmentSystem`, is automatically respected by that graph with no code
in either file aware the other exists -- the same "ask the one shared system" discipline that
makes this whole phase possible in the first place.

## Decoration, honestly scoped

Section 8.4 asks for bounded scattered rocks, scree and debris without creating climb exploits.
`EscarpmentClass::Set_Toe_Width` already draws exactly that apron -- "the scree/debris skirt
real rock piles up at its own base," in that class's own words -- flat on the lower bench and
never a separate collider, so it cannot become a climbable step. `InteriorCliffGenerator` sets
it on every feature it builds. A separate scatter of individual rock/vegetation props is not
attempted here; that is a decoration-layer concern (Section 3D.1 names decoration as never
mutating authoritative state), not this generator's.

## The chunk-band overlap gate, verified rather than assumed

Every candidate chunk is checked against the registry's own `CliffBand` flag before it can ever
become a station. `Check_No_Cliff_Band_Overlap` re-derives the same classification independently
(`WorldBoundsService::Classify_Point` on every built station) as a regression net -- the
candidate gate should already make this impossible; the check exists to prove it stayed that
way rather than to discover it.

## Debug drawing

`debug_layer interior_cliffs 1` draws one open polyline per feature -- rim, toe, and a riser
per station coloured green (non-traversable wall) or orange (names a ramp/tunnel/bridge
candidate), plus a yellow cross at any feature's bypass point. `DEBUG_LAYER_INTERIOR_CLIFFS`
(`Code/ww3d2/debugrendersystem.h`) is new this phase, kept separate from `boundary_cliffs` for
the same reason that layer is separate from `world_bounds`: two different generators feed the
one shared `EscarpmentSystem`, and seeing which cliffs came from which is exactly how a
generator writing into the wrong owner's chunks would be spotted.

## What is proven, and what is not

`terrain_interior_cliffs`/`fds_terrain_interior_cliffs` (`Code/Commando/terrainselfcheck.cpp`,
`-terrainselfcheck interior`) prove, device-less: parameter refusal leaves nothing behind; a
*synthetic uniform-slope ramp* (not `MacroTerrainGenerator`'s own noise, deliberately -- see the
test's own header comment on why a real generated seed cannot honestly be asserted to produce a
feature) produces at least one feature with every structural diagnostic holding; every chunk an
interior cliff occupies reads non-buildable in the registry; the same seed and terrain reproduce
the same checksum, and a different seed does not; and a configured boundary's cliff-band chunks
never become a candidate however steep the synthetic ramp makes them.

What is not proven, because nothing here runs with a display or against `MacroTerrainGenerator`'s
own noise: whether a real generated seed's terrain produces interior cliffs that read as
canyons/escarpments/mountain faces rather than as an arbitrary scatter; whether the walk's
chunk-to-chunk path looks like a wall from inside the world. The reachability gap Phase 4
carried -- this generator only running when macro terrain itself ran, which the one real
caller's `fit_to_level` profile made never -- is closed by V2 Phase 10's fitted-terrain
sampling; see `completed_features.md` and `GeneratorDeterminism.md`. What remains is a live pass
with a person watching, per `WIP_features.md`.
