# Awaiting Answers

Only unresolved questions needing the user. Add a `Q-###` as soon as one appears.

---

## Active

- **Q-017** Nothing is constructible yet, and the gap is data rather than code. A
  descriptor becomes buildable by setting `build.buildable` plus a `game_object_preset`
  naming the `BuildingGameObjDef` preset the runtime building is created from (so a
  Commander-built Power Plant is a real `PowerPlantGameObj`, not a bare controller wearing
  a power-plant model). Those preset names live in `always.dat`'s definition data, which is
  not in this repo, so guessing them would produce a descriptor that loads and then refuses
  at placement. Needs either the exact preset names for Power Plant / Communications Center
  / Advanced Communications Center / Tiberium Silo, or one run of the existing
  `DefinitionMgrClass::List_Available_Definitions` console output to read them off. Not
  blocking any other work; `WIP_features.md`'s P38 entry points here.

- **Q-006** P08's acceptance -- that repeated map load and unload keeps what was
 retained and leaks nothing -- is proved as arithmetic by `asset_residency` but not
 against real game data, and it cannot be automated here: the client takes no map
 argument, so reaching a second level means driving the menus. Needs one manual run:
 `renegade --gamedir "C:\Westwood\Renegade_full"`, start a level, change to another,
 and report the `WWDEBUG` `Log_Report` lines. Expected: permanent texture count
 unchanged across the change, live prototype count dropping back. Not blocking any
 other work.


- **Q-012** `fds_shader_particles` failed once with exit code 3 and no output, while
 `shader_particles` passed on the client with the same code. The check's scratch arrays
 were heap-allocated and never freed and are now static, which is the suspected cause, but
 it has not been rerun since -- that session was asked not to run the game, and neither has
 any session since. P23 through P35 have each added a check family that has never been run at
 all, for the same reason: `shader_weather`, `shader_tracers`, `shader_debris`,
 `shader_attachments`, `terrain_shroud`, `shader_radar`, `shader_overlays`, `shader_guardai`,
 `shader_telemetry`, `shader_debugrender`, `shader_buildingjson`, `shader_powergrid`,
 `shader_commandermode` (each with an `fds_` server-side twin). Needs one run covering all
 fourteen:
 `ctest -C Debug -R "shader_weather|shader_particles|shader_tracers|shader_debris|shader_attachments|terrain_shroud|shader_radar|shader_overlays|shader_guardai|shader_telemetry|shader_debugrender|shader_buildingjson|shader_powergrid|shader_commandermode" --output-on-failure`
 from `build/win32`.
 If `fds_shader_particles` still fails, the abort is in `WorldParticleManager` teardown rather
 than the check. Not blocking any other work, but thirteen phases are now resting on checks
 nobody has executed. Several gaps no automated check can close, all needing a person in game:
 P25's `DebrisManager` (the real-collision fragment tier) needs a live combat scene, so
 destroying a building and a bridge span and reporting `debris_status` is the only way to see it
 run; P26's attachment layer has every *success* path unverified for want of real `.w3d` art;
 P28's HUD radar refactor needs a look to confirm it still draws the same blips it always did;
 P32's `building_footprints` debug layer needs `debug_layer building_footprints 1` and a
 building on screen; P35's Commander Mode needs `commander_mode 1`/`commander_mode 0` and a
 death while active, and its `Pick_At_Cursor` was deliberately left untested even device-less
 (see `docs/zerohour/CommanderModeController.md`). P29/P30 have no live-match check
 possible yet at all -- each is documented in `WIP_features.md` as waiting on a real GameObj
 wiring step (P30/P39's Commander AI production) rather than a manual verification. P27/P33/P34's
 own wiring gap (`BuildingGameObj` `DescriptorName`, `BaseControllerClass` integration) closed in
 a later pass -- see `docs/zerohour/TeamPowerGridService.md` -- but still has no live-match check
 possible until a preset actually sets `DescriptorName`, which is authoring, not code. That same
 pass added two more manual-only items, neither with a new ctest family: `debug_layer
 radar_contacts 1` and `debug_layer shroud_cells 1` (P32) need a look in game the same way
 `building_footprints` already did, and `telemetry_status`/`telemetry_dump` (P31) need a kill and
 a building destruction to confirm the counts move.

 **Update, a later pass.** Phases 36-40 added five more check families that have also never been
 run, for the same standing reason: `shader_sidebar`, `shader_placement`, `shader_construction`,
 `shader_production`, `shader_rtssaveload` (each with an `fds_` twin). The single run above should
 therefore be:
 `ctest -C Debug -R "shader_|fds_shader_|terrain_shroud" --output-on-failure` from `build/win32`,
 which covers all nineteen families at once rather than a hand-maintained list. These five are
 device-less and self-contained (no file I/O, no scene, no device), so a failure in one of them is
 a real defect rather than an environment problem.

 **Update, a third pass.** Phases 46-50 added three more: `shader_visualsettings`,
 `shader_skyclock`, `shader_lightcurves` (each with an `fds_` twin), bringing the total to
 twenty-two -- still covered by the same `"shader_|fds_shader_|terrain_shroud"` pattern above, no
 change needed to the command. Also still needing a person in game, with no ctest family possible:
 P47's DistLOD/terrain LOD dissolve and P48's moving sun/P49's relighting both need eyes on a running
 client over real time (`docs/zerohour/VisualFidelityProgram.md` states plainly that nothing in that
 doc has been seen on screen), and P50's building-power light response needs a fixture actually
 authored with its `Group ID` set to a building's `Get_ID()`, which nothing in this codebase does yet
 (`docs/assets/VisualFidelity.md`).

 **Update, a fourth pass.** Phases 51-55 added three more: `shader_heightfog`, `shader_skyocclusion`,
 `shader_skydriventoggles` (each with an `fds_` twin), bringing the total to twenty-five -- still
 covered by the same `"shader_|fds_shader_|terrain_shroud"` pattern, no change needed to the command.
 Also still needing a person in game, with no ctest family possible: P51's height fog, P52's terrain/
 contact occlusion, P53's cloud ground-shadows and P54's metal reflections all need eyes on a running
 client the same way P47-49 already do, and P55's sun shafts need the same plus a look specifically at
 whether the streak-sprite approximation (`docs/zerohour/VisualFidelityProgram.md`'s P55 section)
 reads acceptably against a real occluder before deciding whether the P56/57 post-process follow-on is
 worth doing sooner rather than later.


 **Update, a fifth pass.** Phases 56-65 added nine more: `shader_tonemap`, `shader_bloom`,
 `shader_windfield`, `shader_wetness`, `shader_damagetint`, `shader_weaponkick`,
 `shader_worldmetrics`, `shader_escarpment`, `shader_traversability` (each with an `fds_` twin),
 bringing the total to thirty-four -- still covered by the same `"shader_|fds_shader_|terrain_shroud"`
 pattern, no change needed to the command. This pass also **closes** two of Q-012's older
 manual-only items by making them testable arithmetic rather than by running anything: P55's
 streak-sprite tradeoff no longer needs a judgement call, because the streak sprites are gone and
 the shafts are a real radial blur (`shader_bloom` proves the weights). What is new and needs a
 person in game: P57's bloom composite and P55's reworked shafts both switch render targets
 mid-frame, which has no precedent inside an already-rendering frame anywhere in this tree, and is
 the single highest-risk thing in either of the two programs -- both are off by default and should
 be turned on one at a time with eyes on the screen. Also still manual-only, for the same reasons
 P47-P54 already are: P56's adaptation reading correctly when walking indoors, P58's sway and
 drift agreeing on a direction, P59's wet ground and wet vehicles, P61's soot ramp and night
 embers, and P62's kick feeling right rather than merely measuring right. Nothing in
 `docs/zerohour/WorldStructureProgram.md` has been run against a real level either: no graph has
 been built over real terrain and no escarpment exists in any level to look at.

 **Update, a sixth pass.** The Commander pointer (`completed_features.md`, "P35/P36/P37-E") adds no
 new ctest family -- every part of it needs a cursor, a camera and a physics scene, none of which
 `-shaderselfcheck` has -- so it is entirely manual, and it also finally makes several older
 Commander items checkable in one sitting rather than by typing console commands. One run covers
 all of it: `commander_mode 1`, then confirm the arrow appears and the soldier neither turns nor
 fires while it is up; click the four mode tabs and the page arrows; click a unit card at a bound
 facility and confirm a unit is queued; click a building card, move the pointer over legal and
 illegal ground and confirm the ghost follows the cursor and its red/green verdict tracks what is
 underneath; left click to place and right click to cancel; open and close the in-game menu while
 still in Commander Mode and confirm the pointer survives it; `commander_mode 0` and confirm FPS
 controls come back exactly as they were. That single pass is also the live-scene exercise
 `Pick_At_Cursor` and the placement world-query branches (overlap/slope/bounds/exit clearance) have
 never had -- see this file's P35/P36/P37 entries in `WIP_features.md`.

- **Q-013 (resolved by decision, reversible)** Roadmap Section 39 asks for `nlohmann-json`
 "through the existing vcpkg/CMake dependency system." This machine has no `VCPKG_ROOT` and no
 vcpkg installation at all, and the `build/win32` tree `b.bat` actually builds has no vcpkg
 toolchain wired into it (`vcpkg.json` at the repo root lists unrelated dependencies --
 ffmpeg/icu/openal/sdl3/freetype/qt -- for a preset this build does not use). Bootstrapping
 vcpkg from network mid-session was judged disproportionate to one dependency. Taken instead:
 the standard alternative distribution, a single vendored header,
 `Code/wwlib/nlohmann/json.hpp`, pinned to the tagged release v3.11.3 and fetched directly from
 `raw.githubusercontent.com` (recorded here since it did not go through vcpkg or any package
 manager). `#include "nlohmann/json.hpp"` resolves because `wwcommon`'s `INTERFACE` include
 directories already expose `wwlib` to every target. Say the word and this moves to real vcpkg
 once `VCPKG_ROOT` exists on the build machine; nothing about `BuildingDescriptorManager`'s own
 code would need to change, only the include path and the CMake wiring.

## Resolved

- **Q-014 (resolved by decision)** Answered: the wallet is capped. Base 10,000;
  each Tiberium Silo adds +5,000 to the cap. A Silo destroyed reduces the cap and wipes
  any credits above the new ceiling. Implemented as
  `TeamPowerGridService::Get_Wallet_Capacity`/`Enforce_Wallet_Cap` plus
  `PlayerDataClass::Apply_Wallet_Cap`; base retunable with the `wallet_cap` console
  command. See `completed_features.md` P34.

- **Q-015 (resolved by decision)** Answered: reading (b) -- the Phase 42 title is literal.
  The `-shaderselfcheck` families satisfy the test-strategy half, and Phase 42 additionally
  required editor-facing descriptor validation reusing the existing parser/validator rather
  than duplicating it. Implemented as `BuildingDescriptorManager::Validate_Text`/
  `Validate_File`/`Validate_Directory` plus LevelEdit's **Report -> Validate Building
  Descriptors...** command; P42 is now closed. See `completed_features.md` P42.

- **Q-016 (resolved by decision)** Answered: option (a). Runtime-constructed
  buildings get a second `BuildingGameObj` component collection path deriving structure,
  bounds, physical components and named anchors from the attached W3D hierarchy, leaving
  the map-authored proximity/prefix path unaltered. Damage visuals for them are
  data-driven through `BuildingModelDescriptor` with explicit intact/damaged/destroyed
  models and optional extra health-threshold states; a model transition rebuilds the
  state-dependent collision/physical components while preserving the logical
  `BuildingGameObj`; MCT/PCT, production, repair and targeting are disabled once destroyed.
  Existing map-authored buildings keep their legacy aggregate behaviour unchanged.
  Implemented; see `completed_features.md` P38.

- **Q-007 (resolved by decision, reversible)** What a player is told before a
  picture of their game window leaves their machine. P05 held `ssurl`/`sshot`
  back as a policy question; it is answered and implemented. The capture is the
  game window and there is no path that captures anything else; the player is
  always told, naming the destination host, before the upload starts; the URL is
  held in memory only, so the feature is off after every restart until an operator
  turns it on; and every request is answered, refusals and failures included. No
  client-side opt-out was added, on the grounds that a silent refusal removes the
  only reason the feature exists while leaving the appearance of it. Say the word
  and any of the four moves. See `docs/tt484/RemoteScreenshots.md`.

- **Q-011 (resolved by decision, reversible)** Where the tech level and the
 mine limit live. Taken as recommended: both are `SSGMSettingsClass`
 settings read from `ssgm.ini`, which is where 4.8.4 kept them. The tech
 level splits in two -- the starting value is the setting, the running
 value is `PurchaseAvailabilityClass::TechLevel`, because a script moves
 it during a match and every client's menu has to follow. Say the word and
 it moves.

- **Q-010** Team defaulting. User directed using TT's handling, so the removal
  stands: the stock swap-the-scores path that handed a win to the losing team
  when the leading team's last player disconnected is gone for good.

- **Q-001** TT tree was named `tt_4.9.4/` but roadmap pins 4.8.4. User renamed it to
  `tt_4.8.4/`; `scripts/engine_common.h:18` confirms `TT_VERSION 4.8f`.
- **Q-002** ~93 untracked Commander/RTS files from the superseded pre-v1.1.0 roadmap,
  wired into no CMakeLists. User deleted them; P33-P39 start fresh from the v1.1.0
  spec. They existed in no commit and are not recoverable.
- **Q-003** Roadmap directive 0.2 pins OpenW3D `f6f9db43...`, absent from this clone.
  User directed using the present codebase, so the baseline is `b970f592` — a
  recorded, authorized deviation that all provenance must cite.
- **Q-004** Phase 0 runtime matrix not run. User accepted Phase 0 as build-only, so
  P00-BUILD closed and the runtime items stayed in `unstarted_features.md`. Premise
  later corrected: assets do exist, at `C:\Westwood\Renegade_full`, not `Run/` — so
  those items are unblocked, not blocked. Its shipped binaries are out of scope.
- **Q-005** Phase 1 output, project state and the donor trees were all untracked.
 User directed committing everything locally (no remote is under our control yet).
 Done in four commits on `main`: `ce4c3b75` analysis + state files, `cdd1ed2a` TT
 4.8.4 tree and the pinned `GeneralsGameCode` submodule, `0a191e58` the standalone
 `Code/` side projects, `41164d71` the ~900 MB `models/` art. Local agent/tool
 caches are gitignored instead.
