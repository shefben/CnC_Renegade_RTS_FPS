# Strategic Planning (V2 Phase 7)

Roadmap Section 34 Phase 7 -- "candidate bases, strategic anchors, POIs, and resource-site
planning." Sections 15.1, 16.1, 16.2, 17. See `Hydrology.md` and `InteriorCliffs.md` for the two
generators this phase reads candidates back out of rather than re-deriving.

## Candidates, not a choice

Phase 7 step 12 is explicit: "do not choose final bases yet." `StrategicPlanner`
(`Code/Combat/strategicplanner.h/.cpp`) produces exactly that -- a scored, deterministic pool of
possibilities for a later phase to accept, reject or combine. `WorldGenerationManifest::
SelectedBaseCandidateIds` and its four siblings are named "Selected" and stay empty from this
phase; nothing here selects anything. `StrategicPlanner`'s own node list is where a later phase
reads candidates from.

## Reuse over rediscovery

Phase 5's interior cliff bypass candidates and Phase 6's hydrology crossing candidates already
identified the barrier points a road/tunnel planner will need to resolve. `StrategicPlanner`
folds both straight into its own node graph -- an interior cliff feature tagged
`BRIDGE_CROSSING` becomes a `CROSSING_CANDIDATE` node, one tagged `TUNNEL_BYPASS` becomes a
`TUNNEL_CANDIDATE`, and every hydrology feature's own `FORD`/`BRIDGE` crossing becomes a
`CROSSING_CANDIDATE` too (a hydrology feature's `ROUTE_AROUND` candidate is a routing hint, not
infrastructure to plan, and is not turned into a node). Neither generator is required to have
run -- `StrategicPlanner` degrades to zero nodes of that kind rather than failing when either
did not build, which matters given both currently share the same unreached-in-practice gap this
whole chain carries (see below).

Water/cliff exclusion (Section 8.1) reads the same owners' own answers rather than a second
test of the same fact: `WaterSystem::Is_Point_In_Water` and the chunk registry's own
`Buildable` flag (already folding in `CliffBand`/`InteriorCliff`/slope) are the only exclusion
tests run.

## The suitability grid

Phase 7 step 1's six named inputs collapse into what is actually measurable at this point in
the pipeline:

- **slope/buildability, water/cliff exclusion** -- the chunk registry's own `Buildable` flag,
  combined with `WaterSystem::Is_Point_In_Water`, as a hard eligibility gate.
- **boundary distance** -- `WorldBoundsService::Get_Inward_Distance`, normalized against a fixed
  reference distance.
- **reachable area estimate, expansion room, roadability** -- one proxy for all three: how many
  of a chunk's own four neighbours are themselves eligible. This is also exactly Phase 7 step
  4's own approach-count rejection test, so eligibility and approach counting are one walk, not
  two, and the profile's own `fairness.min_vehicle_approach_count` (already validated, already
  meaningful to Section 10A.7) is the threshold, not a second invented number.
- **resource potential** -- added only to a base candidate's own score, as a bonus for standing
  near a Tiberium candidate position (see below); not part of the shared terrain score every
  other category reads.

A small deterministic hash-based jitter (`TerrainMaskClass::Hash_Value`, weight 0.05) is added
to each chunk's terrain score. This is the generator's only use of its own seed: every ranking
here is otherwise a pure function of the terrain, and on a large regular field many chunks tie
exactly on the terms above. Without the jitter, two different seeds over similar terrain would
rank ties identically every time; with it, they do not.

## Bases, and the ordering problem step 5 creates

Step 2/3 place bases; step 5 places Tiberium; but step 1's own suitability grid wants "resource
potential" as an input to base scoring, and step 11 wants Tiberium candidates checked for a
nearby base *after* bases exist. `StrategicPlanner` resolves the ordering explicitly rather than
picking one of the roadmap's two orderings and hoping it does not matter:

1. Tiberium **candidate positions** are chosen first, by terrain score alone (not yet nodes).
2. Base candidates are scored by terrain score plus a proximity bonus toward the nearest
   Tiberium candidate position, then accepted in alternating GDI/Nod order (first-ranked → GDI,
   second → Nod, and so on), each rejected if it falls under `fairness.min_vehicle_approach_
   count` or within `min_base_separation` of an already-accepted base. A faction that reaches
   its own target count first yields its next slot to the other rather than wasting a candidate
   neither needed.
3. Only now do the Tiberium candidate positions become `RESOURCE_CANDIDATE` nodes -- and only
   the ones with a base of either faction within `max_economic_range`. Step 11's "eliminate
   candidate combinations that are obviously economically disconnected" is this filter, made
   concrete rather than left as a phrase.

### Planned sites (D-003)

With a map plan, `ChunkScore` carries `PlanBase` and `PlanField` beside `Terrain`. Step 5
ranks Tiberium candidates by `Terrain + PlanField * 2.0` and skips planned base discs; the base
ranking adds `PlanBase * 2.0`; settlements use `Terrain` alone. See `MapPlan.md`.

## Settlements, POIs, and expansion areas

Settlement candidates (step 6) and POI candidates (step 7) both use terrain score alone, placed
on whatever eligible ground remains once bases and Tiberium have claimed theirs. Forward
expansion/outpost areas (step 8) are their own pass, scored by *distance from the nearest base*
rather than raw terrain score -- a frontier position is deliberately picked far out, not merely
buildable.

Step 9's own closed node-type vocabulary (`BASE_CANDIDATE`, `RESOURCE_CANDIDATE`,
`SETTLEMENT_CANDIDATE`, `POI_CANDIDATE`, `CROSSING_CANDIDATE`, `TUNNEL_CANDIDATE`) has no
separate slot for expansion areas, so an expansion candidate is filed as its own
`POI_CANDIDATE` node -- the closest fit the roadmap's own vocabulary offers, stated here rather
than silently assumed.

## The travel-cost graph

Step 10's "approximate straight/topographic travel costs among nodes" is exactly that:
approximate and straight. `Get_Travel_Cost` returns Euclidean distance between two nodes'
positions from a full N×N matrix built once after every node is placed. A real topographic or
routed cost needs a road graph to measure one along, which is Phase 8's job -- this number is
what a phase without one yet can still reason with.

## What is proven, and what is not

`terrain_strategic`/`fds_terrain_strategic` (`Code/Commando/terrainselfcheck.cpp`,
`-terrainselfcheck strategic`) prove, device-less, against a large synthetic uniform-slope ramp
(six chunks per side, entirely buildable, no boundary and no water configured, so there is room
for every candidate category without separation/range parameters needing to sit at the edge of
failing): parameter refusal; each faction reaches Phase 7's own targeted-validation floor of 2
base candidates; at least 1 Tiberium candidate survives the economic-range filter; no node
overlaps water or the cliff band; no resource candidate is economically disconnected; every base
candidate carries a real faction; the travel-cost matrix is symmetric and zero to itself; and the
same seed and terrain reproduce the same checksum.

What is not proven: whether a real generated seed's own terrain (uneven, with real cliffs and
water already carved into it) still offers each faction a comparable pair of sites -- the
targeted validation's own "at least 2 plausible approach directions across candidate
combinations" is not separately checked, since route independence in the sense Section 17A.2
means (two paths that do not immediately merge into the same choke) needs a road graph that does
not exist until Phase 8. The reachability gap every phase since 4 carried -- this generator only
running when macro terrain itself ran, which the one real caller's `fit_to_level` profile made
never -- is closed by V2 Phase 10's fitted-terrain sampling; see `completed_features.md` and
`GeneratorDeterminism.md`.
