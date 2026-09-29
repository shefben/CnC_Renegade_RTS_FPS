# Awaiting Answers

Only unresolved questions needing the user. Add a `Q-###` as soon as one appears.

---

## Active

- **Q-020** Hybrid rewrite I12 (runtime Phase 11). The I12 gate requires `surface_pass`, `road_cut` and
 `ramp_pass` each to execute in a real-pipeline fixture. Under the supported native configuration none of
 the three can, and the reasons follow from the plan's own constants and from D-004, not from a missing
 seed. Nothing has been weakened to force them. Measured over 200 seeds of each of the seven archetypes
 (`renegade -terrainselfcheck hybridinteractionsearch`): 1400 attempts, 135 worlds through Phase 10, 67
 solved, and 0 of each of the three methods selected.

 **`road_cut` is impossible on every native explicit cliff.** Section 5 gives a cliff
 `height difference = tier_step * (1.00 .. 2.00)` = 18..36 at the native `tier_step = 18`, against
 `grading.max_cut_depth = grading.max_fill_height = 6`. D-004 realized a cliff as a compactly supported
 *raised band* -- zero at the talus toe, full height at the upper edge, one talus width of shelf, back to
 zero across one more -- so both approach ends of a station stand on the same bench (measured: approaches
 0 and 0, face 31.68). A road cut is the straight profile between the two approach elevations, which must
 be `<= bench + 6` at both ends and `>= bench + H - 6` on the shelf between them; a straight line cannot
 rise and fall, so this needs `H <= 12`, and every cliff is `H >= 18`. Even a true step would need
 `H <= 6 + 6 + (2e + t) * tan(12 deg) <= 15.2`. Phase 6 now measures the cut per class (`CutClasses`);
 no cliff window in the census has a class for which a cut fits.

 **`ramp_pass` is built for the wrong shape.** The shared local-pass builder's cliff ramp is one diagonal
 between the two approach elevations, which describes a *step*. Under D-004 both approaches stand on the
 same bench, so that line is flat and runs straight under a band 18..36 high; it is refused on cut/fill at
 every cliff station (the gate's two solving worlds: 45 and 57 `cut_fill` refusals, 24 and 24
 `cross_slope`). A surface crossing of a band would be ascend-shelf-descend -- the canyon floor pass
 inverted, still one ascent and one descent and no switchback -- and its shelf is `TalusWidth = 1W..3W`
 = 3.3..10.0 against the `swept + 2 * clearance = 8.80` continuous floor the heavy envelope needs, so it
 could only ever work on the widest talus.

 With no cut, no ramp and -- in the self-check's content registry -- no tunnel module, **every explicit
 cliff is a sealed wall with no window at all** (census: 0 cliff windows in 135 worlds). In game the
 tunnel answer comes from `TunnelSystem`'s runtime registry, so a registered portal module would reopen
 cliff windows for tunnels only. That is the honest consequence of
 the above; before this item's fixes, cliff windows survived on a bridge bit no cliff method uses and on a
 tunnel promise the content cannot keep, and 55 routed cliff crossings ended in exhausted reroutes.

 **`surface_pass` cannot follow a legitimate bridge refusal.** Canyon/ravine order is bridge first. The
 widest canyon Section 5 allows spans `2 * rim_half_width <= 2 * (3W + 8W) = 22W = 73.1` (widest
 measured: 63.9); the registered bridge resolves to a 200 maximum span and a 40 clear width, against a
 largest Section-12.4 requirement of 15.8 (`HIGHWAY_4`'s carriageway). So `SPAN_TOO_LONG` and
 `WIDTH_TOO_NARROW` cannot occur, and every other bridge refusal is a station hard-validity rule the pass
 shares. Census: no routed canyon or ravine span is pass-capable and not bridge-capable. The pass geometry
 itself works -- `canyon_crossroads` seed 31337 carries two pass-capable canyon windows.

 Blocks: closing I12's coverage gate, and so I13. Evidence checked: Section 5's landform table, Section
 10.2, Section 8 Phase 11's road-x-cliff and road-x-canyon orders, D-004, the content fixture's bridge and
 tunnel registry, the Phase-6 per-class cut measurement, and the full seed census. Smallest decision
 needed, per branch:

 - `road_cut`: (a) accept it as unreachable on native explicit cliffs, proved above, with the compile path
   exercised by an owner-level test on a synthetic low cliff; or (b) change a constant -- the cliff height
   range, `tier_step`, or the grading limits.
 - `surface_pass`: (a) accept it as unreachable behind the native bridge registry, covered the same way;
   or (b) require a content configuration whose bridge cannot span every canyon.
 - `ramp_pass`: (a) generalize the cliff ramp to D-004's band (ascend, shelf, descend -- the floor pass
   inverted, validated identically); (b) revise D-004 so a cliff is a step between an upper and a lower
   bench, as Section 10.2's `upper-side sign` suggests, which changes Phases 5, 6 and 7; or (c) accept it
   as unreachable and cover it at owner level.

- **Q-019 CLOSED (2026-09-29, option (a)).** Section 5's canyon/ravine bridge quota is Phase 5's to
  satisfy: it lays the required systems across a heavy route corridor rather than leaving Phase 9 with
  nothing bindable. Implemented, with four owner-level defects fixed alongside it (landform `StableId`
  ownership, the line-feature control frame, the `SURFACE_SADDLE` crossing obligation, and
  `highland_pass`'s wrongly tagged alternate guide). See `DECISIONS.md` D-011. Full suite 233/233.

- **Q-018** Hybrid rewrite I6 (runtime Phase 5, vector landforms). At LARGE the plan's hill
 target -- `10 + RoundHalfUp(terrain.hill_density * 6)`, so 11 hills at the native
 `hill_density = 0.15` -- cannot be satisfied together with rejection rule 6's "overlap area of
 two accepted independent features of the same type must be <= 25% of the smaller polygon" and
 the `BASE_PROTECTED` exclusion, because the same section draws each hill radius from
 `0.08D..0.16D`, which is 192..384 world units at `D = 2400`. Evidence: an exhaustive search of
 every candidate cell that can hold the feature (3180 cells, so not a retry-budget shortfall)
 refuses the eleventh hill on the 25% cap; measured over twelve seeds, 6/12 LARGE seeds fail
 Phase 5 the same way, and the two rules are individually correct, so no source, donor or runtime
 data resolves which one yields. Blocks: closing I6's gate at LARGE, and the D-002/D-003 native
 visual run, whose profile is `2400 x 2400`. Phase 5 currently reports the named attempt failure
 Section 5 prescribes, the self-check requires success at MEDIUM and tolerates the LARGE failure,
 and nothing half-built is ever kept. Smallest decision needed: which of these the user wants --
 (a) scale hill radius by size class, for example `0.05D..0.10D` at LARGE; (b) exempt hills from
 rule 6 and cap them by a total-area budget instead; (c) reduce the LARGE hill base count; or
 (d) accept the attempt-failure rate and let whole-world retries carry it.

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
 **Automated half answered at V2 close (2026-09-23).** Every check listed here now runs in the
 full suite: 197 entries, 193 passing, `fds_shader_particles` included. `terrain_shroud`,
 `shader_overlays`, `shader_guardai`, `shader_debugrender` and `shader_powergrid` failed on their
 first real runs; each was diagnosed and fixed (see `completed_features.md`, V2 Phase 30). Only
 `shader_commandermode` (a sentinel pointer the engine has dereferenced since P42) and
 `shader_visualsettings` (needs a D3D device) still fail; root causes are in
 `docs/procedural_world/V2Completion.md`. **Both were fixed in D-001 priority 1; every listed check
 now passes (201/201).** What follows -- the in-game checks no automated run can
 close -- is still open.
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

 **Resolved for three of the five, V2 Phase 28.** `shader_sidebar`, `shader_placement` and
 `shader_rtssaveload` (+ their `fds_` twins) were finally run and were indeed failing for a real
 reason: `shaderselfcheck.cpp`'s own `SIDEBAR_PLANT_JSON`/`SIDEBAR_FACTORY_JSON` fixtures were
 missing `game_object_preset`. Fixed; all six now pass. See
 `docs/integration/Phase28IntegrationGate.md`. The other two, `shader_construction`/
 `shader_production` (+ `fds_` twins), were also run and are also real defects, but a *different*
 one each (construction: footprint half-width/half-depth reading as uninitialized garbage;
 production: one completion-id mismatch) -- left unfixed as out of Phase 28's own multiplayer/
 save-load/late-join scope; still open, still needing their own focused investigation.

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

**Update, a seventh pass.** Two more device-less families exist and are registered for the first
time: `shader_runtimedoors` (the check itself was written during the door-service pass, but never
added to `Code/Commando/CMakeLists.txt`'s `add_test` list until now -- a gap in that earlier
session, not a new check) and `shader_worldprofile` (V2 roadmap Phase 1 -- world profile schema,
deterministic seed derivation, generation manifest; `docs/procedural_world/GeneratorDeterminism.md`
lists exactly what it proves). Both have `fds_` twins. Neither has been run, same standing reason
as every other family here; both are covered by the same `"shader_|fds_shader_|terrain_shroud"`
sweep, bringing the total to 36 (was 34 as of the fifth pass; `shader_runtimedoors` was never
counted there because it was never registered, `shader_worldprofile` is new this pass).

**Update, an eighth pass.** `shader_worldbounds` (V2 roadmap Phase 2 -- the authoritative finite
world boundary) added, with its `fds_` twin, bringing the total to 37; still the same
`"shader_|fds_shader_|terrain_shroud"` sweep. What it proves and what it cannot is listed in
`docs/procedural_world/WorldBoundsAndBoundaryCliffs.md`. New manual-only items, none with a ctest
family possible, all needing a live physics scene: infantry, a ground vehicle and an aircraft each
failing to cross the perimeter; the Commander camera refusing to pan outside it; a deliberately
teleported actor being restored by a live server; and `debug_layer world_bounds 1` drawing the
band rings. All six are one sitting in a procedural world, which cannot exist until Phase 3+ puts
terrain in one -- so this is genuinely blocked on later v2 phases rather than merely unrun.

**Update, a ninth pass.** `shader_worldcliffs` (V2 roadmap Phase 3 -- the procedural boundary
cliff ring) added, with its `fds_` twin, bringing the total to 38; still the same
`"shader_|fds_shader_|terrain_shroud"` sweep, and `ctest -N` now lists 135. What it proves and
what it cannot is listed in `docs/procedural_world/WorldBoundsAndBoundaryCliffs.md`.

The eighth pass said the six manual boundary items were blocked until a procedural world could
exist. **That block is now lifted**: the "Procedural Skirmish" button on the main menu creates
one over a stock skirmish map, so all six of those items plus this pass's new ones are reachable
in a single sitting. They are no longer blocked on later phases, only unrun.

New manual-only items, all needing a display: whether the cliff ring renders at all and whether
it renders as rock rather than untextured (the default `TERRAIN_LAYER_CLIFF` layer names no
texture, the same gap P13 carries); whether a vehicle can crest it; whether the toe apron reads
as scree; whether `debug_layer boundary_cliffs 1` shows any red riser, which would mean a
climbable station; and whether the menu button's whole path -- request, skirmish map choice,
fit-to-level against a real map's extents, generation inside `Post_Load_Level` -- works at all.
It has never been clicked.

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

- **Q-017 (resolved from installed data)** Answered: real presets resolved by enumerating
  `C:\Westwood\Renegade_full\Data\Objects.DDB` in full through a new permanent LevelEdit CLI
  mode, not guessed. GDI/Nod Power Plant and Communications Center use the real stock
  `mp_GDI_*`/`mp_Nod_*` presets (`PowerPlantGameObjDef`/`ComCenterGameObjDef`); no stock
  Advanced Communications Center or Tiberium Silo exists, so both are project-owned
  definitions constructed in code. All 8 descriptors are now `build.buildable`. See
  `completed_features.md`'s P38 addendum and `docs/zerohour/BuildingPresets.md`.

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
