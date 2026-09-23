# Commander AI — Roadmap Section 24.1-24.4 (V2 Phase 24)

Autonomous AI Commander: seat reservation, the strategic world snapshot, the economy planner,
and base-expansion site scoring. `Code/Combat/commanderai.h` / `commanderai.cpp`.

## The v1 human Commander API is the only action surface this uses

An AI-held seat is a real entry in `CommanderProductionService::_Commanders[]`
(`AI_COMMANDER_ID = -1`, distinct from `0` = vacant and every real client id, which
`cNetwork::Get_My_Id()` guarantees is always positive). Every building an AI Commander creates
goes through the exact same pipeline a human's placement does:
`CommanderPlacementValidator::Validate` → `RuntimeBuildingFactory::Create_Constructing_Building`
→ `RuntimeBuildingFactory::Get_Construction_Bounds` → `BuildingConstructionService::Begin_Construction`
→ `CommanderStateClass::Changed()` — the same sequence `cCommanderPlacementEvent::Act` uses for a
human, read directly from that file to keep the two paths honestly identical rather than a second
copy that could silently drift.

The one place they necessarily diverge: there is no `SoldierGameObj`/`PlayerDataClass` to charge
an AI seat's purchases against, so `CommanderAIService` keeps its own per-team `AITreasury`
instead of reading `PlayerDataClass::Get_Money()`. That treasury starts at, and stays at, zero —
this codebase's Tiberium income only ever credits a connected human's own `PlayerDataClass`, and
no `HarvesterClass` is ever spawned for a generated world (`tiberiumfieldgenerator.h`'s own
disclosed gap), so a bot-only team has no income path yet. `Set_Treasury`/`Increment_Treasury` are
real, settable state — not simulated away — so the economy planner is exercised with a genuine
nonzero balance by the self check and by anyone who wants to test it live, and is already correct
the day an income path exists.

## Section 25.1 governance applies to an AI seat too

`Call_Commander_Vote`'s `ELECT` path only opens against a *vacant* seat (`current == 0`); an
AI-held seat is a real nonzero id, so a human challenges it with `commander_vote remove` exactly
as they would a human Commander. A team is never permanently stuck with a bot Commander it cannot
vote out, with no special case anywhere in the vote engine.

The one place an AI seat needed its own carve-out: `CommanderProductionService::Update_Commander_Vacancy`
polls for "is there still a soldier holding this seat", and an AI seat has no soldier by design —
without a fix it would read that as abandonment and vacate a *working* AI Commander after one
`COMMANDER_VACANCY_GRACE_SECONDS`. Fixed by treating any non-positive id (`0` = vacant, or
negative = AI-held) as never subject to that poll — `commanderproduction.cpp`'s
`Update_Commander_Vacancy`, `player <= 0` in place of the old `player == 0`. `CommanderAIService::Update`
is what notices an AI seat has actually been taken away (a lost `REMOVE` vote, a direct claim
against a vacancy this poll correctly still creates on team-change or a real disconnect) — it
checks `Get_Team_Commander(team) == AI_COMMANDER_ID` every planning tick and stands down the
moment that stops being true.

## Strategic world model — Section 24.2

`Build_World_Snapshot` (internal to `commanderai.cpp`) is the one real per-tick roster walk:

- **Power / wallet**: `TeamPowerGridService::Get_Generation`/`Get_Allocated_Demand` — real team
  totals, not re-derived.
- **PowerPlantCount / SiloCount**: every real building on this team, classified by its own
  resolved descriptor (`BuildingGameObjDef::Get_Descriptor_Name()` →
  `BuildingDescriptorManager::Get_Descriptor`) — `Power.Generation > 0` / `TiberiumSilo.Enabled`.
  The same resolution `building.cpp`'s own subcomponent code uses, so it counts a map-authored
  building exactly as it counts a Commander-built one, never inferred from a model name.
- **DamagedBuildingCount**: `DefenseObject`'s own health vs. max, the identical derived-read
  pattern Phase 20's repair-bay threshold and Phase 22's subsystem damage both use — no second
  health pool.
- **FriendlyUnitCount**: `CommanderProductionService::Get_Population` summed over infantry/
  ground/air.
- **KnownEnemyBuildingCount**: `WorldShroudManager::Is_Explored(this_team, enemy_position)` for
  every real enemy building — never the enemy's own existence directly. An unexplored enemy
  structure is invisible to this file exactly as it would be to a human Commander's own sidebar.
  This is the whole proof for *"does not inspect hidden enemy truth."*

## Economy planner — Section 24.3

`Decide_Economy_Priority(snapshot, policy)` is a pure function, checkable with plain structs: no
`GameObj`, no descriptor, no treasury balance — "can we afford it" is `Update`'s question, once
this has answered "what do we need". Fixed priority order: power first (`PowerAllocatedDemand`
past `PowerHeadroomFraction` (default 0.85) of `PowerGeneration`, or any real demand against zero
generation), then wallet/Silo headroom against `TargetSiloCount` (default 1).

`Find_Buildable_Descriptor(power_generating, team)` is the one data-driven descriptor search:
load order, first descriptor that is `Build.Buildable` and whose `Build.Team` either names this
team or is `-1` ("any faction") — the identical field `CommanderPlacementValidator::Validate`
already reads for a human's placement. A descriptor for the *other* real faction is never a
candidate regardless of load order.

**Not decided by this planner**: *"more Refinery/resource infrastructure"* (Section 24.3's third
bullet). Nothing in `BuildingModelDescriptor` distinguishes a Refinery from any other buildable
building, and Tiberium income has no path to a Commander's wallet yet regardless — deciding "build
a Refinery" would be a decision with no way to ever pay for itself honestly. A real, disclosed
scope limit, not a silent one.

## Site search for the economy planner's own buildings

Not base expansion — ordinary in-base construction. `Find_Team_Building_Centroid` anchors the
search on the *average position of this team's own real buildings*, not `BaseSelectionPlanner`'s
selected site, because this AI has to work on a legacy static map exactly as well as a generated
world (Directive 1.10) and a static map never ran `BaseSelectionPlanner` at all. `Find_Construction_Site`
then sweeps four concentric rings (600/1000/1500/2200 world units, 8 points each) around that
anchor through `CommanderPlacementValidator::Validate` — the real ghost-colour path, swept
automatically instead of by a cursor — and builds at the first point it accepts.

**A real, disclosed limit**: a team with zero standing buildings has nothing to anchor a search
on, and this phase has no MCV-deploy logic to found a base from nothing. The economy planner can
build *outward* from an existing base; it cannot start one.

## Base expansion — Section 24.4

`Score_Expansion_Candidate(candidate, weights)` is a pure weighted sum of five 0..1-normalised
terms (resource proximity, road access, safety, power headroom, distance from the map boundary),
so a self check can move one term and watch the score move with it. An unbuildable candidate
(`BuildableAreaOk == false`) always scores exactly `0`, regardless of every other term.

The real candidates behind that score (`Refresh_Expansion_Candidates`, run every planning tick,
exposed as `Get_Expansion_Candidate_Count`/`Get_Expansion_Candidate`) reuse Phase 7's own
`StrategicPlanner` node graph rather than a second, invented one — *"reuse over rediscovery"* is
that file's own stated rule for exactly this later-phase use. Every `STRATEGIC_NODE_BASE_CANDIDATE`
for this team's own faction is a candidate, except the one `BaseSelectionPlanner::Get_Selected_Site`
already resolved as this team's starting base (that is the base being expanded *from*, not a
candidate). Every `STRATEGIC_NODE_SETTLEMENT_CANDIDATE` is a candidate for either faction
(`StrategicNode::Faction` is `PLAYERTYPE_NEUTRAL` for every non-base node). Each candidate's terms
are real measurements: nearest `TiberiumFieldGenerator` field, nearest `LogisticsGraphService`
road node, nearest shroud-*explored* enemy building (the same Section 24.2 rule, never a raw
enemy position), this tick's own power headroom, `CommanderPlacementValidator::Validate` against
a representative buildable descriptor, and `WorldBoundsService::Get_Inward_Distance` when a
boundary is configured.

**Deliberately not acted on this phase.** `Update` refreshes and scores the list, and stops there
— it does not found a second base on the winner. Founding one well is a materially larger behavior
(protecting, powering and connecting a whole new base cluster) than *"evaluate candidate sites"*
requires, and founding one badly is worse than not founding one. A real, disclosed limit, exposed
today for Section 24.9 diagnostics and ready for whichever later phase decides to act on it.

Empty (`count 0`) whenever `StrategicPlanner::Has_Plan()` is false — a legacy/static map has no
candidate graph to read, and that is a correct, unremarkable empty result, not a failure.

## Console

`commander_ai <activate <nod|gdi>|deactivate <nod|gdi>|status>` — real calls into
`CommanderAIService`, not a display-only mock. `activate` seats the same `AI_COMMANDER_ID`
`Update` would and is subject to the identical "never displaces a real Commander" refusal;
`status` prints both teams' seat state, treasury, last decision, build counts, the current
snapshot, and the expansion-candidate count.

## Self-check coverage

`renegade -shaderselfcheck commanderai`, registered as ctest `shader_commanderai` and
`fds_shader_commanderai`. Device-less. Pins: `Activate` refusing to seat over a real Commander and
never touching the other team; `Deactivate` never clearing a seat this AI no longer holds, and
always clearing one it does; treasury get/set/increment, clamped at zero, per-team isolated;
`Decide_Economy_Priority`'s full priority table (healthy economy, zero generation with and without
real demand, demand past the headroom fraction, a missing Silo, power prioritised over a missing
Silo); `Find_Buildable_Descriptor` matching this team's faction or "any", skipping the other real
faction, and returning null once nothing matching is loaded; `Score_Expansion_Candidate`'s zero
floor for an unbuildable site, near-1.0 for a maximal one, near-0 for a minimal-but-buildable one,
and closer-beats-farther on one varied term with every other term held fixed.

**Not covered by any automated check**: `Update` itself — the one real building/roster/site-search
walk needs a live scene this harness has no way to construct, the same gap Phase 22's
`Govern_Subsystem_Speed` and Phase 23's `Count_Connected_Teammates` document. Also not covered:
`Refresh_Expansion_Candidates`'s own real inputs (`StrategicPlanner`, `TiberiumFieldGenerator`,
`LogisticsGraphService`, `WorldBoundsService`) against real generated-world data, and the console
command's output formatting. Both named in `WIP_features.md`.
