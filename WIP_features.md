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

## Four generators deep into the chunk registry and nothing has asked for any of them live

`MacroTerrainGenerator`, `WorldTerrainSystem`'s chunk registry, `InteriorCliffGenerator`,
`HydrologyGenerator` and now `StrategicPlanner` are all in (see `completed_features.md`,
"V2 Phase 4" through "V2 Phase 7", and each phase's own doc under `docs/procedural_world/`). The
open design question Phase 3's own planning note carried forward -- replace the host level's
ground or coexist with it -- is resolved: coexist, by never running the terrain generator at all
when `world.fit_to_level` is true. That resolution has a sharper consequence than Phase 3's
"built but unseen": the "Procedural Skirmish" menu button's own default profile is
`fit_to_level: true`, so **none of the four has ever run through any live caller, not merely
never been looked at** -- interior cliffs, hydrology and the strategic planner all read Phase 4's
chunk registry (the planner also optionally reads interior cliffs' and hydrology's own
candidates), so each carries the exact same gap one or two levels further down the chain. All
four are proven correct by their own device-less self checks (`-terrainselfcheck macro`,
`interior`, `hydrology`, `strategic`; the latter three against synthetic terrain built
specifically so a feature/candidate is certain to exist, since a real generated seed's relief
gives no such guarantee to check against), none of it against a display.

**Phase 7 was the point an earlier note here expected this gap to resolve itself** ("candidate
bases / initial layout selection... the first later phase that actually needs a world with no
host level under it") -- it did not. `StrategicPlanner` is pure data (a candidate pool, Phase 7
step 12 forbids choosing from it yet), so it still only ever runs inside `Generate_World`,
behind the same `fit_to_level` gate every earlier stage shares, and nothing about producing
candidates itself demands a live, host-level-free world any more than producing terrain or
water did. The next phase that plausibly forces the issue is whichever v2 phase actually
*selects* final bases from this pool and needs to build/place something real there -- not yet
reached.

Reaching any of the four live needs either (a) a profile with `fit_to_level: false` plus
something that requests one -- `ProceduralWorldManager.h`'s own doc comment claims a
`procedural_world` console command exists; it does not, checked directly, and building one is
out of any of these phases' own scope -- or (b) waiting for whichever later phase first needs to
act on this data against a real, unfitted world. Next exact action is one of those two, at the
user's direction: add a minimal `procedural_world <seed> <profile>` console command so a
non-fitted profile can be driven through `Request_World`/`Generate_World` in a live client, then
run `renegade --gamedir "C:\Westwood\Renegade_full"`, use it with a profile whose
`world.fit_to_level` is `false`, and report what `debug_layer terrain_chunks 1`,
`debug_layer interior_cliffs 1` and `debug_layer hydrology 1` show -- in particular whether any
chunk reads yellow (generated, but too steep to build on) where the profile did not intend one,
whether the generated relief looks like terrain rather than noise, whether the interior cliff
walk produces something that reads as a canyon or escarpment rather than an arbitrary scatter of
steep chunks, whether the generated rivers/lakes look like hydrology rather than a chunk-grid
scatter of blue rectangles, and whether `StrategicPlanner`'s own base/resource candidates land
somewhere a person would actually put a base.
