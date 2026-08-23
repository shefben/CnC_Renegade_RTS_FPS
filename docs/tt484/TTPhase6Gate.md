# Phase 6 — the 4.8.4 hard gate

Roadmap Section 12. Sixteen things have to be true before Zero Hour integration
may begin. This file says, for each of them, what makes it true and where to
look.

## How the checks run

Two of the sixteen are build steps and one needs two machines. The other
thirteen run inside the game binary itself, before anything is opened:

```text
renegade -scriptcatalogcheck <stock|tt|duplicates|recreate>
renegade -ttselfcheck        <objects|visibility|hooks|dialogs|collision|modules>
```

They live in the binary rather than in a test executable of their own because
the thing being checked is what *this* binary holds after static
initialisation. A separate program would link the scripts and the factories a
second time and prove something about that link instead. `main` takes the check
as its first act and exits with the result, so no window is made, no data file
is read, and nothing needs the game installed.

`ctest` drives all of them:

```text
cd build/win32
ctest -C Debug -R "scripts_catalog|tt_|fds_"
```

17 tests, about a second.

## The sixteen

| # | Required test | How it is met |
| --- | --- | --- |
| 1 | stock script catalog | `scripts_catalog_stock` — every stock script the source declares resolves in `NativeScriptRegistry`, and a merged script is still tagged as stock so a level naming it still finds it. |
| 2 | TT script catalog | `scripts_catalog_tt` — all 841 4.8.4 scripts resolve and are tagged `SCRIPT_SOURCE_TT`, and the registry holds no more of them than the source declares. |
| 3 | duplicate script-name collision | `scripts_catalog_duplicates` — the sorted registry index has no equal neighbours, no alias resolves to itself, and no alias collides with a real name. `tools/check_script_catalog.py` also runs at configure time and fails generation. |
| 4 | save/load script recreation | `scripts_catalog_recreate` — all 2477 entries are rebuilt through `ScriptManager::Create_Script`, the way a load does it, each answers to the name it was made by, and each round-trips its parameter string through the 256-byte buffer the save format uses. |
| 5 | TT building/factory object creation | `tt_objects` — nine building and factory types reach their definition factory by class id and by name, create a definition that agrees about its own class id, and have a persist factory, without which a save naming the type would not load. |
| 6 | TT per-client visibility | `tt_visibility` — per-client dirty bits stay per client; `BIT_RARE` carries the finer bits with it; the all-clients form is refused to a client and honoured for the server; the sentinel slot is what a rejoining client is caught up from. |
| 7 | TT purchase/refill hooks | `tt_hooks` — `GameEventBus::Purchase` and `Refill` dispatch, and a handler's refusal reaches the caller as a refusal rather than as a notification. |
| 8 | TT damage/kill hooks | `tt_hooks` — `Damage` carries the pre-scaling amount and its refusal; `Kill` dispatches. |
| 9 | TT chat/player hooks | `tt_hooks` — `Chat` can swallow a message, `PlayerJoin` and `PlayerLeave` see the player. |
| 10 | TT dialogs | `tt_dialogs` — the dialogs 4.8.4 added are in the table with controls and a size. `tt_hooks` also raises a dialog event naming `IDD_OBJECTIVES_TEXT`. |
| 11 | TT collision groups | `tt_collision` — the 4.8.4 ladder fix, and what it must not cost. See below. |
| 12 | controller behavior where supported | **Not applicable, with reason.** See below. |
| 13 | client/server map transition | **Not automated.** See below. |
| 14 | dedicated server | The eight `fds_*` tests run the same checks through `renegadeserver`, which is a second binary built from the same sources with `FREEDEDICATEDSERVER` defined. It starts clean, holds the same 2477-script catalog, and passes every hook, object, visibility and collision check the client does. |
| 15 | clean startup with no scripts-related DLLs | `tt_modules` — the process is asked what it has actually loaded, rather than the source being read and believed. No `scripts.dll`, `scripts2.dll`, `scripts3.dll`, `tt.dll` or `bhs.dll` is present, and the catalog is non-empty, which is the other half of the same statement: the scripts are in this binary. |
| 16 | tool/editor build | `leveledit` builds and links, against `combate` and `scriptse` — the same script sources the game compiles, with `PARAM_EDITING_ON`. It is a GUI application with no headless mode, so the build is the test; there is no ctest entry for it. |

## 11 — what the collision check asserts

The 4.8.4 ladder fix is `Disable_Collision_Detection(SOLDIER_GHOST, DEFAULT)`.
A ghosted soldier — one inside a ladder or elevator coordination zone, or
squeezing past a team mate — passes through vehicles and placed dynamic objects
as well as through other soldiers. That is the fix rather than a side effect:
ghosting means "do not stop this soldier", and a vehicle parked at the foot of a
ladder was stopping him.

What it must not cost is checked alongside it: a ghosted soldier still stands on
terrain, is still shot, and still has C4 stick to him; an ordinary soldier is
still stopped by a vehicle and by another soldier; and a player-built building
still does not collide with the ghost group used while it is being placed.

Reading the matrix back needed it without a physics scene behind it, since
building one wants a graphics device. Rather than write the matrix out a second
time by hand, `CombatManager::Define_Collision_Groups` describes it once through
a `CollisionGroupSinkClass`: `Scene_Init` passes a sink that forwards to the game
scene, the check passes one that records into a table and reads it back. One
description, two readers.

## 12 — controller behavior

Nothing to check, for the reason `TTPhase5Audit.md` §11.4 records:
`tt_4.8.4/tt/Input.h` declares no member `Code/Combat/input.h` does not already
have, and its only TT-only symbols are address-binding macros. There is no
controller support in this tree to have broken, and `tt/directinput.cpp` is
declined on the roadmap's own instruction. Roadmap Section 12 asks for
controller behavior *where supported*; it is not supported, so this is met by
there being nothing to test rather than by a passing test.

## 13 — client/server map transition

**Not automated, and honestly so.** A map transition is a server telling
connected clients to change level and the clients following; it needs two
processes and a level to change to, which puts it outside a check that runs
before anything is opened.

An in-process check of the server half — `cGameData::Rotate_Map`, which decides
which map comes next and when a non-looping cycle is over — was written and then
withdrawn. `cGameData`'s constructor ends with

```cpp
SettingsDescription = TRANSLATE(IDS_SERVER_SAVELOAD_CUSTOM_DEFAULT);
```

so building one asks the string table for text. The string table is loaded in
`Init` (`init.cpp:932`, `STRINGS.TDB` out of `always.dat`), which a check running
before `Init` has not reached, so `TranslateDBClass::Get_String` asserts. Rather
than load game data into a check whose whole value is that it needs none, or
weaken the engine's constructor to suit a test, the check was dropped.

The manual step, to be run against a real installation:

1. Start `renegadeserver.exe --gamedir <Renegade install>` with a map cycle of at
   least two maps in `server.ini`.
2. Connect a client with `renegade.exe --gamedir <Renegade install>`.
3. End the map (`win` on the server console, or let the time limit run out).
4. The server rotates to the next map in the cycle and the client follows into
   it without being dropped.
5. With `DoMapsLoop` off, the server declares the cycle over after the last map
   rather than restarting.

Note for anyone running the binaries by hand: without `--gamedir` the working
directory becomes the executable's own, which in a build tree has no `Data`
directory, and the first string lookup asserts in
`Code/wwtranslatedb/translatedb.h:259`. That is the missing data, not a fault.

## What a failure looks like

Each check names what is wrong rather than reporting a count. The first run of
the catalog checks found three stock scripts a level can name that are not in the
game — `MS04_Gunboat_Waypath_Movement`, `MXX_Group_Member_DEL` and `Unit_Combat`
— because nine source files under `Code/Scripts` have never been in
`SCRIPTS_SRC`. They do not compile: they are written against a script interface
that never shipped (`Action_Movement_Goto_Location`, a three-argument
`Start_Timer`, `SOUND_TYPE_DESIGNER`), and `MissionS04.cpp` is truncated
mid-function. That is now written down where the source list is, and
`tools/tt484/script_catalog_manifest.py` reads the list so the manifest and the
registry agree.
