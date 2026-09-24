# Physical Logistics Layer (V2 Phase 21)

Roadmap Section 22. Canonical owner: `LogisticsGraphService`
(`Code/Combat/logisticsgraphservice.h/.cpp`). Schema: `ProceduralWorldProfile::Logistics`
(`Code/Combat/proceduralworldprofile.h`, schema version 7).

## The philosophy: inert by default, real when armed

Section 22.1 states the whole shape of this phase: "keep core player purchases simple...
logistics primarily affects Commander/AI strategic throughput... default gameplay must remain
viable without mandatory micromanagement." Nothing here ever touches a human player's purchase
path. `ProceduralWorldProfile::Logistics.Enabled` defaults to `false`; with it off,
`ProceduralWorldManager::Generate_World` never calls `LogisticsGraphService::Rebuild_Nodes` at
all, the graph stays empty, and every other system in this tree behaves exactly as it did before
this phase existed. `ConvoyEnabled` is a second, further opt-in gate specifically for step 6-8's
convoy orders, and `ConstructionSupportRadius` defaults to `0.0f`, which is documented as "no
bonus" rather than an error.

## Seven canonical node kinds, four of them real today

Section 22's own node list -- `BASE`, `REFINERY`, `FACTORY`, `REPAIR`, `FORWARD_BASE`,
`RESOURCE_FIELD`, `LOGISTICS_POI` -- is `LogisticsNodeKindType`. Each kind has a real source, but
only four of the seven produce a node anywhere in this tree today:

| Kind | Source | Real today? |
|---|---|---|
| `BASE` | `BaseSelectionPlanner::Get_Selected_Site` (Phase 11) | Yes -- one per faction with a selection |
| `FORWARD_BASE` | `ExpansionRegionRegistry` regions where `IsForward` (Phase 17) | Yes, once at least one construction has completed |
| `RESOURCE_FIELD` | `TiberiumFieldGenerator::Get_Field` (Phase 12) | Yes |
| `LOGISTICS_POI` | `SettlementGenerator::Get_Poi` (Phase 13) | Yes |
| `REFINERY` | a live `BuildingGameObj` whose descriptor has Phase 20's `SUBCOMPONENT_REFINERY_PROCESSOR` role | No buildable Refinery descriptor exists (P33's own gap) |
| `FACTORY` | a live `BuildingGameObj` whose descriptor's `Production.Kind != BUILDING_PRODUCTION_NONE` | No buildable Barracks/Hand of Nod/War Factory/Airstrip descriptor exists yet |
| `REPAIR` | a live `BuildingGameObj` where `As_RepairBayGameObj() != nullptr` | `RepairBayGameObj` exists in code (a real TT-donor class) but no buildable descriptor names it |

The last three are not stubs: `Rebuild_Building_Nodes` walks every real `BuildingGameObj` in
`GameObjManager`'s own object list (the same two-step `BaseGameObj -> ScriptableGameObj ->
BuildingGameObj` cast Phase 20's `building_subcomponents` console command already established)
and asks each one the same three real questions. They are empty today for the identical,
already-disclosed reason Phase 20's own four unwired effect groups were: no buildable descriptor
of that category exists in `data/building_descriptors/*.json` yet.

`Rebuild_Building_Nodes` is deliberately never called from this phase's own self-check, for the
same standing harness limitation Phase 20's doc already records: `-terrainselfcheck` never brings
up a `GameObjManager`/`PhysicsSceneClass` scene, so nothing with a real `BuildingGameObj` can be
exercised device-less. It is called from `ProceduralWorldManager`... actually it is **not**
called anywhere yet either -- see "What is not proven" below.

## The routes: real road distance, never a guess

Section 22.3: "avoid abstract global modifiers with no physical counterplay." A route's `Length`
and its waypoint polyline come from `RoadGraphGenerator::Find_Anchor_Path`, the same Dijkstra
search over `RoadSystem`'s real, built road network that `BaseSelectionPlanner`'s own fairness
metrics already use -- never a straight-line estimate. A node with its own `StrategicPlanner`
origin (`BASE`, `RESOURCE_FIELD`, `LOGISTICS_POI` all carry the originating candidate's id) routes
from that node directly; a node with no plan-time origin of its own (`FORWARD_BASE`, and every
live-building node) routes from the nearest real `StrategicNode` to its own position -- a
documented proxy, not its own exact position.

The topology `Rebuild_Routes` builds is hub-and-spoke from each team's own `BASE`/`FORWARD_BASE`
nodes, matching Section 22.2's own concept list:

- **Tiberium harvester routes**: a `BASE`/`FORWARD_BASE` to a `RESOURCE_FIELD` of the same team.
- **Forward-position routes**: a `BASE` to its own team's `FORWARD_BASE`.
- **Staging/repair routes**: a `BASE`/`FORWARD_BASE` to its own team's `FACTORY`/`REFINERY`/`REPAIR`.
- **POI routes**: any `BASE`/`FORWARD_BASE` to any `LOGISTICS_POI` -- nobody owns a POI, so team
  does not gate this one.

A route never connects two different teams' `BASE`/`FORWARD_BASE` nodes directly, and never
connects two facility nodes to each other -- both are proven negative assertions in
`terrain_logistics`, not merely undocumented omissions.

## Step 9: blockage detection is real, "enemy obstruction" is not attempted

`Update_Route_Status` re-samples every OPEN/BLOCKED route's own waypoints against
`BridgeSystem::Find_Bridge_At`/`Is_Traversable` every server tick it runs (gated behind
`LogisticsGraphService::Has_Graph()`, so it is a no-op with the profile's own default). A broken
bridge on a route's own path flips it to `LOGISTICS_ROUTE_BLOCKED_BRIDGE` and records which
bridge; a bridge repaired flips it back. "Destroyed road-critical structure" is covered
indirectly -- a destroyed `FACTORY`/`REFINERY`/`REPAIR` building stops appearing in
`Rebuild_Building_Nodes`'s own live scan the next time it is called, which removes its routes on
the next `Rebuild_Routes`. "Enemy obstruction where relevant" (Section 22's third named cause) is
honestly not attempted: no logistics-facing threat model exists yet. That is Phase 25's own
recon/defense planner territory.

## Step 5: construction support radius is a real, tested, unconsumed predicate

`LogisticsGraphService::Is_Under_Construction_Support(position, team)` is real and proven by
`terrain_logistics` (inside the radius of a live `FORWARD_BASE`, outside it, and across teams).
No caller applies a construction-speed or -cost bonus from it yet. This mirrors this project's own
established precedent for an optional mechanism with no consumer -- `StatusOverlaySystem` and
several `DebugRenderSystem` layers "toggle but draw nothing" until a caller exists -- rather than
inventing an unrequested balance change (how much faster, by what curve) with no specification to
implement against.

## Step 6-8: convoys are existing AI vehicles walking a real route, nothing more

`Start_Convoy`/`Update` is the same "controller ticked once a server frame, translating intent
into `ScriptEngine::Action_Goto`" shape `CommanderGuardAI` already established for guard orders,
applied to a route's own waypoint list instead of one guard point. Refused outright when
`ConvoyEnabled` is false (the profile's own further opt-in past `Enabled` itself). Per Section
22.4/step 7's own instruction, no cargo or inventory abstraction is invented: whatever vehicle is
given a convoy order is what it carries -- a Harvester remains a Harvester, driven by its own
existing route logic (Phase 12); nothing here duplicates that.

## Wiring: the completion hook this phase needed and found dormant

Building the `FORWARD_BASE` node kind exposed a real, pre-existing gap: Phase 17's own
`ExpansionRegionRegistry::Note_Construction_Complete`/`Rebuild_Regions` (steps 12-13) had **no
caller anywhere in this tree outside their own self-check** -- confirmed by grep, not assumed.
The `ExpansionRegion.IsForward` metadata Phase 21's own `FORWARD_BASE` kind depends on could
never have existed at runtime. Fixed by wiring both calls into `combat.cpp`'s own real "a
construction just finished" event -- the `RuntimeBuildingFactory::Activate_Completed_Building`
loop, server-only, right after activation. This is an actual pre-existing incompatibility this
phase's own work demonstrated, not an opportunistic Phase 17 revisit: the roadmap's own top-level
instruction (Section 0) permits exactly this ("an actual bug/incompatibility is demonstrated").

Deliberately **not** also wired: step 10's `Reserve`/`Bind_Building`. Q-016's own design already
creates the real `BuildingGameObj` -- with real spatial-index collision -- the instant a placement
is accepted (`commanderplacementevent.cpp`), which is exactly what
`CommanderPlacementValidator::Validate_Server_Request`'s own `WorldSpatialIndex::
Query_Placement_Overlap` call sees on the very next placement. The placement race
`ExpansionRegionRegistry::Reserve` was designed to close looks already closed by that, confirmed
by reading both call sites rather than assumed; wiring a second, redundant reservation on top of
it is outside what Phase 21 actually needed and risked disagreeing with Q-016's own design intent
without a demonstrated bug to justify it.

## What is proven, and what is not

**Proven, device-less, by `terrain_logistics`/`fds_terrain_logistics`** (both green): `Rebuild_Nodes`
tolerates an empty world; a real chain (terrain through settlements, the same setup
`Check_Combat_Space` already established) produces real `BASE` and `FORWARD_BASE` nodes; node ids
resolve by their own stable id; `Rebuild_Routes` over that real world produces at least one real
OPEN route with positive length and real waypoints, and an unreachable route reports a real
negative length rather than a fabricated one; the graph's checksum is deterministic across an
identical rebuild; a route is never created straight between two factions' own bases;
`Is_Under_Construction_Support` respects both radius and team; convoys are refused with
`ConvoyEnabled` false and accepted against a real OPEN route once true; `Update` drops a convoy
whose unit does not resolve rather than leaving a dangling record; `Clear` leaves nothing behind.

**What is not proven**: `Rebuild_Building_Nodes` (REFINERY/FACTORY/REPAIR) has never executed
against a real `GameObjManager` scene -- the same standing harness limit every runtime-building
construction path in this tree already carries (`Run_Runtime_Door_Service`'s own note, Phase 20's
own doc). `Update_Route_Status` has never seen a route that actually crosses a bridge that then
breaks mid-match. A convoy has never actually driven a real vehicle along a real multi-waypoint
route on screen. The Commander tactical-map visualization Section 22.5 asks for
("critical logistics routes... blocked routes... damaged infrastructure... forward repair
coverage... subject to team intelligence rules") is not attempted at all -- `logistics_status`
(console, both binaries) is the debug/status report step 10's counterparts in other phases already
established as sufficient for this stage, not a Commander-facing UI; a real tactical overlay is a
rendering consumer nothing in this tree has built yet, the same gap `DebugRenderSystem`'s own
fourteen empty layers already carry for unrelated systems.

## Next exact action

Rolled into the standing live-pass backlog every other device-less procedural-world phase already
carries: run `renegade --gamedir "C:\Westwood\Renegade_full"`, click **Procedural Skirmish** with
a profile that sets `logistics.enabled` (and, to see convoys, `logistics.convoy_enabled`) true,
read the new `ProceduralWorldManager: logistics graph ...` log line, then `logistics_status` at
the console and confirm the node/route counts look right for the seed. Because no buildable
Refinery/Factory/Repair Bay descriptor exists yet, `REFINERY`/`FACTORY`/`REPAIR` nodes will not
appear in that live pass either, until P33's own backlog closes -- a content gap, not a bug to
chase.

## Enemy obstruction (D-001)

Re-audited against Phase 25 as D-001 asked: no second threat model. A team's known enemies are
`TacticalIntelligenceModel::Build_Strategic_Contacts` for that team (visible, radar-only or
last-known enemy infantry, vehicles and buildings). A route with one within
`logistics.enemy_threat_radius` (schema 9, default 0 = off) of its polyline is closed to that
team's convoys (`Is_Route_Open_For`); `Start_Convoy` refuses it and a convoy on it stops. The other
team is unaffected. Re-evaluated every 2 seconds in `Update`. Checked in `terrain_logistics`.
