# Performance, Scaling, and Soak Hardening

V2 roadmap Phase 29 (Section 30). Three representative world profiles
(`data/procedural_world/{small,default,large_supported}.json`), an inventory of the budget/LOD
mechanisms earlier phases already built, and one real defect found and fixed along the way.

## The one honest limit on this phase, same as Phase 28's

Section 30/Phase 29's own steps are almost entirely empirical: "capture baseline metrics," "measure
generation time/client frame time/server tick/...," "identify actual bottlenecks before
optimizing," "run long server/client soak," "check for memory growth... fix demonstrated issues."
All of that requires a real running match over real wall-clock time, driven by an actual game
client -- which, as `docs/integration/Phase28IntegrationGate.md` already established, this session
cannot do (no game-client automation tool exists here). Picking budget *numbers* without measuring
anything would be fabrication, not engineering, so this phase does not invent tuning values it has
no data for. What it does do for real: define the three profiles Section 29 step 1 explicitly asks
for, audit what budget/LOD control already exists so a future tuning pass does not duplicate it, and
found a real, previously-unknown, blocking defect while doing so.

## Representative profiles (step 1)

| Profile | File | World size | Chunks (60u each) |
|---|---|---|---|
| SMALL | `data/procedural_world/small.json` | 600 x 600 | 10 x 10 |
| MEDIUM | `data/procedural_world/default.json` (unchanged size -- not a fourth, competing "default") | 1200 x 1200 | 20 x 20 |
| LARGE_SUPPORTED | `data/procedural_world/large_supported.json` | 2400 x 2400 | 40 x 40 |

All three otherwise share the shipped default's terrain/water/fairness/retry settings verbatim --
this phase varies exactly the one axis Section 29 step 1 asks for (world size), not density/fairness
values it has no measured justification to change. `shaderselfcheck perfbudgets`
(`Run_Performance_Budgets`, `Code/Commando/shaderselfcheck.cpp`) embeds the same three profiles as
inline JSON (the same `Load_Profile_From_Text` convention every other self check in this file
already uses -- no self check loads a profile from a real file path, so this does not start now) and
asserts all three parse, validate, and are strictly ordered SMALL < MEDIUM < LARGE_SUPPORTED.

## A real defect found and fixed: the shipped default profile was broken

While building these profiles, `Validate_Profile` refused all three with `"unsupported
schema_version"`. Investigation: `ProceduralWorldProfile::CURRENT_SCHEMA_VERSION` is `8`
(`proceduralworldprofile.h:458`; `CURRENT_GENERATOR_VERSION` is `6`), but
`data/procedural_world/default.json` -- the file `ProceduralWorldManager::DEFAULT_PROFILE_PATH`
actually points at, and the one the menu's procedural button and every `procedural_world` console
request fall back to -- still said `"schema_version": 4, "generator_version": 4`. `Validate_Profile`
requires exact equality (`proceduralworldprofile.cpp:285`) with no migration path for profile files
(confirmed: no `Migrate`-named logic exists anywhere in `proceduralworldprofile.cpp`). This means
the shipped default profile, as of before this fix, would have been **rejected outright** the
moment anything actually called `Generate_World` against it -- a real, blocking defect that had
gone unnoticed because, per `awaiting_answers.md`'s own Q-012 and `docs/integration/
Phase28IntegrationGate.md`, nothing in this repository has ever actually driven
`ProceduralWorldManager::Request_World`/`Generate_World` against a live level. Fixed: bumped
`default.json`'s `schema_version`/`generator_version` to `8`/`6` (every field the four intervening
schema revisions added -- `Logistics`, `VehicleSubsystems`, and others -- already defaults safely
via the same `json::value(key, default)` pattern every field in this loader uses, so only the two
version numbers needed to change, not new content).

**Later, at V2 close.** The root cause was also fixed. `Validate_Profile` now accepts any schema
1..8, as `proceduralworldprofile.h` had always said it should, and refuses only newer ones. The
generator moved to 7 (tunnels, coastline, street lamps). All three shipped profiles are at 8 / 7.
See `NetworkingAndPersistence.md`.

## Budget/LOD mechanisms already built (Section 30.2/30.3) -- reused, not duplicated

Per this phase's own scope check (`PROJECT_RULES.md`: search before creating), the following
already exist from earlier phases and are the real answer to most of Section 30's "add/stabilize"
list. None of this is new in Phase 29; it is catalogued here so a future tuning pass finds the
existing owner instead of building a second one.

| Section 30 item | Existing owner | Status |
|---|---|---|
| AI think-rate LOD | `CommanderAIService::Get_Planning_Interval_Seconds`/`Get_Difficulty_Settings` (Phase 24/25) | Real, self-check-covered |
| Staggered path replans | `GuardStateControllerClass::Begin_Tick`/`Set_Max_Replans_Per_Tick`/`Get_Replans_This_Tick`, plus `Set_Max_Active_Controllers`/`Get_Refusal_Count` | Real, pre-existing |
| Snapshot compaction | `WorldSnapshotCompactionPolicy` (`MaxOperations`=256, `MaxAgeSeconds`=300, `MaxOperationBytes`=16384), `WorldSnapshotService::Should_Compact`/`Compact`/`Set_Compaction_Policy` (Phase 20) | Real, already bounded, already configurable |
| World-operation rate limits | `WORLD_OP_RESULT_RATE_LIMITED`, enforced in `WorldOperationJournal::Submit` via `CraterLimits.MaxCratersPerWindow`/`RateWindowSeconds` (Phase 20) | Real, but scoped to `WORLD_OP_CREATE_CRATER` only |
| Far terrain | `WorldTerrainSystem::Build_Far_Terrain_Representation`/`Update_Far_Terrain`/`Get_Far_Terrain_Drop` | Real, single-tier |

## Genuine, unbuilt gaps (not fixed this phase -- disclosed, not fabricated)

- **Multi-tier chunk LOD and foliage LOD/culling do not exist.** Only the single far-terrain tier
  above does. Building graduated LOD tiers is real rendering engineering whose distance thresholds
  need measured frame-time data to set correctly -- exactly the empirical gap this phase's own
  intro section explains this session cannot close. Inventing threshold numbers with no
  measurement would be the fabrication `PROJECT_RULES.md` forbids.
- **World-operation rate limiting covers only crater creation.** Building placement, terrain
  mutation more broadly, and other world-operation types have no per-tick/per-client cap. A real
  gap, scoped narrowly enough (extend `CraterLimits`'-style windowing to more operation types) that
  a future pass could close it without needing live measurement first -- correctness of a bound
  does not require tuning data, only its exact number does.
- **Nav mesh rebuild queueing** (as distinct from the guard-AI path-*replan* budget above) was not
  found anywhere in `Code/Combat`/`Code/wwphys`. Not investigated further this phase; flagged for
  whoever picks up chunk LOD, since both concern the same terrain-mutation-triggers-rebuild path.

## Performance report / soak result

Not producible this phase for the reason stated at the top: no live client, no live soak run, no
measured baseline for any of `SMALL`/`MEDIUM`/`LARGE_SUPPORTED`. **Next exact action** for the
user: on a machine that can run a real client and server, for each of the three profiles above --
`world_tools preview` the generated world to confirm it looks reasonable at that size, time the
generation (`ProceduralWorldManager: world generated -- ...` log line minus the request time),
watch client frame time and server tick during a normal match, and specifically soak-test
`LARGE_SUPPORTED` for at least the memory-growth/stale-chunk/leaked-nav-object/runaway-journal
checks Section 30 step 10 lists. Report back the numbers; that becomes the real baseline this
document's budget table currently has none of.
