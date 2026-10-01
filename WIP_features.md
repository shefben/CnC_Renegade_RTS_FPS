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

## P09 -- the layer exists, the pipelines do not

`ShaderManagerClass` is in place and checked (see `docs/zerohour/ShaderManager.md`),
with stock W3D content registered as a program, debug overlays registered against a
real consumer (`BoxRenderObjClass::render_box`) and, since P22, particles registered
against `WorldParticleBatchManager::Render_Group`. Ten pipelines are left enumerated
and unregistered -- terrain, terrain detail, roads, bridges, water, foliage, projected
shadows, tracers and beams, status markers, ghost building tint -- because none of them
has anything to draw until the terrain framework (P11, P13 to P15, P17) and the
Commander work land. Each registers itself when its system arrives.

One thing remains unproved: nothing here has been seen on a screen, because the checks
run without a device, so whether a debug box still looks like a debug box wants one run
with a display mask on. The acceptance line *new donor systems share one state/shader
management layer* is now carried by a donor system rather than by the overlays -- the
particle pool goes through `Set_Program`/`Reset_Program` for every group it draws.
Next exact action: register the terrain and terrain-detail pipelines against
`RenegadeTerrainPatchClass`, which now has material passes to draw with (P13-A); the other
eight still wait on their systems.

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

A session running on Linux cannot build the client at all -- the CI's Linux job builds
only the maths and utility libraries -- but the device-less self checks can still be
*run*, which is where the world systems' acceptance evidence lives.
`tools/linuxcheck/selfcheck.sh <check>` does it: the check's own translation unit, the
sources it actually exercises, four real support libraries, and an automatically
generated empty stub for every other engine symbol the linker asks for. It is g++ and
not MSVC and nothing in it draws, so it proves policy and arithmetic and not a build;
it found two real faults in P22 before any Windows build existed (a protected
`Invalidate_Cached_Bounding_Volumes`, and a particle program whose `Init` would have
dereferenced a null `VertexMaterialClass` preset table in a process with no device).
`g++ -fsyntax-only` with the same include set type-checks a single wwphys, ww3d2 or
Commando translation unit in seconds; `Code/Commando/consolefunction.cpp` is the one
that cannot, because it reaches Win32 headers no stub replaces.

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

## P21 -- the lights are filtered and nothing has switched one on

`WorldLightManager` is in and checked (P21-A in `completed_features.md`,
`docs/zerohour/WorldLightManager.md`, twenty-eight green terrain ctest entries). No gameplay code
creates a dynamic light yet -- weapons, explosions and Commander feedback are later phases -- and
particles are unlit so they have no light set to filter. Next exact action is a manual one for the
user: run `renegade --gamedir "C:\Westwood\Renegade_full"`, load a level, then `light_test 8 15`
and `light_status` at the console, walk into and out of the ring, and report whether the world
actually lights up, whether the lights-per-query number stays flat after `light_test 64 15`, and what
`light_clear` leaves behind.

## P22 -- budgeted, batched, and nothing has emitted one in anger

`WorldParticleBatchManager` is in and checked (P22-A in `completed_features.md`,
`docs/zerohour/WorldParticleBatchManager.md`), and the acceptance is proved as numbers
device-less -- 1800 particles in two groups are six submissions and two buffer allocations.
What this entry carries is the half no arithmetic can supply: nothing in `Combat` emits into
the pool yet (its callers are P23 weather, P24 tracers, P25 debris and the Commander
feedback of P36/P38, and the authored W3D emitters deliberately keep their own path), and
no batched particle has been drawn, because the six default kinds name no texture. Next
exact action is a manual one for the user: run `renegade --gamedir
"C:\Westwood\Renegade_full"`, load a level, then `particle_texture <a texture name from
that level>`, `particle_test 600` and `particle_status` at the console, and report whether
sprites, points and streaks appear around the camera and what the submission and headroom
numbers say.
## P23 -- the air is counted and nobody has seen a mote

`WeatherEnvironmentRenderer` is in and checked (P23-A in `completed_features.md`,
`docs/zerohour/WeatherEnvironmentRenderer.md`), and the acceptance is proved device-less: six
thousand motes' worth of density comes back as the 900 the budget allows. What this entry
carries is what the numbers cannot show: the three atmospheric modes name no texture, so no mote
has been drawn; the new kinds are reachable from C++ and the console but have no script command
and no LevelEdit radio button; and `Set_Region`, which pins the air to a tiberium field rather
than to the camera, has no caller because nothing reads a weather region out of a level yet.
Next exact action is a manual one for the user: run `renegade --gamedir
"C:\Westwood\Renegade_full"`, load a level, then `weather_texture <a texture name from that
level>`, `weather dust 1`, `weather tiberium 1` and `weather_status` at the console, and report
whether the air fills with motes around the camera and whether the live counts sit on their
targets.
