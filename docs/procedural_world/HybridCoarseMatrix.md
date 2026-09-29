# Hybrid rewrite I5 — runtime Phase 4, coarse semantic matrices

Owner of the plan's runtime Phase 4 (`Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md`
Section 8 Phase 4, Sections 2.2, 4.3, 6.9, 9 and 16.3).

| Concern | Owner |
|---|---|
| Layer population, reservations, conflict resolution, biome ownership, gate, checksum | `Code/wwphys/hybridcoarsematrix.{h,cpp}` |
| Section-2.2 layer storage (rewritten for this item) | `Code/wwphys/hybridplanrecords.{h,cpp}` — `HybridSemanticMatrix` |
| Section-6.1 ring repair (shared for this item) | `Code/wwphys/hybridmapidentity.{h,cpp}` — `Hybrid_Repair_Ring_Search` |
| Role hard-validity rules (exported for this item) | `Code/wwphys/hybridstrategicgraph.{h,cpp}` — `Hybrid_Role_Is_Hard_Valid` |
| Playable/placeable classification | `WorldBoundsService` (unchanged; Phase 4 only consumes it) |
| Profile adapter, per-attempt arena and counters | `ProceduralWorldManager::Build_Hybrid_Coarse_Matrices` |
| Gate self-check | `Check_Hybrid_Coarse_Matrix` in `Code/Commando/terrainselfcheck.cpp`, ctest `terrain_hybridcoarsematrix` + `fds_terrain_hybridcoarsematrix` |

Same split as I2/I3/I4: the phase is pure over `(HybridCoarseMatrixParams, HybridMapIdentity,
HybridArchetypeIntentSet, HybridStrategicGraph)` plus the configured placeable boundary, and Combat
fills the params from the validated profile and the frozen preflight metric cache. Phase 4 runs once
per attempt in `Generate_World_Attempt`, after Phase 3.

## What Phase 4 produces

`HybridCoarseMatrices` — the Section-2.2 SoA layers at spacing `S`, dimensions
`ceil(world.width/S) x ceil(world.height/S)` row-major `(y,x)`; the reservation set; the resolved
Section-4.3 biome divider; and the gate checksum.

Population order is the plan's exact ten steps: boundary classification, middle tier, archetype macro
zones, `BASE_PROTECTED`/`BASE_CORE`, Tiberium discs, heavy corridors `max(6S,0.08D)`, service/trail
corridors `max(4S,0.05D)`, the `open_frontier` `NO_IMMUTABLE_BARRIER` layer, biome ownership,
`NO_DECORATION`. No crossing window exists yet.

## Decisions this item had to make

**Reservations are resolved before anything is rasterized.** Conflict resolution moves a role, and a
role that moved after its disc was stamped would leave the matrix describing a position nothing
occupies. So the phase builds the reservation geometry from the graph, repairs conflicts, and only
then writes the ten steps. The plan's ten-step order is the order the *layers* are written in, which
is what later phases depend on, and that is unchanged.

**A corridor is layered intent, never an exclusive reservation.** A heavy family corridor is built to
leave a base and a service corridor is built to reach the field it serves, so "corridor overlaps
reserve" is the normal case, not a conflict — Section 2.4 already has `ROAD_X_BASE` and
`ROAD_X_TIBERIUM` interaction records for deciding what it means on the ground. Only role reserve
discs are exclusive, so the conflicts Phase 4 can actually detect are disc-versus-disc, which is also
the only kind Section 6.1 repair can fix by moving a role. The priority comparator still carries all
eight ranks, unchanged, because crossing-window insertion after Phases 5 and 6 reuses it.

The plan's list has no rank for a service/trail corridor; it ranks with optional route corridors,
which costs nothing because it is not exclusive.

**`OPEN_MANEUVER` is not exclusive with roads or resources.** The plan says so outright: its rank
applies only to immutable landform/water conflicts, and those are Phase 5's and Phase 6's to honour
by reading the dedicated `NO_IMMUTABLE_BARRIER` layer. Phase 4's own job for these discs is the
Section-6.9 acceptance rule — at least 95% of the disc's sampled area at spacing `A` inside the
placeable boundary — with the same ring repair on failure and an attempt failure when a required disc
cannot be placed.

**A macro band is one coarse cell wide wherever the plan fixes only a centreline.** `PRINCIPAL_RIDGE`,
`PRINCIPAL_CANYON` and `MAIN_RAVINE_GUIDE` are frozen at Phase 2 as "centred at `(0,0)`,
approximately along strategic `v`" with no cross-section: the cross-section is Phase 5's. `S` is the
finest thing this representation can express, so the band is `S` either side of the strategic `v`
axis at the intent's own `u`, spanning the playable `v` range. Same reasoning for the coast band (the
`S`-wide strip inside the sea boundary) and the `plateau_basin` rim ring. The intent kinds that are
point intents with no band at all — `SURFACE_SADDLE`, `PENINSULA_NECK`, `BASIN_ENTRY`, the crossing
guides — are deliberately left alone here.

`plateau_basin`'s dry basin floor needed a bit of its own: Section 2.2's list has no basin flag and
`WATER_BASIN` is water intent, which a dry basin is not. `HYBRID_FLAG_BASIN` was added, along with
`HYBRID_FLAG_PLACEABLE` (buildable is a narrower question than playable — the perimeter cliff band is
playable and never buildable) and `HYBRID_FLAG_NO_IMMUTABLE_BARRIER` (the plan asks for it by name).
Section 2.2's list is "flags include", so extending it is in-contract.

**A region narrower than `S` must not vanish.** A cell belongs to a stamped region when its *centre*
is inside it, which is the only rule that cannot drift — but a Tiberium field core at
`resource.field_radius_min` on a 30-unit grid can miss every cell centre entirely, and the phase gate
reads exactly the cell holding a role's centre. So every stamp also writes the cells holding its own
endpoints. This was found by the gate self-check failing on a `D=600` fixture, not reasoned about in
advance.

**The field "core" is `resource.field_radius_min`.** Step 10 stamps `NO_DECORATION` over the
"provisional Tiberium field core", and before Phase 13 draws the real polygon the part of the reserve
a field is guaranteed to fill is exactly the profile's minimum field radius. The reserve outside it
stays decorable.

**Primary biome owns the GDI side.** Section 4.3 fixes the divider and the 35% rule but not which
side is which; `u < divider(v)` is primary, which keeps the primary biome on the GDI half of the
strategic frame. The divider's four control `v` values are equally spaced (`-1, -1/3, +1/3, +1`), so
the Bezier's own `v(t)` is exactly `2t-1` and `u` at a given `v` is an exact evaluation with no root
find. Only playable land counts toward the 35% test — sea is neither biome's land — and the six
`0.05` shifts toward zero and the straight `u=0` fallback are Phase 4's, as the plan assigns them.

**The matrix lives in the attempt arena.** Section 16.3 lists semantic matrices as arena tenants, so
`HybridSemanticMatrix` was rewritten from `DynamicVectorClass` layers to one aligned block carved
into layer pointers, allocated from a `HybridScratchArena` when one is supplied and from a single
heap block otherwise (self-checks, offline tools). It never grows: a block that does not fit fails
the resize and Phase 4 reports `SCRATCH_EXHAUSTED`. The manager reserves the arena here because
Phase 4 is the first phase that needs scratch, sized by the size-class budget preflight computed, and
releases it with the rest of the attempt's records.

**Section 6.1's ring search has one implementation.** Phase 3 had it inline; Phase 4 needs it for a
moved reservation and for an `OPEN_MANEUVER` disc, and Section 6.9 explicitly points at it. It moved
to `Hybrid_Repair_Ring_Search` beside the strategic frame it is defined in terms of, with the
ring/station order unchanged (radii `A, 2A, ...` up to `0.10D`, 16 stations from strategic `+u`,
counter-clockwise, first accepted candidate wins). The candidate *rule* for a moved role stays with
Phase 3 — `Hybrid_Role_Is_Hard_Valid` is now exported rather than copied — so there is still exactly
one answer to "may this role stand here".

**A repaired role is written back into the graph.** `HybridStrategicGraph` is the one owner of role
positions, so Phase 4 takes it mutable instead of shadowing a second position table. When a move
happens the graph's checksum is recomputed, because that field always digests the graph's current
content; Phase 3 checked its own gate against it before Phase 4 ran.

**A base on hard intent fails the attempt.** `BASE_PROTECTED` is the top rank, so the gate's
"no base disc overlaps water or a cliff/canyon/ravine/coast band" has nothing lower to move: the
archetype macro zones are frozen intent. It is reported as a named attempt failure and the manager
retries with a new seed. `FRONT_CENTER` is excluded from the "role centre is inside its reserved
region" test — Section 6.1 gives it reserve radius `0`, so it owns no region to be inside of — and a
route waypoint is tested only for its corridor, because it may legally sit on the obstruction its
family must cross.

## Gate

`terrain_hybridcoarsematrix` / `fds_terrain_hybridcoarsematrix` cover the arena contract and the
over-budget refusal, the priority comparator and the conflict/overlap predicates, dimensions and
row-major indexing, the middle-tier initialization, the base/field/corridor stamps and their masks at
all three size classes, the `NO_DECORATION` core rule, single-biome ownership, the
`BASE_PROTECTED`-versus-hard-intent promise measured cell by cell, `open_frontier`'s three
`NO_IMMUTABLE_BARRIER` discs at `9W`, determinism (identical inputs, identical checksum; a changed
cell changes it), and the conflict path end to end — a contested field dropped onto a home field is
repaired, the repaired centre lands in the graph, and the move stays within `0.10D`.
