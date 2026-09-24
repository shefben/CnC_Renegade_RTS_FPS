# FPS/RTS Combat-Space Analysis (V2 Phase 15)

Roadmap Section 34 Phase 15 -- "FPS/RTS combat-space analysis and deterministic local repair."
Section 17, Section 1.2.

## A validator, the same shape as item 6

`CombatSpaceValidator` (`Code/Combat/combatspacevalidator.h/.cpp`) reads state every earlier
phase already produced -- `BaseSelectionPlanner`, `TiberiumFieldGenerator`, `SettlementGenerator`,
`RoadGraphGenerator` -- and reports rather than mutates, the same shape
`TransportationNetworkValidator` (item 6) already is. Section 17A.1's own thresholds
(`MaxMandatoryExposedInfantryRunTimeSeconds`, `MinVehicleLaneClearance`,
`MinVehicleApproachCount`, `MinInfantryApproachCount`, `PreferredCoverInterval`) were already
parsed onto `ProceduralWorldProfile::Fairness` in Phase 1 for exactly this phase; no new schema
field was needed.

## Real sightline analysis (Section 17A.4)

Section 17A.4 requires documenting: representative eye height, sample interval, directional
sample count, max analysis distance, and which terrain/objects count as occluders. This generator
answers all five for real: eye height is `WorldMetricsClass::Get_Soldier_Eye_Height`'s own real
value (1.62m); eight compass directions per sample point; 250-world-unit max distance;
`WorldSpatialIndex::Query_Sightline` is the occlusion test (D-001). It asks three owners, each
through its own spatial structure: the heightfield's ray (`WorldTerrainSystem::Ray_Intersect_Terrain`),
the physics scene's ray at projectile collision (placed structures, settlement buildings, every
collidable static), and `FoliageSystem::Query_Segment_Occlusion`, which walks only the foliage
cells the segment crosses and treats each live, blocking tree with a proxy as a vertical cylinder
(grass never occludes). The nearest hit wins. No query scans every object. P50/P90/max are reported from real sorted samples at every
selected base and every resolved settlement, never a single averaged number (Section 17A.4's own
explicit "do not rely on one average sightline number").

## Hard failures, re-derived rather than trusted

Every hard failure in step 6's own list is checked against fresh, re-derived data rather than
cached generator state, the same defense-in-depth every `Check_*` accessor in this chain already
applies to its own generator:

- **Disconnected base**: `RoadGraphGenerator::Find_Anchor_Path` re-walked between the two
  selected bases' own strategic-node indices, not `BaseSelectionPlanner`'s own cached result.
- **No resource path**: every `TiberiumFieldGenerator` field with a real owning faction and no
  resolved Harvester route.
- **Impossible choke**: `BaseSelectionPlanner`'s own `SingleBridgeDependency` flag, real data
  already computed during base selection.
- **Exposed infantry march**: the same real base-to-base road distance, divided by a fairness-
  estimate reference infantry speed (the same undefined-physical-quantity treatment
  `BaseSelectionPlanner`'s own reference vehicle speed already gets, documented rather than
  invented silently), compared against the real profile threshold.
- **Unusable vehicle lane**: every real `RoadGraphEdge`'s own `RoadSplineClass::Get_Width`
  compared against the real profile threshold.

## Repair: honestly not attempted this pass

Step 7 names eight repair operations. None is attempted. `BridgePlanner`, `BaseSelectionPlanner`
and `TiberiumFieldGenerator` each already perform real, bounded, in-place repair *during their own
generation*, where undoing a candidate is still cheap; retrofitting a second repair pass after
every generator in the chain has already committed roads, bridges and placed structures is
exactly the unsafe case those files' own refused repair operations already document (rerouting a
road that may already carry a committed bridge, moving a structure another generator's own
exclusion checks already accounted for). Step 12's own "reject... rather than silently accepting"
is honoured a different way: `CombatSpaceValidator::Passed()` folds into
`ProceduralWorldManager::Generate_World`'s existing bounded seed-retry loop (gap report item 5) as
one more input alongside `TransportationNetworkValidator`'s own required-base-routes flag -- a
hard combat-space failure asks the whole world to retry with a fresh seed, the same policy a
required-connectivity failure already gets, rather than either a silent pass or an inconsistent
new hard-refusal path.

## What is proven, and what is not

`terrain_combatspace`/`fds_terrain_combatspace` (`-terrainselfcheck combatspace`) asserts: an
empty world validates trivially (0 failures); a real chain (the same terrain/hydrology/strategic-
plan/road-graph/bridge-plan/base-selection/Tiberium-field/settlement setup `terrain_settlements`
already proves) samples a real, ordered (P50 <= P90 <= max) sightline distribution; every reported
failure names a recognised type; every one of the eight named repair operations resolves a real
diagnostic name even though none is ever produced; and the same seed/world reproduce the same
pass/fail verdict and the same failure count. Not proven: a scenario engineered so a specific hard
failure actually fires (this terrain's own real chain passes cleanly). The foliage occlusion query
has its own check in `terrain_foliage` (a trunk across the segment blocks, grass and a trunk
beside it do not).
