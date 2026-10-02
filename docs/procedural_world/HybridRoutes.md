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

**Phase 10 owns the arrival (Q-022, revised D-009).** The road enters and leaves every frozen crossing along
the window's own axis, `normalize(approach_b - approach_a)`, straight from one approach point to the other,
with every class-radius turn finished outside them. The search is told: A* arrives at the ingress cell and
departs the egress cell on the axis, judged at move generation with the run the class's turn still needs;
the two approach points are frozen vertices with lead-ins and lead-outs along the axis; a corner that would
have to sit on a frozen point, or crowd it, refuses the candidate. Phase 10 guarantees `8W` of straight
approach on each side -- the asset-specific remainder is Phase 11's to find or refuse. A road that cannot
line up in an envelope fails that envelope; after the third the edge fails. Nothing is left at a crossing for
Phase 11 to straighten. The old D-009 behaviour -- keep the waypoint and leave a corner there for Phase 11 --
is revoked.

A crossing's own span exempts only its own samples from the open-road checks, and the open road between
spans is graded by Section 12.1's interval propagation, so a road that touches its crossings but cannot be
graded between them is refused here rather than in Phase 11.

Overpass obligations are routed through their exact node along each edge's intent segment, straight for
the deck's half length (`Hybrid_Overpass_Half_Deck`, the topology owner's measure) either side.

`terrainselfcheck`'s controlled crossing worlds (`Check_Hybrid_Crossing_Alignment`) pin this: a road already
on the axis is accepted unbent; a sideways arrival turns on its own bank and enters aligned; an arrival from
past the approach end swings round or is refused; and a crossing whose straight leaves no room for the turn
after it is refused in every envelope while the same crossing with room routes.

## One physical trunk per heavy family (Q-025)

A heavy route family is one road. Its logical topology edges -- base entrance to waypoint to waypoint to base
entrance -- keep their own `StableId`s, envelopes, Phase-11 envelope floors and route records, but Phase 10
routes them as one physical road (`Route_Road`): the edges in travel order, each with its own envelope and
expansion budget, the heading a leg ends on offered to the next across the ordinary family waypoint between
them, and a leg arriving at that waypoint first asked to arrive toward the next edge's first stop. Each falls
back to the existing free search when it leads nowhere. The centerline is built once over the whole
concatenated chain, so an ordinary waypoint is a pinned corner the class-radius arc rounds (standing on the
waypoint's own frozen position, its arc required to stay in the waypoint's cell) -- not the end of one road and
the start of another. Structural waypoints keep their exact Q-022 pose.

A failure is charged to the logical edge it happened on -- a leg with no path to the edge that leg belongs to, a
centerline failure to the edge whose chain lies nearest the failing point -- and only that edge moves to its
next envelope; after its third the edge fails. A Phase-11 reroute raises only that edge's floor, and the whole
trunk is laid again in one piece. Each logical edge's `HybridRoute` is the exact slice of the one centerline its
interval covers, so consecutive slices share their end sample. `HybridRouteSet::Trunks` holds the physical road
(`HybridTrunkRoad`: family, class, capability, logical edges and their intervals, node sequence, junction nodes,
centerline, crossing intervals); `Find_Trunk` maps a logical edge to it.

## Independent roads keep their roadbeds apart (Q-025)

The `-0.25d` reuse modifier is revoked: no road gets a discount for taking another's corridor. Instead, every
independently realized road obeys one rule against every road already routed: it may not run within 30 degrees
of it, closer than `ParallelCenterlineClearance(A,B) = 0.5*RoadbedWidth(A) + 0.5*RoadbedWidth(B) +
fairness.min_vehicle_lane_clearance`, for a continuous `6W`. The only exemption is the junction a real shared
topology node owns, `JunctionReach(class) = max(4W, class minimum turn radius)` along each road from the node.
`First_Parallel_Overlap` enforces it exactly on the centerline. The A* applies it conservatively
(`Move_Runs_Parallel`): half a cell of extra clearance, one cell less run, half a cell less exemption, and the
angle judged over the whole run's displacement, so the search does not hand the centerline check a road the
check then refuses. The existing heavy-family and optional-link collinear rules are unchanged.

## Failure census

With `HybridRouteParams::CollectCensus` set (the `hybridinteractionsearch` self-check sets it), a refused attempt
reports its failed logical edge in `HybridRouteResult::Census`. That record holds the owning reason (`HybridRouteBlock`:
envelope, world bounds, standing water, river or wall outside a window, base, Tiberium, grade, cross slope,
turn/approach pose, an existing road, the expansion budget, or the centerline). It also holds the no-path frontier
counted by rule, the class, role, envelope, structural-waypoint count, the failing leg and its end nodes, and for
a centerline refusal its geometric condition (`HybridSplineFailureClass`: control/fillet construction, structural
transition, ordinary-guide boundary, quantization, real minimum radius, or a ground rule).

## Diagnostics

`Hybrid_Route_Census` in `hybridroutes.h` answers "what refuses this region?" with the same cached rules
the router uses: how much of the analysis grid each of the five refusal rules rejects, what a flood from
the leg's start actually reaches, what refuses the frontier it stopped at, and what refuses each of the
goal's own eight neighbours. Nothing in a normal build calls it.
