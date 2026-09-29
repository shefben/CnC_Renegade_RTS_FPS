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

**I12's local-pass geometry is correct and its coverage is blocked by two further owner defects, both
measured.** D-012 records the geometry decisions, D-013 the `plateau_basin` audit. 235/235 ctest green on
client and dedicated server.

**Phase 10 now records the crossings it was never sent to make.** A cell carrying a cliff face or a canyon
or ravine wall is hard-invalid unless it lies inside an accepted window, so a window is the only hole in an
immutable barrier -- and a route was free to drive through one it was never given as a stop, with nothing
recorded. Phase 11 therefore compiled nothing for those traversals and Phase 12 would have laid road across
an unmodified cliff face. `Note_Unplanned_Crossings` (`hybridroutes.cpp`) records them as what they are,
using the window's own frozen approach pair oriented by the side the route arrived from, exactly as a
planned crossing does. Measured over 200 seeds x 7 archetypes: 39 of 162 built worlds cross a cliff
obstruction this way. A cut or fill past the class limit is also now reported as `cut_or_fill` rather than
as a steep grade, and the Phase-11 gate prints each seed's Phase-11 refusal.

`renegade -terrainselfcheck hybridinteractionsearch` is the search tool this used. It is registered with
nothing, runs only when named, and asserts nothing.

**Blocker 1 -- the plan's "one more envelope" reroute step is not implemented.** Phase 11 raises
`RerouteRequests` and sets `HybridInteractionResult::RerouteEdgeId`, and **nothing in the tree reads
`RerouteEdgeId`**: grep finds writes only. So the last step of every method order never runs, and a world
with one uncrossable crossing simply fails. That is what recording the unplanned crossings exposed: solved
worlds fell from 79 to 63 of 162, and all 16 are worlds where a route drives through a cliff window it
cannot cross. Phase 10 already loops envelopes 0..2 per edge internally, so the missing piece is a per-edge
minimum-envelope input to `HybridRouteParams` plus a caller loop -- on `RerouteEdgeId != 0`, raise that
edge's floor, re-run Phase 10 and Phase 11, bounded by the two spare envelopes -- in both
`proceduralworldmanager` and the self-check's `Build_Hybrid_Pipeline`. **This is the next action.**

**Blocker 2 -- `Ground_Carries_Swept_Width` makes every ground method impossible below `HIGHWAY_4`, and
needs a ruling.** It requires `max(heavy,harvester) + 2 * min_vehicle_lane_clearance <= Roadbed_Width`.
On the native metrics that is `6.80 + 2.00 = 8.80` against these roadbeds (Section 7's table x `W =
3.322739`):

| class | roadbed in W | roadbed | carries 8.80 |
| --- | --- | --- | --- |
| `HIGHWAY_4` | `4 + 2*0.45 + 0.75 = 5.65` | 18.77 | yes |
| `PRIMARY_2` | `2 + 2*0.30 = 2.60` | 8.64 | no, short by 0.16 |
| `SECONDARY_2` | `2 + 2*0.20 = 2.40` | 7.97 | no |
| `SERVICE_DIRT_2` | `2.40` | 7.97 | no |
| `TRAIL_1` | `1 + 2*0.10 = 1.20` | 3.99 | no |

So a ford, surface pass, ramp pass or road cut can only ever compile on a `HIGHWAY_4` edge, and
`PRIMARY_2` misses by 0.16 world units. Phase 6 proves pass windows for `PRIMARY_2` per step 18, which no
`PRIMARY_2` edge can then use. Evidence that the predicate may be the wrong one for *ground*: Section 12.4's
`max(carriageway, swept + 2*clearance)` is explicitly a **structure** clear width, while Phase 9's own
corridor half width is `0.5 * Roadbed_Width(PRIMARY_2) + min_vehicle_lane_clearance` -- clearance reserved
*outside* the roadbed rather than contained in it. Under that reading the ground test is `swept <= roadbed`
(6.80 <= 8.64, 0.92 spare each side) with the clearance carried by the corridor reservation. **Nothing was
changed**: this is a constraint, and which rule governs ground is the user's call.

Measured refusals on the routed cliff crossings, which is the whole of the remaining coverage gap:
`clearance` where the edge is `PRIMARY_2` (blocker 2), and `cut_or_fill` where it is `HIGHWAY_4`. The
second is legitimate arithmetic: Section 5 gives a cliff a drop of `tier_step * [1.0, 2.0]` = 18 to 36
over a band of `0.5*EdgeWidth + TalusWidth` = 4.15 to 12.46 either side, so only a window where the field
has decayed near a cliff's end can be crossed inside `max_cut_depth`. Phase 6's `cut_capable` is not a
measured capability -- step 18 sets it for every cliff station that survives the approach and grade rules
-- so the 36 to 67 cut-capable windows per world are not evidence that a cut fits.

**Required before I13**, unchanged: `surface_pass`, `road_cut` and `ramp_pass` each executing in the real
pipeline, each independently asserted through real Phases 6, 9, 10 and 11 with the earlier methods in the
order handled correctly, the compiled record valid, the checksum reproducing and client and FDS agreeing.
Then `terrain_hybrid{localpass,hydrology,topology,routes,interactions}` and their `fds_` twins, both full
ctest suites and `tools/validate_feature_state.py`, and update the I12 completion record so it no longer
says those branches are unexercised.

Remaining upstream refusals across the 40 fixtures, each its own owner's defect and none of them to be
weakened for a test: overpass quota short, river bridge quota short, `hill 13 exhausted 32 alternate
candidates` (`Q-018`), `HIGHWAY_4 has no legal route`, `field reserve is illegal`.
