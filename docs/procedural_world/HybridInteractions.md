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

## Failure

Every unsolved conflict becomes a `HybridInteractionIssue` naming the problem, and more than twelve on one
edge fails the phase. The reported reason is the **furthest stage any station reached** -- a station that
passed every common test and then failed on the asset is a more useful diagnosis than one that never got
that far -- and a fallback attempt never overwrites the primary method's reason. A refused phase leaves no
compiled solution behind; the issue records are kept, because they are what a refused attempt is read from.

Phase 11 is the last phase allowed to change a crossing's method or location, or to spend a remaining
Phase-10 corridor envelope.
