# Hybrid Phase 10 -- road centerlines

Runtime Phase 10 decides where every road in the hybrid generator actually runs. It realizes nothing on
the ground: its output is one accepted centerline per mandatory topology edge, the analysis-cell chain the
search produced, and one unresolved structural record per crossing.

Owner: `Code/wwphys/hybridroutes.{h,cpp}`, reached through
`ProceduralWorldManager::Build_Hybrid_Routes` and gated by `renegade -terrainselfcheck hybridroutes`.

## The search

Plan-exact. Edges are routed in the eight priority groups then by `StableId`. A search state is a
`(cell, incoming_direction)` pair, nine per cell, expanded in the order `E,NE,N,NW,W,SW,S,SE`. Each edge
gets three corridor envelopes -- preferred, `+S`, `+2S` -- and the envelope is a hard bound on the search,
not only a term in its cost. The cost is the plan's `Ks/Kt/Kc` table with the reuse modifier, the
collinear rules measured over the run lengths the plan states, and the `20A` penalty for entering a window
the edge is not obliged to use. The heuristic is `0.75*Euclidean`; states are ordered by
`(f,g,cell,dir)`; a state reopens only on a strictly lower quantized `g`. Expansions are capped at `8N`
per route and `R*8N` per attempt, and exceeding either refuses the attempt by name.

An edge is routed as a sequence of legs between *stops*: its two endpoint nodes, its frozen waypoints,
and -- for a crossing waypoint -- the two frozen approach points of its window, in the order the journey
reaches them. A leg whose two ends are one window's approach points is the crossing itself, and it is the
only leg allowed inside the obstruction.

## The centerline

**A road centerline is straight runs joined by circular arcs of the class minimum turn radius.** It is not
a spline through the chain's cell centres, and the reason is arithmetic rather than aesthetic: an 8-way
chain can only turn 45 degrees at a cell vertex, `A = clamp(chunk_size/4, 8, 16)` is 15 on a MEDIUM world,
and a `HIGHWAY_4` needs `8W = 26.6` of radius. Any curve through those controls has whatever curvature the
search's choice of cells implies. `DECISIONS.md` D-009 and its 2026-09-28 revision record the measurements
and the deviation from Section 8 Phase 10's "centripetal Catmull-Rom through those controls".

The construction:

1. **Corridor polyline.** The frozen endpoints, every cell where the chain changes direction, every frozen
   waypoint, every cell step 5 inserted. A crossing contributes its window's two frozen approach points
   and nothing between them; the interval between those two points is marked structural.
2. **Straight through every frozen point.** An arc that rounds a corner does not pass through the corner,
   and a frozen position -- a base entrance, a field access point, a crossing approach point -- must be
   interpolated exactly. So each one is given a lead-in and a lead-out along its own axis, which moves the
   corner onto open road either side of it. For a crossing that axis is the line between its two approach
   points, which is also what makes the crossing straight.
3. **Fillet.** Each remaining corner takes an arc of the class radius. A turn of `theta` consumes
   `R tan(theta/2)` of the straight on each side; where two corners cannot share the leg between them the
   shallower is dropped -- a corridor vertex, never a frozen one -- and the pass restarts, because dropping
   a vertex changes the angles at its neighbours. Two frozen corners crowding each other is reported as
   such, which points at the phase that froze them.
4. **Sample and validate.** The straights and arcs are sampled at `min(W,A/2)` and the whole validator
   runs: inside the current envelope, clear of every hard obstacle, at or above the exact Section-7 class
   radius, and within the class grade and cross slope through `RoadVerticalInterval`. An obstacle,
   envelope, grade or cross-slope refusal takes the plan's step-5 insertion of the nearest path cell; a
   curvature refusal raises the construction margin one step, which can only flatten the curve. Both are
   bounded by `retry.max_local_retries_per_stage`.

Arcs are built at `1.05 * Rmin` so the 0.001 position quantum cannot decide an acceptance. The limit
itself is never moved: `Check_Hybrid_Routes` measures the radius again, independently of the router, as
the circumradius over a class-radius baseline at every sample of open road, and allows no tolerance beyond
`Q`.

## Crossings

A crossing is **known from the leg that built it**, not deduced from the finished chain. Phase 10 routes
it as its own leg between the two frozen approach points and records a `HybridRouteStructuralSpan`: the
window, the ingress point, the egress point and the range of samples the straight interval covers. The
method -- bridge, overpass, bore or cut -- is deliberately not decided here; Phase 11 chooses it.

Inside such a span the road is not on ordinary ground, so grade, cross slope and curvature are not asked
of it, and standing water under it is legal: the two approach points stand on the two banks and every cell
between them is wet. A frozen crossing waypoint is discharged by the span, which is a stronger statement
than touching the cell the node's position falls in -- that cell is the middle of the obstruction, which
the road spans rather than drives through.

## Diagnostics

`Hybrid_Route_Census` in `hybridroutes.h` answers "what refuses this region?" with the same cached rules
the router uses: how much of the analysis grid each of the five refusal rules rejects, what a flood from
the leg's start actually reaches, what refuses the frontier it stopped at, and what refuses each of the
goal's own eight neighbours. Nothing in a normal build calls it.
