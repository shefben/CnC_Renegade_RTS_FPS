# Bridge Planning (V2 Phase 9)

Roadmap Section 34 Phase 9 -- "sectional bridge planning, generation, damage integration, and
rerouting hooks." Section 11. See `RoadGraph.md` for the unresolved-crossing lists this phase
consumes and `docs/zerohour/BridgeSystem.md` for the v1 system that turns a resolution into a
real bridge.

## Consume, resolve, realize through the one bridge system

`BridgePlanner` (`Code/Combat/bridgeplanner.h/.cpp`) never re-derives which roads need a
crossing -- every candidate comes from `RoadGraphGenerator::RoadGraphEdge::UnresolvedCrossingIds`
(Phase 8 step 15). `BridgeSystem` (complete before this roadmap started) already owns bridge
layout, collision, damage state and multiplayer state-block serialization; this generator calls
`Add_Bridge`/`Build_Layout`/`Build_Geometry`/`Set_Damage_State`/`Connect_Roads`, the same entry
points the console command and self check already use.

## Finding the banks

A crossing candidate names a point, not a span. `Find_Bridge_Banks` walks outward from that
point along the perpendicular of the nearest road's own local tangent (`RoadSplineClass::Get_
Point`'s own `Tangent`, not a guessed axis), testing at each step whether the obstruction is
still there -- `WaterSystem::Is_Point_In_Water` for a water crossing, `EscarpmentSystem::
Sample_Face` for an interior-cliff one, re-derived from the crossing point itself rather than
stored on the `StrategicNode`, since neither Phase 5 nor Phase 6 tags which generator a crossing
candidate came from. The walk is bounded (`BANK_SEARCH_MAX_STEPS`); a crossing whose obstruction
never clears within that bound is Phase 9 step 3's own "reject impossible bridges" -- the
crossing is left unresolved rather than forced.

## Deck height, for the case there is real data for

Section 11A.2's minimum-valid-elevation solver needs real inputs to be more than a formula. For
a water crossing, `WaterSystem::Get_Water_Height` gives one: `deck_height` (above the higher
bank) is `max(0, water_height + water_clearance_margin + deck_structural_depth -
higher_abutment)` -- the lowest elevation that actually clears the water, never more. For an
interior-cliff crossing, `deck_height` stays 0: the two banks a cliff crossing spans are already
the toe/rim heights the cliff itself was built at, so the deck following the straight line
between them is the natural case, not a gap. Section 11A.2's own fallback chain (shift the
crossing, lengthen approaches, lower the deck further, choose an alternate candidate) is not
implemented; a span this simpler check rejects is left unresolved rather than iteratively
repaired.

## Bridge kind, and initial damage state

`RoadGraphEdge::Class` maps to one of the three already-registered default bridge kinds --
`ow_bridge_timber`/`ow_bridge_concrete`/`ow_bridge_steel_truss`, already sized to v1's three road
classes (`BridgeSystem::Define_Default_Bridges`, called automatically by `BridgeSystem::Init`) --
so no new bridge definition is authored here. Initial damage state (Section 11.4) is one
deterministic hashed draw per bridge, weighted heavily toward `PRISTINE` (cumulative 70/85/95/100
across PRISTINE/DAMAGED/CRITICAL/BROKEN) -- fixed constants, since no profile field for damage-
state weighting exists and none was added; document rather than invent one.

## Connectivity, kept conservative

Step 10 says a broken initial bridge does not count as required connectivity "unless an
alternate route exists." Detecting a genuine alternate route needs the same route-independence
analysis (Section 17A.2) Phase 8 already deferred, so `ResolvedBridge::CountsAsConnectivity` is
simply `false` for every initially-`BROKEN` bridge, with no attempt to credit an alternate --
the conservative reading of "unless," not the permissive one.

## Nav and damage hooks are free

Step 11 asks to link runtime bridge damage state to nav invalidation. `WorldTraversabilityGraph`
(Section 52B Phase 65, already complete) already reads live `BridgeSystem` state
(`Is_Traversable`/`Get_Traversable_Extent`/`Crossing_Carries`) every time it is built, so a
bridge this generator places is respected by it with no new code in either file -- the same
"ask the one shared system" integration Phase 5 gets for free from the same graph.

## What is NOT resolved this phase

**Road-to-bridge handoff (step 6) is incomplete.** The road a bridge crosses under still runs
through as one continuous centre line from Phase 8 rather than being split and re-terminated at
the bridge's own two abutments. `RoadSystem` has no way to remove or split an already-added road
(the same add-or-clear limitation every shared collection in this tree has), and rebuilding
Phase 8's roads to pre-split at every crossing is real scope this phase does not take on. The
bridge itself is a real, correctly-placed, correctly-conformed object either way --
`BridgeClass::Conform_Point` answers for its own deck -- what is missing is a caller at that
exact point knowing to prefer it over the ground/water. `BridgeSystem::Connect_Roads` is still
called (correct, harmless, and forward-compatible), but finds nothing to bind given the current
road structure.

**AI Commander consumption of strategic importance (step 12)** is storage only --
`ResolvedBridge::StrategicImportance` is a coarse, by-road-class heuristic with no consumer yet,
the same "reserved, unconsumed" state several earlier fields in this chain were in before their
own consumer arrived.

## What is proven, and what is not

`terrain_bridgeplan`/`fds_terrain_bridgeplan` (`Code/Commando/terrainselfcheck.cpp`,
`-terrainselfcheck bridgeplan`) prove, device-less, against a real chain -- hydrology, a
strategic plan, and a road graph, all run for real over the same large synthetic uniform-slope
terrain earlier phases' own checks use: parameter refusal; every structural diagnostic (a
non-broken bridge is traversable, a broken one is not, the connectivity flag agrees with the
state) holds for whatever `BridgePlanner` actually resolves; every resolved bridge names a real
`BridgeSystem` id; and the same seed, terrain, plan and road graph reproduce the same checksum.

**Deliberately not asserted: that any bridge gets built at all.** Dijkstra routes around an
obstruction whenever a clear path exists, so a real hydrology feature carved into the same
terrain a road graph is planned over may simply be routed around rather than crossed -- this
check cannot honestly guarantee a crossing occurs the way `Check_Hydrology` guarantees a river
exists. What is checked holds for zero bridges or several. What is not proven, because nothing
here runs with a display: whether a real generated seed's own bridges look right once art
exists, and the same reachability gap every phase since 4 carries, now four generators deep --
this planner only runs when the road graph itself ran, which only ever happens when the
strategic plan ran, which only ever happens when macro terrain ran, which today is never, since
the one caller that exists always requests a `fit_to_level` profile.
