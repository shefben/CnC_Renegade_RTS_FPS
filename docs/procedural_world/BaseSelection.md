# Base Selection (V2 Phase 11)

Roadmap Section 34 Phase 11 -- "final base selection and deterministic fairness repair."
Sections 16, 17.4, 17.5. See `StrategicPlanning.md` for the candidates this phase chooses from,
`RoadGraph.md`/`BridgePlanning.md` for the real road/bridge data it scores them against, and
`docs/procedural_world/WorldProfileSchema.md` for the Section 17A.1 thresholds this generator
gates against (parsed onto `ProceduralWorldProfile::Fairness` in Phase 1, unconsumed until now).

## Choose, not discover

`BaseSelectionPlanner` (`Code/Combat/baseselectionplanner.h/.cpp`) never re-derives terrain,
roads or bridges. `StrategicPlanner` (Phase 7) already produced every `BASE_CANDIDATE`;
`RoadGraphGenerator` (Phase 8) already connected every candidate -- bases and resources alike --
into one minimum spanning tree, so a real road distance between any two candidates already
exists; `BridgePlanner` (Phase 9/10) already resolved or rejected every crossing that tree
needed. This generator's own job is Phase 11's purpose line: pick the one GDI/Nod pair that is
actually fair, given everything already built.

Because `RoadGraphGenerator`'s own anchor graph is a tree, the path between any two anchors is
unique and is found by a bounded breadth-first walk over the edge list (`Find_Tree_Path`,
file-local to `baseselectionplanner.cpp`) rather than a second Dijkstra pass -- there is exactly
one route to find, not a shortest one to search for.

## Site score ranks pairs; named thresholds gate them

Step 1's ten criteria collapse into two different things this file computes, on purpose kept
separate:

- **A composite site score, 0..1**, used only to *rank* GDI x Nod pairs against each other before
  any is tried -- buildable-area fraction at a footprint radius (`sqrt(Fairness.
  MinBaseExpansionArea / pi)`), the same fraction at twice that radius for expansion room, a
  high-ground exposure ring at the footprint radius (how much of the surrounding terrain stands
  meaningfully higher), air-operating clearance to `WorldBoundsService::Get_Air_Ceiling`, and
  boundary distance. Fixed weights, the same kind of implementer-chosen, deterministic, documented
  formula `StrategicPlanner`'s own chunk-scoring already uses -- not itself a pass/fail gate.
- **Section 17A.1's own named thresholds**, which are: whether both sides even have a reachable
  resource candidate at all, whether the difference between their two resource-travel times stays
  under `MaxInitialResourceTravelTimeDifferenceSeconds`, whether the base-to-base route itself
  stays under `MaxBaseToBaseTravelTimeDifferenceSeconds`, and whether the route crosses a bridge
  at all when `AllowSingleBridgeDependency` is false. A pair is selected only when every one of
  these passes -- the site score never overrides a failed threshold, it only decides which pair is
  tried first.

**One reading called out rather than guessed past.** `MaxBaseToBaseTravelTimeDifferenceSeconds`
and its sibling `MaxBaseToFrontTravelTimeDifferenceSeconds` are both named "difference" in Section
17A.1's own list, but exactly one GDI/Nod pair has exactly one physical route between them -- there
is no second measurement for a literal difference to be taken against. This generator reads the
base-to-base field as a direct cap on that one route's own travel time (a "difference" from
immediate adjacency), documented here rather than invented silently. `MaxBaseToFrontTravelTimeDifferenceSeconds`
needs a defined "front" (Section 17's own contested-area concept) this phase does not establish and
is not attempted -- Phase 11's own "Targeted validation" block names only resource access,
required connectivity and "travel-time difference," never a front-line term, so this is a scoped
omission, not a silent one.

## Real, bounded repair -- and what it does not attempt

Step 4 names six repair operations (`BaseRepairOperationType`, `baseselectionplanner.h`); this
generator performs two of them for real and refuses the rest rather than fabricating them:

- **`BASE_REPAIR_MOVE_RESOURCE_CANDIDATE`** -- when a resource-access failure is the reason a pair
  is rejected, the worse-off side's own nearest resource candidate is excluded and the next-nearest
  one (still a real `StrategicPlanner` node, never a coordinate this file invents) is tried instead.
- **`BASE_REPAIR_MOVE_BASE`** -- when a bridge-dependency or travel-time failure is the reason, the
  lower-scoring side of the pair is substituted for its own next-ranked same-faction candidate,
  holding the better-scoring side fixed -- a real, distinct search from step 6's own "try the next
  ranked pair," since it does not also discard the side that was already a good fit.
- **`BASE_REPAIR_ADJUST_ROAD_EDGE`, `BASE_REPAIR_ADD_RAMP`, `BASE_REPAIR_ADD_TUNNEL_CANDIDATE`,
  `BASE_REPAIR_ADJUST_LOCAL_TERRAIN`** -- named in the enum for diagnostic vocabulary completeness
  (the same reason `CrossingFallbackStageType`'s own alternate-crossing-type stage is named despite
  never being reachable), never attempted. Rerouting a road edge that may already carry a
  committed bridge is exactly the scope boundary `BridgePlanner`'s own fallback chain already
  refuses for the same reason (a committed bridge/road split has no cheap undo); no ramp or tunnel
  system exists to add a candidate to; and no owner exists at this layer for an arbitrary local
  terrain edit. A repair attempt this generator cannot perform is logged with `Applied = false`
  rather than silently skipped, so a failed seed's own repair log still tells the whole story.

Bounded by `Fairness`'s own `Retry.MaxLocalRetriesPerStage` (Section 3C.1's shared ceiling, not a
second number invented for this file), per candidate pair; step 6's own "choose the next base
pair" bounds the outer search by the finite number of GDI x Nod combinations. Step 7's "return a
generation failure with exact diagnostic" is `Generate`'s own ordinary `false` return with
`error_out` set, the same failure contract every other generator in this chain already has.

## Reserved region, not a base layout

Step 8 asks Phase 11 to "lock initial base footprints/approach zones against later
settlement/resource placement." `SelectedBaseSite::ProtectedRadius` is that lock, sized from the
same footprint radius the site score's own buildable-area term already used -- a conservative
circle, not Section 16A's real footprint/clearance/entrance shape, which needs canonical building
descriptors this generator never touches. Section 16A's own contract (initial building transforms,
factory-exit corridors, Harvester corridors, infantry spawn sets, match bootstrap) is a distinct,
larger piece of work; `WorldGenerationManifest::InitialBaseLayoutIds` stays empty from this phase,
the same "reserved, unconsumed" state several earlier fields in this chain were in before their own
consumer arrived. `SelectedBaseCandidateIds` is populated for real -- the actual chosen pair, not
every pair considered, the same distinction Phase 7's own header draws for candidates versus a
selection.

## What is proven, and what is not

`terrain_baseselection`/`fds_terrain_baseselection` (`-terrainselfcheck baseselection`) chain the
same terrain/hydrology/strategic-plan/road-graph/bridge-plan setup `terrain_bridgeplan` already
proves sufficient, then assert: both refusal paths (no strategic plan, and by extension no road
graph or bridge plan) report a reason and leave no state; a real chain selects a real GDI/Nod pair
with a non-empty reserved region each; the selected pair independently re-passes its own four
named threshold checks; every repair log entry names a recognised operation; and the same
seed/terrain/plan/road-graph/bridge-plan reproduce the same checksum and the same GDI selection a
second time. **Not proven**: a scenario engineered so a repair genuinely changes the outcome (the
terrain this check reuses gives the first-ranked pair a clean pass), and the real Section 16A
building-placement contract, which does not exist yet in any file. Both are open, honestly, the
same way item 5's true stage-1-succeeds case was left open in `docs/procedural_world/BridgePlanning.md`.

## Base layout, spawns and bootstrap (Section 16A, D-001 priority 5)

Section 16A is implemented by `ProceduralBaseLayoutService` (`Code/Combat/proceduralbaselayout.*`),
on the server, right after a generated world is accepted (`CombatManager::Post_Load_Level`):

- **Initial composition is profile data** (schema 9, `base.gdi_initial_buildings` /
  `base.nod_initial_buildings`, descriptor names in placement order). The shipped profiles name
  Construction Yard, power plant, refinery, infantry production, vehicle production and the
  faction's base defence. The first entry is the anchor and stands on the selected site.
- **Placement** tries the site, then rings outward starting behind the base, and judges every
  spot with `CommanderPlacementValidator::Validate` -- the same rules a Commander's build obeys
  (bounds, slope, water, cliffs, bridges, tunnels, resource routes, chokepoints, overlap, the
  production exit). `CommanderPlacementRequest::InitialBase` lets the non-buildable Construction
  Yard through and nothing else. Buildings face the enemy base; one with a `vehicle_exit`
  semantic anchor is turned so that exit faces it. A building with no legal spot is refused and
  logged by name, never forced.
- **Creation** is `RuntimeBuildingFactory::Create_Constructing_Building` followed at once by
  `Activate_Completed_Building`: a finished building, on its team's power grid and base.
- **Spawn pool** (`Generate_Base_Spawn_Set`): up to `base.spawn_candidate_count` points inside the
  protected circle, behind the base first, each on walkable ground (slope <= 30 degrees), inside
  the bounds, clear of the generated world's exclusions and of every building footprint and
  production exit the faction has standing. The fallback zone (`base.spawn_zone_radius`) is centred
  on the pool.
- **Respawn** (`Choose_Respawn_Point`): `SpawnManager::Get_Multiplayer_Spawn_Location` asks it
  first; it picks the clear point farthest from any living enemy. On every map without a generated
  base it declines and the level's own spawners answer.
- **Saves**: runtime buildings now save their descriptor and transform (`BuildingGameObj`), so a
  loaded save brings the base back itself; only the spawn pools are rebuilt around it.

Everything else in 16A.8 (credits, wallet cap, starting units, Commander role, shroud policy, AI
state) stays the game mode's, as on a stock map. Checked by `terrain_baselayout` (+ `fds_`);
creating the buildings needs the preset database and is a live check.
