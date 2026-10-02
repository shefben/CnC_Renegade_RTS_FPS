# Hybrid rewrite I7 — runtime Phase 6, vector hydrology

Owner of the plan's runtime Phase 6 (`Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md`
Section 8 Phase 6, Sections 2.3, 5, 6.6, 6.10, 11 and 16).

| Concern | Owner |
|---|---|
| Drainage solve, channels, selection, lakes, coast, islands, windows, gate, checksum | `Code/wwphys/hybridhydrology.{h,cpp}` |
| The Phase-7 `water_carve` term, standing-water queries and the island bias | `Hybrid_Water_Carve_Depth` / `Hybrid_Point_In_Standing_Water` / `Hybrid_Island_Height_Bias` |
| The provisional Phase-6 surface (`tier + Phase-5 landforms`) | `Hybrid_Provisional_Height`, used by Phase 6 and by Phase 7's guided-river samples |
| Accepted water intent, window flags and the `NO_DECORATION` approach stamp | Phase 6 writes them into the Phase-4 `HybridSemanticMatrix` |
| `HYBRID_RESERVE_CROSSING_WINDOW` reservations | Phase 6 appends them to `HybridCoarseMatrices::Reservations`, reusing Phase 4's comparator |
| Carve depths | `WaterDefinitionClass::BedDepth` through `WaterSystem`, read in the manager adapter |
| Centripetal Catmull-Rom sampling | `Hybrid_Sample_Centripetal_Catmull_Rom` (`hybridplanning`), unchanged |
| Profile adapter, per-attempt arena and counters | `ProceduralWorldManager::Build_Hybrid_Hydrology` |
| Gate self-check | `Check_Hybrid_Hydrology` in `Code/Commando/terrainselfcheck.cpp`, ctest `terrain_hybridhydrology` + `fds_terrain_hybridhydrology` |

## What the phase produces

`HybridHydrologySet` holds three record lists plus the Section-2.3 planning spatial hash:

- `HybridRiverSystem` — one per selected river: the vector centerline sampled at `A/2`, and per
  sample its station, drainage accumulation, Section-11 width and the drainage surface Phase 7
  step 1 asks for (the filled Priority-Flood height for a D8 river, the provisional terrain height
  for a guided one).
- `HybridWaterBody` — lake, coast and island polygons as closed simple rings, with the water
  elevation, the polygon area, the `BedDepth` that carves it and, for an island, Phase 7's
  `island_macro_height_bias` and its `2W` shoreline blend.
- `HybridCrossingWindow` — one per accepted crossing or ramp/pass station: the obstruction it
  belongs to, the local span, the reserved straight approach, the two endpoint elevations and the
  method flags computed once from the frozen preflight metrics.

Rivers, bodies and windows share **one** StableId space in creation order, so a spatial-hash query
answers with ids that are unique across all three kinds.

No heightfield is written: Phase 7 owns terrain and takes this phase's carve through
`Hybrid_Water_Carve_Depth`. No road exists yet either — a window is a permission, and Phases 9 to 12
decide which windows are used and what physical object satisfies each one.

## Resolutions the plan leaves open

These are implementation decisions taken from the plan's own evidence rather than invented
alternatives. Each is deterministic and recorded in `DECISIONS.md`.

**A "system" is a flow-connected channel component.** Step 9 says to trace every headwater to a sink
and "merge shared downstream chains by analysis-cell id". That merge is realized as a union-find over
channel cells joined to their D8 destination, so one system is exactly one flow-connected channel
component and a tributary never counts as a second system (Section 5's own qualification). The
system's stored centerline is its *primary* headwater's chain: longest quantized chain, then more
accumulation at the source, then the lower analysis-cell id.

**Basin polygons come from the cell outline.** A retained depression's ring is traced as a closed
lattice outline of its four-connected cell set, walked from the basin's lowest-index cell so the
start is always on the outer boundary, then stripped of collinear corners. Four-connectivity is the
plan's own consequence of wanting one non-self-intersecting polygon per basin: two blobs touching
only at a corner are two basins. A corner two unused outline edges still start from would make the
ring touch itself, so that basin is reported as not simple and is illegal under step 13, exactly as
the plan requires. A dry hole inside a depression is folded into the outer ring; step 17's islands are
the plan's mechanism for dry land inside water, not a basin hole.

**Islands are their own record, not a hole.** Step 17 says to subtract the island from the host water
polygon. That subtraction is realized by every water query treating an island polygon as land
(`Hybrid_Point_In_Standing_Water` answers false, `Hybrid_Water_Carve_Depth` returns zero), which is
the same result without a polygon boolean and without a second description of the host's outline for
Phase 7 to disagree with.

**The coast polygon is the world rectangle clipped to the sea side.** Step 15 gives the sea a
strategic `v` boundary and a side but no outline. The polygon is therefore the world rectangle
clipped by that one half plane, which keeps the sea convex and inside the world bounds; a local retry
moves the boundary outward by exactly `S` in strategic units (`S / Frame.Scale`).

**A river mouth ends at the shoreline it reaches.** Step 16 binds a mouth to the lake or sea its sink
chain intersects. The record is truncated at the first sample inside that polygon, so the channel
never runs through standing water, and that sample's stored surface is set to the sink's water level —
which is Phase 7 step 2's requirement already satisfied in the record.

**Fords stay inert until a fordability query exists.** Step 18 asks the "preflight canonical
fordability query" to approve a swept crossing. No such query exists (`FORD_CAPABILITY_UNAVAILABLE`),
so `HybridHydrologyParams` carries the capability's two depth limits and the manager passes zero:
no window carries a ford flag today, and the one place to ask the query when it ships is where the
limits are compared.

**Cut metadata for a cliff window is the station itself.** Step 18 wants cliff stations to carry
"pass/tunnel/cut metadata". A cliff station is retained only when both approaches are legal and its
two endpoint elevations connect within `PRIMARY_2` grade, which is precisely the graded cut Phase 11
tries first, so `HYBRID_WINDOW_METHOD_CUT` is set on every retained cliff station and the cut's own
numbers are the `LowElevation` / `HighElevation` already on the record.

**The window gate is about *obstructed* corridors.** The Phase-6 gate requires a window wherever a
river, cliff, canyon or ravine intersects a required heavy corridor. A barrier that only clips the
edge of the corridor band still leaves a dry way past it, and Phase 5's own rejection rule
deliberately allows that (it preserves `4W` of dry width rather than forbidding the overlap). The
gate therefore demands a window only where the obstruction leaves no contiguous
`heavy_vehicle_width + 2*min_vehicle_lane_clearance` run clear of it at some station inside
`MaxCorridorHalfWidth(PRIMARY_2)` — the checklist's own wording, "every obstructed required corridor
has a window". A river is effectively always obstructing; a short cliff usually is not.

**Only preferred windows become reservations.** Every accepted window stamps `NO_DECORATION` and its
own `CROSSING_WINDOW` / `RAMP_PASS_WINDOW` flag over its approach. A window on a required heavy
corridor (or within `2S` of a Section-6.10 crossing guide) is additionally inserted as a
`HYBRID_RESERVE_CROSSING_WINDOW` reservation, which is the "required window" Phase 4's priority list
reserved that rank for; an optional window is a permission, not a reservation.

## Per-class approach capability (Q-023)

Each window records `ApproachClasses`, one bit per road class: the bit is set when that class's own roadbed
half width stays on legal ground over both straight approaches and, past each approach point, the road has a
legal way off at its own turn radius -- straight on for one radius, or a quarter turn of that radius either way
(the departure fan), swept at the roadbed width. Legal ground here is the exact `BASE_PROTECTED` and Tiberium
reserve discs Phase 10 refuses, not the coarse flag of the cell a sample falls in. The window's existence test is unchanged (`PRIMARY_2`, approach
only); the mask only tells Phase 9 which classes may be bound to it. It is part of the window checksum.
`Hybrid_Obstruction_Stations` and `Hybrid_Crossing_Ground_Is_Clear` are exported so Phase 11 measures its
stations on the obstruction and judges their ground by the same rule.

## What Phase 6 deliberately does not do

- It writes no heights, no water areas and no `WaterSystem` records. `WaterSystem` needs no interface
  change for this item: it already takes closed areas and flowing centerlines with per-station width
  and surface height (`WaterAreaClass` / `WaterStationClass`), which is exactly the shape of these
  records, so the adapter that hands them over belongs to the phase that realizes water.
- It creates no windows on lakes or coast (step 20): they are not bridgeable road obstacles in this
  generator version, and water that would block a mandatory heavy corridor is rejected at steps 13
  and 15 instead.
- It creates no ponds. `water.ponds` is parsed for compatibility and carries no quota (Section 5).

## Failure behavior

Every failure is a named attempt failure the manager retries with a new seed, and `out` is left
empty: `river_quota` (too few systems after the three permitted `0.80` threshold reductions),
`lake_quota` (a required basin survived no local retry), `coast_unplaceable`, `flow_not_acyclic`,
`water_in_reserve`, `corridor_blocked`, `no_crossing_window`, `record_incomplete`, plus
`invalid_input` and `scratch_exhausted` for a degenerate request or an exhausted arena.
