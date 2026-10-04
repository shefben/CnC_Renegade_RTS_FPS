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

**I12 is closed (2026-09-30), and `Q-021` is closed (D-016).** Section 10.2's cliff talus range is
`1W..5.25W`, generator identity 12, and the band ramp accepts a `tier_step`-high cliff from `4.0W` of talus.
235/235 ctest on client and dedicated server, the five `terrain_hybrid*` gates and their `fds_` twins green,
`tools/validate_feature_state.py` green.

Carried, not blocking: no *routed* native `ramp_pass` has been found yet. The census over 1400 seeds shows
the range occurring naturally and one pass-capable native cliff window (`coastal_peninsula` seed 137), none
crossed by a route; D-016 estimates a routed one at the order of thousands of seeds. The search tool
(`renegade -terrainselfcheck hybridinteractionsearch`) prints each pass-capable cliff as `RAMPWINDOW` and the
cliff dimension distribution, so a wider walk needs no new instrumentation; when one is found, freeze it as
the permanent native fixture in `Check_Hybrid_Interactions`. `Q-018` (LARGE hill quota) remains open.

**The Q-022/Q-023 crossing ownership correction is closed (2026-10-02)** -- D-017, D-018, D-009's 2026-10-02
revision, generator identity 13; 237/237 ctest on client and dedicated server. Carried, not blocking: the
end-to-end yield is low -- of 1400 census attempts 172 pass Phase 9, 32 Phase 10 and 6 Phase 11 -- and what
stops the rest is upstream owner refusals (Phase-9 overpass and river-bridge quotas, Phase-10 hard-invalid
ground and no-path, Phase-11 road x road crossings on synthesized cross-links). No native seed resolves a
reroute; the one-reroute fixture is the controlled world in `Check_Controlled_Reroute`. `Q-018` remains open.

**Planner-yield stabilization pass closed (2026-10-02)** -- Q-024 (D-019) and Q-025 (D-020), generator identity 14.

**Ordinary-guide / structural-transition stabilization closed (2026-10-02)** -- D-021, generator identity 15.
- Ordinary guides are strategic anchors: the chain passes the guide's cell, and the centerline need not.
- Logical slicing takes the nearest point to the guide cell's centre.
- Five Phase-10 crossing defects were fixed: river moves and steps between cell centres, the own window re-crossed,
  unplanned crossings never aligned, departure room, and the stale lead decision.

1400-world yield is 448 / 107 / 35 (was 448 / 69 / 24). Structural transitions fell from 91 to 20, every one
classified by exact cause in D-021. None is a Q-023 / Phase-10 mismatch.

Accepted 2026-10-03 (checkpoint `5384130c`). The plan now authorizes each of the following as written, and the code
follows them:
- `Phase10NaturalCrossingAlignmentRepair`, in Plan Section 8 Phase 10 and Section 17: one local retry per promotion
  or withdrawal, sharing the envelope budget with the step-5 insertions; the window is unavailable to the edge for
  the rest of the envelope; everything resets only by advancing the envelope.
- Segment-vs-river testing.
- Symmetric arrival/departure turn room, judged by one owner (`Transition_Room`).
- Rebuild of stale structural leads.

The remaining structural-transition failures stay strict. After the alignment the census reads 448 / 98 / 33,
with 22 structural transitions (D-021 revision).

Carried, measured, not yet resolved:
- 2 of 196 reroutes resolve.
- The river-bridge quota opposite-bank investigation (I10 open item). The quota stays mandatory. It does not block
  HIGHWAY_RAMP, but **must be resolved before final release acceptance**.

`hybridinteractionsearch` remains a registered census self-check (no environment switches).

**I13 / runtime Phase 12 in progress.** Committed: `39e23bea` (Section 12.1 solver, 12.2 cross-section) and
`0c8328fa` (Q-022/Q-023). Phase 12's inputs exist: `Hybrid_Realized_Route`, `Hybrid_Road_Profile_Structures`,
`Hybrid_Solve_Road_Stretch`. The highway-access-ramp extension (Plan Section 12.6, added 2026-10-02 by another
session) reopens items in I1 -> I2 -> I10 -> I11 -> I12, which the checklist orders before the rest of I13.

**HIGHWAY_RAMP extension in progress (2026-10-03).** `Q-026` is closed: option (b), the auxiliary-lane profile
(D-022, Plan Sections 12.6.6, 12.6.7, 12.6.11, 12.6.13 and 12.6.15 rewritten). Step 1 is done:
- the `HIGHWAY_RAMP` class, with asymmetric shoulders;
- the topology kinds, the interchange, retarget and ramp-interval records;
- route metadata and widened intervals;
- junction roles and merge/diverge records;
- every stage checksum;
- generator identity 16.

`Check_Hybrid_Ramp_Records` covers it (checklist I1 reopened items closed).

I2 reopened items are done (2026-10-03), under the D-022 revision: the ramp reference spline is its travel lane
(`RampLaneCenterline`), its roadbed runs `-0.70W..+0.80W` of it, and the `2W` gore shoulder transition changes only the
edges. Landed:
- `Hybrid_Interchange_Constants`;
- `Hybrid_Ramp_Candidate`, `Hybrid_Ramp_Control_Polygon` and `Hybrid_Auxiliary_Lane_Section`;
- `Hybrid_Ramp_Gore_Shoulders`;
- `Hybrid_Ramp_Centerline`, on Phase 10's own corner construction;
- the preflight `BridgeApproachMax` and `OverpassDepthMax`;
- `Check_Hybrid_Interchange_Construction`.

Phase-9 step 15 (checklist I10 reopened) is done, and the Phase-9 gate is restored (2026-10-03). D-023 removes the
arrival-angle prefilter: test 7 is now turn room plus reachability, and the at-grade diamond angle is `30..150` on
the crossing edge.

Natural-world gate: met. The 1400-world census froze three `SIDE_ACCESS` interchanges, each at `k = 2`, with
checksums identical on rebuild. Of 124 anchors, 121 had no valid candidate. Refusals were dominated by test 1
(2605), then test 5 (984), test 7 (208) and test 2 (81). Phase 10 does not yet construct ramps, so archetype 5
seed 16 fails Phase 10 on its ramp edge (census 448 / 97 / 33).

Phase-10 interchange realization (checklist I11 reopened, Plan 12.6.9) is done, and the Phase-10 gate is restored
(2026-10-03):
- an aligned highway straight over `[X - S_I, X + S_I]`;
- group-1b construction of the ramps and crossing by the shared 5W constructor, re-validated against the routed
  world and the group-1 roads;
- footprint keep-out;
- withdrawal (a) and (b) to the fallback variant;
- the gate clause.

The census is back to 448 / 98 / 33. Interchange and world outcomes are separate facts (user, 2026-10-03): a
withdrawal stands when the restored ordinary road later fails. `WORLD_FAILED_BEFORE_INTERCHANGE_REALIZATION` is
reserved for a world that fails before the interchange is realized or withdrawn. Measured on the three natural
interchanges:
- archetype 5 seed 16: `INTERCHANGE_WITHDRAWN`, `WORLD_ROUTED`. Rule (a): its straightened highway enters a
  Tiberium reserve. The ordinary junction at `X` is restored, and the world fails later, in Phase 11.
- archetype 5 seed 199: `INTERCHANGE_WITHDRAWN`, `WORLD_FAILED_AFTER_INTERCHANGE_WITHDRAWAL`. Rule (a): aligned
  edge 5 meets hard-invalid ground. The restored fallback then fails on edge 6's grade, exactly as before
  interchange support.
- archetype 3 seed 129: `INTERCHANGE_WITHDRAWN`, `WORLD_FAILED_AFTER_INTERCHANGE_WITHDRAWAL`. Rule (a): aligned
  edge 8 has no path. Its fallback then fails on turn/approach pose, exactly as before.

None is a ramp-geometry failure. Only realized and withdrawn outcomes count when judging ramp correctness.

Phase-11 interchange compilation (checklist I12 reopened, Plan 12.6.10) is done, and the Phase-11 gate's
interchange clause is restored (2026-10-03). It compiles:
- one `HIGHWAY_RAMP_MERGE`/`DIVERGE` record and sample set per ramp, and one `HIGHWAY_RAMP_LOCAL_JUNCTION` per `L_s`
  carrying `z_L`;
- no junction at `X` on a `SIDE_ACCESS` highway;
- the grade-separated DIAMOND crossing as its interchange's mandatory overpass over the continuous highway;
- gore-reserve station and overpass-assignment tests, and the footprint + `RoadOuterBlendWidth` at-grade rule;
- whole-ramp Section-12.1 solves, and the swept proxy.

The interchange element is never rerouted. Phase 11 reads `Hybrid_Route_Edge_End`, so a withdrawn interchange's
fallback junction at `X` compiles. Readings taken are recorded in D-024, for review.

D-024 is resolved (2026-10-03): the effective ramp roadbed `max(1.50W, swept)`, `E` frozen by Phase 9. The
Section-14 gate before I13 is met (see completed_features.md). Census r9: 448 / 98 / 33, interchange outcomes
unchanged.

Phase-12 step 1 is done (2026-10-03): `Hybrid_Plan_Realization`, `Hybrid_Deform_Terrain` and `BridgeAssembly`,
plus the Section-12.1 clamp-walk rounding fix. See `docs/procedural_world/HybridRealization.md`.

Phase-12 step 2B is done (2026-10-03, D-025):
- `HybridRoadRealizer` realizes once on `WorldTerrainSystem`, `RoadSystem` (with the edge profile) and
  `BridgeSystem` (one bridge per `BridgeAssembly`), verifies, and tears down in reverse order;
- the manager's `Realize_Hybrid_Roads` replaces the legacy strategic plan, road graph, bridge and tunnel plans and
  base selection on the native hybrid path;
- migration adapters publish into `StrategicPlanner`, `RoadGraphGenerator`, `BridgePlanner` and
  `BaseSelectionPlanner`;
- a live gate re-verifies before road geometry is built;
- tunnels are deferred (`tunnel_available = false`).

A native world cannot be generated through the manager inside the self-check environment: `Generate_World` needs a
loaded level and real content for preflight. The live path is therefore covered by:
- the same `Realize` / `Verify` / `Publish_Adapters` / `Clear` path on controlled worlds with real terrain, roads and
  bridges;
- the in-attempt live gate.

Phase-12 step 3 is done (2026-10-04, D-026): Section 12.3's physical half (`Verify_Physical`) runs after
realization and in the live gate. It found and fixed two native-cell deformation defects: the whole-cell roadbed and
abutment seats over water. All I13 live-gate items are met except the in-game run.

**Next exact action:** the I13 in-game smoke, which needs the user's GUI: no command line or server path starts a
native world, and Release builds log nothing. Steps:
1. Main menu, Start Procedural Game.
2. Once loaded, open the console and run `procedural_world status`.
3. Expect `Phase 12 realized 1 time(s) ... Section 12.3 green`, with RoadSystem and BridgeSystem counts equal to the
   stretches and assemblies.

Then close I13 and start Phase 13's Tiberium asset inventory.

**Deferred:** tunnel realization on the native hybrid path (user, 2026-10-03). No Phase-12 tunnel owner or adapter
exists. To be planned as its own item.

**I14 finding (2026-10-03), recorded for Phase 13.** Nothing spawns Tiberium crystals today:
- `TiberiumFieldGenerator` emits position records only (`TiberiumCrystalPlacement`) and stamps the Tiberium mask;
- no crystal preset, definition or W3D model is registered in the procedural path or in `ProceduralAssetRegistry`
  (the header names only the intended `ow_tiberium_crystal` family);
- `TT_Game`'s `Plant_Crystals` takes a free-form `Crystal_Preset` parameter with no default;
- crystal positions are not checked against roads, bridges or water: only the field footprint's centre and rim are
  checked, and bridges are not checked at all.

The user's I14 directive (2026-10-03) sets the order:
1. Inventory `always.dat` and the loaded definitions (`ow_tiberium_crystal`, crystal W3D models, crystal
   presets/definitions, harvestable-crystal scripts/components).
2. Use an original harvestable crystal preset directly if one exists.
3. Otherwise, define one native-hybrid crystal definition on an original Renegade crystal model, reusing the
   canonical Tiberium/harvesting behaviour.
4. Otherwise, record `MISSING_TIBERIUM_CRYSTAL_ASSET` and stop crystal realization.

Field positions are filtered first: no collision-bearing crystal may intersect roads, bridge/overpass reserves,
water, the Harvester apron or the turning disc.

Generator identity stays 16: there is no bump for ramp substeps. Keep the remaining structural-transition failures
separate. The river-bridge quota's opposite-bank investigation stays open and must be resolved before release
acceptance.