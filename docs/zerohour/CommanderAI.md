# Commander AI — Roadmap Section 24.1-24.9 (V2 Phases 24-25)

Autonomous AI Commander: seat reservation, the strategic world snapshot, the economy planner,
base-expansion site scoring (Phase 24, Section 24.1-24.4), and the production planner, defense
planner, attack planner, difficulty and diagnostics (Phase 25, Section 24.5-24.9). One service,
`CommanderAIService` (`Code/Combat/commanderai.h` / `commanderai.cpp`), not two — Section 24.1's
own role list is one AI Commander with many responsibilities, and splitting the file at the
Phase 24/25 boundary would be exactly the parallel-system split the project rules warn against
for no real seam in the domain.

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

## Why the attack planner writes its own order dispatch

Section 24.7's attack planner is the one place this file issues real unit orders, and it does so
the same way `Code/Commando/commanderorderevent.cpp`'s own `Dispatch_Order` does for a human
Commander's click — `ScriptEngine::Action_Goto`/`Action_Attack` over an `ActionParamsStruct` —
rather than sharing that function directly. `Dispatch_Order` is `static` to
`commanderorderevent.cpp`, and Combat (where this file lives) is the lower layer Commando links
against, so Combat code cannot call into Commando code regardless of linkage visibility.
`CommanderGuardAI` already reimplements the identical small dispatch for the same reason — this
file's own `Dispatch_AI_Goto`/`Dispatch_AI_Attack` follow that established precedent rather than
inventing a third shape. Section 24.6's defense planner does not reimplement guard behaviour at
all: it calls `CommanderGuardAI::Guard` directly, the same real entry point a human Commander's
own `ORDER_GUARD` click reaches.

## Production planner — Section 24.5

`Decide_Production_Mix(friendly_infantry, friendly_vehicle, known_enemy_vehicle, policy)` is
pure: the enemy's own known vehicle count (shroud-explored, Section 24.2's rule again) overrides
an `InfantryPerVehicle` ratio (default 2) whenever this team's vehicles are behind it; the ratio
governs otherwise; `AI_PRODUCTION_NONE` only once both are satisfied — genuinely nothing needed,
not a treasury or population-cap question.

The real purchase reuses exactly what a human Commander's sidebar reads from —
`PurchaseSettingsDefClass`/`PurchaseAvailabilityClass` — through `Find_Purchasable_Unit`, the same
page/entry lookup `cCommanderProductionEvent`'s own `Find_Purchase_Entry` performs, re-read
directly rather than guessed at. **Air is never proposed**: `commanderproductionevent.cpp`'s own
comment says why — "Air has no purchase page in this tree's content" — and this planner inherits
that disclosed gap rather than working around it. `Find_Team_Production_Facility` mirrors every
readiness check a human's own click passes through `cCommanderProductionEvent::Act` — alive,
construction complete, powered, exit clear (`WorldSpatialIndex::Query_Game_Objects_In_Sphere` at
the facility's own `Get_Commander_Exit_Point`) — so a facility this function accepts is one a
human's click would accept too. Population caps (`CommanderProductionService::Get_Population_Cap`)
and the difficulty's own treasury reserve fraction are checked as real gates in `Update_Production`,
the same "pure decision, then can-we-actually-do-it" split `Update_Economy` already uses.

## Defense planner — Section 24.6

`Score_Defense_Point(point, weights)` is a pure weighted sum (chokepoint 0.5 / value 0.35 / power
0.15 by default) of three 0..1-clamped terms. The real points behind it
(`Refresh_Defense_Points`) come from two sources, neither invented: every `BridgePlanner::
ResolvedBridge` that `CountsAsConnectivity`, scored by its own `StrategicImportance` — Phase 9's
own field, whose header has said *"no AI Commander consumer exists yet"* since it was written, and
is that consumer now — and every one of this team's own Power Plants/Silos (already known from the
snapshot's building walk), maximal `ValueProximity` by definition. `PowerVulnerability` is one real
per-team headroom number applied identically to every point this tick: a short grid raises the
priority of defending everything, not any one place more than another.

`Update_Defense` picks the single highest-scored point and calls `CommanderGuardAI::Guard` on up
to `difficulty.MinForceSize` real, unassigned, non-human units — never a human's own unit (Section
3A.4's rule, enforced by never issuing the order in the first place, not by relying on a refusal).

## Attack planner — Section 24.7

A real seven-stage machine (`IDLE → RECON → ASSEMBLE → ROUTE → ENGAGE → RETREAT/EXPLOIT`), not a
single `ATTACK_MOVE` — Section 24.7's own explicit instruction.
`Decide_Attack_Transition` is a pure function of the stage plus seven real signals
(`known_enemy_buildings`, `assembled_force_size`, `force_losses`, `force_size_at_engage_start`,
`arrived`, `target_destroyed`, `force_spent`) and the policy, so the whole machine is checkable
with no scene:

- **RECON → ASSEMBLE** once `known_enemy_buildings >= ReconBuildingThreshold` — a real target must
  exist before a force is gathered, not "attack blind".
- **ASSEMBLE → ROUTE** once `assembled_force_size >= MinForceSize` (difficulty-scaled).
- **ROUTE → ENGAGE** on arrival; **→ EXPLOIT** if the target died en route; **→ RETREAT** if the
  force was wiped out getting there.
- **ENGAGE → EXPLOIT** on `target_destroyed`; **→ RETREAT** once losses reach
  `RetreatLossFraction` (default 0.5) of the force `ENGAGE` actually started with — a fraction of
  what was sent, not an absolute count, so one policy scales with whatever `MinForceSize` produced.
- **EXPLOIT/RETREAT → IDLE** once the force is spent; **EXPLOIT → ROUTE** otherwise, for
  `Update_Attack`'s own real code to pick the next target the identical way `ROUTE`'s own entry
  already does — a genuine "nothing left to press toward" is folded into `force_spent` by that
  same caller rather than adding an eighth pure-function parameter used nowhere else.

`Pick_Attack_Target` is Section 24.7's *"choose route"* made concrete without a waypoint-following
primitive this codebase has never had (every AI-issued order in this tree, human or bot, relies on
the engine's own navigation between click and destination): it prefers the shroud-*explored* enemy
building reachable by the shortest real `LogisticsGraphService` road distance over the merely
nearest straight-line one, falling back to straight-line distance with no road graph built — a
real, not fabricated, choice either way. Orders are real `ScriptEngine::Action_Goto`/`Action_Attack`
dispatch to a tracked, bounded force (`MAX_ATTACK_FORCE = 8`), pruned every tick of anything no
longer live or on-team.

## Difficulty — Section 24.8

`Get_Difficulty_Settings(difficulty)` returns exactly three fields — `PlanningIntervalSeconds`,
`MinForceSize`, `ReserveFraction` — and nothing else, which is what makes *"do not simply grant
omniscience or impossible resources"* true by construction rather than by promise: the struct has
no field that could touch `AITreasury` income, `WorldShroudManager` visibility, or any validator.
Easy is slower and more cautious (10s planning interval, wants 5 units before committing, holds
back a quarter of its treasury); Hard is faster and leaner (2.5s, 3 units, no reserve); Normal sits
between (5s — `AI_PLANNING_INTERVAL_SECONDS`, the difficulty-independent default — 4 units, a 10%
reserve). `Set_Difficulty`/`Get_Difficulty` are per-team; `commander_ai difficulty <nod|gdi>
<easy|normal|hard>` sets it live.

## Console

`commander_ai <activate <nod|gdi>|deactivate <nod|gdi>|difficulty <nod|gdi> <easy|normal|hard>|
status>` — real calls into `CommanderAIService`, not a display-only mock. `status` prints both
teams' seat state, difficulty, treasury, last economy decision, build counts, the current snapshot
(now including friendly/known-enemy infantry/vehicle counts), the expansion-candidate count, the
production-mix decision, the defense point/defender counts, and the attack stage/force
size/target position.

## Self-check coverage

`renegade -shaderselfcheck commanderai`, registered as ctest `shader_commanderai` and
`fds_shader_commanderai`. Device-less. Phase 24's own coverage (seat rules, treasury, economy
priority table, descriptor search, expansion scoring) is unchanged; Phase 25 adds:
`Decide_Production_Mix`'s enemy-override and ratio rules and its `NONE` floor; `Score_Defense_Point`'s
weighted sum, zero floor and out-of-range clamping; `Decide_Attack_Transition`'s full stage cycle,
one real signal at a time (RECON's threshold, ASSEMBLE's force size, ROUTE's three exits, ENGAGE's
loss-fraction threshold and target-destroyed exit, RETREAT/EXPLOIT's force-spent exits); the three
difficulty presets being genuinely distinct and strictly ordered (Easy more cautious than Normal
more cautious than Hard) on all three of their fields; and every new diagnostic reading a real,
correct empty/idle state before `Update` has ever run.

**Not covered by any automated check**: `Update` itself, and every real sub-planner it calls
(`Update_Economy`, `Update_Production`, `Update_Defense`, `Update_Attack`) — the one real
building/unit roster walk, purchase-page lookup, `CommanderGuardAI::Guard` call and
`ScriptEngine::Action_Goto`/`Action_Attack` dispatch each needs a live scene this harness has no
way to construct, the same gap Phase 22's `Govern_Subsystem_Speed` and Phase 23's
`Count_Connected_Teammates` document. Also not covered: `Refresh_Expansion_Candidates`'s and
`Refresh_Defense_Points`'s own real inputs (`StrategicPlanner`, `TiberiumFieldGenerator`,
`LogisticsGraphService`, `WorldBoundsService`, `BridgePlanner`) against real generated-world data,
and the console command's output formatting. All named in `WIP_features.md`.
