# Hybrid rewrite I3 — runtime Phase 2, MapIdentity

Owner of the plan's runtime Phase 2 (`Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md`
Section 8 Phase 2, Sections 4, 5, 6.1 and 6.10).

| Concern | Owner |
|---|---|
| Records, strategic frame, resolver, checksum | `Code/wwphys/hybridmapidentity.{h,cpp}` |
| Named-key helper `Hybrid_Key` | `Code/wwphys/hybridplanning.{h,cpp}` (with the other Section 0.3 helpers) |
| Profile/metric adapter and per-attempt lifetime | `ProceduralWorldManager::Build_Hybrid_Map_Identity` |
| Gate self-check | `Check_Hybrid_Map_Identity` in `Code/Commando/terrainselfcheck.cpp`, ctest `terrain_hybridmapidentity` + `fds_terrain_hybridmapidentity` |

The split follows I2's: the pure phase lives in `wwphys` over a plain `HybridMapIdentityParams`,
and Combat fills that struct from the validated profile plus the frozen preflight metric cache —
the same shape `MapPlanGenerator`/`MapPlanParams` already use. The resolver reads no profile, no
registry and no global, so the self-check drives all of Phase 2 with hand-filled params and no
game content loaded.

## What Phase 2 produces

`HybridMapIdentity` — size class, symmetry, archetype, strategic frame, biome mode and biomes,
island slot bits, flank sign `s`, route-family count, the Section 6.1 anchors and the gate
checksum.

`HybridArchetypeIntentSet` — the Section 6.10 records, the `RIVER_ADJACENT_CONTESTED` flag and
the Section 4.3 mixed-biome divider parameters.

Both are built in the plan's exact order (size class → symmetry → archetype → strategic axis →
biome mode → primary biome → secondary biome → island slot bits → `ArchetypeIntentSet`). No
terrain is touched and no geometry is realized.

## Decisions this item had to make

**Strategic frame scale is `D/2`.** Section 0.4 fixes the meaning of `(u,v)` but not the world
scale, and the axis angle is seeded, so a rotated axis on a non-square world needs a scale that
keeps the whole `|u|,|v| <= 1` square inside the configured extents. `D = min(width,height)` is
the only such scale, and it matches every Section 6 guide being written as a fraction of `D`.
The frame centre is `world.center_x/center_y`, which is where `ProceduralBoundsBuilder` already
puts the boundary polygon.

**The axis is rebuilt from the quantized angle.** `Hybrid_Build_Strategic_Frame` quantizes the
angle first and derives `AxisU`/`AxisV` from that value, so a frame restored from a checksummed
record is identical to the frame that produced the checksum.

**Named keys are the plan's own spellings.** `Hybrid_Hash01` takes integer keys, so
`Hybrid_Key(name)` (the repository string CRC) turns `"flank_sign"`, `"island"`, `"river_amp"`
and the rest into keys. Where the plan names a draw, that exact spelling is used. Where it does
not, Section 6's rule applies: archetype name + feature `StableId` + parameter name. No draw
uses a hand-assigned number.

**Seed domains.** Map identity and strategic choices use `map_plan`; the guided-river meander
draws use `hydrology`, as Section 6.10 writes them; the mixed-biome divider uses `biomes`.
Nothing introduces a new domain, per Section 0.3.

**Intent `StableId` is creation order, 1-based.** The Section 6.10 table is walked left to right,
so a draw keyed by `StableId` is stable for a given archetype.

**A barrier guide records its centre and axis, not its extent.** `PRINCIPAL_CANYON`,
`PRINCIPAL_RIDGE` and `MAIN_RAVINE_GUIDE` carry `Guide = (0,0)` and the
`ALONG_STRATEGIC_V` flag. The plan's "approximately along strategic `v`" is resolved by the
generic Phase 5 line-feature construction, so Phase 2 must not pick the endpoints.

**Guided-river sampling.** Stations are `RoundHalfUp(length/A)` intervals spread evenly over the
straight start-to-end axis, so spacing is `A` to within one quantum *and* the endpoints stay
exactly where the intent put them, rather than drifting by the remainder of a fixed-`A` walk.
`river_valley` clamps each station's strategic `u` to `±0.10` (Section 6.2);
`COASTAL_GUIDED_RIVER` halves the amplitude and forces its last station onto the sea boundary.
The resulting polyline is quantized and frozen; hydrology promotes this same spline rather than
re-deriving it.

**`COAST_GUIDE` carries the sea boundary.** Section 6.6's `|v| = 0.55..0.70` is drawn in Phase 2
because `COASTAL_GUIDED_RIVER` has to end on it, and both records plus `PENINSULA_NECK` share
the one `v_sign` draw.

**Explicitly asking for `coastal_peninsula` without a coastline fails.** Section 6.6 says the
archetype exists only when `water.coastline=true`; the auto list filters it, and an explicit
request for it is a contradiction no seed rescues, so Phase 2 rejects the request with
`ARCHETYPE_REQUIRES_COASTLINE` instead of generating a sea-less peninsula. The other Phase 2
failures are `NO_AVAILABLE_BIOME`, `NO_AVAILABLE_BIOME_PAIR` and `ILLEGAL_BIOME_PAIR`
(Sections 4.2 and 4.3, including the rejected `snow + desert` pair). Every one is a
profile/content contradiction, so `ProceduralWorldManager` fails the request rather than
retrying a different seed.

**What Phase 2 deliberately leaves alone.** The 35% biome-ownership test, its six `0.05` shifts
and the straight `u=0` fallback belong to Phase 4, which rasterizes the divider; the 95%
placeable test and repair search for an `OPEN_MANEUVER` disc belong to Phase 4 as well; the
`SURFACE_SADDLE` amplitude ramp belongs to Phase 5; role placement and guide legality repair
belong to Phase 3. Phase 2 only freezes the inputs those phases consume.

## Lifetime

`Build_Hybrid_Map_Identity` runs at the top of `Generate_World_Attempt`, right after the world
bounds are configured and before any geography, because a Phase 2 failure is worth finding
before anything has been built. The records belong to one attempt and `Destroy_World` clears
them; the immutable content metrics survive a retry, since nothing preflight reads depends on a
seed. `Has_Hybrid_Identity` is false for the legacy generator, which has no Phase 2 at all.

## Gate

`terrain_hybridmapidentity` (and its `fds_` twin) covers: size class and island slots at each
reference dimension; identical params giving identical records and checksum, and a different
seed giving a different one; `mirror_x` keeping the literal world X axis, every other symmetry
drawing in `[0,pi)`, the frame scale, the `(u,v)` round trip and axis perpendicularity over 64
seeds; the Section 6.1 anchors and the symmetry pairing rule; the archetype coastline filter in
both directions over 256 seeds and the explicit-request rejection; biome mode at probability 0
and 1, the divider control ranges and transition width, the `snow + desert` rejection and the
availability filters; island slot bits at chance 0 and 1; and the full Section 6.10 intent table
for all eight archetypes, including guided-river endpoints, meander ranges, station count, the
`u` clamp, the coastal halved amplitude and forced sea endpoint, bridge/distinct-crossing tags
and the `OPEN_MANEUVER` disc count and radius on SMALL and MEDIUM.
