# WIP Features

Only work that is actually open now, grouped by the categories in `DECISIONS.md` D-001. Finished
work is in `completed_features.md`, detail is in `docs/`, and every piece of art or content the
engine asks for and does not have is in `docs/assets/MissingAssets.md`.

**State:**

- Every IMPLEMENT NOW and RE-AUDIT item of D-001, priorities 1 to 6, is implemented and checked.
- 201/201 ctest entries pass, and `renegade`, `renegadeserver` and `leveledit` build with no
  warnings.
- What remains needs a running client and server (priority 7), live measurements (priority 8),
  and then the final checklist (priority 9). It also needs the art listed in
  `docs/assets/MissingAssets.md`, which never blocks the code.

**Next exact action:** D-001 priority 7. Run the manual verification list below with a client
and a dedicated server, starting with a Procedural Skirmish on a generated-terrain profile. Then
run the Phase 28 gate exactly as written. Every failure becomes a narrow new issue.

---

## IMPLEMENT NOW

Nothing. A failure found in priority 7 comes back here as a narrow item.

---

## MANUAL VERIFICATION ONLY

These need a running client and/or server. They are D-001 priority 7; after them, run the
Phase 28 gate exactly as written.

- **Procedural Skirmish (a profile with `fit_to_level: false`, so terrain is generated):**
  - check `tunnel_status` and `debug_layer tunnels 1`;
  - walk and drive a tunnel, a lamp-lined street and the coast;
  - check the boundary cliff ring (toe apron, face, vehicle climbability, seams, bounds, riser
    colours);
  - check roads (S bend, crossing and tee) (P14);
  - look at the terrain stand-in textures (P13), the water surface (P17: carving, shoreline,
    flow, physics tagging) and, once `ow_ter_detail` ships, the detail layer (P09).
- **Generated base (Section 16A):**
  - both starting bases stand, finished and powered, facing each other;
  - the factory exit opens toward the enemy, and no building overlaps another;
  - players spawn from the pool, behind the base and clear of buildings, and respawn away from
    enemies;
  - save, quit and reload: the bases come back as runtime buildings with their models.
- **Harvesters:** the refinery requests the stock harvester from the factory, and it drives to
  its team's generated field, harvests, docks at the refinery's `harvester_dock` and pays out.
- **Commander interaction** (`docs/zerohour/CommanderInteraction.md`, "Needs a live client"):
  - the controls row;
  - key and console entry;
  - click-select and control groups;
  - selection rings;
  - the power panel seen from a second client;
  - a patrol;
  - a vote cast from a second client, and a transfer;
  - the minimap: it draws, its markers are in the right places, and a click moves the camera.
- **World transport** (`docs/procedural_world/NetworkingAndPersistence.md`):
  - identity reaching a second machine;
  - a late join;
  - a `worldop` crater seen on both screens (live gameplay submits no operations yet; `worldop`
    drives the journal);
  - repair of a corrupted client;
  - save, quit, reload and compare.
- **Bridges (P15):** walk and drive intact spans, shell one until it falls, see it fall on a
  second client, and fall through it.
- **Campaign:** `campaign launch` from the menu console starts a Procedural Skirmish with the
  territory's seed. Its game over moves the territory to the winner. A restart restores the
  campaign file.
- **Rendering:**
  - stock W3D models unchanged (P09);
  - weapon and explosion lights (P21);
  - particle batching on and off, with emitters per draw call recorded (P22);
  - projectile trails on rockets and grenades (P24);
  - tank and harvester tracks drawn with the `d_tk_dirt` stand-in (P18);
  - rain slanting when you run sideways through it; dust, Tiberium haze and `weather_falling`
    visible where turned on (P23);
  - debris on the live tier (P25);
  - cached shadows on runtime buildings and bridges (P20);
  - bloom first, then light shafts, on a real device: composite, HUD, and no render-target
    state leaks (P55/P57).
- **Gameplay:**
  - Communications Center team visibility (P27);
  - stock FPS radar unchanged, plus the strategic-contact layer (P28);
  - telemetry after real kills and building destruction (P31);
  - placement on slope, overlap, bounds, exit clearance and grading, including the factory's
    +X exit (P37);
  - Commander save and network round trip (P40);
  - runtime terrain collision: infantry, vehicles, stock geometry, transitions (P11).
- **AI Commander:**
  - Phase 24: seating, economy, construction, expansion, treasury debit
    (`commander_ai treasury` to set it), removal vote;
  - Phase 25: production, defence, attack;
  - a damaged AI vehicle acquires at shorter range, and a player's vehicle is unaffected;
  - convoys refuse a route an enemy is known to sit on when `logistics.enemy_threat_radius` is
    set.
- **Carried from P05-P07:** the client/server map transition, and the remote screenshot against
  a real endpoint.
- **Asset residency (P08):** the map-transition resource-retention check (Q-006).
- **World tools (V2 Phase 27):** reproduction string, JSON report, PPM preview and seed batch, on
  a real generated world.
- **Phase 28 gate:** run `docs/integration/Phase28IntegrationGate.md` exactly. Every failure
  becomes a narrow new issue.

---

## DEFER TO REAL DEPENDENCY

| Item | Waits for |
| --- | --- |
| P08 `SECTOR` residency consumers | Real world streaming |
| P25 city-prop debris | Destructible prop assets, or a prop system in settlements |
| P24 rope and cable strips | Content or a feature that needs them (nothing in the donor or content does) |
| Naval navigation and behaviour, `NavalFactoryGameObj` | A canonical naval locomotor (Zero Hour Feature 7) |
| Building interiors in navigation | Interior and portal metadata, or assets |
| Tiberium storage gameplay effect | A design that specifies one |
| Repair and escort vehicles | Real vehicle presets and descriptors |
| Settlement building models | Content (`docs/assets/MissingAssets.md`) |
| Incremental nav rebuild | A measured hitch (Phase 29) |
| Scorch and dirt surface marks, scorch and drag ribbons | A real event that produces them |
| Entrance, infantry-exit and street-side anchors; the Airstrip's vehicle exit | A source for them in the model data, or hand authoring |
| ZH-04, ZH-18, ZH-24 (`EAValidation`) | Retrieving and comparing the archival EA implementation first |
| Nod Shrine, helipads as Commander buildings | A building definition (Shrine) and model manifests (helipads) |
| Nod Communications Center component assembly | Hand curation of its uncurated parts |
| Commander sidebar air page and building icons | Content: a purchase page, and packaging of existing art |
| Section 16A.8 match settings (credits, wallet cap, starting units, shroud policy) as profile data | A profile that needs different values from the game mode's |

## ACCEPTED LIMITS

- **P51 height fog:** camera-relative, not per-vertex. Reopen only if a programmable shader tier
  arrives for another reason.
- **P56 tonemap:** a global operator. Local tonemapping is a candidate if a shader tier arrives.
- **Decal pipeline duplicate:** `SurfaceRibbonSystem` and `WorldSurfaceMarkManager` keep
  near-identical private shader functions. Section 15 names no decal pipeline, so this belongs to
  whichever section owns decals.
- **Governance voting from FPS mode:** the governance panel lives in Commander Mode, where there
  is a pointer. A player in FPS mode presses `B` to vote or uses the console.
- **Loaded Commander orders:** orders are restored but not re-dispatched. Guard and patrol alike
  resume only when reordered.
- **One sector graph:** the PathfindClass sector graph stays the worst-case (heavy) answer, so
  the stock AI's pathing is unchanged. Per-class traversal is `NavMeshGenerator::Is_Reachable`
  over the capability masks.
- **Environment weather is per client:** dust, Tiberium haze and scripted falling particles are
  derived on each client and are not saved or sent.

## PERFORMANCE AND FINAL CHECKLIST

- **P43 / V2 Phase 29 (D-001 priority 8):** measure first, during the live session:
  - frame and worst frame, server tick, draw calls;
  - world-system and power-grid cost, generation time, memory;
  - object and batch counts, foliage and chunk submissions;
  - nav rebuild cost and journal growth;
  - at SMALL, MEDIUM and LARGE_SUPPORTED.

  Only then tune chunk and foliage LOD and distance culling. Run a long `large_supported` soak
  watching memory, stale chunks and render resources, leaked navigation data and journal growth.
- **P45 final Definition of Done (D-001 priority 9):** the 110-item checklist runs only after the
  live gates are substantially complete.

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

## D-003 native visual run (started 2026-09-26)

State: every plan consumer is in and green (see `completed_features.md`, D-003 consumers).
`data/procedural_world/native.json` enables the plan. No self-check drives the whole
`ProceduralWorldManager` pipeline, so plan-to-world integration is proven per owner only.
Next action: run the native profile once in-engine with the plan enabled (the open D-002
visual gate), confirm bases sit on the planned discs, the Tiberium field on the centre disc,
roads along the lanes and pools where the plan put water; record the result in
`completed_features.md` or file the failure as a WIP entry against the owner at fault.

## Hybrid generator rewrite (started 2026-09-26)

Continue `Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md` using
`Procedural_World_Generator_Rewrite_CHECKLIST.md`. I0 through I10 are recorded in
`completed_features.md`: I7 (runtime Phase 6, vector hydrology), I8 (Phase 7, macro terrain
realization), I9 (Phase 8, frozen base/Tiberium anchors) and I10 (Phase 9, transportation topology)
all closed on 2026-09-27 with their gates green on client and dedicated server, decisions D-005 to
D-008, and docs under `docs/procedural_world/Hybrid{Hydrology,MacroTerrain,Anchors,Topology}.md`.
**Q-018** (LARGE hill quota) and **Q-019** (canyon bridge quota on archetypes with no canyon
obligation) are open and neither blocks code.

**I11 (runtime Phase 10) is closed** -- see `completed_features.md` and `DECISIONS.md` D-009 with its
2026-09-28 revision. The whole 231-test ctest suite is green on client and dedicated server.

**I12's bridge/ford-disabled/junction/overpass behaviour is closed, and `Q-019` is closed** (D-011).
Full suite 235/235 on client and dedicated server.

**I12: the reroute fallback and the ground width rule are both fixed; the three ground-method fixtures are
still outstanding.** D-012 records the local-pass geometry decisions, D-013 the `plateau_basin` audit.
235/235 ctest green on client and dedicated server, `tools/validate_feature_state.py` green.

**Phase 10 records the crossings it was never sent to make.** A wall cell is hard-invalid unless it lies
inside an accepted window, so a window is the only hole in an immutable barrier, and a route could drive
through one it was never given as a stop with nothing recorded -- Phase 11 compiled nothing and Phase 12
would have laid road across an unmodified cliff face. `Note_Unplanned_Crossings` records them on the
window's own frozen approach pair, oriented by the side the route arrived from. 39 of 162 built worlds
cross a cliff this way.

**The plan's next-envelope reroute is implemented end to end.**
`Hybrid_Solve_Routes_And_Interactions` (`hybridinteractions.cpp`) is the shared owner of Phase 10, Phase 11
and the reroute between them, and both drivers -- `ProceduralWorldManager::Build_Hybrid_Interactions` and
the self-check's Phase-11 gate -- go through it. `RerouteEdgeId` is no longer write-only. The rules: the
next envelope is strictly the one after the envelope that produced the refused route, so a reroute always
moves outward; a floor already at or past the one being asked for ends the attempt, so the same route state
cannot come round twice; `next > 2` fails with the interaction failure Phase 11 named; Phase 9 and the
topology are never touched. `HybridRouteParams::EnvelopeFloors` carries the per-edge floor and
`HybridRouteParams` is kept on the manager (`HybridRouteInputs`) and on the self-check fixture so the
re-run has the same inputs.

Phase 10 is re-run whole rather than patched for one edge, and that is deliberate: a route is routed to
avoid the routes accepted before it, so moving one spline makes the later edges' avoidance data stale, and
splicing a new spline into the old set would leave exactly the half-updated state transactionality forbids.
Re-running makes the step transactional for free -- the accepted state is one Phase-10 run and one Phase-11
run over it, so no old crossing record, interaction, unplanned crossing or checksum contribution survives --
and every edge but the rerouted one is byte-identical, because the search is deterministic and only that
edge's starting envelope changed.

Measured: the Phase-11 gate recovered from `bridge 3, junction 3, overpass 3` to `bridge 8, junction 4,
overpass 7` -- exactly the pre-regression figures -- spending **1 reroute**. The 16 worlds that recording
unplanned crossings had cost are back. `Check_Hybrid_Interactions` also proves the floor is honoured
directly: a route asked to start at envelope 1 must not come back in envelope 0.

**`Ground_Carries_Swept_Width` now compares the right two quantities.** It is
`required_swept_width <= RoadbedWidth(class)`. The `+ 2 * min_vehicle_lane_clearance` expansion is the
plan's *clear width* -- a bridge deck, a tunnel bore, a canyon pass's continuous floor -- and not the
definition of a roadbed; requiring the roadbed to contain it made every class below `HIGHWAY_4` unable to
carry any ground method, `PRIMARY_2` failing by 0.16 world units, which was an artifact of comparing the
wrong quantities. The clearance checks the plan does name are untouched: `Floor_Is_Legal` still requires
`2 * floor_half >= swept + 2 * clearance`, Section 12.4 still governs a bridge deck and Section 12.5 a
tunnel bore. No road was widened.

**The required swept width follows the edge's own capability**, through `Required_Swept_Width`: the heavy
reference for `HIGHWAY_4` and `PRIMARY_2` (Section 9.5's heavy route classes), the Harvester for
`SECONDARY_2` (as the ford path already reads it), and the light reference below that. A trail no longer
becomes heavy-capable because a heavy vehicle was measured somewhere. `HybridInteractionParams` gained
`LightVehicleWidth`, wired from `HumveeWidth` in both the manager and the fixture.

**Still open, in order:**

1. **`cut_capable` is still a semantic lie.** Phase 6 sets `HYBRID_WINDOW_METHOD_CUT` for every cliff
   station that survives the approach and grade rules without measuring whether a legal cut fits, so the
   36 to 67 "cut-capable" windows per world are not evidence a cut is possible. Either compute it in Phase 6
   against the same planning-level requirements Phase 11 revalidates (minimum centerline correction within
   `grading.max_cut_depth`, no sample needing fill past `grading.max_fill_height`, the cut inside
   `RoadDeformationHalfWidth` of the canonical planning class, the outer blend reaching untouched terrain
   with no immutable wall, base or Tiberium conflict), or stop calling it `cut_capable` and store it as an
   unresolved method possibility. The diagnostics have to distinguish "structurally available" from
   "actually valid for this road".
2. **The three ground-method fixtures.** `surface_pass` (canyon/ravine, bridge fails, pass succeeds),
   `road_cut` (explicit cliff, cut succeeds) and `ramp_pass` (explicit cliff, cut fails, pass succeeds) are
   distinct cases and none may be counted for a branch it never executes. The corrected width rule may make
   `PRIMARY_2` cases reachable that were impossible before, so the deterministic seed search
   (`renegade -terrainselfcheck hybridinteractionsearch`, registered with nothing, asserts nothing) has to
   be redone before any other generator policy is touched. Freeze the first qualifying seed of each kind as
   a permanent fixture.
3. Then the I12 gate: all three branches through the real pipeline, each independently asserted through real
   Phases 6, 9, 10 and 11 with the earlier methods in the order handled correctly, the compiled record
   valid, the checksum reproducing, client and FDS agreeing; `terrain_hybrid{routes,interactions}` and their
   `fds_` twins; both full suites; `tools/validate_feature_state.py`. Update the I12 completion record,
   commit, then I13.

Remaining upstream refusals across the 40 fixtures, each its own owner's defect and none of them to be
weakened for a test: overpass quota short, river bridge quota short, `hill 13 exhausted 32 alternate
candidates` (`Q-018`), `HIGHWAY_4 has no legal route`, `field reserve is illegal`.
