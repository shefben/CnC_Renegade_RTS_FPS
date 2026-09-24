# WIP Features

Only work that is actually open now, grouped by the categories in `DECISIONS.md` D-001, and worked
in D-001's priority order. Each entry names its owning phase and its next exact action. Finished
work is in `completed_features.md`, and detail is in `docs/`.

**State at `9f1df2ba`:**

- D-001 priorities 1 to 3 are done, and so is P15 bridge damage from priority 5.
- Priority 4 is done except the P09 routing and P26 audit below.
- 199/199 ctest entries pass, and `renegade`, `renegadeserver` and `leveledit` build with no warnings.

**Next exact action:** finish priority 4. Route tracers, status markers and the ghost-building
tint through `ShaderManagerClass`; the particle retirement is written up under P09 below. Then
work through priority 5 in the order listed.

---

## IMPLEMENT NOW

### P09: renderers outside the pipeline layer (D-001 priority 4)

`ShaderManagerClass` registers eight programs. Four later renderers build their own device state
instead of using it.

- **Tracers** (`Code/ww3d2/tracerrenderer.cpp`), **status markers**
 (`Code/ww3d2/statusoverlayrenderer.cpp`) and the **ghost building tint**
 (`Code/Combat/commanderghostrenderer.cpp`) each draw with one fixed shader. Register
 `MATERIAL_PROGRAM_TRACER`, `_STATUS_MARKER` and `_GHOST_TINT` with that exact state, and have each
 renderer take its state from `Get_Pass_State`.
- **Particles** carry per-emitter W3D shaders authored in their definitions, so one program cannot
 describe them. Retire `MATERIAL_PROGRAM_PARTICLE` and write the reason in
 `docs/zerohour/ShaderManager.md`.
- **Terrain detail** remains a real gap. It needs a second UV rate on one layer plus a detail
 layer entry. Do not fake it with the base UVs.

### P26: modular draw components (D-001 priority 4)

Audit each component contract against the consumers that exist now: selection and status visuals,
waypoint and order visuals, ghost buildings, RTS overlays, and procedural-world attachments. Give
each consumer that exists a concrete component. Add the strongest success-path attachment test a
real known W3D asset allows.

### D-001 priority 5: procedural-world gameplay gaps

- **Initial base layout and spawn bootstrap (Section 16A):**
 - faction base transforms, the required initial buildings (the Construction Yard descriptors now exist), spawn locations, team ownership, player spawn sets, and match bootstrap;
 - all taken from `BaseSelectionPlanner`'s selected sites and the transportation layout.
- **Harvesters:** spawn the normal Renegade harvester through the existing vehicle/AI APIs, bound to the generated field/route data. The Refinery descriptors now exist.
- **Building semantic anchors as sidecar data:** entrance, vehicle exit, infantry exit, street-facing side, door, PCT/MCT and destroyed model, all in descriptor/manifest data.
- **Object- and foliage-aware sightline occlusion:**
 - uses the terrain ray plus world-spatial and foliage queries;
 - closes P10's foliage-spatial gap;
 - no full object-list scans.
- **Locomotion capability masks (V2 Phase 16):** one topology with per-sector/per-edge infantry, wheeled, tracked and heavy masks, instead of "most restrictive vehicle wins".
- **Minimap renderer:** a real consumer for `MinimapBaseLayer`, so that logistics routes, blockages and coverage (V2 Phase 21) can be drawn over it later.
- **Campaign launch:** `campaign launch` starts its battle through the Procedural Skirmish load path, carrying profile, seed, territory and factions. A finished match reaches `Apply_Battle_Result`.
- **Functional subcomponents (V2 Phase 20):** resolve map-authored buildings' anchors against the loaded model hierarchy, without requiring `RuntimeParts`.
- **Logistics enemy obstruction (V2 Phase 21):** re-audit against Phase 25's tactical intelligence and threat data.
- **Vehicle subsystems (V2 Phase 22):**
 - a narrow health-derived sensor factor for AI acquisition and detection, which never affects a human's FPS view;
 - weapon accuracy via a spray scale at fire time.
- **AI treasury command (V2 Phase 24):** a deterministic developer/server command to set it, if none exists.
- **Front and travel-time fairness (V2 Phases 11-15):** re-audit `StrategicPlanner` for a meaningful front. Implement `MaxBaseToFrontTravelTimeDifferenceSeconds` if there is one; if not, keep the limit explicit.

### D-001 priorities 4 and 5: effects content and wiring

- **P18 surface ribbons:** give a verified default track texture, then wire vehicle tracks. Add scorch or drag ribbons only where a real event exists.
- **P23 weather:**
 - run the checks;
 - restore camera-relative rain if practical;
 - add invocation paths for `TIBERIUM` (generated fields), `DUST` (profile/biome) and `FALLING` (scripted);
 - supply the documented weather textures.
- **P24 projectile trails:** implement the `AmmoDefinitionClass::TrailEmitter` reader on the pooled renderer.

### D-001 priority 6: automated edge coverage

- **Bridge fallback (V2 Phase 10):** deterministic headless terrain for:
 - stage 1: the original crossing fails and a nearby reposition succeeds;
 - stage 2: the original approach fails on grade and a longer approach succeeds.
- **Attachments:** real attachment success cases, where headless testing permits.
- **Save/load to disk:** disk round trips for RTS state and the procedural world package, where headless testing permits.

---

## MANUAL VERIFICATION ONLY

These need a running client and/or server. Run them as D-001 priority 7, after the implementation
items above that affect them, and then the Phase 28 gate exactly as written.

- **Procedural Skirmish:**
 - check `tunnel_status` and `debug_layer tunnels 1`;
 - walk and drive a tunnel, a lamp-lined street and the coast;
 - check the boundary cliff ring (toe apron, face, vehicle climbability, seams, bounds, riser colours);
 - check roads (S bend, crossing and tee) now that ground and verge textures exist (P14);
 - look at the terrain stand-in textures (P13) and the water surface (P17): carving, shoreline, flow, physics tagging.
- **Commander interaction** (`docs/zerohour/CommanderInteraction.md`, "Needs a live client"):
 - the controls row;
 - key and console entry;
 - click-select and control groups;
 - selection rings;
 - the power panel seen from a second client;
 - a patrol;
 - a vote cast from a second client, and a transfer.
- **World transport** (`docs/procedural_world/NetworkingAndPersistence.md`):
 - identity reaching a second machine;
 - a late join;
 - a `worldop` crater seen on both screens;
 - repair of a corrupted client;
 - save, quit, reload and compare.
 - Live gameplay submits no operations yet; `worldop` drives the journal.
- **Bridges (P15):** walk and drive intact spans, shell one until it falls, see it fall on a second client, and fall through it.
- **Rendering:**
 - stock W3D models unchanged (P09);
 - weapon and explosion lights (P21);
 - particle batching on and off, with emitters per draw call recorded (P22);
 - debris on the live tier (P25);
 - cached shadows on runtime buildings and bridges (P20);
 - bloom first, then light shafts, on a real device: composite, HUD, no render-target state leaks (P55/P57).
- **Gameplay:**
 - Communications Center team visibility (P27);
 - stock FPS radar unchanged, plus the strategic-contact layer (P28);
 - telemetry after real kills and building destruction (P31);
 - placement on slope, overlap, bounds, exit clearance and grading (P37);
 - Commander save and network round trip (P40);
 - runtime terrain collision: infantry, vehicles, stock geometry, transitions (P11).
- **AI Commander:**
 - Phase 24: seating, economy, construction, expansion, treasury debit, removal vote;
 - Phase 25: production, defence, attack. Both use the new descriptors.
- **Carried from P05-P07:** the client/server map transition, and the remote screenshot against a real endpoint.
- **Asset residency (P08):** the map-transition resource-retention check.
- **World tools (V2 Phase 27):** reproduction string, JSON report, PPM preview and seed batch, on a real generated world.
- **Phase 28 gate:** run `docs/integration/Phase28IntegrationGate.md` exactly. Every failure becomes a narrow new issue.

---

## DEFER TO REAL DEPENDENCY

| Item | Waits for |
| --- | --- |
| P08 `SECTOR` residency consumers | Real world streaming |
| P10 foliage in `WorldSpatialIndex` | The sightline occlusion work above, which is its first real consumer |
| P25 city-prop debris | Destructible prop assets or a prop system in settlements |
| P24 rope and cable strips | Content or a feature that needs them |
| Naval navigation and behaviour, `NavalFactoryGameObj` | A canonical naval locomotor (Zero Hour Feature 7) |
| Building interiors in navigation | Interior and portal metadata or assets |
| Tiberium storage gameplay effect | A design that specifies one |
| Repair and escort vehicles | Real vehicle presets and descriptors |
| Missing city/settlement building assets | Content |
| Incremental nav rebuild | A measured hitch (Phase 29) |
| `IsForward` from a real front | A front concept (see fairness above) |
| ZH-04, ZH-18, ZH-24 (`EAValidation`) | Retrieving and comparing the archival EA implementation first |
| Nod Shrine, helipads as Commander buildings | A building definition (Shrine) and model manifests (helipads) |
| Nod Communications Center component assembly | Hand curation of its uncurated parts |

## ACCEPTED LIMITS

- **P51 height fog:** camera-relative, not per-vertex. Reopen only if a programmable shader tier arrives for another reason.
- **P56 tonemap:** a global operator. Local tonemapping is a candidate if a shader tier arrives.
- **Decal pipeline duplicate:** `SurfaceRibbonSystem` and `WorldSurfaceMarkManager` keep near-identical private shader functions. Section 15 enumerates no decal pipeline, so this belongs to whichever section owns decals.
- **Governance voting from FPS mode:** the governance panel lives in Commander Mode, where there is a pointer. A player in FPS mode presses `B` to vote or uses the console.
- **Loaded Commander orders:** orders are restored but not re-dispatched. Guard and patrol alike resume only when reordered.

## PERFORMANCE AND FINAL CHECKLIST

- **P43 / V2 Phase 29:** measure first, during the live session: frame and worst frame, server tick, draw calls, world-system and power-grid cost, generation time, memory, object and batch counts, foliage and chunk submissions, nav rebuild cost, and journal growth, at SMALL, MEDIUM and LARGE_SUPPORTED. Only then tune chunk and foliage LOD and distance culling. Run a long `large_supported` soak watching memory, stale chunks and render resources, leaked navigation data, and journal growth.
- **P45 final Definition of Done:** the 110-item checklist runs only after the implementation and live gates are substantially complete (D-001 priority 9).

---## Working notes that cost a session each

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

