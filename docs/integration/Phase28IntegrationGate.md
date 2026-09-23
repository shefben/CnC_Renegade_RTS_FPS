# Phase 28 -- Multiplayer / Save-Load / Late-Join / Dedicated-Server Integration Gate

V2 roadmap Phase 28 (Section 28). Required outputs: multiplayer integration report, save/load
integration report, late-join report, dedicated-server dependency confirmation.

## What this gate actually is, and the one honest limit on running it here

Every step this gate's own "Implement/fix in order" list names -- start a dedicated server, join
a client at match start, exercise player movement/vehicles/Commander Mode, mutate terrain,
destroy a bridge, late-join a second client, disconnect/reconnect, save, restart, reload -- is a
live, interactive, multi-process action. This session has shell/PowerShell access to build and run
processes, and can drive one dedicated-server process headlessly (already proven throughout this
whole project: every `-shaderselfcheck` self check runs `renegadeserver.exe` for real, with no
device, and reports results). It has no way to drive an actual game *client* window -- no
keyboard/mouse/render-frame automation for a native Win32 DirectX application exists in this
session's toolset (the browser-automation tools that do exist target web pages, not this game).
So the interactive half of this gate -- everything from "join client at match start" onward --
cannot be executed by this session. What follows is the real evidence this session could produce,
and an exact manual procedure for the interactive half, in the same "next exact action" form every
earlier phase's own disclosed gap already uses.

## 1. Dedicated-server dependency confirmation

**Verified for real.** `renegadeserver` (`Code/Commando/CMakeLists.txt`, `FREEDEDICATEDSERVER`
compile definition) links the *same* `commando`/`combat` static libraries `renegade` does -- there
is no separate, drifting server source tree. Client-only/render/UI code is compiled out via
`#ifdef FREEDEDICATEDSERVER` at the exact call sites that need a device or a window (30 files
across `Code/Commando`, `Code/Launcher` and `Code/WWOnline` gate real code behind it, not stubs
pretending to). This session's own repeated use of `renegadeserver.exe -shaderselfcheck <key>`
across every phase of this project (Phase 22 through this one) is itself running proof the binary
starts, runs and exits cleanly with **no display, no render device and no window** -- exactly what
"server does not load client renderer/UI dependencies" asks to be shown. The pre-existing, still-
unfixed `shader_debugrender`/`shader_powergrid`/etc. self-check failures are diagnostic-content
mismatches (see below), not startup or device crashes; the process itself never fails to run.

**Not verified:** an actual network listen socket accepting a real remote client connection --
this session never opened one, since nothing on the other end could connect to it.

## 2. Save/load integration report

**A real, demonstrated defect was found and fixed by this gate.** `awaiting_answers.md`'s own
Q-012 already flagged `shader_rtssaveload` (RTS save/load: `RTSPersistenceClass`) as one of several
self checks that are "device-less and self-contained... so a failure... is a real defect rather
than an environment problem" -- and had never actually been run. Running it here showed exactly
that: `shaderselfcheck.cpp`'s own `SIDEBAR_PLANT_JSON`/`SIDEBAR_FACTORY_JSON` test fixtures were
missing `game_object_preset`, which `BuildingDescriptorManager`'s own validation correctly refuses
(`build.buildable requires game_object_preset`, `buildingdescriptormanager.cpp:163`) -- so the
save/load self check never got past loading its own test descriptors. Fixed by adding
`"game_object_preset":"ow_test_sb_plant_preset"`/`"...factory_preset"` to both fixtures, the same
fix pattern Phase 24's own AI-Commander test fixtures needed. This one fix cleared
`shader_rtssaveload`/`fds_shader_rtssaveload` **and**, since the same two fixtures are shared,
`shader_sidebar`/`fds_shader_sidebar` and `shader_placement`/`fds_shader_placement` -- six
previously-failing tests, zero new failures. Full regression suite dropped from 30 known failures
to 24 with this one change; see the regression table below for the remaining 24, unchanged by this
phase and out of its scope (a different, deeper defect -- see "Known remaining defects" below).

**What this proves:** `RTSPersistenceClass`'s own save/restore logic (power-grid manual-disable/
priority-override persistence, production-queue restore with elapsed time and ticket
non-collision, post-load derived-state recalculation) is real and now actually exercised by a
passing self check, not merely present in source.

**What this does not prove:** an actual multi-megabyte save file written and reloaded by a real
running match with real buildings/units/terrain mutations/AI state -- `RTSPersistenceClass` is one
`SaveLoadSubSystemClass` among several the real save-game file format aggregates
(`Code/Combat/savegame.cpp`); this session never drove a real save/load through that top-level
path, only the one subsystem's own in-memory Save()/Load() logic via the self check.

## 3. Late-join report

**Verified for real (pre-existing, unchanged by this phase):** `terrain_worldsync`/
`fds_terrain_worldsync` (Phase 20's `WorldSyncService`/`WorldSyncHandshakeClass`, checksum/
manifest negotiation) already pass, both before and after this phase's changes -- the late-join
handshake contract (generator/profile/version exchange, checksum verification per Section 20.4) is
unit-tested and green.

**Not verified:** an actual second client joining an actual running match mid-session and
receiving a real snapshot + operation-journal replay. No test in this repository exercises that
end to end (confirmed by search before writing any Phase 27/28 code: no existing self check drives
`ProceduralWorldManager::Request_World`/`Generate_World` against a live physics scene at all).

## 4. Multiplayer integration report

**Verified for real:** the full regression suite (`ctest --test-dir build/win32 -C Debug`, 191
tests) runs clean except the 24 pre-existing failures below, across all three binaries
(`renegade`, `renegadeserver`, `leveledit`), zero warnings on every build this whole session.

**Not verified, and not verifiable by this session:** steps 2-17 of the roadmap's own gate
procedure -- starting a real dedicated server hosting a real match, a real client joining it,
exercising player movement/vehicles/Commander Mode/building construction/power/AI production,
mutating terrain, destroying a bridge, building a structure, a second client late-joining,
checksum verification against that live state, an actual save, an actual restart and reload,
verifying terrain mutations/buildings/power/AI state/bridge state/world revision survived, a
deliberate regional checksum mismatch and repair, and a disconnect/reconnect test. All of these
need a human (or a game-automation tool this session does not have) driving at least one, and for
late-join/disconnect-reconnect two, real game client windows.

## Known remaining defects (found, not fixed -- out of this gate's own scope)

`shader_construction`/`fds_shader_construction` fail for a *different* reason than the fixture bug
above: after fixing an unrelated missing `game_object_preset` in that test's own separate "tiny-
block" fixture, the remaining failures show footprint half-width/half-depth reading as
uninitialized-looking garbage values (e.g. `-107374176.000000`, the classic `0xCCCCCCCC`-adjacent
debug-heap pattern) rather than the descriptor's real dimensions. This is a real, distinct,
findable bug in the construction-placement footprint path, unrelated to save/load, multiplayer or
late-join -- squarely a future *building construction/placement* work item, not this gate's. Left
unfixed here rather than pulled into this integration gate's own already-broad scope.
`shader_production`'s one remaining failure ("the completion named definition 20, not the first
purchase's 10") looks similarly self-contained and unrelated. Both should get their own focused
investigation rather than being folded into a gate whose own theme is multiplayer/save-load/
late-join/dedicated-server, not construction visualization.

## Regression table (this phase's own before/after)

| | Before this phase | After this phase |
|---|---|---|
| Total tests | 191 | 191 |
| Failing | 30 | 24 |
| Newly fixed | -- | `shader_sidebar`, `shader_placement`, `shader_rtssaveload` (+ `fds_` twins) |
| Newly broken | -- | none |

Remaining 24: `shader_overlays`, `shader_guardai`, `shader_debugrender`, `shader_powergrid`,
`shader_commandermode` (SEGFAULT), `shader_construction`, `shader_production`,
`shader_visualsettings`, `terrain_bridges`, `terrain_shroud`, `terrain_macro`, `terrain_hydrology`
(each with its `fds_` twin) -- every one pre-existing, none touched by this phase, none newly
introduced.

## Next exact action for the user

This is the one Phase in this whole V2 sequence whose own gate cannot be closed by an agent alone.
A person needs to, on two machines or two processes on one machine:

1. Start `renegadeserver.exe` hosting a procedural-world match (`procedural_world` console
	request, or whatever the normal listen-server startup flow is for this build).
2. Connect a `renegade.exe` client, confirm manifest/version negotiation succeeds and the world
	matches what the server generated (`world_tools repro` on both ends, compare).
3. Play normally for a few minutes: move, drive a vehicle, enter Commander Mode, place a
	building, let power/AI production run.
4. Mutate terrain, destroy one bridge span, build one new structure.
5. Connect a second client mid-match; confirm it loads the same world state (no visible
	desync, matching `world_tools validate`/checksum output if exposed to the client console).
6. Save, restart the server process, reload; confirm the terrain mutation, the new building,
	power state, AI state and bridge state all survived.
7. Disconnect one client and reconnect it; confirm `commander_ai status`/`campaign status`-style
	diagnostics show it picked back up correctly (Phase 23's `Handle_Player_Reconnect` is the
	code path this exercises).

Report back what broke, if anything -- that becomes this gate's own real "integration defects
discovered," fixed in a following, narrowly-scoped pass rather than guessed at here.
