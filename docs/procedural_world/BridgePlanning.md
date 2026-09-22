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
point along the nearest road's own local tangent (`RoadSplineClass::Get_Point`'s own `Tangent`,
not a guessed axis) -- the direction the road is actually travelling through the obstruction,
not sideways to it -- testing at each step whether the obstruction is still there --
`WaterSystem::Is_Point_In_Water` for a water crossing, `EscarpmentSystem::Sample_Face` for an
interior-cliff one, re-derived from the crossing point itself rather than stored on the
`StrategicNode`, since neither Phase 5 nor Phase 6 tags which generator a crossing candidate came
from. The walk is bounded (`BANK_SEARCH_MAX_STEPS`); a crossing whose obstruction never clears
within that bound is Phase 9 step 3's own "reject impossible bridges" -- the crossing is left
unresolved rather than forced.

**V2 Phase 10 correction.** This walked the *perpendicular* to the road's tangent until Phase 10,
not the tangent itself -- an obstruction a road meets roughly head-on is narrow along the road's
own direction of travel and typically runs parallel to it for a long way in the perpendicular
direction (a riverbank, a cliff face), so the old search usually found two banks off to the side
of the road rather than ahead of and behind the crossing. Found and fixed while building Phase
10's own bridge-required test: a road cut computed from those two off-to-the-side points landed
at the same distance along the road for both, which `RoadSystem::Trim_Road_Range` correctly
refuses (`distance_b > distance_a` never held). No caller depended on the old direction --
`Check_Bridge_Plan`'s own assertions never looked at `Get_Start`/`Get_End`'s relationship to the
road -- confirmed by running the full `-terrainselfcheck` suite with the fix staged and with it
stashed out and finding the same 11 pre-existing, unrelated failures both times.

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

**Road-to-bridge handoff (step 6) closed in V2 Phase 10.** `RoadSystem::Trim_Road_Range` now
splits a road at two distances along its own tessellated centre line, reusing the original id for
whichever side survives on the low end and a new id for the other, and repointing any junction
arm that referenced the original id. `BridgePlanner::Generate` calls it once per resolved bridge
against whichever segment is still live for that edge, sets `ROAD_ENDPOINT_BRIDGE` and the cut
position/direction on both new segments' `RoadConnectionClass`, and adopts a new segment's id
into `RoadGraphGenerator::OwnedRoadIds` so `Clear` sweeps it up correctly too.
`BridgeSystem::Connect_Roads` -- already written, previously given nothing to bind -- is now
called with a tolerance sized to `Find_Bridge_Banks`' own bank search rather than its 8-unit
default, since that search can legitimately place an abutment well away from where a straight
road cut lands, and it now finds a real match. See `completed_features.md`, "V2 Phase 10", and
`docs/procedural_world/RoadGraph.md`.

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

**Deliberately not asserted here: that any bridge gets built at all.** Dijkstra routes around an
obstruction whenever a clear path exists, so a real hydrology feature carved into the same
terrain a road graph is planned over may simply be routed around rather than crossed -- this
check cannot honestly guarantee a crossing occurs the way `Check_Hydrology` guarantees a river
exists. What is checked holds for zero bridges or several. `terrain_bridgerequired`
(`-terrainselfcheck bridgerequired`), added in V2 Phase 10, is the check built specifically to
guarantee a crossing: two base candidates separated by a river that spans a bounded terrain's
full width, so routing around it is not possible, asserting bridge count >= 1 and both abutments
bound to a real road. What is still not proven, because nothing here runs with a display:
whether a real generated seed's own bridges look right once art exists. The reachability gap
every phase since 4 carried -- `fit_to_level: true` skipping macro terrain and therefore every
generator downstream of it, since that is the only profile the "Procedural Skirmish" button has
ever requested -- is closed by V2 Phase 10's fitted-terrain sampling; see `completed_features.md`
and `GeneratorDeterminism.md`.
