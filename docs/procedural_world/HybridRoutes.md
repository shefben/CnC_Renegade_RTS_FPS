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
   a vertex changes the angles at its neighbours. Dropping a vertex beside a lead, or beside a frozen point
   (whose need for a lead was judged against that vertex), lays the leads again. Two frozen corners crowding
   each other is reported as such, which points at the phase that froze them.
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

**Phase 10 owns the arrival and the departure (Q-022, revised D-009).** The road enters and leaves every frozen crossing along
the window's own axis, `normalize(approach_b - approach_a)`, straight from one approach point to the other,
with every class-radius turn finished outside them. The search is told: A* arrives at the ingress cell and
departs the egress cell on the axis, judged at move generation with the run the class's turn still needs;
the two approach points are frozen vertices with lead-ins and lead-outs along the axis; a corner that would
have to sit on a frozen point, or crowd it, refuses the candidate. Phase 10 guarantees `8W` of straight
approach on each side -- the asset-specific remainder is Phase 11's to find or refuse. The structural sequence is symmetric -- ordinary road, class-radius arrival
transition, straight approach, structure, straight departure, class-radius departure transition, ordinary road --
and one owner judges both sides. The corner construction lays the lead-in and the lead-out by the same rule
(`Lead_Before`, along the axis and against it). The search applies one transition-room measure
(`Transition_Room`: `R tan(phi/2) + R tan(turn/2)` plus the kept straight) to the arrival (`Arrival_Fits_Crossing`)
and to the departure (`Departure_Fits_Crossing`). Here `phi` is the turn between the axis and the chain's grid
heading, and `turn` is the chain's own next turn. Without the departure side, the search turned in the cell after
the egress and the lead-out was refused in every envelope. Leads are never cached: every centerline build lays
them afresh from the current corridor vertices, and a dropped vertex beside a lead or a frozen point lays them
again, so the gate validates only the final rebuilt geometry. A road that cannot
line up in an envelope fails that envelope; after the third the edge fails. Nothing is left at a crossing for
Phase 11 to straighten. The old D-009 behaviour -- keep the waypoint and leave a corner there for Phase 11 --
is revoked.

A crossing's own span exempts only its own samples from the open-road checks, and the open road between
spans is graded by Section 12.1's interval propagation, so a road that touches its crossings but cannot be
graded between them is refused here rather than in Phase 11.

**A road crosses an obstruction only as a crossing (2026-10-02).** A river is 2.5 to 6 units wide and an
analysis cell 8 to 16, so a river runs between cell centres almost everywhere, and the hard-infinite cell list --
which judges cells by their centres -- did not see it: the search stepped across rivers between two dry centres,
and the centerline's own point-and-cell test (a sample is refused only when its cell is refused too) let the road
follow. So the moves and the centerline steps are judged as continuous segments (`Segment_Meets_River`). A search
move (`Move_Crosses_Channel`, cached per cell and direction, since the river geometry is fixed) that meets a river
must meet it inside a window the leg may use. A centerline step may meet a river only within the road's own accepted
crossing stretch: inside one of its structural spans, or stepping into or out of one at a point that span's own
window covers (`Window_Covers`). A road
crosses its planned window once, on the leg that is that crossing. The legs that approach or leave the window find its obstruction closed
(`RouteBuilder::ClosedWindow`): left open, a leg could reach the near bank from the far one by crossing the same
window outside its frozen interval -- a crossing nothing recorded and nothing would build. A window the search
was never sent to stays usable at the plan's `20A` window penalty, and such a crossing is recorded like a planned
one (`Note_Unplanned_Crossings`, which asks the chain's own moves whether they meet a river inside the window's
footprint -- the same segment test the search used to allow them; comparing the run's end cells about the window's
centre had missed crossings near the footprint's ends, leaving water crossed with no structure), and aligned like
one by `Phase10NaturalCrossingAlignmentRepair` (Plan Section 8
Phase 10, authorized in Section 17). When the centerline is refused at a corner that involves such a crossing
(`Unplanned_Crossing_At_Fault`, exact involvement only), the window's existing frozen approach pair becomes two
constrained stops of the leg it happened on, in travel order, and the logical edge is searched again from its start
in the same envelope. The failing corner is the earliest along the road, so crossings are repaired one at a time in
route order. If a promoted crossing is still the one refused, its stops are withdrawn and the window is unavailable
to that edge for the rest of the envelope (`RouteBuilder::ForbiddenWindows`, set per edge before its legs are
searched). Each promotion and each withdrawal consumes one local retry of the edge's envelope. The budget is
`retry.max_local_retries_per_stage`, shared with the centerline's step-5 insertions: a rebuilt centerline gets only
what is left. With the budget spent the edge advances to its next envelope, which starts with no promotion, no
unavailable window and a fresh budget. The repair never moves a window, invents a station, changes an obstruction or
method, or widens the corridor outside the envelope sequence. An unplanned crossing the centerline can align as it
stands is left alone. `Check_Hybrid_Crossing_Discipline` pins both: a road whose two ends stand on one bank
of a river with one window is refused by the search, and a road that must cross a window it was not sent to
crosses it straight between the approach points and records it once, and a road across a 3-unit river whose
centerline runs midway between two columns of cell centres, with no window, is refused by the search.

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
concatenated chain -- an ordinary waypoint is not the end of one road and the start of another.

**Ordinary guides are strategic anchors (2026-10-02).** An ordinary family waypoint, including a trunk joint
between two logical edges, constrains the search: the A* chain must pass through the guide's analysis cell, and
the Phase-10 gate checks that on the chain. The final class-radius centerline need not intersect the guide's
cell; a sharp family turn at a guide is rounded at the class radius wherever that puts the arc. The guide still
gives the corridor polyline a corner (`RouteVertex::Guide`) -- at a trunk joint it stands on the waypoint's own
frozen position, which Phases 3 and 9 made legal ground -- held against ordinary corridor corners, but dropped
when a structure's lead needs its room. Released altogether, the corner fell back to chain cell centres inside
the field reserves guides stand beside, and ordinary fillet crowding dropped it: in the 1400-world census
Phase 10's yield fell from 93 to 54. Held absolutely, it refused a crossing's lead-in that the terrain and the
Q-023 connector both allow. Phase 9's Q-023 connector (`Hybrid_Road_Poses_Connect`) builds the same guide corner. Structural waypoints stay exact: a crossing obligation, an overpass obligation, a
base entrance or field access point where the corridor ends exactly, a ramp pass window (and later a highway
ramp's gore, merge and diverge points) keeps its position, axis, straight approach and departure reserve and
turn-radius transition (Q-022, Q-023).

**Logical slicing.** The logical edges are slices of the one centerline, bookkeeping only. The boundary at an
ordinary guide is the point of the centerline nearest the centre `G` of the guide's analysis cell: the sample
nearest `G` after the previous boundary (quantized distance, ties to the earlier sample), then the exact closest
point to `G` on the segment before or after it, clamped and quantized to `Q`. A boundary inside a structural
interval moves to that interval's nearer end; boundaries are strictly ordered and no edge has zero length. The
boundary point is shared by the two slices and lies on the polyline, so slicing never changes the road.

A failure is charged to the logical edge it happened on -- a leg with no path to the edge that leg belongs to, a
centerline failure to the edge whose chain lies nearest the failing point -- and only that edge moves to its
next envelope; after its third the edge fails. A Phase-11 reroute raises only that edge's floor, and the whole
trunk is laid again in one piece. Each logical edge's `HybridRoute` is the exact slice of the one centerline its
interval covers, so consecutive slices share their boundary point. `HybridRouteSet::Trunks` holds the physical road
(`HybridTrunkRoad`: family, class, capability, logical edges and their intervals, node sequence, junction nodes,
centerline, crossing intervals); `Find_Trunk` maps a logical edge to it.

## Highway access interchanges (Plan 12.6.9)

Phase 10 realizes each interchange Phase 9 froze, or withdraws it. It never moves a gore, swaps a side or picks
another candidate.

- **Alignment.** A `HIGHWAY_4` trunk through an active anchor `X` gets three stops: `a_I = X - a*S_I`, `X`, and
  `b_I = X + a*S_I` (`stop_align`). The legs outside them arrive on and leave along `a`. The legs between them form
  one aligned record: the chain cells are replaced by the straight `a_I -> b_I` exactly as a crossing's are. The
  record is open road, not a structural span, so every sample is validated.
- **Group 1b** (`Construct_Interchanges`). After all group-1 roads are routed, every active interchange is built
  in processing order: the crossing edge or edges first, then the ramps (`RIGHT` then `LEFT`, `OFF` then `ON`), by
  `Hybrid_Ramp_Centerline` on the routed highway.
- **Re-validation.** Before anything is added, Phase 10 re-checks:
  - the highway runs straight through `X` over `+-S_I`;
  - each ramp ends exactly at its frozen gore;
  - every sample and both asymmetric roadbed edges stand on legal ground, with no river step and no wall;
  - the class grade holds;
  - no routed group-1 roadbed is met. The road's own highway may be met only inside the ramp's converged gore
    wedge, or by the crossing at `X`.

  Ramp and crossing routes have priority `INTERCHANGE`, and the highway's logical routes carry their widened
  intervals.
- **Footprint.** Each realized side adds its footprint polygon: the diverge interval's lane, the `OFF` ramp, `L_s`,
  the `ON` ramp, the merge interval's lane, and back along the highway. It is hard-infinite for every road routed
  after group 1b (`HYBRID_REFUSE_FOOTPRINT`). A road's own `L_s` is its start or goal cell, so it is exempt.
- **Withdrawal** (the only fallback). Either path restores the frozen fallback variant, recorded in
  `HybridRouteSet::WithdrawnInterchanges` and the stage-10 checksum:
  - (a) A highway edge carrying alignment fails its third envelope: the interchange at its ends that is later in
    processing order is withdrawn, and the road is routed again from its first envelope.
  - (b) A group-1b element fails re-validation.

  `Hybrid_Route_Edge_End` gives the effective endpoints: retargeted arrivals end at `X` again, and the interchange's
  ramps and crossings are inactive.
- **Gate** (`Check_Interchanges_Gate`). Every active interchange is checked:
  - the highway is aligned;
  - every crossing edge is routed;
  - every ramp runs from `L_s` to its frozen gore, meets it on the highway tangent within 0.001 rad, and never turns
    under `R_r` (measured over the validator's `R_r` baseline);
  - no road routed after group 1b enters a footprint outside its end cells.

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
transition, quantization, real minimum radius, or a ground rule). A structural transition carries its exact cause
(`HybridStructuralCause`): an unplanned crossing at fault; the chain crossing a planned window's obstruction
outside its crossing (`far_bank_approach` -- the road reaches the approach from the far bank, which Q-023's
empty-plane facing test cannot see); two structures, or a structure and a road end, too close
(`sequence_too_close`); the road meeting a frozen point from behind (`wrong_side`); the ideal straight `2R`
transition beyond the structure standing on hard ground, leaving the envelope, or breaking the grade or cross
slope; Q-023's empty-plane connector refusing what Phase 9 accepted (`q023_phase10_mismatch`); or, all of those
legal, the searched chain leaving the lead before the approach (`lead_before_approach`) or the departure after
the structure (`departure_after_structure`) no room.

## Diagnostics

`Hybrid_Route_Census` in `hybridroutes.h` answers "what refuses this region?" with the same cached rules
the router uses: how much of the analysis grid each of the five refusal rules rejects, what a flood from
the leg's start actually reaches, what refuses the frontier it stopped at, and what refuses each of the
goal's own eight neighbours. Nothing in a normal build calls it.
