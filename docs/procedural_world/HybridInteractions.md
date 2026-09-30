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

`half_length = max(8W,4S)`, spacing `min(W,A/2)`, stations marching along the obstruction, centred on the
crossing the road made -- or, for a frozen obligation, on its own waypoint, because the obligation does not
move. More than 64 stations collapses to exactly 64 uniformly spaced ones including both endpoints. Every
station is quantized to `Q` and deduplicated before scoring.

A station is hard-valid only when the road can actually reach it on its frozen centerline and cross the
obstruction rather than run along it, both approach-reserve endpoints are inside placeable bounds, neither
reserve touches a base or Tiberium reserve the edge has no business in, the approaches are gradeable to the
station inside the class maximum grade, and it stands at least `4W` from an unrelated compiled crossing.

## Bridges

Metrics resolve through `Hybrid_Resolve_Bridge_Max_Span`: **the bridge definition's own value first,
`BridgePlannerParams::MaxSpanLength` second.** A definition that supplies no span of its own is not a
missing metric -- the planner value is the plan's named fallback source -- and only when neither supplies
one has preflight already refused the request. Kinds that share the planner-wide span have genuinely equal
span capability, and selection among them falls to the remaining suitability tests and then the lower
`StableId`.

A kind is rejected when its clear width is below Section 12.4, when its resolved maximum span cannot cover
the obstruction plus both abutment seats, or when its required approach reserve does not fit. Among those
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
slope into the road's grade.

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

## Failure

Every unsolved conflict becomes a `HybridInteractionIssue` naming the problem, and more than twelve on one
edge fails the phase. The reported reason is the **furthest stage any station reached** -- a station that
passed every common test and then failed on the asset is a more useful diagnosis than one that never got
that far -- and a fallback attempt never overwrites the primary method's reason. A refused phase leaves no
compiled solution behind; the issue records are kept, because they are what a refused attempt is read from.

Phase 11 is the last phase allowed to change a crossing's method or location, or to spend a remaining
Phase-10 corridor envelope.
