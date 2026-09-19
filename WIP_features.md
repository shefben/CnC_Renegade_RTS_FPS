# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P08 -- one scope left, and a check only a person can run

`PERMANENT`, `GAME_MODE` and `WORLD` all have callers now (see
`docs/zerohour/AssetResidency.md`, "What is left"). What remains of P08, still
absorbing the backlog's acceptance line *Repeated map/world load/unload does not
invalidate retained assets or leak unbounded resources*: `SECTOR` has no consumer
until something streams part of a level, which belongs to the terrain work, and
materials and generated world buffers have record kinds and no registration site for
the same reason. No prototype is claimed permanently, deliberately, until a profile
says which ones are shared.

The acceptance condition is proved as arithmetic by `asset_residency` -- sixteen
cycles leak nothing, and a texture-only permanent scope retains a record while naming
nothing -- but not against a real map. That check cannot be automated here: the client
takes no map argument, so reaching a second level means driving the menus. Next exact
action is a manual one for the user: run
`renegade --gamedir "C:\Westwood\Renegade_full"`, start a level, change to another,
and read the `WWDEBUG` `Log_Report` line, confirming the permanent texture count is
unchanged across the change and the live prototype count drops back.

---

## P09 -- eight pipelines drawing, five with nothing to draw

`ShaderManagerClass` and eight programs are in and checked (P09-A, P09-B and P09-C in
`completed_features.md`, `docs/zerohour/ShaderManager.md`, four green shader ctest entries).
Five of Section 15's pipelines are left enumerated and unregistered, and this entry carries
them: **terrain detail, particles, tracers and beams, status markers, ghost building tint**.
Terrain detail is the one that looks ready and is not -- it needs a second UV rate on one
layer, which the terrain pass cannot carry, and there is no detail entry in the layer table;
the other four arrive with Sections 26, 27 and the Commander work. This entry also still
carries the backlog line *stock-asset compatibility: existing Renegade W3D materials,
shaders and texture formats render unchanged through the new layer*, which is structural
today (`MATERIAL_PROGRAM_LEGACY_W3D` has nothing to say about a stock model) and unseen.

Nothing here has been on a screen: the checks run without a device, so whether terrain,
water, foliage, the roads and the shadows still look the way they looked before they were
moved onto the layer wants one run with a display. Next exact action is that manual run,
folded into the checks already recorded against P11 and P13 to P20 -- there is no separate
console command for this, because the layer has no appearance of its own.

One deliberate omission, recorded so it is not mistaken for a miss: `SurfaceRibbonSystem`
and `WorldSurfaceMarkManager` each keep a private shader function of the shape the six new
pipelines gave up, and are near enough identical to each other to be a real duplicate.
Section 15 enumerates no decal pipeline and the program enum is a contract, so adding one
belongs to whichever section owns decals.

---

## P10 -- the service is there, the later consumers are not

`WorldSpatialIndex` and the consumers that existed are done (see `completed_features.md`
P10-A and P10-B, and `docs/zerohour/WorldSpatialIndex.md`). Two backlog lines are left in
`unstarted_features.md` rather than here because neither is blocked on this phase: the
second consumer line (Commander placement, nearby lights, foliage cells, roads/bridges/
water, procedural generation) waits on systems that do not exist, and the acceptance line
is structural today and unmeasured. Next exact action: nothing, until one of those systems
lands and calls `Query_Placement_Overlap` or `Query_Nearby_Lights` for real.

---

## P11 -- built and unwalked

The service, the heightfield and the collision are all in (P11-A and P11-B in
`completed_features.md`; `docs/zerohour/WorldTerrainSystem.md`). This entry still carries
the phase's acceptance line, *FPS and vehicle gameplay works on runtime-created heightfield
terrain while arbitrary W3D geometry remains supported*: the machinery is there and nobody
has stood on it, because the checks run device-less. Next exact action is a manual one for
the user: run `renegade --gamedir "C:\Westwood\Renegade_full"`, load a level, then
`terrain_test` at the console, and report whether a soldier walks on the generated ground
and whether the level's own W3D geometry still behaves.

---

## P14 -- generated, unseen

`RoadSplineClass` and `RoadSystem` are in and checked (P14-A in `completed_features.md`,
`docs/zerohour/RoadSystem.md`, fourteen green terrain ctest entries). What this entry carries out
of the absorbed acceptance line is that no screen has shown a road: the checks run device-less and
the default roads name no texture. Next exact action is a manual one for the user: run
`renegade --gamedir "C:\Westwood\Renegade_full"`, load a level, then `road_test` at the console
-- it works on a stock level's own floor, and takes an optional texture name -- and report whether
the S bend, the crossing and the tee look like roads.

---

## P13 -- decided, undrawn

`TerrainTextureSystem` is in and checked (P13-A in `completed_features.md`,
`docs/zerohour/TerrainTextureSystem.md`, twelve green terrain ctest entries). What this
entry carries out of the absorbed acceptance line is the half no rule can supply: the
default layers name no textures, so a generated map has coherent material *selection* and
nothing to put on the ground, and detail, biome and scorch have kinds and no entries.
Next exact action: point the six default layers at stock Renegade texture names as a
stand-in (`docs/assets/TerrainLayers.md` says what each layer has to be, what the purpose-made
`ow_ter_*.tga` replacements are, and why the stock ones are a stand-in), then run
`terrain_test` and `terrain_dress` in a client and look at it.

---

## Carried forward out of P05, P06 and P07

P05, P06 and P07 are all closed (see `completed_features.md`, and
`docs/tt484/TTPhase5Audit.md`, `docs/tt484/TTPhase6Gate.md` and the four
`docs/zerohour/` documents). Of the five things they named for later work, three
are done -- `ssurl`/`sshot`, `mapch` and `tag`, all in `completed_features.md`
under P05-Z -- as is the `cGameData` constructor fault they recorded rather than
fixed. Two are left, neither of them blocked on code: the **client/server map
transition** manual step, which stays manual until there is a harness that can
run two processes against real game data, and `NavalFactoryGameObj`, which
remains reassigned to Zero Hour Feature 7.

The remote screenshot pair carries one thing that is done but unexercised: it has
never run against a real client and a real endpoint, which needs two processes and
a web server. Same shape of manual check as the map transition one.

Three capabilities in the port matrix carry an `EAValidation` note, meaning EA's
archival tree must actually be consulted before they are implemented: ZH-04 if
the community's `Cast_Ray` fixes conflict, ZH-18 before adopting the
`Drawable`/`Locomotor` merge (`4990ca933`), and ZH-24 when a pathfinder change
alters retail behaviour. EA's repository is not cloned here; only its head SHA
is recorded.

---

P01 through P07 are complete (see `completed_features.md`). The backlog is in
`unstarted_features.md`.

---

## Working notes that cost a session each

Assembling a generated `.cpp` from parts: write the parts with the Write tool
and concatenate them with Python opened `newline=''`. `cat a b > c` in this
shell silently drops blank lines and any line carrying an inline
`/*comment*/`, which produced a file that looked like a compiler bug. Bash
heredocs are worse: a `<<'PY'` heredoc carrying a Python script silently
mangled a TSV, deleting fifty-one lines it never touched. Put any non-trivial
script in a file with the Write tool and run the file. A heredoc is fine for a
short `str.replace` patch, but check it actually replaced -- one printed `ok`
having matched nothing -- and never put `\n` inside a heredoc'd Python string
meant to produce a C escape: it produces a real newline and the compiler
reports a newline in a constant. Indentation is the other heredoc trap: this
tree indents with tabs, a patch anchor typed with spaces never matches, and the
fastest recovery is to locate the line by `index` and splice rather than to
keep re-typing the anchor.

`re.compile` returns the *same object* for two identical pattern strings, so
never decide what a match means by comparing pattern identity. That cost a
debugging round in `script_catalog_manifest.py`.

`git log` in this shell truncates to 50 commits when its output is piped unless
`-n` is given explicitly. `git rev-list --count HEAD` disagreeing with
`git log | wc -l` is that, not a shallow clone. The donor has 1712 commits.

`ls` here prints a size column, so `ls dir | grep -E '\.cpp$'` matches nothing.
Pipe through `sed 's/ .*//'` first.

`grep -r` over the repository root now walks the `GeneralsGameCode/` submodule
and times out. Give it a path under `Code/`, or use the Grep tool, which is
indexed.

Two builds cannot run at once: the second dies with
`LINK : fatal error LNK1104: cannot open file 'Debug\renegade.ilk'`, which
looks like a corrupt object file and is not one. Let a backgrounded build
finish before starting another. `b.bat` builds one target, so `renegade` and
`renegadeserver` are two separate invocations, and a ctest entry that runs the
one you did not rebuild fails against a stale binary.

`docs/tt484/TTScriptApiGap.tsv` is generated by `tools/tt484/apigap_tsv.py`,
which writes the file itself -- do not redirect its stdout over it. Anything
resolved by a seam, by a rename, or by a decision not to add a command must be
written into `TTScriptApiRenames.tsv`, or the next regeneration reverts it.
`Code/Scripts/tests/ScriptCatalogManifest.h` is likewise generated, by
`tools/tt484/script_catalog_manifest.py`; regenerate it whenever a script is
added or removed or the catalog tests fail on a count. The four
`docs/zerohour/*.tsv` files are generated by the five tools in
`tools/zerohour/`; regenerate them if the donor submodule pin moves.
`docs/RejectedItems.md` is generated by `tools/rejected_items.py` -- re-run it
after any pass that declines something, and put an in-code rejection into that
tool's `PHASE_DECISIONS`, not into the document.

`b.bat` must be invoked through the PowerShell tool
(`cmd /c "...\b.bat renegade"`); through Bash the argument never reaches the
batch file, and the PowerShell tool's working directory persists between calls,
so `cd` back to the repository root before invoking it. The `vswhere.exe`
warning it prints first is harmless. Reconfiguring must be
`cmake -S . -B build/win32` with no `-A Win32`, which the existing cache
rejects.

`VectorClass` compares its elements, so any struct stored in a
`DynamicVectorClass` needs `operator ==` and `operator !=` or the error arrives
from inside `vector.h` naming `const T`. `StringClass` has no `str()` --
`Peek_Buffer()` is the accessor -- and `HashTemplateClass::Get(key, value&)` is
the lookup that reports whether it found anything.

A self check that runs before `Init` must not touch anything the engine loads
during `Init` -- the string table above all. A modal `WWDebug_Assert_Fail` box
in a ctest run hangs the suite until somebody clicks it.

---

## Reference: pinned baselines (not a lifecycle entry)

OpenW3D `b970f592282f89c7a4a2f86d54c42c1dafe7c388` on `main` (user-authorized
deviation from directive 0.2). TT donor `tt_4.8.4/`. Zero Hour donor submodule
`9fee97a1792efc50edfe27300581970b4c022951`, `Core` + `GeneralsMD` only, frozen
in `docs/zerohour/ZeroHourDonorBaseline.md`; EA's archival reference is
`0a05454d8574207440a5fb15241b98ad0b435590`, recorded but not cloned. Canonical
build `build/win32` (x86); `build/win` (x64) secondary. Game data at
`C:\Westwood\Renegade_full` -- assets only, its binaries are out of scope. Run
a built binary by hand with `--gamedir "C:\Westwood\Renegade_full"`, or the
working directory becomes the build tree, which has no `Data`, and the first
string lookup asserts. Builds must run through `vcvarsall.bat x86`; this
machine's system-wide `INCLUDE` points at Visual Studio 6 and a plain shell
compiles against the VC98 CRT, whose errors look like broken source rather than
a bad environment.

Renegade models, maps and textures must keep loading unmodified. Engine work may
augment, fix and extend them; it may never require content to be re-authored or
re-exported.

The tag `tt-native-complete` marks the commit where TT 4.8.4 is natively
implemented and the hard gate passes, before any Zero Hour work.

---

## P15 -- built, unwalked, and nothing damages it

`BridgeSystem` is in and checked (P15-A in `completed_features.md`,
`docs/zerohour/BridgeSystem.md`, sixteen green terrain ctest entries). This entry carries the
absorbed acceptance line *Bridges support variable length, damage, broken spans, collision, and
multiplayer state*: four of the five are proved device-less, but collision is not, because a check
with no physics scene has nothing to put a static object into, and no gameplay damage, path-finder
or packet calls any of it yet. Next exact action is a manual one for the user: run
`renegade --gamedir "C:\Westwood\Renegade_full"`, load a level, then `bridge_test 120` and
`bridge_break` at the console, and report whether a soldier walks onto the deck and falls through
the broken span.

## P16 -- counted, unseen, and nothing plants it

`FoliageSystem` is in and checked (P16-A in `completed_features.md`,
`docs/zerohour/FoliageSystem.md`, eighteen green terrain ctest entries). This entry carries the
absorbed acceptance line *Large forests render with substantially fewer submissions than individual
`RenderObj` instances*: the ratio is proved device-less at about sixteen to one, but no batch has
ever been drawn, nothing reads foliage out of a level, no weapon damages a plant, and nothing calls
`Update_Visibility` per frame to switch levels of detail. Next exact action is a manual one for the
user: run `renegade --gamedir "C:\Westwood\Renegade_full"`, load a level, then
`foliage_test 2000 200 <a tree model name from that level>` at the console, and report the printed
plant-to-batch numbers and whether the wood looks like a wood.

## P17 -- carved, stamped, tagged, and nothing has been seen

`WaterSystem` is in and checked (P17-A in `completed_features.md`, `docs/zerohour/WaterSystem.md`,
twenty green terrain ctest entries). This entry carries the absorbed acceptance line *Water works
visually and physically without a competing collision system*: the physical half is proved by
construction (the surface mesh is tagged into Renegade's existing water surface type rather than
answered by a new system), but no pond has ever been drawn, nothing reads water areas out of a
level, flow direction and scroll speed are stored but drive no per-frame animation, there is no
reflection, and no naval/amphibious unit yet asks `Get_Water_Depth`/`Is_Navigable`. Next exact
action is a manual one for the user: run `renegade --gamedir "C:\Westwood\Renegade_full"`, load a
level, then `water_test 20 <a texture name from that level>` at the console, and report whether the
pond carved into the ground, whether the shoreline looked blended, and whether it drew.

## P18 -- every vehicle is laying tracks and nobody has seen one

`SurfaceRibbonSystem` is in and checked (P18-A in `completed_features.md`,
`docs/zerohour/SurfaceRibbonSystem.md`, twenty-two green terrain ctest entries). This entry carries
the absorbed acceptance line *Vehicles can leave stable marks without unbounded allocations or
per-mark heavyweight objects*: the cost half is proved by construction and by the checks (fixed
pool, fixed ring, one object per kind), but no mark has ever been drawn because no kind names a
texture, only vehicles emit (scorch trails and drag marks are defined and unwired), the harvester
kind has no harvester, and marks do not cull by distance within their mesh. Next exact action is a
manual one for the user: run `renegade --gamedir "C:\Westwood\Renegade_full"`, load a level, then
`ribbon_texture <a texture name from that level>` at the console, drive a vehicle over dirt, and
report whether tracks appear behind it and what `ribbon_status` prints for marks against objects.

## P19 -- the marks are batched and nobody has looked at one

`WorldSurfaceMarkManager` is in and checked (P19-A in `completed_features.md`,
`docs/zerohour/WorldSurfaceMarkManager.md`, twenty-four green terrain ctest entries). This entry
carries the absorbed acceptance line *Hundreds or thousands of marks can exist without one draw call
or one network object per mark, and mark lifetime/eviction cannot grow memory without bound*: the
bounds are proved by construction and by the checks, but no batched mark has ever been drawn on
screen, the seven named kinds have no art, and nothing yet places a CONSTRUCTION, DIRT or
OIL_OR_STAIN mark. Next exact action is a manual one for the user: run
`renegade --gamedir "C:\Westwood\Renegade_full"`, load a level, then `mark_texture <a texture name
from that level>`, `mark_test 500` and `mark_status` at the console, and report whether the scorches
appear on the ground, whether they follow slopes, and what `mark_status` prints for marks against
objects.

## P20 -- the shadows are cached and nobody has watched one

`WorldShadowManager` is in and checked (P20-A in `completed_features.md`,
`docs/zerohour/WorldShadowManager.md`, twenty-six green terrain ctest entries). The caching is proved
structurally and by the device-less checks, but no shadow has been seen: the checks cannot allocate a
render target, nothing registers a world-system caster yet, and local lights still do not cast. Next
exact action is a manual one for the user: run `renegade --gamedir "C:\Westwood\Renegade_full"`,
load a level, then `shadow_mode 3` and `shadow_status` at the console while standing still, and
`shadow_status` again while running and turning, and report whether shadows still look right and
whether the render count drops to the cache-hit count when nothing is moving.

## P12 -- the boundary is safe by arithmetic and nobody has stood at one

The far layer is in and checked (P12-A in `completed_features.md`, `docs/zerohour/FarTerrainLOD.md`,
thirty green terrain ctest entries). What this entry carries out of the absorbed acceptance line is
the clause a check cannot settle, *no visible cracks at near/far boundaries under normal camera
ranges*: far-to-far seams are exact and far is proved nowhere above near, so a boundary can only
show a bounded step down, but no boundary has been looked at, `LOD_MEDIUM` is enumerated and
treated as near because no field here is big enough for a third tier to be worth switching to, and
nothing streams. Next exact action is a manual one for the user: run
`renegade --gamedir "C:\Westwood\Renegade_full"`, load a level, then `terrain_test 256 2` and
`far_terrain` at the console, walk away from the field until the numbers show far blocks drawing,
and report whether the boundary shows anything.

## P21 -- the lights are filtered and nothing has switched one on

`WorldLightManager` is in and checked (P21-A in `completed_features.md`,
`docs/zerohour/WorldLightManager.md`, twenty-eight green terrain ctest entries). No gameplay code
creates a dynamic light yet -- weapons, explosions and Commander feedback are later phases -- and
particles are unlit so they have no light set to filter. Next exact action is a manual one for the
user: run `renegade --gamedir "C:\Westwood\Renegade_full"`, load a level, then `light_test 8 15`
and `light_status` at the console, walk into and out of the ring, and report whether the world
actually lights up, whether the lights-per-query number stays flat after `light_test 64 15`, and what
`light_clear` leaves behind.

## P22 -- batched, unwatched, and one check unconfirmed

`WorldParticleManager` is in (P22-A in `completed_features.md`,
`docs/zerohour/WorldParticleManager.md`); `shader_particles` is green on the client. Two things are
open. `fds_shader_particles` failed once with exit code 3 and no output while the client passed; the
scratch arrays in the check were heap-allocated and are now static, which is the suspected cause but
is unconfirmed because the binary has not been run since. Nothing has been on a screen either: no
batch has been drawn, nothing sets a budget class other than AUTO, and `W3D_EMITTER_RENDER_MODE_LINE`
is unbatched by design. Next exact action is a manual one for the user: run
`ctest -C Debug -R shader_particles --output-on-failure` in `build/win32`, and then
`renegade --gamedir "C:\Westwood\Renegade_full"`, load a level, fire a weapon, and compare
`particle_status` with `particle_batching off` against `particle_batching on` -- the emitters-per-draw-call
number is the acceptance, and it is 1.00 when batching is off.

## P23 -- the weather is arithmetic and nobody has stood in it

`WeatherEnvironmentRenderer` is in and the stock per-particle weather is deleted (P23-A in
`completed_features.md`, `docs/zerohour/WeatherEnvironmentRenderer.md`). Three things are open.
`shader_weather` and `fds_shader_weather` are written and registered but have never been run,
because this session was asked not to run the game -- see Q-012, which now covers both check
families. Nothing has been on a screen: no weather has been drawn with a device, and the one
deliberate visual regression is that a raindrop is aligned to its own velocity rather than to its
velocity minus the camera's, so running sideways through rain no longer slants it. And three of the
six modes Section 36 names -- `DUST`, `TIBERIUM`, `FALLING` -- have no caller, because
`WeatherMgrClass` only knows rain, snow and ash; four textures are missing and listed in
`docs/assets/WeatherParticles.md`, with rain drawing untextured until `ow_weather_rain.tga` exists.
Next exact action is a manual one for the user: run
`ctest -C Debug -R "shader_weather|shader_particles" --output-on-failure` in `build/win32`, then
`renegade --gamedir "C:\Westwood\Renegade_full"`, load an outdoor level, type `rain 1` and then
`snow 1` at the console, and report `weather_status` -- whether the rain looks at least as good as
stock, and whether it still looks right standing under a roof and at the edge of one.

## P24 -- the tracer is pooled, two donor visuals in its own section are not

`TracerRenderer` is in and the per-shot `Line3DClass`/`TimedDecorationPhysClass` pair is deleted
(P24-A in `completed_features.md`, `docs/zerohour/TracerRenderModule.md`). This absorbs the P24
acceptance in full -- it names only the tracer -- but Section 27's own "Implement" list has two
items the acceptance does not cover and this pass did not touch: **projectile trail/stream**
(`W3DProjectileStreamDraw`; `AmmoDefinitionClass::TrailEmitter` is saved and loaded and has no
reader anywhere) and **rope/cable strip** (conditional on a donor visual needing one; none inspected
so far does). `shader_tracers` and `fds_shader_tracers` are written and registered but have never
been run, for the same reason as P22's and P23's checks -- Q-012 now covers all three check
families. Next exact action is a manual one for the user: run
`ctest -C Debug -R "shader_weather|shader_particles|shader_tracers" --output-on-failure` in
`build/win32`, then fire a `DisplayLaser` weapon in game and report `tracer_status`.

## P25 -- buildings and bridges scatter debris, vehicles and city props do not yet

`DebrisRenderer`/`DebrisManager` are in (P25-A in `completed_features.md`,
`docs/zerohour/DebrisRenderModule.md`). This absorbs the P25 acceptance in full -- it is a general
statement about networked `GameObj`s and neither tier ever creates one -- but Section 28 names
buildings, bridges, vehicles and future generated city props as consumers, and only the first two
are wired. **Vehicles**: `VehicleGameObj::Object_Expired` is server-only and drives a networked
explosion event; the right client-side "this vehicle is now a wreck" hook was not identified this
pass rather than guessed at. **City props**: no generated-prop system exists yet to hook into.
Neither is a missing-art problem, both are a missing call site. The gameplay tier
(`DebrisManager`, real `RigidBodyClass` fragments) is also entirely unchecked by automation: it
needs `COMBAT_SCENE`, which `-shaderselfcheck` never brings up, so `shader_debris`/`fds_shader_debris`
exercise `DebrisRenderer` (the visual tier) only, and have never been run at all, for the same
reason as every other check family this session added -- Q-012 now covers four. Next exact action
is a manual one for the user: run
`ctest -C Debug -R "shader_weather|shader_particles|shader_tracers|shader_debris" --output-on-failure`
in `build/win32`, then destroy a building and a bridge span in game and report `debris_status`.

## P26 -- four of the five component contracts have no implementer, and no success path is checked

`rendercomponent.h` and `CompositeAttachmentClass` are in (P26-A in `completed_features.md`,
`docs/zerohour/ModularDrawComponents.md`). This absorbs the P26 acceptance in full, but Section
29's Initial-components list names eight components and only two exist (tracer renderer, debris
renderer -- both pre-existing, now declaring `IRenderComponent`). The other six
(**selection/status renderer**, **waypoint/order renderer**, **ghost-building renderer**,
**procedural-world visual attachment**, and the RTS-overlay and selection-visual consumers
generally) are deliberately unbuilt: each belongs to a later phase in the roadmap's own
dependency order -- ghost placement, procedural construction, Commander sidebar -- so
`IClientUpdateComponent`, `IRTSOverlayComponent`, `ISelectionVisualComponent` and
`IWorldInteractionVisualComponent` are contracts awaiting their phase, not missed work. Separately,
`shader_attachments` pins only the refusal paths; **every success path is unverified** -- attaching
a real model to a real bone, independent child animation playing, `Set_Slot_Hidden` hiding a
visible module, and a building's bone search finding a component all need real `.w3d` art and (for
the building path) a live combat scene. Next exact action is a manual one for the user: run
`ctest -C Debug -R "shader_weather|shader_particles|shader_tracers|shader_debris|shader_attachments" --output-on-failure`
in `build/win32`.

## P27 -- shroud is real, nothing has stood in a live match to prove it

`WorldShroudManager` (P27-A in `completed_features.md`, `docs/zerohour/WorldShroudManager.md`) is
consumed for real by two other new services this same pass: `TacticalIntelligenceModel` (P28)
queries it, and `TeamPowerGridService` (P34) registers/unregisters sensor sources for
communications-capable buildings. Neither path is reached by a live match yet, because no real
`BuildingGameObj` is registered with `TeamPowerGridService` in production (see P34's own entry).
Next exact action: none until P34's `BaseControllerClass` integration lands; after that, a manual
check that a Communications Center's own team sees its own coverage and an enemy team does not
until it gains real sensor coverage there too.

## P28 -- the FPS radar refactor is live, the strategic query has no renderer

`TacticalIntelligenceModel` (P28-A, `docs/zerohour/TacticalIntelligenceModel.md`) already carries
real weight: `RadarManager::Update` calls `Build_FPS_Contacts` every frame the HUD radar draws,
so this is the one phase in this whole pass whose refactor is exercised by ordinary play the
instant the game runs, not only by a self-check. What has not been confirmed is that it still
*looks* the same -- the filtering logic did not change, but nobody has looked at a screen since.
`Build_Strategic_Contacts` has no renderer (Commander map is Phase 36+). Next exact action is a
manual one for the user: play a multiplayer match, confirm HUD radar blips, brackets and fading
look exactly as before this refactor.

## P29 -- status rings are real, nothing has ever created one in game

`StatusOverlayRenderer`/`StatusOverlaySystem` (P29-A, `docs/zerohour/StatusOverlaySystem.md`) have
no gameplay call site -- unit selection and Commander orders are later phases (Commander sidebar).
Next exact action: none until a selection/order system exists to call
`StatusOverlaySystem::Add_Overlay`.

## P30 -- the guard/retaliate state machine has no unit to guard yet

`GuardStateControllerClass` (P30-A, `docs/zerohour/GuardStateController.md`) has no real caller --
Phase 39's Commander-produced AI infantry/vehicles are the intended first one. The mission-
script layer's own hand-authored guard behavior is untouched. Next exact action: none until
Phase 39 lands.

## P31 -- two real events are counted, nobody has read the snapshot back

`RTSTelemetryService` (P31-A, `docs/zerohour/RTSTelemetryService.md`) counts real kills and real
building destructions the instant either happens in a live match, but there is no console command
to print `Get_Snapshot`'s contents -- only the self check reads it back. Next exact action: add a
`telemetry_status`/`telemetry_dump` console command, then get a kill and destroy a building in
game and confirm the counts move.

## P32 -- one debug layer draws something, seventeen are named and empty

`DebugRenderSystem`/`WorldDebugDisplay` (P32-A, `docs/zerohour/DebugRenderSystem.md`) draw
building footprints for real once `debug_layer building_footprints 1` is set; the other
seventeen named layers toggle but draw nothing, because seven already-shipped world systems
(`WorldTerrainSystem`, `RoadSystem`, `BridgeSystem`, `WaterSystem`, `FoliageSystem`,
`WorldLightManager`, `WorldShadowManager`) have no public per-instance iterator to visualize, and
four more layers await later phases (building semantic nodes, PCT/MCT anchors, the Commander
cursor). Next exact action is a manual one for the user: `debug_layer building_footprints 1` in
game, confirm a yellow wireframe box appears around each building.

## P33 -- descriptors resolve and reload for real, nothing loads one at startup

`BuildingModelDescriptor`/`BuildingDescriptorManager` (P33-A, `docs/zerohour/
BuildingDescriptors.md`) have no production call site -- no `BuildingGameObj` carries a
`DefinitionName`, and nothing reads `data/building_descriptors/*.json` at engine startup. Only
four of the roadmap's twelve named GDI/Nod building categories have example content. This is also
the blocking gap for P34's `BaseControllerClass` integration. Next exact action: give
`BuildingGameObj` a `DefinitionName` field and a descriptor-load call at creation time, then wire
`reload_building_descriptors` to actually matter in a running game.

## P34 -- the allocator is proven, it is not yet the one deciding anything live

`TeamPowerGridService` (P34-A, `docs/zerohour/TeamPowerGridService.md`) is not wired into
`BaseControllerClass::Power_Base` (`basecontroller.cpp:521-552`, the exact coarse whole-base loop
it is meant to replace) -- that surgical change was deferred rather than risked under time
pressure this late in the pass, and needs P33's `DefinitionName`-on-`BuildingGameObj` gap closed
first. No wallet-cap enforcement exists anywhere in this codebase to apply the Silo bonus to.
Next exact action: after P33's gap closes, replace `Power_Base`'s uniform `Enable_Power` loop
with `TeamPowerGridService::RecalculateTeam`, keeping `BasePowered` as a derived summary
(`Get_Generation(team) > 0`) for whatever else still reads `Is_Base_Powered()`.

## P35 -- the camera state machine is proven, nothing enters it but the console command

`CommanderModeController` (P35-A, `docs/zerohour/CommanderModeController.md`) has no production
call site -- only the `commander_mode` console command reaches `Enter_Commander_Mode`. Input
rerouting, Commander overlays/sidebar, and `commander_station_anchor` support are all later
phases and untouched. `Pick_At_Cursor` is untested even device-less, deliberately, since it
reaches `WorldSpatialIndex::Query_Ray` and this pass did not verify that call is safe with no
physics scene under it. Next exact action is a manual one for the user: `commander_mode 1` in
game, confirm the camera detaches from the soldier and pan/zoom/tilt work, `commander_mode 0`
returns control to the same soldier, and that dying while in Commander Mode exits it
automatically.
