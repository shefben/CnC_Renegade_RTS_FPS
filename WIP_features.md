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

## P10 -- one named consumer left unwired, the acceptance line still unmeasured

`WorldSpatialIndex` and the consumers that existed are done (P10-A/B in `completed_features.md`),
and three of the five later-named consumers are now confirmed real (P10 continued, same file):
Commander placement, nearby lights, roads/bridges/water. **Foliage cells** is the one genuine gap
left: `FoliageSystem` keeps its own private grid with no reference to `WorldSpatialIndex` either
way, and nothing outside foliage rendering has ever needed to ask what plants are near a point, so
there is no caller to write this for yet. Procedural world generation is not a gap, it is
permanently out of scope by Section 17's own prohibition. The acceptance line itself -- common
queries no longer scan the full object list -- is still structural and unmeasured, the same status
it has always carried; nothing this pass did changes that. Next exact action: nothing on foliage
cells until something wants to ask that question of the world; the acceptance line waits on the
same live-profile run every other device-less phase in this file does.

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
level, there is no reflection, and no naval/amphibious unit yet asks `Get_Water_Depth`/`Is_Navigable`.
**Flow direction and scroll speed now drive real per-frame animation** (`completed_features.md`,
"P17 continued") -- that clause is closed. Next exact
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
render target, and nothing registers a world-system caster yet. **"Local lights still do not cast" is
not a gap** -- `WorldShadowManager::Update_Object_Shadow`'s own comment says why: "local shadow
casting was disabled in stock Renegade and nothing has asked for it back", so only the sun (always
orthographic) drives a dynamic shadow, matching stock exactly; `LightClass::CastShadows` still
round-trips through the W3D format and LevelEdit's authoring UI, it is just correctly unread by the
runtime shadow pipeline. Next exact action is a manual one for the user: run
`renegade --gamedir "C:\Westwood\Renegade_full"`,
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

## P21 -- the lights are filtered and now two gameplay sources switch them on

`WorldLightManager` is in and checked (P21-A in `completed_features.md`,
`docs/zerohour/WorldLightManager.md`, twenty-eight green terrain ctest entries). **Weapon fire and
explosions now create real dynamic lights** (`completed_features.md`, "P21 continued") -- the two
callers the class's own header comment named and had neither of. Commander feedback lights remain a
later phase, and particles are still unlit so they have no light set to filter. Next exact action is
a manual one for the user: run `renegade --gamedir "C:\Westwood\Renegade_full"`, load a level, then
`light_test 8 15` and `light_status` at the console, walk into and out of the ring, fire a weapon and
trigger an explosion and confirm a brief flash appears at each, and report whether the
lights-per-query number stays flat after `light_test 64 15`, and what `light_clear` leaves behind.

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

## P25 -- buildings, bridges and vehicles scatter debris; city props do not yet

`DebrisRenderer`/`DebrisManager` are in (P25-A in `completed_features.md`,
`docs/zerohour/DebrisRenderModule.md`). This absorbs the P25 acceptance in full -- it is a general
statement about networked `GameObj`s and neither tier ever creates one -- but Section 28 names
buildings, bridges, vehicles and future generated city props as consumers. **Vehicles are now wired**
(`completed_features.md`, "P25 continued"): the hook lives in `VehicleGameObj::Apply_Damage`'s
existing health-crossed-zero check, not in the server-only `Object_Expired`, because that check runs
identically on the server and on every client replicating the vehicle's health -- gating it on
`I_Am_Server()` would have meant only the host ever saw a wreck.
**City props**: still no generated-prop system exists to hook into -- not a missing-art problem, a
missing call site nothing has created yet. The gameplay tier
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

## P27 -- shroud is real and wired end to end; no level content exercises it yet

`WorldShroudManager` (P27-A in `completed_features.md`, `docs/zerohour/WorldShroudManager.md`) is
now reached by a real registration path, not just a self check: every `BuildingGameObj` registers
with `TeamPowerGridService` (P27/P33/P34-B in `completed_features.md`), which registers a sensor
source for any building whose `DescriptorName` resolves to a `communications.enabled` descriptor.
No preset in this tree's own content sets `DescriptorName` on a Communications Center yet -- that
is authoring, not code. Next exact action: once a level or preset does, a manual check that its
own team sees the resulting coverage and an enemy team does not until it gains coverage there too.

## P28 -- the FPS radar refactor is live, the strategic query has a debug renderer only

`TacticalIntelligenceModel` (P28-A, `docs/zerohour/TacticalIntelligenceModel.md`) already carries
real weight: `RadarManager::Update` calls `Build_FPS_Contacts` every frame the HUD radar draws,
so this is the one phase in this whole pass whose refactor is exercised by ordinary play the
instant the game runs, not only by a self-check. What has not been confirmed is that it still
*looks* the same -- the filtering logic did not change, but nobody has looked at a screen since.
`Build_Strategic_Contacts` now has `DEBUG_LAYER_RADAR_CONTACTS` (P28/P32-B in
`completed_features.md`) but still no gameplay renderer (Commander map is Phase 36+). Next exact
action is a manual one for the user: play a multiplayer match, confirm HUD radar blips, brackets
and fading look exactly as before this refactor, and optionally `debug_layer radar_contacts 1` to
see strategic contacts directly.

## P29 -- status rings are real, nothing has ever created one in game

`StatusOverlayRenderer`/`StatusOverlaySystem` (P29-A, `docs/zerohour/StatusOverlaySystem.md`) have
no gameplay call site -- unit selection and Commander orders are later phases (Commander sidebar).
Next exact action: none until a selection/order system exists to call
`StatusOverlaySystem::Add_Overlay`.

## P31 -- two real events are counted, the console can read the snapshot back now

`RTSTelemetryService` (P31-A, `docs/zerohour/RTSTelemetryService.md`) counts real kills and real
building destructions the instant either happens in a live match. `telemetry_status`/
`telemetry_dump` console commands now exist (P31-B in `completed_features.md`). Next exact
action is a manual one for the user: get a kill and destroy a building in game and confirm
`telemetry_dump` shows the counts moving.

## P32 -- three debug layers draw something, fourteen are named and empty

`DebugRenderSystem`/`WorldDebugDisplay` (P32-A, `docs/zerohour/DebugRenderSystem.md`) draw
building footprints for real once `debug_layer building_footprints 1` is set; `radar_contacts`
and `shroud_cells` now draw too (P28/P32-B in `completed_features.md`). The other fourteen named
layers toggle but draw nothing: seven already-shipped world systems (`WorldTerrainSystem`,
`RoadSystem`, `BridgeSystem`, `WaterSystem`, `FoliageSystem`, `WorldLightManager`,
`WorldShadowManager`) have no public per-instance iterator to visualize, `spatial_index_cells`
and `collision_bounds` were not attempted this pass either, and four more layers await later
phases (building semantic nodes, PCT/MCT anchors, the Commander cursor). Next exact action is a
manual one for the user: `debug_layer building_footprints 1`, `debug_layer radar_contacts 1` and
`debug_layer shroud_cells 1` in game, confirm a yellow wireframe box appears around each
building, coloured crosses appear at visible contacts, and coloured quad outlines follow explored/
visible ground.

## P33 -- 8 of 12 GDI/Nod building categories have descriptors; the other 8 do not

`BuildingModelDescriptor`/`BuildingDescriptorManager` (P33-A, `docs/zerohour/
BuildingDescriptors.md`) now has a real production call site, and both gaps this entry used to
track are closed (see `completed_features.md`'s Q-017 addenda): `data/building_descriptors/
*.json` **is** read at real engine startup now (`BuildingDescriptorManager::Load_Directory`,
called once from `combat.cpp`), and `DescriptorName` **is** authored automatically onto every
real preset a buildable descriptor names (`Resolve_Game_Object_Presets`, also called from
`combat.cpp` every level load). Power Plant, Communications Center, Advanced Communications
Center and Tiberium Silo (GDI + Nod, 8 descriptors) are buildable with real presets; three of
those eight now use real art instead of an `ow_` placeholder
(`docs/zerohour/BuildingComponentAssembly.md`). The remaining eight of the roadmap's twelve
named categories (Barracks/Hand of Nod, Refinery, Repair Bay, War/Vehicle Factory, Airstrip,
Construction Yard, defense structures) have real assets in `models/mp buildings/c&c_mode`
(catalogued into `building.json` manifests for most of them already) but no `data/
building_descriptors/*.json` at all yet. Next exact action: author a descriptor for one of
the remaining eight categories, following `gdi_power_plant.json`'s shape (assembly block from
its real `building.json` manifest, real `game_object_preset` from `docs/zerohour/
BuildingPresets.md`'s method if not already known), then confirm `building_json_status`/
`reload_building_descriptors` against a live building.

## P35 -- input rerouting is done; the way *in* is still a console command

`CommanderModeController` (P35-A, `docs/zerohour/CommanderModeController.md`) is entered only by
the `commander_mode` console command: there is still no keybind, which needs a new
`INPUT_FUNCTION_*` plus its defaults and controls-screen row, and `commander_station_anchor`
support is untouched. The input rerouting this entry used to carry is closed
(`completed_features.md`, "P35/P36/P37-E"): `CommanderUIInput` shows the cursor and gates FPS input
while `Is_Active()`, and `Pick_At_Cursor` now has a real caller. Next exact action: add a bindable
Commander Mode toggle function, or leave entry on the console and confirm the rest in game
(`commander_mode 1`/`0`, camera detaches and returns, dying exits) -- that half is manual, see
Q-012.

---

## P36 -- the panel is clickable; the per-building power widget still is not

`CommanderSidebarModel` (P36-A), `CommanderSidebarRenderer` (P36-B) and now the pointer
(`completed_features.md`, "P35/P36/P37-E") close Section 42's main surface: mode tabs, paging and
cards all respond to the mouse through `Hit_Test`, and the model is refreshed every frame so a
clicked tab or a spent credit shows immediately (`docs/zerohour/CommanderSidebar.md`). What is left
is Section 42's per-building power control -- manual on/off and priority override on a *selected*
building -- which has no widget and, more to the point, no building-selection input to hang one on;
all four service calls behind it exist. Absorbs the backlog's P36 acceptance line in full. Next
exact action: add click-to-select on a building in Commander Mode (the same `Pick_At_Cursor` ray
the ghost already uses, resolving to a `BuildingGameObj` instead of a ground point), then a small
power panel bound to that selection.

## P37 -- aiming is closed; nothing here has been seen on screen

`CommanderPlacementValidator` (P37-A), `CommanderGhostRenderer` and `cCommanderPlacementEvent`
(P37-B, P40-B) close the acceptance end to end, aiming included as of `completed_features.md`'s
"P35/P36/P37-E": a building card arms a ghost, the ghost follows the pointer via `Pick_At_Cursor`
fed by `CameraClass::Device_To_World_Space`, a left click commits it and a right click cancels.
What remains is not code -- `Pick_At_Cursor` and the world-query branches (overlap/slope/bounds/
exit clearance) are still unexercised, because they need a real physics scene and
`-shaderselfcheck` has none. Absorbs the backlog's P37 acceptance line. Next exact action is a
manual one for the user, folded into Q-012: enter Commander Mode, click a building card, move the
ghost over legal and illegal ground, and confirm the red/green verdict tracks the pointer and that
a click places where the ghost was drawn.

## P39 -- production, orders, population, exit-clearance and guard are all real; selection input is not

`CommanderProductionService` (P39-A) plus `cCommanderProductionEvent` and `cCommanderOrderEvent`
(P40-B) enforce the whole Section 45 validation list server-side. Every consumer-side gap this
entry used to carry is closed (`completed_features.md`, "P39 continued" and "P39 continued 2"):
`Drain_Completed_Commander_Production` spawns a real
`SoldierGameObj`/`VehicleGameObj` at each completed item's facility; `cCommanderOrderEvent::Act`
turns an accepted order into a real `ActionClass` call; `Set_Population` now has a matching
decrement, from a `Killed()`/`Detach()` observer rather than the `Destroyed()` event nothing in
this tree ever calls; `ExitClear` runs a real spatial query before a slot is ever promised;
`ORDER_GUARD` reaches a real `GuardStateControllerClass` per unit (`Code/Combat/commanderguardai.*`,
closing P30). One honest limit remains: `ORDER_PATROL` degrades to a single-point Goto, because the
wire format this event carries has one point, not a waypath -- a real back-and-forth patrol needs a
second point nothing here has, and that is a protocol change, not a missing caller. Drag-box
selection and control groups are still unwritten. Absorbs the backlog's P39 acceptance line. Next
exact action: none on production/orders/population/exit/guard; patrol waits on a wire-format
decision nobody has asked for, and drag-box selection now waits only on itself -- P35/36's input
rerouting, which it used to sit behind, is done.

## P40 -- replication is live; the save-file round trip is still unverified

`RTSPersistenceClass` (P40-A) and the four network classes (P40-B) are both in and the whole layer
now works across a network (`docs/zerohour/RTSPersistence.md`). What is unproven rather than
unwritten: `-shaderselfcheck` cannot stand up a `ChunkSaveClass`, so the byte-level save/load round
trip has never been executed, and the replication itself has never run against a second machine --
both need a live match, not more code. Absorbs the backlog's P40 acceptance line. Next exact action
is a manual one for the user: run a listen server and a client, enter Commander Mode on the server,
`commander_ghost` + `commander_place`, and confirm the client sees the build-up and the same power
figures.

## P43 -- performance requirements are understood; no baseline has been taken

Section 49's metric list and the FPS-priority degrade-order are read and required no code change --
the existing rule (never shed authoritative local combat/movement to protect distant RTS update
frequency) is already how `CombatManager::Think`'s ordering was written in P36-P40. What cannot be
done here is the measurement itself: frame time, draw calls, power-grid recomputation time and the
rest all need a running client under load, and the standing "do not run the game" constraint blocks
it the same way it blocks the nineteen self-check families **Q-012** already tracks. Absorbs the
backlog's P43 line. Next exact action: take the initial baseline during a live session, the same
session that clears Q-012.

## P45 -- required documentation exists; the final 110-item DoD is deliberately not attempted

Section 50's required-document list is satisfied: `docs/integration/THIRD_PARTY_PROVENANCE.md` is
new (it consolidates pins that were previously scattered across three other docs), and
`docs/RequiredDocumentationIndex.md` maps every other required path onto its actual location, since
this project consistently used `docs/zerohour/<ClassName>.md` rather than splitting a parallel
`docs/commander/` tree, and duplicating fifty files under a second path would itself violate the
project's own no-second-truth rule. Section 52's 110-item final Definition of Done is explicitly
**not** attempted this pass: most of its items depend on phases that are not done yet (P08-P26 are
still mid-flight per their own WIP entries, P46-P65 are unstarted), so scoring it now would either
be premature or dishonest. Absorbs the backlog's P45 line. Next exact action: re-run Section 52 as
a checklist once the phases it actually depends on are closed, not before.

## P51 limit -- height fog is camera-relative, not per-vertex

`WW3D::Compute_Height_Fog_Scale` is correct and real, but is applied once per frame from the
viewer's own height (`DX8Wrapper::Set_Fog`), not per vertex -- true per-vertex height fog needs
either a vertex-format change reaching every renderer or a shader path this engine's fixed-function
terrain/mesh rendering does not have. Absorbs no backlog line (P51 itself is closed). **Answered,
not just deferred**: Phases 56/57 built the post-process chain and confirmed what they found --
no `CreatePixelShader`/programmable tier exists anywhere in this tree (`VisualFidelityProgram.md`).
So a shader path was not opened, and per-vertex height fog stays a permanent limit of this
renderer rather than a revisit-later item. Next exact action: none, unless a shader tier is opened
for other reasons -- the same standing note P56's tonemap carries.

## P56 limit -- the tonemap is a global operator, not a local one

`DX8Wrapper::Tone_Curve` is a real extended-Reinhard shoulder with a real auto-exposure driving it,
so highlights roll off instead of clipping and the acceptance line is substantially met. What it
cannot do is vary the curve across the frame: a gamma ramp is a per-channel lookup, and a local
operator needs the programmable tier, which this build has none of (no `CreatePixelShader` anywhere).
Shows only in a frame holding a deep interior and open sky at once. Absorbs no backlog line (P56
itself is closed). Next exact action: nothing, unless a shader tier is opened for other reasons --
at which point this is one of the first things worth moving onto it.

## P57/P55 -- the render-target composite has never run on a device

Both the bloom composite and the reworked sun shafts switch render targets mid-frame, bracketed the
way `pscene_projectors.cpp` brackets its own (set, clear, draw, restore) -- but unlike the
projectors, these run inside a frame that is already being rendered rather than in their own
`Begin_Render`/`End_Render` pair. That difference has no precedent in this tree and cannot be
checked without a screen, so both ship off by default. Absorbs no backlog line (P55 and P57 are
both closed). Next exact action: with the game runnable, turn bloom on first, confirm the frame
still composites and the HUD still draws, then the shafts.

## Gameplay gap backlog -- ordered, nothing in it started

`docs/GameplayGapResolutionReport.md` (full detail) and the rewritten `gameplay-gaps.md` (13
entries) are the current source of truth for what a player would still notice broken in the
Commander/RTS layer. Nothing in that report's backlog was implemented this pass -- it is
findings plus a dependency-ordered backlog, not code. Highest-value next actions per that
backlog's own ordering: wire `CommanderProductionService::On_Facility_Destroyed` into wherever
`BuildingGameObj` actually transitions to destroyed (currently uncalled outside a self-check
test, so a destroyed production facility's queue is lost instead of refunded); clear
`CommanderProductionService::Get_Team_Commander` on player disconnect (currently never cleared,
so a disconnected Commander locks their team's role for the rest of the match); then the 8
missing building-descriptor categories, which the Decision Log gives recommended starting
numbers for.

## The boundary cliff ring is built and nobody has seen a cliff

`BoundaryCliffGenerator`, `ProceduralWorldManager` and the "Procedural Skirmish" menu button are
all in (see `completed_features.md`, "V2 Phase 3", and
`docs/procedural_world/WorldBoundsAndBoundaryCliffs.md`). What this entry carries is the half no
device-less check can settle: **nothing has been on a screen.** The ring's plan is proven
deterministic, closed, seam-exact and unclimbable by `shader_worldcliffs`, and the triangles are a
function of that plan -- but no cliff has ever been rendered or walked into.

Three specific unknowns, in the order they would bite:

1. **The face will probably draw untextured.** It is dressed by the first `TERRAIN_LAYER_CLIFF`
   layer, and `Define_Default_Layers`' cliff layer names no texture -- the same gap P13 already
   carries. Escarpments degrade to untextured rather than crashing, so this is a look problem,
   not a failure, and it is fixed by pointing the cliff layer at a stock texture name
   (`docs/assets/TerrainLayers.md`).
2. **Whether the toe apron and the face inflection earn their triangles** is a judgement nobody
   can make from arithmetic.
3. **Whether a vehicle can crest the ring** is the acceptance clause that matters most and the one
   that needs a real physics scene.

Also unexercised: the menu button has never been clicked, so the whole path -- request, skirmish
map choice, fit-to-level against a real stock map's extents, generation inside `Post_Load_Level`
-- has run zero times.

Next exact action is a manual one for the user: run
`ctest -C Debug -R "shader_worldbounds|shader_worldcliffs|shader_worldprofile" --output-on-failure`
in `build/win32`, then `renegade --gamedir "C:\Westwood\Renegade_full"`, click **Procedural
Skirmish** on the main menu, and report what the log's `ProceduralWorldManager: world generated`
line says, whether cliffs appear around the map, and what `debug_layer boundary_cliffs 1` and
`debug_layer world_bounds 1` show -- in particular whether any riser draws red.

## V2 Phase 10 closed the live-caller gap; a person in a real skirmish is what is left

The gap the previous seven phases each carried forward -- `fit_to_level: true`, the "Procedural
Skirmish" button's own only profile, permanently skipping macro terrain and therefore every
generator downstream of it -- is closed (see `completed_features.md`, "V2 Phase 10"). A fitted
world now gets a real chunk registry too, sampled from the host level's own geometry through
`WorldSpatialIndex::Query_Ray` rather than generated, so `WorldTerrainSystem::Has_Chunk_Registry`
is true for the one profile that actually reaches a live caller, and interior cliffs, hydrology,
the strategic plan, the road graph and bridge planning all run for real inside
`ProceduralWorldManager::Generate_World` the first time the menu button is clicked. Phase 8's own
road/bridge handoff is closed alongside it: `RoadSystem::Trim_Road_Range` splits a road at a
bridge's own two abutments, `BridgePlanner` calls it and wires both new segments'
`RoadConnectionClass` to the bridge, and `BridgeSystem::Connect_Roads` binds them back
(`Find_Bridge_Banks`'s own bank search was walking perpendicular to the road instead of along it,
which never gave a real cut interval to split at -- fixed as part of this pass, not left for
later). A new `TransportationNetworkValidator` runs after bridge planning every time and reports
unbound abutments, bridges with no connected road, bridges narrower than their own road or the
widest loaded wheeled vehicle's own corridor (`WorldMetricsClass`, not a guessed width), roads
duplicated through a bridge's own footprint, excessive approach grade, and whether the terrain's
own required base candidates are mutually reachable -- logged every generation, not only when a
self check asks. A new deterministic `-terrainselfcheck bridgerequired` (`terrain_bridgerequired`
ctest) proves a bridge actually gets built where the alternative is provably impossible, not
merely permitted, closing the gap `bridgeplan`'s own honest "Dijkstra may route around it" test
could not.

Item 4 closed too, same pass: `RoadGraphGenerator`'s own widths are Section 10A.1's real
Humvee-derived formula now, `GDI_HUMVEE_WIDTH` read directly from `Data/always.dat`'s own
`v_gdi_humvee.w3d` (MIX1 archive format plus the W3D box-chunk layout, both read straight from
this codebase's own `Code/wwlib/mixfile.cpp`/`Code/ww3d2/w3d_file.h`) rather than a live GameObj
lookup this project would otherwise have had to guess a preset name for. See
`completed_features.md`'s own Phase 10 addendum for the full derivation.

Item 5 closed in a later pass, same session: `BridgePlanner` no longer gives up on the first
illegal candidate. `Resolve_Crossing_Position` tries the original crossing, then a bounded,
deterministic reposition along the obstacle (stage 1), then -- for a grade failure specifically
-- a bounded search widening the assumed approach run (stage 2); `RoadGraphGenerator::
Reroute_Edge_Avoiding` (stage 4, a new primitive, backed by two new `RoadSystem` ones,
`Remove_Road` and `Integrate_New_Road`, so a reroute can replace one road without re-tessellating
any road a bridge has already split) asks for an alternate path around the obstruction when
every position/approach at the current one fails; `ProceduralWorldManager::Generate_World` itself
now retries the *whole world* with a deterministically bumped seed, bounded at
`MAX_WORLD_GENERATION_ATTEMPTS` (3), when `TransportationNetworkValidator`'s own required-base-
routes check comes back broken. Stage 3 (alternate crossing type) is an honest no-op -- no
tunnel system exists in this codebase to fall back to, documented rather than fabricated.
Every candidate check (span, abutment, bounds, grade, water clearance, road-width match,
bridge-to-bridge collision) runs before anything is committed to `BridgeSystem`/`RoadSystem`,
since neither has a cheap per-object undo to try-then-discard a candidate with. Failure reasons
are the gap report's own eight named categories (`Crossing_Failure_Reason_Name`), recorded per
crossing in a new `BridgePlanner::Diagnostics` list regardless of whether the crossing resolved,
so a failed seed's own story reads from that list rather than a bare "bridge failed". New
`-terrainselfcheck bridgefallback` (`terrain_bridgefallback`/`fds_terrain_bridgefallback`) proves:
every fallback refused still returns promptly with the crossing explicitly recorded unresolved,
not hung or silently dropped (bounded termination); the same seed/terrain/params reproduce the
same diagnostic, not only the same checksum (determinism); a tightened span limit that the
divided terrain's own genuinely-uniform crossing cannot satisfy anywhere within stage 1's own
search radius is honestly reported `SPAN_TOO_LONG` rather than a false resolution (stage 1's own
mechanism exercised and proven correct, even though this particular terrain has no narrower spot
for it to find -- a real, disclosed limit of the terrain built for this check, not of the
fallback itself); and `Reroute_Edge_Avoiding` itself, called directly against open terrain with
real room to detour, replaces a real edge's own road and keeps the network coherent (the old
road retired, the new one tessellated and folded in without disturbing anything else).

Not closed: item 5's own true stage-1-succeeds and stage-2-succeeds cases (a nearby reposition or
a longer approach actually turning a failure into a resolution) are exercised device-less as far
as this session had time to construct precise terrain for, but not proven on a scenario built
specifically to need them -- see `terrain_bridgefallback`'s own comments for exactly what each
block does and does not establish. Item 7's own live pass remains open: multiple fixed seeds
through the real Procedural Skirmish button, someone watching whether the roads and bridges look
right and join without a gap, and now also which fallback cases (if any) a given seed actually
exercised. Next exact action is that live pass: run `renegade --gamedir
"C:\Westwood\Renegade_full"`, click **Procedural Skirmish**, and report what the log's per-pass
status lines (`ProceduralWorldManager: fitted terrain sampled`, `... transportation network
validation`, and now any `attempt N (seed ...)` retry lines) say, and whether the roads/bridges a
person can actually walk look believable -- covering at minimum a flat seed, a seed with a river
crossing, a seed with multiple crossings, and if reachable, a seed whose first attempt actually
needed a retry, per the gap report's own seed list.

## V2 Phases 11-15 -- five real, tested, self-checked generators; item 7's own live pass now covers all of them, not just bridges

`BaseSelectionPlanner`, `TiberiumFieldGenerator`, `ProceduralAssetRegistry`/`SettlementGenerator`,
`BiomeDecorationGenerator` and `CombatSpaceValidator` are each real, built clean, and proven by
their own `-terrainselfcheck` family (`baseselection`, `tiberiumfields`, `settlements`,
`biomedecoration`, `combatspace` -- `ctest -N` now 161, was 149 before this block of work). See
each phase's own `docs/procedural_world/*.md` for what is proven headless and what genuinely
needs a live client (item 7's own distinction, upheld the same way for these five as for the
bridge work above -- never pretended complete until it has actually been run).

**Genuine, disclosed content/scope gaps, not implementation shortfalls:**
- Section 16A's own base-layout contract (initial building transforms, spawn sets, match
  bootstrap) is not attempted -- `BaseSelectionPlanner` selects a site and reserves a region only.
- `MaxBaseToFrontTravelTimeDifferenceSeconds` is not checked -- it needs a "front" concept
  (Section 17's own contested-area idea) this chain does not establish.
- No caller spawns a live Harvester for a generated world -- `TiberiumFieldGenerator` produces
  real field/route data; `HarvesterClass` already knows what to do with it once something wires
  the two together.
- Section 28A's own connector metadata (entrance/door/pedestrian anchors) is confirmed absent
  (by direct W3D bone-name inspection, not assumed) from every real building asset this tree
  ships -- Phase 13's own step 5 (entrances facing streets) is not merely unattempted, it is
  unsatisfiable with today's asset pool. Only three real building kinds exist at all
  (`gdi_communications_center`, `gdi_power_plant`, `nod_power_plant`); no house/ruin/industrial-
  decoration/POI/street-lamp asset exists anywhere in this tree.
- Object/foliage-aware sightline occlusion (Phase 15) is not attempted -- terrain-only.
- No generator in Phases 11-15 performs repair *after* an earlier phase has already committed
  state (a road, a bridge, a placed structure) -- each phase's own real repair happens during its
  own generation, where undo is still cheap, the same safety boundary `BridgePlanner`'s own
  fallback chain already established. Phase 15's own step 12 ("reject rather than silently
  accept") is answered by folding a hard combat-space failure into `Generate_World`'s existing
  bounded seed-retry loop instead.

**Next exact action, same live pass as above, now covering five more per-generation log lines**:
`ProceduralWorldManager: base selection`, `... Tiberium fields`, `... settlements`,
`... biome decoration`, `... combat space validation` -- report whether each says `built`/`PASS`
or `NOT BUILT`/`FAIL` for a real Procedural Skirmish seed, and whether a selected base site,
placed Tiberium field, and placed settlement/structure look right to someone standing at them,
the same "someone watching" standard the bridge work above has always required.

## V2 Phase 16 -- built, self-checked, documented; the live pass is now the whole gate

`GeneratedWorldIntegrator`, `NavMeshGenerator` and `MinimapBaseLayer` are real, built clean on
both binaries, and proven by `terrain_worldintegration`/`navigation`/`minimap` plus their `fds_`
variants (`ctest -N` now 167, was 161). Step 13's shroud-grid gap was found by verification and
fixed for real rather than recorded as a caveat. See
`docs/procedural_world/FinalWorldIntegration.md`.

**Genuine, disclosed gaps -- none of them implementation shortfalls:**
- One sector graph, not the three steps 3/4 name. `PathfindSectorClass` carries no locomotion-
  class tag and `PathfindClass` is one global singleton, so "walkable for infantry, not for a
  tank" cannot be two answers about the same point without a parallel sector format. Sized by
  the most restrictive loaded locomotion class, per `WorldMetricsClass`'s own documented rule.
- Sector granularity is the chunk registry's, not a per-cell physics floodfill's.
- Naval navigation (step 6) skipped -- no canonical naval locomotor exists in this tree, which
  that step's own "only where canonical support exists" wording allows.
- Tunnel portals and building-interior transitions (steps 7, 12) not produced -- no tunnel
  system exists anywhere in this tree, and no building in this asset pool has an interior
  (Phase 13's own connector-metadata gap).
- The minimap base layer has no renderer to consume it; none exists in this tree yet.
- Step 9's per-chunk incomplete-nav guard reduces to step 15's whole-world barrier: there is no
  incremental nav rebuild here, so no partial state is ever observable.

**Two real runtime conditions found and tolerated rather than crashed through:**
`PathfindClass::Get_Instance()` is legitimately null on the client self-check context (it is
constructed inside `PhysicsSceneClass`'s own constructor -- the same asymmetry `FoliageSystem`
already null-guards everywhere), and with no infantry definition loaded `NavMeshGenerator`
refuses honestly rather than guessing a sector envelope.

**Unattended `ctest` now works** (was previously blocked by a modal assert dialog). Two real
pre-existing defects fixed: six stack-allocated `CameraClass` objects in `shaderselfcheck.cpp`
(a `RefCountClass` whose destructor asserts its reference is gone -- automatic storage trips it
every time), and the missing assert handler on the `-*selfcheck` entry points (every other
harness in this tree installs one; these five did not, so `WWDebug_Assert_Fail` fell back to a
message box). Full suite: 167 tests, 30 failing, all reported as text.

**The 30 remaining failures are pre-existing and now visible** -- worth their own work item, not
Phase 16's. Two clusters: descriptor `game_object_preset` resolution (`shader_sidebar`,
`placement`, `construction`, `production`, `powergrid`, `rtssaveload`, `visualsettings`), and
shroud visibility (`terrain_shroud` "team A is not VISIBLE at its own source's position",
`shader_overlays`, `shader_powergrid`) -- the shroud cluster looks like one root cause across
three test families.

**Next exact action:** rolled into the Phase 17 block below -- one live pass now covers both
phases' log lines.

## V2 Phase 17 -- the Commander can expand; nobody has watched one do it

All thirteen steps built for real, both binaries clean, six new ctest entries green
(`ctest -N` now 173, was 167). `GeneratedWorldPlacementRules` (steps 1-2), `FoundationGradingService`
(steps 3-9) and `ExpansionRegionRegistry` (steps 10-13), plus `HeightfieldClass::Grade_Foundation`
and `WorldTerrainSystem::Apply_Foundation_Grade` as the one canonical terrain primitive. Profile
schema/generator version 6 adds the `grading` block. See
`docs/procedural_world/CommanderExpansion.md`.

**Two real defects the self-checks caught and that are now fixed** -- worth recording because both
were wrong in the dangerous direction: a pad levelled only to the footprint disc left the building's
own rim tilted (0.14 world units off level, because interpolation pulls from grid vertices outside
the disc), and sizing the apron from the height difference *under* the footprint produced a graded
face measured at 0.684 rad against a 0.5 rad limit on a 10 degree ramp. Apron sizing is now a
bounded fixed-point solve against the measured surface.

**Genuine, disclosed gaps -- none of them implementation shortfalls:**
- The tunnel exclusion covers ground a `TUNNEL_CANDIDATE` was planned on, not a tunnel. No tunnel
  system exists anywhere in this tree (the same absence Phase 16 steps 7/12 and
  `bridgeplanner.cpp`'s stage-3 fallback already record). When one arrives it owns that test.
- Nav rebuild after a construction is whole-world, not incremental -- inherited from Phase 16's own
  disclosed gap, not added here.
- `ExpansionRegion::IsForward` is decided by distance from the team's first region, the only real
  "home" signal available without a base-ownership concept this tree does not have.
- Chokepoint granularity is the sector graph's, which is the chunk registry's.
- Nothing renders the expansion regions or the grading report. The exclusions do reach the player,
  through the ghost the existing validator already colours.

**One disclosed interpretation, not a silent one:** step 3 defines a grading request "from a valid
ghost placement", and `PLACEMENT_SLOPE_TOO_STEEP` is treated as gradeable rather than fatal. A
grading that could only ever level ground already flat enough to build on would change no outcome,
and Phase 17's purpose line is letting the Commander build on generated terrain. Every other
refusal, including all seven generated-world exclusions, still stands.

**Next exact action:** rolled into the Phase 18 block below -- one live pass now covers all three.

## V2 Phase 18 -- the journal is deterministic in one process; two processes have never compared

All sixteen steps built for real, all three binaries clean, `terrain_worldops` green on both
(`ctest -N` now 175, was 173). `WorldOperationClass` is the versioned envelope,
`WorldOperationJournal` owns the monotonic revision, the ordered operation list and the base terrain
snapshot, and three new canonical primitives live in `Code/wwphys/` beside the existing ones. The
`worldop` console command is step 16. See `docs/procedural_world/WorldOperationJournal.md`.

**Genuine, disclosed gaps -- none of them implementation shortfalls:**
- **Replication is the format, not the transport.** `Write_Block`/`Read_Block` produce and consume
  the compact operation and are proven by round trip; no packet type sends one yet. Wiring it
  belongs with Phase 19, where the snapshot and late-join formats it must sit beside are defined.
- **The journal lives in memory.** Persisting it is Phase 19 territory; what exists here is the
  ordered, checksummed, replayable structure a save format would write.
- RECLAIM_SHALLOW_WATER is off by default (step 3's own "if enabled") and refuses a partly dry disc
  rather than levelling it -- raising a partly dry region to the waterline would *lower* the dry
  part, which is FLATTEN by another name and without FLATTEN's checks.
- Nav rebuild is whole-world, inherited from Phase 16, not added here.
- The crater limits are not a profile block. The roadmap does not ask for one at this phase, and
  inventing a schema field ahead of its own phase is what Section 34.0's discipline exists to avoid.

**The half of Phase 18's targeted validation that a single process cannot reach.** That validation
asks for "server applies, client reconstructs, collision matches" per operation. The self-check runs
one journal in one process, so what is proven is determinism under replay -- base snapshot plus the
ordered journal reproduces the identical terrain checksum, twice. The cross-peer half needs two
processes and a transport, and is carried forward rather than claimed.

**One thing worth keeping:** smoothing a plane is correctly a no-op, and the first version of the
SMOOTH check asserted otherwise. A linear ramp is already its own four-neighbour mean. The check now
digs a dent and asserts it gets shallower -- an assertion that "some bytes changed" would have
passed on a smoother that did the wrong thing.

**Next exact action:** one live Procedural Skirmish pass covering Phases 16, 17 and 18 together.
1. Read six per-generation log lines -- `ProceduralWorldManager: world integration`,
   `... navigation`, `... minimap base layer`, `... shroud coverage`, `... expansion rules`, and for
   each report whether it says `built`/`armed` or `NOT BUILT`/`NOT ARMED`, and whether the sector,
   portal and chokepoint counts are non-zero for a real seed.
2. Confirm a human and an AI can each cross the world (Phase 16's gate).
3. Place and construct a Power Plant, a ground factory, a defence and a communications building on
   generated terrain and confirm no boundary or nav corruption (Phase 17's targeted validation).
4. Run `worldop begin`, a few `worldop crater X Y R D` at real coordinates, then `worldop replay`,
   and report whether it prints `MATCH` (Phase 18's acceptance line).

Phase 20 is built and self-checked (see its own entry below); the live pass above is what closes
16-18, and Phase 20 has its own, separate live pass.

## V2 Phase 19 -- a world reconstructs exactly, in one process, over no wire at all

All nineteen steps built for real, all three binaries clean, `terrain_worldsync` green on both
(`ctest -N` now 177, was 175). `WorldSnapshotService` owns per-chunk/world checksums, the compact
chunk snapshot, compaction and developer rollback; `WorldSyncService` owns the handshake, the join
state machine, regional repair and the metrics. See `docs/procedural_world/SnapshotsAndLateJoin.md`.

**Three real defects the self-check caught, each a design error rather than a typo**, and all three
worth remembering because they are the kind that would have shipped silently:
- A **derived value in a checksum is fragility without information**. The chunk checksum folded in
  the chunk-registry entry, which is derived from the same heights but truncates float-derived
  slopes at 1e-3 -- so sub-millimetre reconstruction noise flipped it while the ground agreed. The
  repair protocol cannot tell that from real corruption.
- The **quantizer has to round-trip exactly**. Truncation breaks it (`2.999f * 1000.0f` is
  2998.9998 in float); rounding with the division in double does not.
- A **late joiner has to adopt the authority's revision**, because its ground reaches the server's
  without one operation passing through its journal, and step 16 gates building/AI state on that
  number.

**Genuine, disclosed gaps -- none of them implementation shortfalls:**
- **Still a protocol, not a transport.** Every message is a byte block a caller writes and reads;
  nothing opens a socket. This is what lets the whole join/verify/repair sequence be exercised end
  to end without a network, and binding the blocks to the existing packet types is a networking work
  item rather than a generation one. Phase 18's replication gap resolves the same way and to the
  same place.
- Snapshots cover terrain heights. Buildings, vehicles and AI state each already have their own
  replication path; step 16's readiness gate is what this phase owed them, and it is implemented.
- Generation time is recorded rather than measured here, and the profile/content hashes come from
  their real owners rather than being invented.

**The half a single process cannot reach:** two peers on a real network. The self-check throws the
server's world away, regenerates the baseline deterministically, reconstructs from the blocks alone
and gets a byte-identical world checksum -- steps 9-12 exactly, minus a socket.

**Next exact action:** unchanged from the Phase 18 block above -- one live Procedural Skirmish pass,
now also running `worldop status` before and after a few craters to confirm the revision advances and
the journal checksum moves.

## V2 Phase 20 -- functional subcomponents are real; a real building's own health has never driven one

All ten steps built for real (roadmap Section 21), all three binaries clean, `shader_subcomponents`
green on both (`ctest -N` now 179, was 177). `BuildingSubcomponentDefinition`
(`Code/Combat/buildingdescriptor.h`) adds ten canonical roles with no second health pool -- function
is a pure linear read of the building's own existing health, exactly the phase's own gate. See
`docs/procedural_world/BuildingFunctionalSubcomponents.md` for the full account, including why real
W3D bone names were scanned rather than guessed and which two of Section 21's six effect groups
actually have a real building to attach to (`power_generator_module` on both Power Plants,
`communications_array`/`radar_dish` on the GDI Communications Center only). **Correction, a later
pass:** this entry previously said Phase 20 closed the roadmap under roadmap v2.1.0's own 20-phase
count; the roadmap was extended to v2.2.0 (30 phases, Sections 22-30) before that claim was ever
acted on, so Phase 20 is not the last phase -- see the V2 Phase 21 entry below.

**Genuine, disclosed gaps -- none of them implementation shortfalls:**
- Four of Section 21's six effect groups (refinery, factory, repair, defense) have no buildable
  descriptor of that category anywhere in this tree yet -- P33's own backlog entry in this file,
  not a new gap Phase 20 introduces.
- Nod Communications Center and both factions' Advanced Communications Center carry no
  `subcomponents` block -- confirmed, not assumed, that none of their model names exist as real
  files anywhere in this tree, so there is no geometry to verify an anchor bone against.
- `tiberium_storage` is unimplemented: no worked effect in the roadmap's own 21.2 and no real Silo
  asset either.
- Anchor-resolution *status* only works for a runtime-constructed building (`RuntimeParts` is empty
  for a map-authored one); the numeric health-driven degradation itself is not runtime-construction-
  specific and is unaffected by this limit.

**What is not proven:** the same standing harness gap every runtime-building construction path in
`-shaderselfcheck` already carries (no `PhysicsSceneClass`/`GameObjManager` scene -- see
`Run_Runtime_Door_Service`'s own note) applies here too: a real building's own health has never
actually driven `Get_Effective_Power_Generation`/`Get_Effective_Vision_Radius`/
`Get_Effective_Radar_Radius` through a live `TeamPowerGridService` recalculation, and
`building_subcomponents` has never printed a real `resolved` anchor against a real loaded model.

**Next exact action** is a manual one for the user: run
`renegade --gamedir "C:\Westwood\Renegade_full"`, enter Commander Mode, construct a Power Plant and
a Communications Center on generated terrain, damage each below its own subcomponent's health
threshold (0.75) and confirm at the console (`building_subcomponents`, `worldop status`-adjacent
power/shroud figures) that generation and vision/radar radius drop proportionally and that the
anchor bones report `resolved`; repair each back above threshold and confirm the numbers recover.

## V2 Phase 21 -- the logistics graph is real; nothing has driven a convoy on screen

All steps built for real (roadmap Section 22), both binaries clean, `terrain_logistics`/
`fds_terrain_logistics` green (`ctest -N` now 181, was 179). `LogisticsGraphService`
(`Code/Combat/logisticsgraphservice.h/.cpp`) builds a real node/route graph from
`BaseSelectionPlanner`/`ExpansionRegionRegistry`/`TiberiumFieldGenerator`/`SettlementGenerator`
plus live buildings, with real road-network distance/reachability from `RoadGraphGenerator`. Off
by default (`ProceduralWorldProfile::Logistics.Enabled`, schema 7). See
`docs/procedural_world/LogisticsLayer.md`.

**Genuine, disclosed gaps -- none of them implementation shortfalls:**
- `REFINERY`/`FACTORY`/`REPAIR` nodes are a real, working mechanism with nothing to find yet --
  same P33 backlog gap Phase 20 already carries, not new to this phase.
- Section 22's third named blockage cause ("enemy obstruction") is not attempted -- no
  logistics-facing threat model exists; that is Phase 25's own recon/defense planner territory.
- Section 22.2 step 5's construction-support radius is a real, tested predicate
  (`Is_Under_Construction_Support`) with no caller applying a bonus from it yet -- an
  unrequested balance number was not invented to fill that gap.
- Section 22.5's Commander tactical-map visualization (routes/blockage/coverage on a real map) is
  not attempted -- `logistics_status` is a console status report, not a rendering consumer, and no
  such consumer exists in this tree yet for any of the several other systems carrying the same gap
  (`DebugRenderSystem`'s own fourteen empty layers).

**A real, pre-existing Phase 17 wiring gap found and fixed as part of this phase:**
`ExpansionRegionRegistry::Note_Construction_Complete`/`Rebuild_Regions` had no caller anywhere in
this tree outside their own self-check -- confirmed by grep, not assumed -- so the `IsForward`
metadata Phase 21's own `FORWARD_BASE` node kind depends on could never have existed at runtime.
Now wired into `combat.cpp`'s own construction-completion loop, server-only. `Reserve`/
`Bind_Building` (step 10) deliberately left unwired -- Q-016's own design already closes the
placement race that pair exists for; see the doc for why.

**What is not proven:** `Rebuild_Building_Nodes` (the three live-building node kinds) has never
run against a real `GameObjManager` scene -- the same standing harness gap every runtime-building
path in this tree already carries. Bridge blockage has never been tested against a bridge that
actually breaks mid-route. A convoy has never driven a real vehicle on screen.

**Next exact action** is a manual one for the user: run
`renegade --gamedir "C:\Westwood\Renegade_full"` with a profile that sets `logistics.enabled` (and
`logistics.convoy_enabled` to see convoys) true, click **Procedural Skirmish**, read the
`ProceduralWorldManager: logistics graph ...` log line, then `logistics_status` at the console and
confirm the node/route counts look right for the seed; construct a forward base and confirm a
`forward_base` node appears; if a Refinery/Factory/Repair Bay descriptor exists by then, confirm
those node kinds populate too.

---

## V2 Phase 22 -- the subsystem curve is real and tested; no vehicle has run it on screen

All implementable steps built for real (roadmap Section 23), all three binaries clean,
`terrain_vehiclesubsystems`/`fds_terrain_vehiclesubsystems` green (`ctest -N` now 183, was 181).
Engine/mobility scale the live control channels in `VehicleGameObj::Apply_Control`, a speed
governor makes engine damage cost real top speed, weapon rate of fire degrades continuously in
`WeaponClass::Set_State`, and AI retreat-to-repair is wired into `Think`. Off by default
(`ProceduralWorldProfile::VehicleSubsystems.Enabled`, schema 8). See
`docs/procedural_world/VehicleSubsystemDamage.md`.

**Genuine, disclosed gaps -- none of them implementation shortfalls:**
- **Sensor damage (Section 23.1's fourth subsystem) is not implemented at all.** No sensor,
  perception or detection-radius concept exists anywhere in `Code/Combat` -- searched, not
  assumed. "Reduced AI targeting" would mean inventing a targeting-range system first. No
  `sensor_*` profile field is offered rather than shipping one nothing reads.
- **Accuracy is not degraded.** The only per-shot spread value (`SprayAngle`) is definition-owned;
  it could be scaled at point of use the same way the fire rate now is, but nothing asked for it
  and it was left rather than added speculatively.
- **Section 23.3's repair/recovery/escort vehicles are not built.** Step 9 requires them "only
  through normal vehicle/AI APIs", which needs a real vehicle preset; none exists yet (P33's
  backlog). Retreat-to-repair, the fourth item on that list, *is* built.
- **Fuel is not implemented**, per Section 23's own closing line; nothing asked for optional fuel
  either.

**A real pre-existing gap found but deliberately NOT fixed:** the live
`VehicleGameObj::Apply_Control` reads neither `CanDrive` nor `CanFire`, although both are real,
saved, network-replicated fields that TT scripts actively set, and the commented-out
`Apply_Control` at `vehicle.cpp:1604-1667` shows the intended consumption. Left alone on purpose:
Phase 22's own work demonstrated no bug from it (this phase deliberately does not use those fields
-- they are script-owned state that an automatic health-driven write would fight), the scripts
setting them are TT donor content whose intended behavior is not this phase's to decide, and
Section 0 permits an out-of-scope fix only for a *demonstrated* incompatibility. Recorded here so
it is not lost; a future phase that actually needs those fields consumed should fix it then.

**What is not proven:** no real `VehicleGameObj` has ever executed any of this. A
`-terrainselfcheck` run brings up no `GameObjManager` or physics scene -- the same standing
harness limit Phases 20 and 21 already carry -- which is why the response curves were deliberately
made testable as pure functions (`Compute_Subsystem_*`) instead of being left untested behind an
un-constructible object. No vehicle has been observed slowing down, firing more slowly, or driving
itself to a repair bay. `Find_Nearest_Repair_Bay` has never found anything, because no buildable
Repair Bay descriptor exists yet.

**The speed governor is the piece most wanting a live pass.** Its correctness argument rests on a
source reading of the tracked vs wheeled physics paths (tracked terminal velocity is proportional
to throttle; wheeled top speed is set by a throttle-independent redline with drag defaulting to
zero), and its reference speed is learned at runtime rather than derived. Two things to confirm by
hand: that a damaged *wheeled* vehicle now genuinely tops out lower rather than merely accelerating
worse, and that holding a vehicle at its cap does not visibly oscillate or judder.

**Next exact action** is a manual one for the user: run
`renegade --gamedir "C:\Westwood\Renegade_full"` with a profile setting
`vehicle_subsystems.enabled` true, click **Procedural Skirmish**, then (1) note a wheeled vehicle's
healthy top speed on a straight, damage it to ~half health and confirm it now tops out lower;
(2) repeat in a tracked vehicle; (3) confirm turning is worse but reverse/braking still respond;
(4) fire while damaged, confirm the gap between shots grew, and compare against the fire-rate
factor `vehicle_subsystems` reports at the console; (5) confirm an infantry weapon's rate of fire
is unaffected at any vehicle health. Immobilization needs `allow_immobilize` true and the weapon
cut-off needs `weapon_disable_fraction` above 0; AI retreat-to-repair cannot be exercised until a
buildable Repair Bay descriptor exists.

---

## V2 Phase 23 -- Commander governance's vote math is real and tested; no live match has run it

All implementable steps built for real (roadmap Section 25), all three binaries clean,
`shader_governance`/`fds_shader_governance` green (`ctest -N` now 185, was 183). The vote engine
(`Call_Commander_Vote`/`Cast_Commander_Vote`/`Update_Commander_Vote`/`Get_Vote_*`) and
`Transfer_Commander` are real on `CommanderProductionService`; `cCommanderClaimEvent::Act` opens a
vote instead of an instant grant against a contested vacant seat.
`CommanderProductionService::Handle_Player_Reconnect`, called from `cGod::Create_Player`
(`Code/Commando/god.cpp`), carries a held seat across the new client id a rejoining player is
assigned -- Section 25.1's "disconnect recovery" bullet, not just the pre-existing 20s vacancy
grace. See `docs/zerohour/CommanderProduction.md`'s "Governance" section.

**What is not proven:** no real multiplayer session has exercised any of this. The self check
covers the vote math and the reconnect remap as pure functions of ids/bools/a team --
deliberately, the same "no `GameObj` in this harness" limit every Commander-layer check already
carries -- but never sends `cCommanderVoteCallEvent`/`cCommanderVoteCastEvent`/
`cCommanderTransferEvent` over a real connection, never exercises
`Count_Connected_Teammates`/`Is_Client_Connected_On_Team` against a live roster, never drives an
actual `cGod::Create_Player` reconnect, and never renders the `commander_vote`/`commander_transfer`
console output for a human to read. No player has seen a vote actually open, watched its
countdown, had a claim refused because a teammate beat them to it, or dropped and reconnected to
find they still hold command.

**Next exact action** is a manual one for the user, and needs at least two human clients on one
team: (1) with two players connected to the same team and neither holding command, have player A
claim (`b` key or `commander_mode 1`) and confirm `commander_vote status` shows an active `ELECT`
vote rather than an instant grant; (2) have player B cast `commander_vote no` and confirm the seat
stays vacant once the window elapses; (3) repeat with B either voting yes or not voting at all and
confirm A is seated; (4) once seated, have a third player call `commander_vote remove` alone and
confirm the Commander survives an unopposed removal attempt, then have a real majority of the team
vote yes and confirm they are unseated; (5) have the sitting Commander run
`commander_transfer <id>` naming a real teammate and confirm command moves with no vote; (6) have
the Commander die and confirm Commander Mode exits but `commander_rts_status` still names them,
then confirm they can re-enter with no new vote; (7) have the seated Commander disconnect and
reconnect within the 20s grace window under the same player name and confirm
`commander_rts_status` still names them as Commander with no new claim or vote needed.

**No UI beyond console exists**, matching Phase 22's own precedent (console + self-check, no HUD
art) -- Section 25's candidate list, vote countdown and ballot buttons are sidebar/panel work for
whichever phase builds Commander Mode's real UI, not this one.

**`CanDrive`/`CanFire` non-consumption is unchanged from Phase 22's own entry above** -- not
re-investigated, not touched, and still not this phase's to fix; recorded there, not duplicated
here.

---

## V2 Phase 24 -- autonomous AI Commander's real behavior is unproven live

All implementable steps built for real (roadmap Section 24.1-24.4), all three binaries clean,
`shader_commanderai`/`fds_shader_commanderai` green (`ctest -N` now 187, was 185).
`CommanderAIService::Activate`/`Deactivate`/treasury, `Decide_Economy_Priority`,
`Find_Buildable_Descriptor` and `Score_Expansion_Candidate` are real and self-check-covered as
pure functions. See `docs/zerohour/CommanderAI.md`'s full account.

**What is not proven:** `Update` -- the one real building/roster/site-search walk, and
`Refresh_Expansion_Candidates` -- have never run against a live scene. No self check can construct
one (the same limit Phase 22's `Govern_Subsystem_Speed` and Phase 23's
`Count_Connected_Teammates` already document), so none of the following has been observed for
real: an AI Commander actually seating itself and building a Power Plant or Silo when one is
needed; the ring search around a team's own buildings actually finding and using a valid site;
charging its own treasury rather than a player's wallet; standing down cleanly when a human wins a
Section 25.1 `REMOVE` vote against it; or `Refresh_Expansion_Candidates` producing a sane-looking
list against a real generated world's `StrategicPlanner`/`TiberiumFieldGenerator`/
`LogisticsGraphService` data.

**Next exact action** is a manual one for the user, on a generated (not legacy/static) multiplayer
or skirmish map with at least one standing building per faction: (1) `commander_ai activate nod`
(or `gdi`) from the server console and confirm `commander_rts_status`/`commander_ai status` both
show the seat as AI-held; (2) set a nonzero treasury with a debug path if one exists, or confirm
via `commander_ai status` that the AI reads `AI_ECONOMY_PAUSE` when a real need exists but the
treasury cannot afford it; (3) credit enough treasury and wait one `Get_Planning_Interval_Seconds()`
(5s) tick, then confirm a real Power Plant or Silo appears under construction near the team's
existing base and the treasury was actually debited; (4) confirm `commander_ai status`'s
expansion-candidate count is nonzero on a generated map and zero on a legacy one; (5) as a second
player, open `commander_vote remove` against the AI-held seat, get a real majority, and confirm
`commander_ai status` shows the AI stood down (`Is_Active` false) rather than continuing to spend
from a treasury it no longer has authority over.

**Section 24.5-24.9 is now V2 Phase 25's own entry below, not this one.**

**`CanDrive`/`CanFire` non-consumption is unchanged from Phase 22's own entry above** -- not
re-investigated, not touched, and still not this phase's to fix; recorded there, not duplicated
here.

---

## V2 Phase 25 -- production/defense/attack planners are real and unit-tested; none has run live

All implementable steps built for real (roadmap Section 24.5-24.9), all three binaries clean,
`shader_commanderai`/`fds_shader_commanderai` still green with substantially extended coverage.
`Decide_Production_Mix`, `Score_Defense_Point`, `Decide_Attack_Transition` and the three
`Get_Difficulty_Settings` presets are real and self-check-covered as pure functions. See
`docs/zerohour/CommanderAI.md`'s Phase 25 sections for the full account.

**What is not proven:** `Update_Production`, `Update_Defense` and `Update_Attack` -- the real
purchase-page lookup, facility readiness walk, `BridgePlanner`/`CommanderGuardAI` defense
assignment, and `ScriptEngine::Action_Goto`/`Action_Attack` order dispatch -- have never run
against a live scene, the same limit Phase 24's own `Update_Economy` still carries. None of the
following has been observed for real: an AI Commander actually purchasing an infantry or vehicle
unit and having it appear; a defender actually walking to and guarding a real bridge or Power
Plant; an attack force actually assembling at a rally point, marching on a real enemy building,
attacking it, and retreating or exploiting based on real losses; or difficulty actually changing
how fast/cautious/aggressive an AI Commander behaves in a real match.

**Next exact action** is a manual one for the user, on a generated map with both an AI Commander
active (`commander_ai activate nod`, credited via whatever debug treasury path exists) and at
least one enemy building explored: (1) confirm `commander_ai status` eventually shows a
production-mix decision other than 'none' and that a real infantry or vehicle unit appears near a
factory; (2) confirm the status line's defense point/defender counts become nonzero once a bridge
or Power Plant exists, and that the assigned unit(s) are visibly standing guard near it; (3)
confirm the attack stage advances from 'recon' through 'assemble'/'route'/'engage' as real
conditions are met, that units visibly march toward and attack a real enemy building, and that the
stage reaches 'retreat' or 'exploit' appropriately; (4) set `commander_ai difficulty nod hard` and
confirm the AI plans and commits to an attack noticeably faster/with a smaller force than
`commander_ai difficulty nod easy` does in a repeat match.

**No recon/repair/logistics behavior beyond what 24.5-24.7 already cover exists as a distinct
system** -- Section 24.9's "recon" is satisfied by the existing shroud-explored world snapshot
(Section 24.2) feeding the attack planner's own RECON stage; "repair" for an AI-controlled vehicle
is Phase 22's own `Think_Subsystem_Retreat`, unchanged and already team-agnostic; "logistics" is
the existing `LogisticsGraphService` road-distance measurement the attack planner's own target
selection already reads. No new dedicated recon/repair/logistics module was built, because the
roadmap's own listed capabilities under those names were already real, existing systems this
phase reused rather than duplicated -- a real design decision, not an oversight, and disclosed
here rather than silently assumed.

**`CanDrive`/`CanFire` non-consumption is unchanged from Phase 22's own entry above** -- not
re-investigated, not touched, and still not this phase's to fix; recorded there, not duplicated
here.
