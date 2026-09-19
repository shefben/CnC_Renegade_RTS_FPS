# RTS telemetry / statistics instrumentation

Roadmap Section 38, Zero Hour / SAGE feature 25. Donor concept: `AcademyStats`.

`Code/Combat/rtstelemetryservice.{h,cpp}`.

## One shape, not one struct per system

Zero Hour's `AcademyStats` exists to drive an in-match coaching UI; the roadmap explicitly does
not want that UI or its advice rules, only the underlying idea that a broad, cheap set of counters
is worth keeping. `RTSTelemetryService` is a generic `(category, subtype) -> (count, running
total)` table rather than one struct per producer -- the same "one implementation, not one class
per source" discipline this whole pass has applied (`WorldSurfaceMarkManager`'s mark kinds,
`StatusOverlayRenderer`'s overlay kinds, `RTSTelemetryService`'s own category list).

## The category list is the roadmap's own, in full

`TelemetryCategoryType` lists every category Section 38 names, including the ones with no
producer yet. That is deliberate: a category with no current call site still exists in the enum,
so the phase that eventually needs it (the power grid, Phase 34; Commander placement, Phase 37;
PCT/MCT, Phase 38) only has to call `Record_Event`, never invent a category first. This is the
same discipline P26 used for its four unimplemented component contracts -- the absent case is
named, not invented early to look finished.

## What was wired this pass

Two real, existing, server-authoritative call sites:

- **Kills and unit destruction** -- `GameCombatNetworkHandlerClass::On_Soldier_Kill`/
  `On_Soldier_Death` (`Code/Commando/nethandler.cpp`), which already run only on the server
  (`WWASSERT(cNetwork::I_Am_Server())`). `TELEMETRY_KILL` and `TELEMETRY_UNIT_DESTROYED`, tagged
  with the `PLAYERTYPE_*` side. These handlers are infantry-only; no vehicle/aircraft destruction
  notification of the same shape was found this pass, so `TELEMETRY_VEHICLE_DESTROYED` and
  `TELEMETRY_AIRCRAFT_DESTROYED` stay unproduced.
- **Building destruction** -- `BuildingGameObj::On_Destroyed` (`Code/Combat/building.cpp`), guarded
  by `CombatManager::I_Am_Server()` since every client reaches this call to spawn its own debris
  (Section 28) but only the server's count is gameplay-authoritative. `TELEMETRY_BUILDING_
  DESTROYED`, tagged with the building's side.

`TELEMETRY_BUILDING_CONSTRUCTED` is left unwired on purpose rather than fired from a building's
constructor: "constructed" is a completed-construction event, and there is no construction
process to complete until Phase 38's procedural build-up exists. Firing it at raw object creation
would be recording the wrong moment.

## Rules honoured

- **Server authority.** Both wired call sites are already server-only; `RTSTelemetryService`
  itself has no network code and does not distinguish server from client -- that responsibility
  stays with each caller, the same way `CombatManager::I_Am_Server()` already gates other
  server-only behaviour throughout `Code/Combat`.
- **Cannot couple gameplay correctness to telemetry.** `Record_Event` returns nothing; no caller
  anywhere branches on whether an event was accepted. The record table refuses past its bound
  (`TELEMETRY_MAX_RECORDS` = 512 distinct `(category, subtype)` pairs) exactly like every other
  bounded table in this project, and a refusal is silent to the caller -- counted, not surfaced.
- **No PII, no upload.** Every field is a small integer or a float; there is no code path in this
  file that reaches the network or disk.
- **Deterministic, queryable.** `Get_Event_Count`/`Get_Event_Total`/`Get_Snapshot` are pure reads
  over the table `Record_Event` built, which is exactly what a test or a balancing tool needs.

## Enabled by default

"Disabled or low-overhead in normal release mode unless explicitly enabled" describes a
release/debug distinction this codebase does not have wired to anything yet -- no consumer needs
one today. `Set_Enabled`/`Is_Enabled` exist and are checked first in `Record_Event` (a single
branch when off, nothing else), so the mechanism the rule asks for is real; the default is `true`
because turning it off by default with no build-mode signal to flip it back on would just make the
service silently record nothing. The one place a later release-mode gate would hook in is this
flag, not a second code path.

## Checks

`renegade -shaderselfcheck telemetry`, registered as ctest `shader_telemetry` and
`fds_shader_telemetry`. Device-less. Pins counting, subtype narrowing, running totals, the
snapshot's shape and count, category-name lookup (including the `TELEMETRY_CATEGORY_COUNT`
sentinel reading back `"unknown"`), the disabled state recording nothing at all rather than a
suppressed-but-counted event, and the record table refusing past its bound.

**Not covered by any automated check**: the two wired call sites themselves need a real kill or a
real building destruction in a live combat scene, which `-shaderselfcheck` cannot produce. Every
other category in the enum has no call site yet at all.
