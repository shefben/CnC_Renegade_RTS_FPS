# Hybrid Phase 11 -- the interaction solver

Runtime Phase 11 decides what each crossing Phase 10 left unresolved actually is. It realizes nothing: no
`RoadSystem`, `BridgeSystem` or `TunnelSystem` object is created and no terrain is touched. The output is
one compiled solution record per conflict, which is the whole of what Phase 12 may know about a crossing.

Owner: `Code/wwphys/hybridinteractions.{h,cpp}`, reached through
`ProceduralWorldManager::Build_Hybrid_Interactions` and gated by
`renegade -terrainselfcheck hybridinteractions`.

## What it consumes

Phase 10 routes a crossing as a leg of its own between the two frozen approach points of a Phase-6 window
and records it as a `HybridRouteStructuralSpan`: the window, the ingress and egress points, and the sample
range of the straight interval between them, with the method deliberately undecided. Those spans are the
road-obstruction conflicts. Road-on-road conflicts are the geometric crossings of two centerlines, plus
every frozen Phase-9 `OVERPASS_OBLIGATION`.

Conflicts are processed by road edge `StableId`, then by increasing along-road distance.

## The method orders

Fixed, and never reordered to find a cheaper answer. The solver also never invents a method the window
does not already permit -- Phase 6 computed those permissions from the frozen preflight metrics -- and
never moves an obligation to a different obstruction.

| conflict | order |
| --- | --- |
| road x river | bridge, ford, one more Phase-10 envelope, fail |
| road x canyon/ravine | bridge, surface pass, tunnel, envelope, fail |
| road x cliff/ridge | road cut, ramp pass, tunnel, envelope, fail |
| road x road | at-grade junction, overpass, reroute the lower-priority road, fail |

A mandatory canyon or ravine **bridge** obligation skips both of its fallbacks: it compiles as a bridge or
the attempt fails. A reserved `OVERPASS_OBLIGATION` always compiles as an overpass and never downgrades to
an at-grade junction. `HIGHWAY_4` and `PRIMARY_2` never ford, and a ford never satisfies a bridge quota.

## The station search

`half_length = max(8W,4S)`, spacing `min(W,A/2)`, stations measured **on the obstruction itself**
(`Hybrid_Obstruction_Stations`), centred on the crossing the road made -- or, for a frozen obligation, on its
own waypoint. **Candidate 0 is always that anchor** (Q-022); the rest of the range follows, both ends
included, never more than 64 unique stations in all. The plan's literal "exactly 64 uniformly spaced
stations" has an even count and never samples the centre, so it could drop the road's own crossing from its
own candidates. `Hybrid_Crossing_Station_List` is exported so the gate holds the list to this rule.

At the anchor the road is the routed road, already straight across (Phase 10 owns that). Anywhere else the
road reaches the station by a **connector** (`Hybrid_Build_Crossing_Path`) built with Phase 10's own corner
construction: class-radius turns, then the complete straight approach on each side. The connector is
authoritative compiled geometry: it is recorded on the interaction (`Path`), `Hybrid_Realized_Route` splices
it into the road, Phase 12 realizes that road, and a crossing whose two ends are not attached to its realized
road fails the gate (`detached`).

A station is hard-valid only when its approach-reserve endpoints are inside placeable bounds, neither
reserve touches a base or Tiberium reserve the edge has no business in, its crossing path stays on legal
ground at legal cross slope, the whole realized road can be graded by Section 12.1 with this crossing's
structure in it, and it stands at least `4W` from an unrelated compiled crossing.

## Road meetings (Q-025)

A topology node that two or more realized roads carry -- as an end or as a waypoint -- is the junction the
topology made, apart from a heavy family's own consecutive trunk edges, which are one road. Phase 11 compiles one
at-grade junction record for it (`HybridInteraction::NodeId`) and does not rediscover it as an incidental
crossing. Its conditions are the ones the shared surface has to meet on every road arm there: outside every
compiled bridge or tunnel approach reserve, gradeable to each arm inside cut/fill and the road's own grade, and
the larger of the roads' cross-slope limits. The crossing-angle and both-sides arm rules are for two roads that
*cross*; a fork leaves its node on one heading by design, and a road that ends there has one side. A refused
topology junction asks the lowest-priority road there for its next envelope, then fails. The same two roads
crossing again beyond the junction's reach is an ordinary incidental crossing.

"Outside every bridge/tunnel approach reserve" is outside the straight road the structure owns: the segment from
its `ApproachA` to its `ApproachB` at that road's own roadbed width (`Inside_Approach_Reserve`). The test it
replaces used the reserve's *length* as a radius around the whole straight, and refused junctions well beyond a
bridge's approach.

Grading is per physical road (`Hybrid_Realized_Physical_Road`): a heavy family's trunk is graded once, as one road
with every structure on any of its logical edges, by Section 12.1. Judge_Crossing_Path, the overpass deck and the
gate all use it.

`HybridInteractionResult::RoadCrossings` records every meeting (shared-node, obligation, incidental) and, for a
refused one, its edges' kinds, the angle, the end nodes, and the junction and overpass refusals.

## Bridges

Metrics resolve through `Hybrid_Resolve_Bridge_Max_Span`: **the bridge definition's own value first,
`BridgePlannerParams::MaxSpanLength` second.** A definition that supplies no span of its own is not a
missing metric -- the planner value is the plan's named fallback source -- and only when neither supplies
one has preflight already refused the request. Kinds that share the planner-wide span have genuinely equal
span capability, and selection among them falls to the remaining suitability tests and then the lower
`StableId`.

A kind is rejected when its clear width is below Section 12.4, when its resolved maximum span cannot cover
the obstruction plus both abutment seats (each seat is the definition's own start/end cap section length,
from the bridge owner), or when its required approach does not fit: the reserve is
`max(8W, kind.RequiredApproachLength)`, measured on the road's own straight at the anchor, and it is never
shortened -- a kind that does not fit is refused and the next station or the next envelope is tried. The
shipped kinds need 30 against Phase 10's guaranteed `8W` of 26.6, so this is the ordinary case.

A river deck is level at its top; where the carved channel leaves the bank below the road's reach, the deck
extends along its axis until the ground is within `max_fill_height` of it, inside the straight approach,
and the kind is re-selected for the longer deck. A canyon or ravine deck is carried between the two road
profiles at its ends, solved with the rest of the road.

Grading is always the **whole realized road**: Section 12.1 over the road with its connectors spliced in and
every structure already compiled on it holding its own surface. A pinned height is a height the solver
itself produced and quantized, so propagation treats it as its quantization cell and returns the pin
exactly; a deck graded at the class maximum is therefore re-solvable from its own compiled ends. Among those
that remain: smallest sufficient maximum span, then lower `StableId`. The candidate score is
`4*span + 2*elevation + 3*grade + 2*curve + detour`, lowest wins, ties to the earlier station.

Section 12.4's clear width is a question about **what carries the vehicle**, not about the marked
carriageway: a deck is widened to `max(carriageway, swept + 2*clearance)` and both approaches blend up to
it. A method that leaves the vehicle on graded ground -- a ford, a pass, a cut -- asks the same question of
its own roadbed instead.

## Overpasses

The higher-priority road stays lower; a tie on class keeps the lower `StableId` underneath; exactly one
swap is permitted after that, and then the lower-priority road is asked to reroute. Clearance is Section
12.3's `heavy_vehicle_height + max(0.35 * world_units_per_meter, 0.10 * heavy_vehicle_height)`, plus the
deck's structural depth.

The ramp reservation is

```text
L_ramp = max(10W, 1.5 * abs(delta_z) / max(tan(upper_class.max_grade),Q))
```

and the `1.5` is not a fudge: it is the exact peak-slope ratio of a smoothstep, which is steepest at its
midpoint. Without it a ramp sized by `abs(delta_z)/tan(g)` averages `g` and peaks at `1.5 g`, and the
plan's own per-sample grade requirement refuses the shape the same paragraph asks for. The limit does not
move; the reservation does. See `DECISIONS.md` D-010.

The compiled profile runs from the road's own elevation at each ramp foot up to the deck through one
smoothstep, sampled at `min(W/3,A/4)`. It does not follow the terrain, which would put the ground's own
slope into the road's grade. `L_ramp` is a minimum, so the recorded length is the quantum at or above it.

The deck is a registered bridge kind -- the overpass takes its structural depth and approach from it, and a
road x road crossing no registered kind can carry is `method_unavailable`, never an invented structure. The
deck is straight, so the upper road must run straight under all of it (`w_lower / sin(theta) + w_deck *
|cot(theta)|` along the upper road). The ramps run along the upper road's centerline beyond the deck and may
follow its curves. The upper road is then graded whole, as for a bridge: its realized centerline with every
bridge and tunnel of pass one and every overpass already on it, and this deck held level at its top --
grading the ramps alone left their feet free and accepted decks the road beyond could not climb to.

### The overpass count (Q-024)

Every overpass Phase 9 reserved compiles as its own overpass or follows the swap / reroute / fail order; the
quota check is `compiled reserved overpasses == HybridTopology::ReservedOverpassCount`, not the raw Section-5
table.

## Ground methods: passes and cuts

The three ground methods -- `surface_pass`, `ramp_pass`, `road_cut` -- share one owner for their
geometry, `Code/wwphys/hybridlocalpass.{h,cpp}`, and one rule about what is judged: a candidate is planned
road geometry (D-012), so the road's own profile must hold the class grade, its corners the class turn
radius as Phase 10 fillets them (D-009), and every sample of it must be realizable inside its
`RoadVerticalInterval`. Cross slope is Section 12.2's: zero on the roadbed, so the test is whether a level
roadbed at the profile height is realizable, banking up to the class maximum. Phase 6 proves a candidate
against the provisional height and freezes it on `HybridCrossingWindow::Pass`; Phase 11 revalidates that
frozen record on the realized terrain at `min(W/2, A/3)` and compiles its ends as the crossing's own.

**Canyon/ravine floor pass** -- descend the near wall on the diagonal, cross the floor, ascend the far
wall, the two ramps leaning opposite ways so the route returns across the station. Each ramp's
along-obstruction run is solved, not estimated, against the floor its foot actually reaches, and each is
charged to its own direction's arc length along the obstruction.

**Cliff band pass** -- D-004's cliff is a raised band with both approaches on one bench, so the pass is the
floor pass inverted: ascend the near talus face, run along the shelf, descend the far face. Both ramps and
the traverse lean the *same* way (D-015): a native face is a few lane widths wide and 18..36 high, so at the
class grade a ramp runs nearly parallel to the face, and opposite leans would meet the shelf at close to
180 degrees -- a hairpin. The crossing therefore exits some distance along the cliff, bounded by the cliff's
directional arc length; the far approach is frozen with the geometry.

**Road cut** -- the straight profile between the two approach elevations, accepted only when the plan's
four conditions hold. Phase 6 measures it per road class (`CutClasses`); Phase 11 tries a cut only for the
edge's own class and revalidates.

### Reachability on the native configuration (D-015, Q-020, Q-021)

None of the three is reachable on a native world, and each unreachability is arithmetic rather than a
missing seed:

| method | why not native | executable fixture |
| --- | --- | --- |
| `road_cut` | a straight profile over a same-bench band needs `H <= max_cut + max_fill = 12`; every native cliff is `tier_step * 1..2 = 18..36` | 4-high band, cut first in the order, 4.00 of cut, four conditions recomputed |
| `surface_pass` | the widest native canyon spans `22W = 73` and the registered bridge spans 200, so `Compile_Bridge` never refuses for a reason the pass does not share | 80-unit canyon, fixture bridge kind of 30 span refused `span_too_long`, pass compiled; native-sized registry takes the bridge |
| `ramp_pass` | reachable since D-016 widened the talus to `1W..5.25W`: the builder accepts a `tier_step`-high band from `4.0W` of talus (`H / F <= 1.14`), and the census finds one pass-capable native cliff window in 408 cliffs; a *routed* one is expected at the order of thousands of seeds and has not yet been found | 12-high band over a 16.6 face, no cut planned, band ramp compiled, exits 110.9 along the cliff |

Under the original `1W..3W` talus every native explicit cliff was a sealed wall (0 cliff windows in 135
census worlds); `Q-021` (D-016) widened the range so that shallow, broad cliffs can carry a ramp while
tall or narrow ones remain walls. `HybridInteraction::FallbackFrom`
records the first method's refusal when a later one compiled, so the fixed orders are provable from the
record.

## Highway access interchanges (Plan 12.6.10)

Phase 11 compiles each interchange Phase 10 realized (active, not withdrawn) from its frozen records. It never
rediscovers these as road-road intersections, and never withdraws the interchange.

- **Edge ends.** Phase 11 reads edge ends through `Hybrid_Route_Edge_End`. A retargeted arrival ends at its local
  junction while the interchange is active, and at `X` once it is withdrawn, so a withdrawn interchange's fallback
  junction at `X` compiles as an ordinary shared-node junction.
- **No junction at `X`.** A ramp's `NodeB` names `X`, but the ramp ends at its gore, so ramps are not arms at `X`.
  On a `SIDE_ACCESS` highway only the trunk's own two edges remain there, and they are one road.
- **Local junctions.** Each `L_s` is a topology junction compiled by the existing construction with the role
  `HIGHWAY_RAMP_LOCAL_JUNCTION`. Its `JunctionZ` is `z_L`.
- **DIAMOND_ACCESS crossing.**
  - Grade-separated: the interchange's own reserved overpass. It is mandatory, the highway is the lower road, and
    its two edges at `X` are passed as one physical trunk. It is not counted against Section 5's reserved overpasses.
  - At grade: the intended junction at `X`.
- **Priority** follows the routing groups (`Hybrid_Route_Priority_Rank`: trunks, then group 1b, then the rest). A
  constructed ramp or crossing is never the rerouted road. A rule that would reroute one is exhausted, and the
  phase fails (`HYBRID_INTERACTION_INTERCHANGE`).
- **Structure reserves.**
  - The common station tests add test 9: no approach reserve meets an active gore reserve.
  - Every overpass assignment, including the swap, checks its ramp interval against the gore reserves
    (`HYBRID_PROBLEM_GORE_RESERVE`).
  - The gore reserve (`Hybrid_Gore_Reserve_Extent`) runs along the side frame and laterally from the highway's far
    roadbed edge to `E_aux`. It also covers the ramp's roadbed over its `2W` gore shoulder transition.
- **Incidental at-grade junctions** are legal only outside every footprint (`Hybrid_Interchange_Footprint`, shared
  with Phase 10) expanded by `RoadOuterBlendWidth` (`HYBRID_PROBLEM_INTERCHANGE_FOOTPRINT`).
- **Elevations.** These are compiled after every crossing on the roads involved:
  - The highway is graded whole by Section 12.1 with its structures.
  - `z_L` for `DIAMOND_ACCESS` is the crossing's own profile at `L_s`. For `SIDE_ACCESS` it is the
    lower-`StableId` ramp of that side, solved with its gore pinned to the highway.
  - Every ramp is judged whole, from `z_L` to the highway at its gore.
- **Merge/diverge records.** There is one `HIGHWAY_RAMP_MERGE` or `DIVERGE` record per ramp, at its gore, with
  `JunctionZ` set to the highway surface there. Each has a `HybridMergeDiverge` sample set at `min(W/3, A/4)` over
  its interval, in the interval's own frame (chain stations relative to `X`): `(station, full, u, d, h, E, z)`.
- **Swept proxy.** This is the `Sweep_Is_Clear` proxy, run over ramps, parallel intervals and local-junction arms
  with the vehicles the ramp's capabilities require. The swept envelope must stand on placeable ground, and the
  class cross slope must hold across the roadbed.
- **Gate clause.** Each realized interchange must have:
  - one merge or diverge record and one sample set per ramp;
  - one local-junction record per `L_s`;
  - its `DIAMOND_ACCESS` crossing compiled;
  - no incidental solution between its own elements and its own highway.

  In addition, no bridge, tunnel or overpass reserve may intersect an active gore reserve. The stage-11 checksum
  includes `JunctionZ` and the merge/diverge records.

## Failure

Every unsolved conflict becomes a `HybridInteractionIssue` naming the problem, and more than twelve on one
edge fails the phase. The reported reason is the **furthest stage any station reached** -- a station that
passed every common test and then failed on the asset is a more useful diagnosis than one that never got
that far -- and a fallback attempt never overwrites the primary method's reason. A refused phase leaves no
compiled solution behind; the issue records are kept, because they are what a refused attempt is read from.

Phase 11 is the last phase allowed to change a crossing's method or location, or to spend a remaining
Phase-10 corridor envelope.
