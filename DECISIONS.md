# Decisions

Resolved user and architecture decisions. Per `PROJECT_RULES.md`, entries here are authoritative
over roadmap wording, and over anything in `WIP_features.md` that contradicts them.

---

## D-001 — Rulings for the remaining WIP features (2026-09-23)

Source: the user, after the V2 close (`f0ee3c62`, tag `v2.0-baseline`). It governs how every open
entry in `WIP_features.md` is classified and closed, and the order the remaining work is done in.
Recorded as given.

### Classification

Do **not** treat every item in `WIP_features.md` as the same kind of backlog entry. Classify each as:

1. **IMPLEMENT NOW** — real missing behavior required by the project or now unblocked by later work.
2. **RE-AUDIT / STALE WIP** — the original dependency has since been implemented, so inspect the
   current code and either wire the feature or close the obsolete WIP entry.
3. **MANUAL VERIFICATION ONLY** — implementation is already complete and the remaining requirement
   genuinely needs a running client/server.
4. **DEFER TO REAL DEPENDENCY** — there is genuinely no meaningful caller/content/system yet. Do not
   create fake plumbing just to make a checklist green.
5. **ACCEPTED LIMIT** — architectural limitation already investigated and intentionally accepted.

Do not mark an item complete merely because its unit/self-check passes. Conversely, do not invent
code when the WIP explicitly says no real consumer exists.

### Cross-cutting: existing failing tests — IMPLEMENT NOW

Run the complete current test suite first and establish the actual current failure list. The
historical count (30, later 24) must not become the permanent baseline. Investigate and close
project-owned failures involving descriptor/preset resolution, construction, production, shroud
visibility, overlays, power grid, save/load and visual settings. Group failures by root cause rather
than patching individual tests. A failing test may remain only if it is conclusively a
harness/environment limitation, and that reason must be documented beside the test. Do this before
the Phase 28 live integration run.

### Original P-series WIP

- **V2 close / overall live gate — MANUAL VERIFICATION ONLY, AFTER BLOCKERS.** Keep it, but do not run
  it next. First complete the implementation items below that affect what the live pass validates;
  then the Procedural Skirmish tunnel/street/coast check and the Phase 28 multiplayer gate.
- **P08 Asset Residency — DEFER + MANUAL VERIFY.** No invented consumers for `SECTOR`, generated
  buffers or material registrations. Keep `PERMANENT`, `GAME_MODE`, `WORLD`. Add `SECTOR` consumers
  only when world streaming exists. Map-transition/resource-retention check is manual. No
  architecture change.
- **P09 Shader/Pipeline Coverage — RE-AUDIT NOW.** Audit particles, tracers, status overlays and
  Commander ghost buildings against the P09 requirement: if a renderer uses `ShaderManagerClass`,
  close its item; if it bypasses it without technical reason, wire it; if it intentionally owns
  another path, document why the P09 entry retires. *Terrain detail* is a real gap: do not fake it
  with the same UVs; implement only with a second texture-coordinate/detail-rate path and a real
  detail layer entry. *Stock W3D compatibility* stays live visual regression verification; stock
  models/materials stay unmodified.
- **P10 World Spatial Index / foliage — DEFER UNTIL OCCLUSION WORK, THEN IMPLEMENT.** Do not connect
  `FoliageSystem` to `WorldSpatialIndex` just for P10. When object/foliage-aware sightline occlusion
  is built, provide foliage spatial queries through the world-query architecture and use them for
  visibility tests instead of another full-world scan. Performance acceptance needs live profiling.
- **P11 Runtime Terrain Collision — MANUAL VERIFICATION ONLY.** Infantry walking, vehicle traversal,
  stock W3D geometry, generated-terrain/existing-geometry transitions.
- **P13 Terrain Textures — IMPLEMENT NOW.** Populate default terrain layers with verified stock
  Renegade textures as fallback. Do not invent filenames; enumerate real textures from game data and
  pick existing assets for base, dirt, road-adjacent (if applicable), cliff, shoreline and the other
  default layers. Keep future `ow_ter_*` files as preferred replacements. Add detail/biome/scorch
  entries only when their renderer/data path exists. Also fixes the boundary-cliff texture.
- **P14 Roads — MANUAL VERIFY AFTER P13.** No speculative changes. Test S bend, crossing and tee after
  textures exist. Visual defects become focused fixes.
- **P05/P06/P07 carried items — KEEP CURRENT OWNERSHIP.** Map transition and remote screenshot:
  manual integration tests. `NavalFactoryGameObj`: stays with its Zero Hour feature, not this pass.
  EA validation (ZH-04, ZH-18, ZH-24): retrieve and compare the archival EA implementation first; do
  not infer what EA did.
- **P15 Bridges — IMPLEMENT GAMEPLAY DAMAGE INTEGRATION.** Server-authoritative bridge/span damage
  that identifies bridge and span from the hit, accumulates/applies damage, breaks a span exactly
  once, updates collision, route/path accessibility and logistics blockage, replicates the state, and
  reaches late joiners. Use the existing compact bridge/span state and replication, not hundreds of
  replicated objects. Use the engine's existing damage flow, not a second combat system. Then
  manually verify intact spans and falling through a destroyed one.
- **P16 Foliage — RE-AUDIT + IMPLEMENT RUNTIME WIRING.** Check whether `BiomeDecorationGenerator`
  feeds `FoliageSystem`; if not, wire it. Ensure `Update_Visibility` is called from the frame path.
  No aggressive LOD tuning until after Phase 29 measurements. Plant damage not a priority. Distance/
  LOD culling is a real requirement, revisited after the baseline.
- **P17 Water — RE-AUDIT GENERATED-WORLD WIRING.** Verify hydrology reaches `WaterSystem`; wire it if
  not. Assign a valid fallback water texture from real content. No bespoke reflection renderer.
  Naval behavior deferred until a naval locomotor exists. Manual: carving, shoreline, rendering, flow
  animation, water physics tagging.
- **P18 Surface Ribbons — IMPLEMENT ART/WIRING THAT NOW HAS CALLOUTS.** Assign a verified default
  track texture. Wire vehicle tracks; scorch trails only where a real event exists; drag marks only if
  a real drag event exists. No fabricated events. Harvester behavior waits for generated-world
  harvesters. Distance culling with Phase 29's measurements.
- **P19 World Surface Marks — IMPLEMENT CURRENTLY REACHABLE PRODUCERS.** Wire `CONSTRUCTION` marks to
  real construction start/completion. Audit dirt, oil/stain, scorch; wire only kinds with a real
  semantic event. Assign art or a verified stock fallback before visual validation.
- **P20 World Shadows — RE-AUDIT CALLERS.** Local-light shadow behavior is intentional, not a bug. If
  `WorldShadowManager` still has no world-system casters, wire appropriate generated/runtime objects,
  particularly those whose shadows are repeatedly regenerated or not represented by the stock
  object-shadow path. Then live cache-hit verification.
- **P21 Dynamic Lights — MANUAL VERIFY, OPTIONAL COMMANDER FEEDBACK.** Weapon/explosion integration is
  real. Commander feedback lights (selection, placement confirm, construction complete) only if part
  of the UI design.
- **P22 Particle Manager — RUN AUTOMATION + MANUAL VERIFY.** Re-run client and FDS particle tests; the
  old exit-code-3 is not an acceptable unknown. If both pass, close the automated part. Then manually
  compare batching on/off and record emitters per draw call.
- **P23 Weather — IMPLEMENT MISSING INTEGRATION AND CONTENT.** Run automated checks first. Restore
  camera-relative rain orientation if practical from existing camera/player velocity. Provide real
  invocation paths: `TIBERIUM` driven by generated Tiberium fields; `DUST` profile/biome/environment
  driven, not permanently on; `FALLING` a generic scripted/environment mode. Create/assign documented
  weather textures before final visual acceptance.
- **P24 Tracers / projectile streams — IMPLEMENT PROJECTILE TRAILS.** `AmmoDefinitionClass::
  TrailEmitter` is an abandoned data path: implement the trail reader and connect it to the pooled
  renderer. Rope/cable strips: search donor/content requirements; if nothing requires them, defer
  explicitly.
- **P25 Debris — RE-AUDIT CITY-PROP DEPENDENCY.** Building/bridge/vehicle debris valid; run renderer
  tests and live tier. Check whether generated settlements now contain destructible props; if no
  suitable assets/system, keep deferred as a content dependency. No fake prop system.
- **P26 Modular Draw Components — RE-AUDIT ALL LATER IMPLEMENTATIONS.** Implement/adapt concrete
  components for selection/status visuals, waypoint/order visuals, ghost buildings, RTS overlay
  consumers, and procedural-world attachments where a real use exists. No permanently empty
  interfaces. Add the strongest automated success-path attachment test possible with a real W3D asset.
- **P27 Shroud — STALE, LIVE VERIFY AND CLOSE.** The descriptor loader now resolves `DescriptorName`
  onto buildable presets; re-check the "no Communications Center can register" premise. If it
  resolves, the dependency is closed; do the team-visibility test and close P27.
- **P28 Tactical Intelligence — MANUAL REGRESSION VERIFY.** No speculative code. Stock FPS radar
  unchanged; inspect the strategic-contact debug layer. The eventual consumer is the Commander
  tactical map.
- **P29 Status Rings — IMPLEMENT AFTER BUILDING/UNIT SELECTION.** Once P36/P39 selection exists, wire
  overlays to selected unit/building, order destination/state, possibly group selection. Not an
  isolated demo.
- **P31 RTS Telemetry — MANUAL VERIFY.** Real kills and building destruction change the counters.
- **P32 Debug Layers — IMPLEMENT NOW.** Before the major live-test phase. No mutable containers
  exposed for debugging; add read-only enumeration/snapshot/visitor APIs. Layers for terrain chunks,
  roads, bridges, water, foliage, lights, shadows, spatial-index cells, collision bounds, building
  semantic nodes, Commander cursor/ray, expansion regions, grading areas, transportation
  validation/failure points if practical. Layers for absent features may stay empty, documented.
- **P33 Missing Building Descriptors — IMPLEMENT ALL REMAINING CATEGORIES.** Highest-priority blocker.
  Author descriptors for all remaining usable real assets. Priority: infantry production; vehicle
  factories/Airstrip; Refinery; Repair Bay; defenses; Construction Yard; remaining faction
  equivalents. Real `building.json` manifests, real preset names, verified assembly models, no
  invented `ow_*` asset where a real one exists.
- **P35 Commander input — IMPLEMENT KEYBIND NOW.** Real bindable Commander Mode input function: in
  input configuration, sane default, enters/exits, preserves FPS-input gating, exits on death,
  identical to the console command. *Commander station anchor*: optional; for generated worlds derive
  a fallback from the team's base/initial building region; a descriptor anchor may override later.
  Never block Commander Mode on a missing bone.
- **P36 Building selection / power panel — IMPLEMENT NOW.** Click-to-select via the existing cursor
  ray, resolve to `BuildingGameObj`, keep selected-building state in the Commander controller/UI
  model. Power panel: enabled/disabled, priority, powered state, demand/supply. This selection also
  feeds P29 overlays.
- **P37 Placement — MANUAL VERIFY.** Slope, overlap, bounds, exit clearance, grading interactions.
- **P39 Orders / selection / patrol — IMPLEMENT REMAINING RTS INPUT.** *Drag-box selection* via
  screen-space rectangle against selectable team units. *Control groups*: assign, recall, auto-remove
  dead members; local client/UI state, no replication beyond orders. *Patrol*: extend the order to a
  second point; version serialization on both sides together; AI alternates between the two points
  until interrupted.
- **P40 Commander persistence/networking — MANUAL VERIFY.** Fix failures narrowly; no protocol
  redesign because the self-check can't build a full save environment.
- **P43 Performance — MEASURE, DO NOT GUESS.** During the live session record frame time, server tick,
  draw calls, world-system update cost, power-grid cost, generation time, memory, object/batch counts.
  Feed Phase 29 LOD decisions.
- **P45 Documentation/DoD — DEFER FINAL CHECKLIST.** Don't score the 110-item DoD until prerequisites
  and live gates are substantially complete.
- **P51 Height Fog — ACCEPTED LIMIT.** Reopen only if a programmable shader tier arrives for another
  justified reason.
- **P56 Tonemap — ACCEPTED LIMIT.** Keep the global operator; local tonemap is a candidate if a shader
  tier arrives.
- **P55/P57 Post-process render targets — MANUAL VERIFY.** Bloom/shafts off by default until tested on
  a real device. Bloom first (composite, HUD, no RT state leaks), then shafts.

### Gameplay gap backlog

- **Destroyed production facility queues — IMPLEMENT NOW.** Wire `CommanderProductionService::
  On_Facility_Destroyed` into the authoritative building death transition this engine actually runs,
  not an unused `Destroyed()` callback. Execute once. Refund/cancel with the same semantics as normal
  cancellation.
- **Commander disconnect seat — IMPLEMENT WITH THE EXISTING GRACE PERIOD.** Do not clear the seat
  immediately. Disconnect reserves the seat for the reconnect grace interval; the same player
  reconnecting inside it regains the seat; on expiry `Get_Team_Commander` clears and the team can
  elect/claim another Commander.
- **Missing building descriptors** — resolved through P33.

### Procedural-world WIP

- **Boundary cliff ring — IMPLEMENT TEXTURE, THEN LIVE VERIFY.** Geometry unchanged until viewed. Fix
  the cliff texture via P13. Live: toe apron, face shape, vehicle climbability, seams, bounds, debug
  riser colors.
- **V2 Phase 10 transportation fallback — ADD THE TWO MISSING SYNTHETIC TEST CASES.** Deterministic
  headless terrain where (a) the original crossing fails but a nearby reposition succeeds, and (b)
  the original approach fails grade but a longer approach succeeds. Visual believability stays manual.
- **V2 Phases 11–15:**
  - *Initial base layout / spawn bootstrap — IMPLEMENT* the Section 16A contract: initial faction base
    transforms, required buildings, spawn locations, team ownership, player spawn sets, match
    bootstrap, from the generated base site and transportation layout.
  - *Front/travel-time fairness — RE-AUDIT.* If `StrategicPlanner` or equivalent has a meaningful
    contested/front representation, implement `MaxBaseToFrontTravelTimeDifferenceSeconds` with it;
    otherwise keep it explicit, no meaningless midpoint metric.
  - *Harvesters — IMPLEMENT.* Once Refinery descriptors/presets exist, spawn the normal Renegade
    harvester through existing vehicle/AI APIs bound to generated field/route data. No separate
    procedural harvester.
  - *Building entrance/connector metadata — SIDECAR DATA.* Extend manifest/descriptor data with
    local-space anchors (primary entrance, vehicle exit, infantry exit, street-facing side, optional
    door, optional PCT/MCT, destroyed model). Transform through the placed building transform. No W3D
    re-export needed.
  - *Missing city/settlement assets — CONTENT DEPENDENCY.* No meshes fabricated in code.
  - *Object/foliage-aware sightline occlusion — IMPLEMENT* with the terrain ray query plus
    world-spatial/foliage queries; first legitimate consumer resolving P10. No full object-list scans.
  - *Post-commit global repair:* keep bounded seed regeneration; no cross-system rollback unless a
    later feature proves it necessary.
- **V2 Phase 16 Navigation — IMPROVE LOCOMOTION REPRESENTATION.** Not a duplicated navmesh: one
  topology with per-sector/per-edge capability metadata (infantry, wheeled, tracked, heavy ground),
  clearance/slope/traversal masks, traversal filtered by locomotion class. Naval nav deferred until a
  naval locomotor exists. Building interiors deferred until interior/portal metadata/assets exist.
  *Minimap base-layer consumer — IMPLEMENT* the real renderer; foundation for tactical overlays,
  logistics and strategic contacts.
- **V2 Phase 17 Expansion.** Tunnel exclusion: deferred until a tunnel system exists. Whole-world nav
  rebuild kept; measure in Phase 29; incremental only if measured hitch justifies it. `IsForward`:
  replace the distance-from-first-base heuristic once a real front concept exists. Expose expansion
  regions and grading via P32.
- **V2 Phase 18 + 19 — IMPLEMENT REAL NETWORK TRANSPORT.** Bind the world-operation/snapshot protocol
  to Renegade's existing reliable network/event layer: ordered operations from authority, revision,
  client apply, checksum verification, late-join handshake, profile/seed/content identity, current
  revision, snapshot/repair request, regional repair response, ack/failure handling. No second socket
  stack. Server-authoritative. *Save persistence*: integrate the journal/current snapshot with actual
  save persistence so Phase 28's save/restart/reload restores terrain mutations.
- **V2 Phase 20 Functional Subcomponents — FIX MAP-AUTHORED ANCHOR RESOLUTION.** `building_
  subcomponents` must not require `RuntimeParts` to resolve an anchor; for map-authored buildings
  resolve against the loaded render/model hierarchy. Tiberium storage: no invented gameplay effect.
- **V2 Phase 21 Logistics.** Refinery/factory/repair nodes via P33. *Enemy obstruction — RE-AUDIT*: use
  Phase 25 tactical intelligence/threat data for route blockage/cost, not a new threat model.
  Construction-support radius: keep predicate, no invented bonus. Tactical overlays on the Phase 16
  minimap renderer once it exists.
- **V2 Phase 22 Vehicle Subsystems.** *Sensor — narrow model*: health-derived sensor range/detection
  factor for AI target acquisition, tactical/strategic detection, possibly Commander intel range;
  never impair a human's FPS view/aim. *Weapon accuracy — IMPLEMENT*: subsystem affects fire rate and
  accuracy; scale `SprayAngle` (or point-of-use equivalent) at fire time, never the immutable
  definition. Repair/escort vehicles only when real presets exist. Fuel stays out.
  *`CanDrive`/`CanFire` — FIX AS A SEPARATE COMPATIBILITY BUG (authorized now)*: restore intended
  behavior, not tied to vehicle health; `CanDrive == false` gates drive controls, `CanFire == false`
  gates firing; subsystem damage never writes them. Regression-test TT/script behavior.
- **V2 Phase 23 Commander Governance — IMPLEMENT UI NOW.** Governance panel: current Commander, active
  vote type, candidate/target, time remaining, yes/no counts, yes/no buttons, eligible
  teammate/candidate list, transfer control. Console commands stay for debug/admin. Keep reconnect
  grace with the disconnect-expiry fix. Then the multiplayer test.
- **V2 Phase 24 AI Commander Economy — LIVE VERIFY AFTER DESCRIPTORS.** No algorithm rewrite first.
  Provide a deterministic developer/server command to set AI treasury if none exists. Verify seating,
  economy priority, construction, expansion, treasury debit, human removal vote.
- **V2 Phase 25 AI Production/Defense/Attack — LIVE VERIFY AFTER P33/P39.** Ensure unit-producing
  descriptors, real selection/order dispatch, guard behavior and contacts first. No duplicate
  recon/repair/logistics subsystems.
- **V2 Phase 26 Campaign — COMPLETE THE LAUNCH INTEGRATION.** `campaign launch` starts the battle via
  the same procedural level-loading path as Procedural Skirmish; profile, seed, territory and
  factions survive into world initialization. A completed match has an explicit path into
  `Apply_Battle_Result`. Automated disk save/load where practical; restart test manual.
- **V2 Phase 27 World Tools — MANUAL VERIFY.** Repro string, JSON report, PPM preview, seed batch on a
  real world. Improve preview only after seeing a real file.
- **V2 Phase 28 Integration Gate — RUN ONLY AFTER CRITICAL BLOCKERS:** project-owned automated
  failures, P33 gameplay descriptors, facility-destruction queue cleanup, Commander disconnect
  expiry, Phase 18/19 transport, world-operation save persistence, P35/P36/P39 core input,
  terrain/cliff fallback textures. Then the documented procedure exactly; every failure becomes a
  narrow new issue; no broad speculative changes during the gate.
- **V2 Phase 29 Performance / Soak — MEASURE FIRST, THEN BUILD LOD.** SMALL/MEDIUM/LARGE_SUPPORTED:
  generation time, average and worst frame, server tick, draw calls, memory, foliage and chunk
  submissions, nav rebuild cost, journal growth. Then multi-tier chunk LOD, foliage LOD/distance
  culling, and other measured-bottleneck degradation. Long `large_supported` soak watching memory,
  stale chunks, stale render resources, leaked nav data, journal growth.

### Priority order

Use this order rather than walking `WIP_features.md` top to bottom.

1. **Correctness blockers:** failing automated tests; production-facility destruction/refund hook;
   Commander disconnect/grace expiry; P33 descriptors; P13 fallback terrain/cliff textures;
   `CanDrive`/`CanFire`.
2. **RTS interaction layer:** P35 keybind; P36 building selection/power panel; P39 drag selection,
   control groups, two-point patrol; P29 selection overlays; P23 governance UI.
3. **Procedural multiplayer transport/persistence:** Phase 18 operation transport; Phase 19
   late-join/snapshot transport; world-operation save persistence.
4. **Stale renderer/world WIP:** P09, P16, P17, P19, P20, P26, P27, P32.
5. **Procedural-world gameplay gaps:** base bootstrap/spawns; harvesters; semantic building anchors;
   sightline occlusion; locomotion capability masks; minimap renderer; bridge damage; campaign launch.
6. **Automated edge coverage:** bridge fallback stage-1 and stage-2 success; real attachment success
   cases; disk save/load where headless testing permits.
7. **Live validation:** combined procedural-world, Commander, vehicle, rendering and multiplayer tests.
8. **Performance:** Phase 29 measurements, then LOD/culling tuning.
9. **Final DoD:** Section 52's final checklist.

### Closing WIP entries

Do not keep historical gaps in active WIP merely because they were once true. For every pass: move
verified completed behavior to `completed_features.md`; remove stale assertions; retain only
genuinely open implementation or verification work; distinguish *manual verification required* from
*implementation missing*; reference the owning phase for intentional deferrals; do not duplicate one
root problem under several phases. `WIP_features.md` describes work actually open now.

### Notes against current state (at `f0ee3c62`)

- The failing-test baseline is 4/197, not 24: `shader_commandermode` (test passes a sentinel soldier
  pointer; project-owned, fix the test) and `shader_visualsettings` (needs a D3D device), each with
  an `fds_` twin.
- A tunnel system now exists (V2 Phase 10 at V2 close). The Phase 17 tunnel exclusion is already
  wired: placement and settlements use `TunnelSystem::Overlaps_Footprint`.

### Implementation state (D-001 priorities 1-6 done)

- Priorities 1 to 6 are implemented, with automated checks: 201/201 ctest, and zero-warning
  `renegade`, `renegadeserver` and `leveledit`. Per item, see `completed_features.md`.
- `WIP_features.md` now holds only priority 7 (live verification), 8 (measurement) and 9 (the
  final checklist), plus deferrals and accepted limits.
- The art and content still missing are listed in `docs/assets/MissingAssets.md`.
- Choices made where D-001 left room, each recorded in its doc:
  - **Heavy ground:** the worst case of every loaded ground-vehicle class, which is what the
    unchanged sector graph is built from.
  - **The front:** the contested Tiberium fields that both bases can reach.
  - **Environment weather:** derived per client and not replicated.
  - **Tiberium haze, dust and enemy obstruction:** profile data, off by default.
  - **Sensor degradation:** reuses the weapon module's curve and defaults.
  - **Detail texture:** none exists in always.dat, so no stand-in is used.

## D-002 — Procedural Skirmish is a fully native generated map (2026-09-24)

User ruling: the Procedural Skirmish button launches a truly native procedurally generated
map. It does not start from any base/host map (no stock skirmish `.mix` underneath, no
`fit_to_level` overlay); the generator produces the whole world. This supersedes the
"laid over a real level" design in `dlgmainmenu.cpp` (V2 roadmap Section 34 entry point) and
the "Coexist, not replace" host-level approach in `docs/procedural_world/MacroTerrainGeneration.md`
for that button.

The native profile uses the existing large-supported 2400 × 2400 world size for FPS travel.
Terrain relief is raised above sea level with a profile-specific land bias, so rivers and
small basins remain without flooding most buildable land. Generation remains ordered:
terrain and cliffs, water carving, strategic anchors, graded roads and bridges, then base
selection. Each terrain edit refreshes the chunk registry and restores the cliff generator's
owned exclusions before the next planner reads it. Opposing bases on this large profile must
have at least a 60-second reference-vehicle road route; the bounded alternate-seed retry
handles candidate worlds that cannot meet the fairness gates.

## D-003 — Map plan first: Section 34 phases 4-11 reordered behind a 2D layout plan (2026-09-26)

User ruling: the procedural world is laid out in plan view before any 3D terrain exists,
the way C&C battlefields (RA2 random maps, Renegade multiplayer maps, Generals WorldBuilder
plans) are designed. This reorders roadmap Section 34 phases 4-11. The roadmap file itself is
not rewritten; this decision supersedes its phase order and the pipeline doc records the new
sequence.

Evidence that motivated it: `MacroTerrainGenerator` was pure noise with no design intent,
interior cliffs were leftover steep chunks joined by a greedy walk, hydrology ran at chunk
resolution with four neighbours, bases were chosen at Phase 11 after roads and bridges, and
fairness depended on search, repair and three whole-world retries. Neither open-source
donor offers a generator to borrow: Tiberian Dawn/RA1 are hand-built tile templates, Generals
/Zero Hour is a hand-authored heightmap (WorldBuilder) with cliff/impassable cell flags. RA2's
random map generator is closed source; its tier/plateau/cliff/ramp model is what is imitated.

New phase order (Phase numbers below are the roadmap's; 3B is inserted):

1. Phase 1-3 unchanged: bounds, boundary, cliff ring.
2. **Phase 3B (new) — Map plan.** `MapPlanGenerator` (WWPhys, one owner) builds a 2D grid at
   chunk resolution from the seed and the profile `plan` block: L0 layout graph (GDI/Nod base
   sites, centre and flank Tiberium fields, lanes, chokes, symmetry mode), L1 integer height
   tiers, L2 water plan, L3 passability (open/base/lane/field/cliff seam/ramp/water), and a
   checksum for the manifest. Deterministic; retryable without touching the 3D world.
3. Phase 4 macro terrain becomes a realizer: the heightfield is the plan tier height plus
   in-tier noise scaled by `plan.detail_weight`; noise is detail, not structure. Cliff band
   and approach flattening unchanged.
4. Phase 5 interior cliffs, Phase 6 hydrology: unchanged owners, now operating on terrain
   whose seams and basins come from the plan. Tier seams produce the steep chunks the cliff
   generator already looks for.
5. Phase 7 strategic candidates: plan base-site cells receive a scoring bonus so base
   anchors land on planned sites; existing eligibility and fairness gates still apply.
6. Phase 8-11 roads, bridges, tunnels, base selection: unchanged owners and gates. Base
   selection validates the plan; whole-world retries remain the last resort.
7. Phase 12+ unchanged.

Constraints resolved in this decision: the Zero Hour donor is absent locally and is used as
a semantic reference only (`WorldHeightMap` cell flags, cliff-cell classification); WWPhys
`HeightfieldClass` and `WorldTerrainSystem` stay the owners. The plan stage is profile-gated
(`plan.enabled`); `fit_to_level` worlds skip it. The D-002 native profile enables it. The
D-002 visual verification remains open and now targets the plan-driven world. Schema 11,
generator 10.

## D-004 - Hybrid rewrite I6: how runtime Phase 5 resolves the plan's open landform geometry (2026-09-27)

Section 8 Phase 5 fixes most of the vector-landform algorithm exactly, but a few values it needs
are not in the geometry table. These are the readings runtime Phase 5
(`Code/wwphys/hybridlandforms.{h,cpp}`) implements; the reasoning and the code locations are in
`docs/procedural_world/HybridLandforms.md`.

1. **Ravine centerline length** is the canyon row (`0.35D..0.75D`) scaled by `2/3`, the same
   canyon-to-ravine ratio every shared row of the table already uses, and a Section-6.8 attached
   branch is that range scaled by `2/3` again. The table gives a ravine floor, rim and depth but
   no length.
2. **A cliff field is compactly supported.** Section 10.2 gives a cliff only an edge width and a
   rock/talus transition width, so the field uses no other length scale: zero at the talus toe,
   full height at the upper edge, one talus width of shelf, then back to zero across one more,
   with the same taper longitudinally at each end of the run. An unbounded one-sided step would
   lift terrain the cliff never reserved, including a base far outside its influence polygon.
3. **A density-created canyon/ravine system is a ravine.** `terrain.valley_density` is the knob the
   ravine target adds; a canyon exists only where an archetype requires one.
4. **Basin and coast are not Phase-5 features.** `plateau_basin`'s basin is already a Phase-4 macro
   zone and every water polygon belongs to Phase 6, so step 1 of the creation order builds only
   `PRINCIPAL_CANYON`, `PRINCIPAL_RIDGE` (with its `SURFACE_SADDLE`) and `MAIN_RAVINE_GUIDE` plus
   its branches.
5. **Rejection rule 5's `4W` heavy-corridor dry width binds generic features only.** An archetype
   principal canyon or ravine exists to obstruct the axis and Phase 3 already recorded the crossing
   obligation Phase 6 must satisfy with a window.
6. **The candidate budget counts alternates, not misfits.** A cell where the feature does not fit
   inside the placeable world is stepped over without spending a
   `retry.max_alternate_candidates_per_feature` slot; a cell that conflicts with other planned
   intent spends one. The keyed candidate order and the set of acceptable positions are unchanged.
7. **Horizontal draws are clamped to the placeable interior**, exactly as Phase 5 already clamps its
   vertical draws to `terrain.min_elevation..max_elevation`. The clamp only shrinks, never leaves
   the drawn range and never goes below a Section-5 minimum; when the minimum itself does not fit,
   the phase reports its named attempt failure.
8. **Reservation, boundary and elevation tests are taken at the feature's own sample points** (its
   influence outline plus its spine), not only at coarse cell centres, because a cliff band or a
   ravine floor is narrower than one coarse cell. A line feature also stamps the cell each
   centerline sample stands in, so a thin feature still appears in `LandformMask`.
9. **`data/procedural_world/native.json` raises `retry.max_alternate_candidates_per_feature` from 6
   to 32.** Phase 5 places 20 or more features that must each miss two base reserves, the Tiberium
   reserves, the `OPEN_MANEUVER` discs and each other; measured over twelve seeds, 6 alternates per
   feature failed nearly every attempt while 32 passes 8/12 at MEDIUM. The plan parameterizes
   exactly this (Section 17: local candidate limits come from the profile) and forbids weakening
   quotas instead. The value also feeds `HybridPlanningInputHash` and the Tiberium reposition
   budget, both in the same search-harder direction.

The one thing these readings cannot resolve is **Q-018**: at LARGE the hill target and rejection
rule 6's 25% same-type overlap cap are mutually unsatisfiable for many seeds even after an
exhaustive candidate search. Phase 5 reports the named attempt failure and the question is open.

The centripetal Catmull-Rom sampler every plan curve needs is added once, to the shared planning
owner (`Hybrid_Sample_Centripetal_Catmull_Rom` in `Code/wwphys/hybridplanning.{h,cpp}`), so Phases
6 and 10 do not each grow a spline rule.

## D-005 - Hybrid rewrite I7: how runtime Phase 6 resolves the plan's open hydrology detail (2026-09-27)

Section 8 Phase 6 fixes the drainage solve exactly -- Priority-Flood from the exterior, then D8 over
the filled surface with a fixed tie order -- but a handful of records it asks for have no stated
shape. These are the readings runtime Phase 6 (`Code/wwphys/hybridhydrology.{h,cpp}`) implements;
the reasoning and the code locations are in `docs/procedural_world/HybridHydrology.md`.

1. **A river "system" is one flow-connected channel component.** Step 9's "merge shared downstream
   chains by analysis-cell id" is a union-find over channel cells joined to their D8 destination, so
   a tributary is never a second system. The stored centerline is the system's primary headwater's
   chain: longest quantized chain, then more accumulation at the source, then the lower cell id.
2. **A basin polygon is the closed lattice outline of its four-connected cell set**, walked from the
   basin's lowest-index cell and stripped of collinear corners. A corner that two unused outline
   edges still start from would let the ring touch itself, so that basin is not simple and is
   illegal under step 13. A dry hole inside a depression is folded into the outer ring; step 17's
   islands are the plan's mechanism for dry land inside water.
3. **An island is its own record, not a hole in its host.** Step 17's subtraction is realized by
   every water query treating an island polygon as land, which avoids a polygon boolean and a second
   description of the host outline.
4. **The coast polygon is the world rectangle clipped to the sea side of the guide's `v` boundary**,
   so the sea stays convex and inside the world bounds; a local retry moves that boundary outward by
   exactly `S`.
5. **A river mouth bound to a lake or the sea ends at that shoreline**, and the mouth sample's stored
   surface is the sink's water level, so Phase 7 step 2 holds in the record and no channel runs
   through standing water.
6. **A selected system carries the river bed depth.** Section 5 defines a counted river as a
   source-to-sink network, so a stream depth would never apply to a selected record;
   `WaterDefinitionClass::BedDepth` for `ow_water_river` and `ow_water_lake` are read through
   `WaterSystem` in the manager adapter, never duplicated in the generator.
7. **Fords stay inert until a fordability query exists.** Preflight reports
   `FORD_CAPABILITY_UNAVAILABLE`, so the params carry the capability's two depth limits and the
   manager passes zero: no window carries a ford flag today, and the comparison is the one place to
   ask the real query when it ships.
8. **A retained cliff station is cut-capable by construction.** Step 18's "cut metadata" is the fact
   that the station's two endpoint elevations connect within `PRIMARY_2` grade -- which is what
   retention already proved -- plus the `LowElevation` / `HighElevation` on the record.
9. **The window gate is about obstructed corridors, not touched ones.** Phase 5's rejection rule
   deliberately allows a generic barrier to overlap a required heavy corridor as long as `4W` of dry
   width survives, so Phase 6 demands a window only where the obstruction leaves no contiguous
   `heavy_vehicle_width + 2*min_vehicle_lane_clearance` run clear of it inside
   `MaxCorridorHalfWidth(PRIMARY_2)`. This is the checklist's own wording ("every obstructed required
   corridor has a window") and it keeps a short cliff clipping a corridor edge from failing an
   otherwise legal seed.
10. **Only a preferred window becomes a reservation.** Every accepted window stamps `NO_DECORATION`
    and its own window flag; a window on a required heavy corridor, or within `2S` of a Section-6.10
    crossing guide, is additionally inserted as the `HYBRID_RESERVE_CROSSING_WINDOW` reservation
    Phase 4 reserved that rank for.

`WaterSystem` needs no interface change for this item: it already stores closed areas and flowing
centerlines with per-station width and surface height, which is the shape of these records, so the
adapter that hands them over belongs to the phase that realizes water.

## D-006 - Hybrid rewrite I8: how runtime Phase 7 realizes the plan's height expression (2026-09-27)

Section 8 Phase 7 gives the height formula exactly; what it does not say is who writes the
heightfield, how the four-key noise draw maps onto the shared three-key helper, or what "carve" means
once a river surface has been fixed. These are the readings runtime Phase 7
(`Code/wwphys/hybridmacroterrain.{h,cpp}`) implements; the reasoning is in
`docs/procedural_world/HybridMacroTerrain.md`.

1. **The formula and the write are separate owners.** `WorldTerrainSystem` stays the heightfield and
   `MacroTerrainGenerator::Generate` stays the one thing that writes it; a request carrying the Phase
   4-6 records takes `Hybrid_Macro_Sample_Height` per vertex instead of the noise-geography channels,
   which are replaced rather than joined. `UseHybrid` and the D-003 `UsePlan` path are mutually
   exclusive, and an authored/legacy request takes exactly the code it always did.
2. **The detail-noise octave folds into the named key.** The plan's
   `Hash01(terrain,"detail_noise",octave,ix,iy)` is four keys and the Section-0.3 helper takes three,
   so the octave is mixed into the named key with the standard golden-ratio odd constant and `ix`/`iy`
   stay the lattice keys.
3. **The mouth is re-pinned after the downstream walk.** Step 3's minimum walk can lower the mouth
   step 2 just fixed, so the pin is re-applied before step 4's upstream maximum walk -- otherwise a
   river bound to a lake ends below its own lake.
4. **Phase 7's carve is a bed target.** `Hybrid_Macro_Water_Carve` returns `base_height - bed` from
   the frozen longitudinal walk, blended out across `max(river_width,2W)`, so a bed follows its own
   water surface downhill. Phase 6's `Hybrid_Water_Carve_Depth` keeps its own meaning -- how deep the
   water is -- for the callers that ask that.
5. **A coast is cut to sea level.** Ocean and coast carry a zero `BedDepth`
   (`WaterSystem::Define_Default_Water`) and the plan names that field as the only carve source, so the
   sea floor is cut exactly to the water surface rather than to an invented depth.
6. **Protected profiles are excluded from slope relaxation, not from the write.** Explicit cliff,
   canyon and ravine walls, river beds and banks, and lake/sea polygons with their shoreline band are
   written with their computed height and then left out of the relaxation loop, which is how a
   vertical-faced cliff survives a pass whose job is to flatten steep ground.
7. **"Every Phase-5/6 feature still represented" is a sign test.** The gate compares the realized
   ground at each feature's own mid-sample against `tier_height + macro_bias`: positive for a hill or
   ridge, negative for a canyon or ravine floor. Probes the water carved or the elevation clamp pinned
   are skipped, because the formula itself makes water outrank relief there.
8. **The gate re-measures window grade on the written ground.** Phase 6 retained each crossing window
   from the provisional surface; the same `PRIMARY_2` grade rule is checked again after the heightfield
   exists, so a window detail noise or relaxation pushed out of reach fails this attempt instead of
   Phase 11's routing.

## D-007 - Hybrid rewrite I9: how runtime Phase 8 measures the anchors it freezes (2026-09-27)

Section 8 Phase 8 lists the hard requirements exactly and forbids moving a centre to satisfy one. What
it leaves open is how three of those requirements are measured. These are the readings runtime Phase 8
(`Code/wwphys/hybridanchors.{h,cpp}`) implements; the reasoning is in
`docs/procedural_world/HybridAnchors.md`.

1. **The starting-building proxy area is the `R_BASE` core disc** -- the disc Phase 4 already reserves
   as `BASE_CORE` for exactly the starting buildings -- and the buildable-slope test is measured over
   it.
2. **Expansion area is measured as clear buildable land inside the protected disc**, at `A/2` samples,
   using the existing owner's own radius rule (`BaseSelectionPlanner`'s `sqrt(area/pi)`, which is
   Section 0.5's `R_EXPAND` and already part of `R_BASE_PROTECTED`).
3. **Slope comes from the written heightfield**, as the central-difference gradient over one analysis
   spacing, because Phase 8 validates the ground that exists after the ordinary-slope relaxation pass,
   not the formula that produced it.
4. **"Outside BASE_PROTECTED" is measured on the frozen disc, not on the rasterized coarse flag.** A
   cell is `S` wide and is flagged for its whole area when its centre is inside the disc, so the flag
   would refuse a field reserve that legally clears the disc by less than half a cell. Phase 3 already
   separates the two centres by `R_BASE_PROTECTED + field_radius_max`.
5. **`V_REF` is applied exactly as `BaseSelectionPlanner` applies it**: world-unit distances divided by
   `WorldMetricsClass::Get_Reference_Vehicle_Speed()`, with no unit conversion invented, which is what
   Section 0.10 means by using the legacy value unchanged.
6. **A field's approach is one of its own Phase-3 edges**, verified by walking the straight run from
   the field centre toward that edge's other endpoint out to the reserve radius and requiring it clear
   of water and immutable walls.
7. **`PlanningDistance` belongs to the graph owner.** Section 0.10's helper is implemented once, as
   `Hybrid_Planning_Distance` in `hybridstrategicgraph`, with Dijkstra over quantized straight-line
   edge lengths and ties broken by `StableId`; Phase 19's authoritative re-check calls the same
   function rather than growing a second distance rule.



## D-035 - The game registers its bridge kinds at startup and loads every building descriptor; the native profile then passes preflight and no seed passes Phase 5 (2026-10-06)

**Direction (user, 2026-10-06: "continue").** D-034 stopped at the empty bridge registry and asked which owner registers
bridge kinds at startup. The user's reply was to continue from the last session, so this record takes the smallest
canonical answer and states it for review: the existing defaults, registered by the existing owner's own `Init`.
D-033's stop rule is unchanged: find a requested seed that passes Phase 12 on the production profile and content, and
stop without changing generator rules if the bounded search finds none.

### Bridge kinds are registered by `WWPhys::Init`

- **Owner.** `BridgeSystem` lives in wwphys, and `WWPhys::Init` already defines that library's default registries at
  startup (`SurfaceRibbonSystem::Define_Default_Ribbons`, `WorldSurfaceMarkManager::Define_Default_Marks`). It now
  calls `BridgeSystem::Init` too, which defines `Define_Default_Bridges`' three kinds: `ow_bridge_timber` 4,
  `ow_bridge_concrete` 8 and `ow_bridge_steel_truss` 16 wide. `WWPhys::Shutdown` calls `BridgeSystem::Shutdown`.
- **Content.** These are the three kinds the self checks and `bridge_test` already use. No kind is authored, and no width
  or span is changed. A map may still retune a kind by name (`Define_Bridge` replaces by name).
- **Teardown order.** `ProceduralWorldManager::Shutdown` clears placed bridges before `WWPhys::Shutdown` runs, so
  `BridgeSystem::Shutdown` finds no geometry left to remove.
- **Reach.** The client, the dedicated server and LevelEdit all call `WWPhys::Init`. The `bridge_test` console command's
  own `BridgeSystem::Init` now re-defines the same three kinds and is otherwise unchanged.

### Every building descriptor loads: the project presets are registered first

With bridges registered, preflight stopped at `MISSING_STARTING_BUILDING_FOOTPRINT (gdi_construction_yard)` for every
seed.
- **Cause.** `CombatManager::Load_Game_Definitions` loaded `data/building_descriptors` before
  `ProjectBuildingDefs::Register` added the project-owned presets. A descriptor validates only when its
  `game_object_preset` resolves to a loaded definition, so the six descriptors naming an `ow_` preset were refused and
  never loaded: both Construction Yards, both Tiberium Silos and both Advanced Communications Centers. The probe showed
  16 of 22 loaded, last error `game_object_preset does not resolve to any loaded definition`.
- **Scope.** This path is the game's own. Every level load had no Construction Yard, Silo or Advanced Comm Center
  descriptor, not only the census.
- **Fix.** `ProjectBuildingDefs::Register` now runs before the directory load. It already re-registers every level load
  and skips a name already present, so nothing else changes. Now 22 of 22 load and all twelve starting buildings resolve.
  The Construction Yards' radius is 44.749 and the Airstrip's 78.054.

### Diagnostics

The census start now prints the descriptor count and last load error, then one `METRIC building` line per starting
building: its team, whether it resolved, whether its descriptor is loaded, and its radius and height.

**Finding (not fixed; no consumer).** Every building height reads 0. `Get_Building_Footprint` asks the asset manager for
the descriptor's `model_name` verbatim (`mgcon_ext.w3d`). That is D-034's defect in a second place, and the building
models live in `data/building_models/`, not `always.dat`. Neither preflight nor its hashes, nor the generator, reads a
building height; navigation and settlement placement discard it. It is left for the item that first needs a building
height.

### Production census, requested seeds 1..256 (`--procedural-census 1 256`, native.json, real content)

- **Preflight passes** for every seed. Content revision `442E9F6A`, planning input hash `D0705A53`.
- **Result.** 768 attempts over 258 distinct effective seeds. **None reaches Phase 6**, so none reaches Phase 12 and no
  smoke test is possible. The stop rule applies, and no generator rule was changed.
- **In game.** `--procedural-skirmish 1` prints the same three attempts as the census, now past preflight: identity
  checksums `B1D45BE8`, `F9C44201` and `5A33DABB`, each stopping at the same Phase-5 ridge.

Where each distinct effective seed stops:

| Stage | Failure | Seeds |
| --- | --- | --- |
| Phase 5 | ridge `candidates_exhausted`, last refused by `BASE_PROTECTED` | 120 |
| Phase 2 | `NO_AVAILABLE_BIOME_PAIR` (no primary biome has an available legal secondary) | 32 |
| Phase 5 | ridge `candidates_exhausted`, same-type overlap | 24 |
| Phase 3 | `NO_LEGAL_BASE_POSITION` (GDI 18, Nod 11) | 29 |
| Phase 5 | hill `candidates_exhausted` (`BASE_PROTECTED` 11, same-type overlap 3) | 14 |
| Phase 5 | cliff `candidates_exhausted` (Tiberium reserve 10, reserve disc 4, `BASE_PROTECTED` 8, heavy corridor dry width 1) | 23 |
| Phase 3 | `NO_LEGAL_RESOURCE_POSITION` (REMOTE_TIBERIUM) | 10 |
| Phase 4 | `BASE_ON_HARD_INTENT` (a `BASE_PROTECTED` cell carries water intent) | 6 |

Phase 5 stops 181 of 258 seeds. The ridge failures are spread over every archetype and always fall on ridge stable
ids 10 to 12, after all 32 alternate candidates (`retry.max_alternate_candidates_per_feature`). This is the first time
the shipped 2400 m native profile has run past preflight; the 1200 m fixture census never exercised it.

**Next.** Which Phase-5 input the native profile violates is a generator-rule question, outside this decision's stop
rule. The candidates are the ridge count and dimensions for the native size class, the `BASE_PROTECTED` extent, and the
alternate-candidate bound. It needs the user's direction.

**Tests.** `terrain_bridges`, `terrain_native_bridges`, `terrain_bridgeplan`, `terrain_bridgerequired`,
`terrain_bridgefallback`, `terrain_hybridpreflight`, `shader_buildingjson` and their `fds_` twins pass; full suite 237/237 (client and FDS). No self check
can reach the ordering fix, because the harness loads no definition database; the census probe is the production check.

## D-034 - WorldMetricsClass resolves a definition's model as PhysClass does; the next production blocker is an empty bridge registry (2026-10-05)

**Direction (user, 2026-10-05).** Fix `WorldMetricsClass::Resolve_Extents`'s model-name resolution at its owner, with
the engine's own canonical model-name semantics: no preflight bypass, no Humvee special case, no hard-coded
dimensions, no change to the metric contract. Then probe every production vehicle metric, rerun real-content
preflight and the production census, and smoke-test the first Phase-12 world. Stop at any further seed-independent
blocker.

### The engine's model-name path

`PhysClass::Init` (wwphys/phys.cpp) creates every physics object's model:
- **A name containing a '.'** is a filename. It goes through `create_render_obj_from_filename`, which keeps what
  follows the last backslash and removes the last four characters (`.w3d`). So
  `vehicles\gdi humvee\v_gdi_humvee.w3d` becomes `v_gdi_humvee`.
- **Any other name** is passed to the asset manager as it is.
- **The lookup.** `WW3DAssetManager::Create_Render_Obj` then looks the name up in its prototype table
  (`Find_Prototype`, `stricmp`, so case-insensitive). It never strips paths itself: load-on-demand loads the file named
  by the stem, then looks up the original string again.
- **What is not handled.** Only the backslash is a separator; the engine treats no other slash or alias.
- **Combat's helper differs.** Combat's `Get_Render_Obj_Name_From_Filename` always strips four characters, even from a
  name without an extension, so it is not the physics path's rule.

### The defect and the fix

- **Defect.** `WorldMetricsClass::Resolve_Extents` passed `def->Get_Model_Name()` verbatim to `Create_Render_Obj`.
  Every shipped definition stores a filename, so every lookup returned null. No `WORLDBOX` was ever read in the game:
  every metric fell back to the default envelope, and preflight refused the Humvee.
- **Fix.** The physics path's own rule is now one public helper, `PhysDefClass::Render_Obj_Name(model_name)`.
  `create_render_obj_from_filename`, and so `PhysClass::Init`, uses it unchanged. `Resolve_Extents` asks the asset
  manager for `Render_Obj_Name(def->Get_Model_Name())`.
- **What it does not add.** No search, fallback, basename guessing or other slash handling. A name without a '.' is
  returned unchanged, so canonical names resolve exactly as before. A missing model still returns no extents.
- **Consumers.** The metrics cache is keyed by definition id, so keys are unchanged. Navigation, vehicles, placement and
  preflight now receive each definition's real `WORLDBOX` instead of the default envelope; that is the correction itself.
- **Diagnostics.** Preflight's vehicle metric inputs carry the definition tried, its model string and the render object
  asked for (diagnostics only: neither hashed nor compared). A failed vehicle metric names all three.

### Production probe (`renegade --gamedir C:\Westwood\Renegade_full --procedural-census`, real always.dat)

Every census start now prints the four reference metrics and the bridge registry:

| Role | Definition | Model string | Render object | Length x width x height |
| --- | --- | --- | --- | --- |
| Humvee | physics 327740003 | `vehicles\gdi humvee\v_gdi_humvee.w3d` | `v_gdi_humvee` | 4.722971 x 2.555953 x 1.548488 |
| Heavy reference | tracked aggregate, 47 measured, 0 without a WORLDBOX; widest physics 1963 | `vehicles\gdi mammoth tank\v_gdi_mammth.w3d` | `v_gdi_mammth` | 10.500445 x 6.801906 x 3.064066 |
| GDI Harvester | gdi_refinery's CnC_GDI_Harvester, physics 327800032 | `vehicles\gdi harvester\v_gdi_hrvstr.w3d` | `v_gdi_hrvstr` | 7.428105 x 4.514225 x 2.754598 |
| Nod Harvester | nod_refinery's CnC_Nod_Harvester, physics 327800033 | `vehicles\nod harvester\v_nod_hrvstr.w3d` | `v_nod_hrvstr` | 7.399116 x 4.514225 x 2.754598 |

The Humvee matches `VehicleCollisionMetrics.md`'s offline WORLDBOX measurement exactly. The heavy and Harvester widths
match the values the fixtures carried. None of those values is an input to the resolver.

### The next production blocker: the bridge registry is empty

With every vehicle metric resolved, preflight fails at `NO_HEAVY_CAPABLE_BRIDGE (HIGHWAY_4 needs clear width 15.783)`.
The probe shows `bridges registered 0`.
- **Cause.** `BridgeSystem::Init` defines the default bridge kinds: timber 4, concrete 8, steel truss 16 wide. In the game
  process it is called only by the `bridge_test` console command; nothing on the game's startup or level-load path calls
  it.
- **Scope.** Seed-independent: the census over requested seeds 1..16 failed 16 of 16 identically.
- **In game.** The real in-game start (`--procedural-skirmish 1`) fails the same way. A Procedural Skirmish therefore
  opens on an empty native level with no terrain. The failure is only logged in `Post_Load_Level`; recorded in
  `gameplay-gaps.md`.
- **Not fixed (this decision's stop rule).** Which owner registers the bridge kinds at startup, and with what content (the
  three defaults, or authored kinds), is the next decision.

**Tests.** `Check_World_Metrics_Model_Key` (`-terrainselfcheck hybridpreflight`): a probe
prototype with a WORLDBOX of 4.5 x 2.5 x 1.5 measures identically from a definition path, the render-object name, the
bare filename and an upper-case path; a missing model stays unmeasured; `Render_Obj_Name` leaves a canonical name
unchanged. With the old verbatim lookup restored, three of the four forms fail. Full suite on the exact committed tree:
237/237 (122 client, 115 FDS).

The census therefore stopped at 16 seeds, with 0 attempts entering Phase 2, and nothing reached Phases 9-12. No
smoke test was possible.

## D-033 - Hybrid rewrite: a production census in the real game; the first production blocker is preflight (2026-10-05)

**Direction (user, 2026-10-05).** The 1,400-world census is a fixture harness (1200 m worlds, fixture constants, one
`test_bridge` kind). Before chasing another census-only failure, establish a measurement path that runs the shipped
native profile on the production content through the game's own generator, use it to find a real requested seed that
passes Phase 12, and smoke-test that exact seed in game. Stop and report if the bounded search finds none, without
changing generator rules.

### The production path (observational only)

- **Attempt journal.** `ProceduralWorldManager` records every attempt `Generate_World` runs (`ProceduralWorldAttemptRecord`,
  `Get_Attempt`). Each record holds:
  - the effective seed;
  - the stage reached (`Procedural_Attempt_Stage_Name`) and the error that stopped it;
  - archetype and biome;
  - the Phase 2, 9, 10, 11 and 12 gate checksums;
  - the manifest's reproduction string: generator version, seed, profile hash and, past preflight, the hybrid planning
    hash and content revision.

  Nothing reads it back. Every procedural level load prints its `WORLD` lines, in Release too, where `WWDEBUG_SAY`
  compiles out.
- **Census.** `ProceduralWorldToolsService::Run_Seed_Census` is the seed batch's Destroy / Request / Generate loop. It
  differs in three ways: it never stops on a failed seed, it prints the journal and per-stage counts, and it stops at the
  first requested seed whose loaded world passed Phase 12.
- **Developer launches.** In the real client, after the game's own startup instead of the intro movies:
  - `--procedural-census FIRST COUNT [PROFILE]`;
  - `--procedural-skirmish SEED [PROFILE]`, which is the main menu's Procedural Skirmish (`Start_Procedural_Skirmish`) at a
    chosen requested seed;
  - `--procedural-log PATH`.

  The census mounts what a native level load mounts before `Post_Load_Level` generates: the definitions
  (`CombatManager::Load_Game_Definitions`, now shared with the load thread) and the always assets.
- **Proof it is production.** It runs in `renegade.exe` with `--gamedir C:\Westwood\Renegade_full`:
  - it reads that install's `always.dat` / `Always2.dat` (1,897 physics definitions), its `data/building_descriptors`,
    and its registered bridge definitions;
  - it uses the repository's shipped `data/procedural_world/native.json` (2400 m; the install's copy is a stale generator-7
    file);
  - it calls `Request_World` and `Generate_World`, the same calls the menu makes.

  Nothing in it substitutes a metric, a bridge, a dimension or a feature flag. The in-game skirmish for requested seed 1
  printed the identical `WORLD` line the census printed for it.

### Result: no production seed reaches Phase 2

Requested seeds 1..256 make 256 attempts, all stopping at preflight with
`MISSING_HUMVEE_COLLISION_METRIC (v_gdi_humvee)`. That is one attempt each: a preflight failure ends the request, and
preflight is content-only, the same for every seed. The search was not widened to 1023 or 4095 because every seed
would fail identically.

The real in-game start (`--procedural-skirmish 1`, the menu's own path) fails the same way. That makes the first
production blocker preflight, not geometry: today every production request stops there.

**Cause (probed in the real client).**
- `HybridPreflightInputs::Gather_From_Runtime` finds the five physics definitions whose model is
  `vehicles\gdi humvee\v_gdi_humvee.w3d`.
- `WorldMetricsClass::Resolve_Extents` passes that raw definition path to `WW3DAssetManager::Create_Render_Obj`, which
  returns null, so no `WORLDBOX` is read and the metrics come back valid but without extents.
- The engine's own `PhysClass` never does that. A model name with an extension goes through
  `create_render_obj_from_filename`, which strips the directory and extension (`v_gdi_humvee`) first.

The heavy vehicle and both Harvesters are measured through the same `Resolve_Extents`. Preflight stops at the Humvee,
its first check, so whether their definitions carry paths too was not probed. The offline-measured `WORLDBOX` values in `VehicleCollisionMetrics.md` exist, so the asset is fine; only the
runtime name resolution is wrong.

**Not fixed here (D-033's own rule).**
- `WorldMetricsClass` is a shared owner (navigation, vehicles, placement read it). Making it resolve models as `PhysClass`
  does would change every consumer that currently falls back to the default envelope, which is a cross-cutting change
  that needs its own decision.
- Phase-12 geometry, the fixture census categories, archetype 5 seed 64 and every other measured category are untouched.

**Validation.** Client, FDS and leveledit build with no warnings. Full suite on the exact committed tree: 237/237 (122
client, 115 FDS).

**The smoke test** was not run. No production world reaches Phase 12, so none can be smoke-tested, and nothing synthetic
was substituted.

## D-032 - Hybrid rewrite: the native generator builds no tunnels (2026-10-05)

**Decision (user, 2026-10-05).** Tunnels are removed from the native hybrid procedural-world generator: they are no
longer an allowed generated crossing solution, fallback, quota or procedural feature. Where a tunnel was a fallback, the
road continues to the next existing legal fallback (one more Phase-10 envelope, then failure); no new crossing method
replaces it. Authored-map and legacy engine tunnel support is preserved. This is an explicit divergence from the audited
plan's Sections 8 (Phase 11 method orders) and 12.5 (tunnels), recorded here rather than made silently.

**Before.** Tunnels were already never generated: `Hybrid_Native_Tunnels_Available` forced `tunnel_available = false`
in Phases 6, 9 and 11 since the 2026-10-03 deferral. The branches stayed, unreachable.

**Removed from the native hybrid path:**
- Phase 11: `HYBRID_METHOD_TUNNEL`, `Compile_Tunnel` and both its fallback calls (canyon/ravine: bridge, surface pass,
  envelope; cliff/ridge: road cut, ramp pass, envelope), `HybridTunnelModule`, the interaction's `TunnelModule` /
  `TunnelLength` (and their checksum contributions), the `TunnelsEnabled` / `TunnelAvailable` / `TunnelModules`
  params, and every tunnel branch in the approach-reserve, swept-proxy, structure and gore-reserve checks.
- Phase 6: `HYBRID_WINDOW_METHOD_TUNNEL` (bit 1 << 2 retired, the others keep their values) and the
  `TunnelsAvailable` / `TunnelRequiredWidth` / `TunnelRequiredHeight` params.
- Phase 9: the `TunnelsAvailable` param and the tunnel-obligation branch; Phase 2's `HYBRID_CROSSING_TUNNEL` (never
  produced) and Section 2's `HYBRID_RESOLUTION_TUNNEL` (never set).
- `Hybrid_Native_Tunnels_Available` itself.
- Preflight: the tunnel-module gathering from `TunnelSystem`, `HybridTunnelModuleInput`, the metric cache's
  `TunnelsAvailable` / `TunnelPortalModule` / `TunnelRequiredWidth` / `TunnelRequiredHeight`, their content-revision
  and planning-hash contributions, and `world_features.tunnels` from the planning hash.
- The manager's Phase 6/9/11 tunnel params and module copy; the manifest's canonical profile string now holds
  `world_features.tunnels` constant on the hybrid path, as it already did for settlements.
- `native.json`'s `world_features.tunnels`. The field still parses (the legacy fit-to-level profiles use it) and is
  inert on the native hybrid path; a test proves it changes neither preflight hash.
- Tests that only validated generated tunnel selection or preflight tunnel availability; the census's per-window
  `tunnel` column.

**Retained, deliberately:**
- `TunnelSystem` (wwphys): authored and legacy map tunnels -- data, module chain, geometry, portal cuts, navigation
  links, collision, save state, `tunnel_status`, `debug_layer tunnels`; its own `-terrainselfcheck tunnels` suite.
- `TunnelPlanner` and `ProceduralWorldManager::Build_Tunnel_Plan`: the legacy (fit-to-level, non-hybrid) generator's
  tunnel stage, which runs only when the native hybrid path does not (`!hybrid_roads`); the shipped default, small and
  large_supported profiles are that generator. `SEED_DOMAIN_TUNNELS` and the manifest's `SelectedTunnelCandidateIds`
  belong to it.
- Three verbatim plan quotations in code comments that mention tunnels.

**Generator identity** stays 16 (existing policy: no bump for in-progress rewrite changes). Checksums of every native
interaction set change, because the dead tunnel fields left the stream and the method enum lost a member; nothing pins
them.

**Validation.** Client, FDS and leveledit build with no warnings. Full suite on the removal tree: 237/237 (122 client,
115 FDS). Fixture census r16 (same 1,400-world corpus) is identical to r15 in every one of the 430 worlds it records
(448 / 98 / 1 solve Phase 11; Phase 12 now 1 realized and verified, 0 failed, since D-031): no fixture world selected or
needed a tunnel, as expected while the fallback was already forced off.


## D-031 - Hybrid rewrite: a bridge deck's ends conform to the ulp; the D-029 junction cut is verified (2026-10-05)

**Direction (user, 2026-10-05).** Q-028 stays accepted and closed. The next fix is Phase 12's bridge deck-end
conformance failure (archetype 1 seed 93), as numerical robustness only: no bridge geometry, span, abutment, approach
reserve, selection, clearance or crossing location changes. Resolve the unverified D-029 follow-up rule by evidence:
prove it with a regression or remove it. Do not touch archetype 5 seed 64's unrelated-road overlap.

### The numerical cause (probed on seed 93, exact floats)

Seed 93's assembly 3 end pin is the bridge's own `End`, bit for bit. Two things put it off the deck:
1. **The last deck piece did not end on `End`.** `Layout_Bridge` placed it at `Start + direction * length`, which in
   float lands a coordinate ulp beside `End` (here it happened to land on it; in a sweep of 1,200 bridges it misses in
   about one in five).
2. **The parameter's denominator.** `Conform_Point` took `t = (d . A) / (plan_length * plan_length)` with `plan_length`
   the rounded square root of `|A|^2`. That square came out one ulp below `|A|^2` (0x1.b752fa vs 0x1.b752fc), so the end
   point -- whose numerator is exactly `|A|^2` -- projected to `t = 1.00000012` and was refused (`t > 1`).

With those two fixed, seed 93 then failed one step further, at assembly 1's start corner: Section 12.3's deck sweep
asks at `Start + across * reach` (t = 0, at the paved edge). No float pair lies on the deck's end line there; the nearest,
as computed, stands 3.2 micrometres before it (exact parameter -5.3e-7), beyond the projection arithmetic's error
(2.0e-7) and within the point's own representation (2.2e-6). The same boundary class, from the input's rounding rather
than the arithmetic.

### The rule (`BridgeSystem`, the canonical owner)

- `Layout_Bridge` ends the last piece (end cap, or the last span when there is none) on the bridge's `End` exactly
  (`Deck_End`). The near end needs nothing: `Start + direction * 0` is `Start`.
- `Conform_Point` takes `t = (dx*Ax + dy*Ay) / |A|^2` with `|A|^2` itself, so a piece's own end gives exactly 1.
- What remains is floating point's own error, bounded from the operations, no tuned constant (standard model,
  `u = 2^-24`, `gamma_k = k*u / (1 - k*u)`):
  - the arithmetic: `gamma_5 * (|dx*Ax| + |dy*Ay|) / |A|^2 + gamma_6 * |t|` (subtractions, products, sum and division,
    one u more for evaluating the bound in float);
  - the point's representation: `gamma_2 * (|x|*|Ax| + |y|*|Ay|) / |A|^2` (a place on the end line is up to `u*|x|`,
    `u*|y|` from the nearest float pair).
- A `t` within that of `[0, 1]` is the endpoint: clamped to exactly 0 or 1, and the height is the piece's own end
  height. Anything farther is off the deck, as before. The allowance is the coordinates' few ulps -- tens of
  micrometres -- and does not grow with span (a 420 m bridge gets no more than a 2 m one). The lateral (width) test is
  unchanged.
- No existing engine tolerance governs this: `LineSegClass::Find_Point_Closest_To` clamps without a containment test,
  and `WWMATH_EPSILON` (1e-4) is a world-space margin, which this must not be.
- Phase 11's plan-side mirror (`Pin_Deck_At` in hybridrealization.cpp) is unchanged: it already divides by `|d|^2` over
  the whole assembly; it was not part of this failure.

### The D-029 follow-up cut: option A (proven, retained)

The evidence: the follow-up commit added the cut for "interpolation leakage at held ends", measured only together with
fix 1. A full 1,400-world census with the cut disabled (and the D-031 bridge fix, which touches nothing before
Phase 12) against r15:
- final failure categories: identical in all 1,400 worlds;
- rejection paths: different in 17 worlds. Two of them, archetype 5 seeds 95 and 182, regain D-029's own
  "landing vs its own junction" conflict: the later road's junction piece at stations 4.98..6.65 straddles its landing's
  end at 5.30 and reads 0.036 off the landing (2Q is 0.002), where the true surfaces agree. With the cut: 0 such conflicts.

Regression `junction_piece_cut_at_landing_end`: a branch leaving a trunk's node at 15-40 degrees, on the 0.05 slope,
with the node junction's landing, with and without the 1.5-cell margin -- 12 cases. On the branch the trunk's surface
stays level past the branch's landing end and then climbs; the branch's sampled junction chord spans that turn. With the
cut all 12 compile and the branch stands 0.0000 off the trunk's surface at its landing end. Without it 8 of 12 are
refused with exactly the census conflict ("landing constraint ... stations 0.00..5.30 ... and junction constraint ...
stations 4.98..6.65 ... hold one stretch 0.006 apart").

The invariant it enforces: wherever a junction piece meets another held height on the same road, the piece's held line
passes through the earlier road's solved surface at that station, so two holds of one surface are compared as one.

### Tests

`terrain_bridges` (`Check_Bridge_Deck_Ends`):
- seed 93's assembly 3, bit for bit on the census's bridge definition: the old projection refuses its end; now it
  conforms at exactly 4.5;
- seed 93's assembly 1 start corner as Phase 12 computed it: exact parameter -1.1e-6, bound 4.5e-6; conforms at 4.5;
- 1,200 bridges (every 3 degrees, lengths 2 / 13.1 / 47.3 / 160 / 420, two map positions, sloped decks): every exact end
  conforms at its own height (123 ends the old projection dropped); every interior point the old projection accepted is
  still accepted at the same height within 4.8e-7 (2 points on a joint between spans, which the old projection dropped
  as 1.0000001 on one and below 0 on the next, now conform); 153,600 outward one-ulp steps from both ends, none past
  twice the bound conforms; widest accepted overrun 0.12 / 0.12 / 0.12 / 0.18 / 0.18 mm by length.
- Each half is needed (measured before the representation term was added): with d3c3d10d's `bridgesystem.cpp`, seed
  93's end and 187 sweep ends fail; with the projection fix alone (no `Deck_End`), 42 sweep ends fail. The
  representation term is what seed 93's assembly-1 corner needed in the pipeline run.

`terrain_hybridrealization`: `junction_piece_cut_at_landing_end` as above.

### Seed 93

Archetype 1 (twin_lakes) seed 93 through the census pipeline: solves Phase 11 (one reroute, 21 interactions, 6
landings) and **passes Phase 12**: realized on the canonical owners and verified (`Verify`, `Verify_Physical`,
Section 12.3). `phase 12: 1 realized and verified on the canonical owners, 0 failed.`

### The in-game smoke test: still blocked, for a reason found here

The census is a fixture harness, not the in-game native profile: it builds 1200 m worlds from fixture constants
(`Graph_Identity_Fixture`) and fixture content (one bridge kind `test_bridge`, width 40), while
`data/procedural_world/native.json` is a 2400 m world on the real content. "Archetype 1 seed 93" is therefore not a
world the game can load, and in-game seed 93 is a different world the census never measured. Earlier reports said the
smoke test needed "a native world that solves Phase 11"; that conflated the census with the in-game native profile. No
in-game native-profile world is known to pass Phase 12, so the smoke test was not run and nothing was substituted.

Full suite on this exact tree: 237/237 (122 client, 115 FDS), the own-pass regression, the bridge fix and the cut's
regression included.

**Status: implemented. Stopped for direction; archetype 5 seed 64's overlap not touched.**

## D-030 - Hybrid rewrite Q-028: a realization-aware grade budget, and a compiled junction's raster support (2026-10-05)

**Decision (user, 2026-10-05), resolving Q-028.**
- **Curve/grid grade amplification (mechanism A):** option (b). The profile solve takes a realization-aware grade
  budget. It is derived from the native rasterizer's own geometry, not from an observed margin.
- **D-026 is not relaxed.** The realized-grade check, the class maximum grades and every other limit are unchanged.
- **No semantic widening of c3b18778.** Only an explicit compiled Phase-11 junction creates a shared surface.
- **Grid-band conflicts (mechanism C):** a deterministic raster support footprint, tied only to an explicit compiled
  junction, so the grid triangles realizing that junction cannot be given incompatible vertex ownership. The semantic
  shared surface and the raster support are separate concepts.
- **Pin-moved vertices (mechanism B)** stay under D-027's landings and rejections, untouched.

### Why a legal curved profile realized steeper (measured)

A probe over the three worlds that reached the grid judgement found that every vertex under a failing envelope held
exactly its road's plan. Each roadbed vertex holds the profile at its nearest centerline foot (12.2 step 1); the grid
interpolates the three vertices of a triangle. On a straight road the foot station is affine in position, so the
triangulated surface rises along the road exactly as fast as the profile (the triangle gradient's component along the
road is a weighted mean of profile steps, weights summing to one). On a curve the foot station is not affine: inside the
curve it advances faster than position, outside slower, and on the sampled polyline it jumps at the inner bisectors.
The surface between an envelope's two grade points can then rise faster than any profile step under it.

### The budget (`Hybrid_Apply_Grade_Budget`)

For each envelope a road must carry and each sample Section 12.3 sweeps from:
1. the two grade points exactly as `Pin_Sweep` places them (the roadbed midpoint on the road's realized paved edges, or
   all three placements when the roadbed is narrower than the envelope);
2. the grid triangle under each (`Hybrid_Grid_Height_At`'s low-to-high split) and its weights;
3. each vertex's nearest centerline foot (`Deform_Roads`' rule: nearest segment, the first of equals, clamped).

The realized rise between the points is then a fixed linear sum of the profile's steps, `D = sum(b_k * dZ_k)`. If
every step obeys `|dZ_k| <= s_k * ds_k`, then `|D| <= sum(|b_k| * ds_k * s_k)`. Each step takes the amplification of its
worst envelope, `A_k = max(1, sum(|b_k| * ds_k) / (L + Q / G))`, `G = tan(class + Q_angle)`, and the solver grade
`atan(G / A_k) - Q_angle`. The solver's own quantization keeps every step within `tan(grade + Q_angle) = G / A_k`, so
`|D| <= G * L + Q`, inside D-026's unchanged `G + 2Q / L`. One Q of the 2Q is left for float rounding.

- **Conservative by construction.** The bound holds for any profile whose steps keep their grades, at the grid
  alignment the road actually has, because it is computed on that alignment itself. It is no fudge factor: no number
  in it comes from the census.
- **Straight roads unchanged.** `b_k >= 0` and `sum(b_k * ds_k) = L` exactly: `A = 1`, the class maximum.
- **No fixed penalty.** A broad curve's few steps whose amplification exceeds the `Q / G` slack are budgeted by that
  amount only.
- **Exempt steps.** A step with both samples fixed (a deck, landing or junction hold) or touching a deck/bore interior
  keeps the class: its height is a compiled constraint its owner realizes. Envelopes over a deck, or with vertices under
  one, are the deck end's (D-027).
- **Own pass only.** A vertex under an envelope is within `r` (widest paved half + a cell diagonal) of the centerline at
  its grade point, so its nearest foot on the envelope's own pass lies within `pi * r` of arc. A foot farther along the
  road is another pass of the same road (a road returning beside itself); that height is not a grade of this pass and
  no budget can change it, so the envelope is not modelled. Found on the first filtered run: without this rule the
  budget read such self-proximity as amplification 36-238 on dead-straight stretches and refused bridges in 13 of
  r11's 34 worlds as `grade_too_steep`; with it the worst amplification on the same three worlds is 1.13 (a real 75
  degree corner, R 32), none above 1.25.
- **One solver.** All four Section 12.1 steps (forward, backward, clamp walk, post-quantization revalidation) use each
  step's grade (`HybridProfileSample::MaxGrade`). There is no second pass and no post-hoc shaving.
- **Phase 11 / Phase 12 parity.** `Build_Plan` takes the budget on the road's realized paved edges
  (`Sample_Edges`) and the native grid's origin and the swept envelopes (`HybridRealizationParams`), so Phase 11's
  compile and Phase 12 grade with the same one. Phase 11's own crossing gradings (`Hybrid_Solve_Road_Stretch`, the
  ramp profile) take the same helper on the class's own edges. Phase 11's analytic pre-filters (junction arm interval,
  overpass ramp length, scoring) are unchanged; the compile remains the acceptance.

### The junction's raster support (`Hybrid_Junction_Raster_Support`)

- **Semantic surface:** the later road's paved roadbed over every station its `JUNCTION` records hold. Unchanged.
- **Raster support:** every grid vertex one of whose own triangles meets that surface. With the low-to-high diagonal
  split, a vertex's triangles cover the hexagon `|x|, |y| <= cell`, `|x - y| <= cell` about it. So the support is
  exactly the vertices a triangle under the surface references, at most one cell from it.
- **Ownership inside it:** the earlier road keeps its paved roadbed; its band beyond its paved edge (cell reach) yields
  to the junction, graded by the later road's own roadbed construction (the shared surface where the foot is held,
  its own profile past the hold). After the later road is deformed the whole support is protected; no road after the
  earlier one, except the later, writes it.
- **Outside it:** unchanged. Roads with no compiled junction have no support.
- It is never a height constraint: the later road is not held to the earlier road's height because a vertex lies in
  the support.

### Surviving rules from the D-029 investigation, stated by verified behaviour

- A compiled junction's `STRUCTURAL_PIN_LANDING` holds every road its junction record carries (`Junction_Carries`:
  either edge, or the road through its node), level about that road's station nearest the pin. Other pins apply to
  roads whose centerline passes within 2Q. Test: `junction_landing_off_centerline`.
- A junction stretch on the later road is cut at every end of another held structure inside it, so where the two meet
  they are compared at the earlier road's surface itself, never at a sampled chord spanning the other's end. Verified
  since D-031 (test `junction_piece_cut_at_landing_end`, and census evidence); it was unverified when D-030 was
  written.
- Only an explicit compiled junction shares a surface; incidental overlap holds nothing (c3b18778). Tests: D-028's
  parallel-road checks, and Q-028's `incidental_overlap_still_independent`.

### Tests (`Check_Hybrid_Grade_Budget`, in `terrain_hybridrealization`)

- `straight_grade_unchanged`: a straight PRIMARY_2 across the grid diagonal at the class maximum 0.2126: amplification
  exactly 1, solved at 0.2130 (the class plus the solver's angular quantum), realized clean.
- `curved_grid_amplification`: a 16 m half circle at the class maximum. Without the budget it solves at 0.2128 and the
  unchanged D-026 check refuses it (an envelope on a 0.2144 grade). With it (worst amplification 1.0267) its budgeted
  steps solve at most 0.2122, every one within its own budget, and it realizes clean.
- `radius_dependence`: the same arc length at radius 12 / 24 / 48 / 96 / 1500: amplification 1.0458 / 1.0133 / 1.0126 /
  1.0038 / 1.0000.
- `no_unnecessary_curve_penalty`: the 1500 m curve budgets 0% of its steps.
- `pin_behavior_unchanged`: a fixed hold on the 16 m curve keeps the class; its free steps take the budget. D-027's own
  landing and rejection tests (`Check_Hybrid_Pin_Landings`) pass unchanged.
- `own_pass_only`: a road returning 4-7 m beside itself keeps amplification exactly 1 on its straight leg (12.21
  without the rule).
- `compiled_junction_raster_guard`: on D-028's overlapping parallel roads with a compiled junction, 43 support vertices;
  the 22 in the earlier road's band beyond its paved edge hold the later road's own surface (4 where the two surfaces
  differ), and away from the junction that band keeps the earlier road's surface.
- `guard_is_not_semantic_hold`: 2 of those vertices lie past the held stretch and hold the later road's own profile,
  not the earlier road's height.
- `incidental_overlap_still_independent`: the same roads with no compiled junction have no support at all, and D-028's
  parallel-road checks (each road its own profile, no pin) pass unchanged.

### Measured on r11's 34 (filtered census, unchanged corpus)

| After Q-028 | Before (1fb67210) | Failure |
| --- | --- | --- |
| 16 | 16 | `pin_landing` |
| 12 | 11 | `profile_conflict` |
| 3 | 3 | road crossing |
| 1 | 3 | ordinary roadbed on the grid |
| 1 | 1 | ground-only profile |
| 1 | 0 | **solves Phase 11** |

- **Archetype 1 seed 93 solves Phase 11.** Phase 12 then refuses it at Section 12.3: the deck end of assembly 3 reads
  "-4.5000 off its compiled 4.500". Cause, probed: the pin is the bridge's exact end point, and
  `BridgeSystem::Conform_Point` rejects the parameter `t = 1.0000001` (one float ulp past the end, `t > 1.0` with no
  tolerance), so no deck is found there. With a temporary 1e-5 tolerance (probe only, not committed) the world passes
  Phase 12 entirely: realize, verify, Section 12.3. Not a Q-028 defect; reported, not fixed here.
- **Archetype 5 seed 95** no longer fails on the grid. It now fails at `pin_landing` (the topology junction at node
  15). Among its rejections, a landing (interaction 12) and a junction (interaction 7, earlier road 5) hold road 12's
  start 0.014 apart: the existing different-interaction landing/junction overlap category D-029 counted.
- **Archetype 5 seed 64** still fails on the grid, now on cross slope (0.2330) rather than grade: SERVICE_DIRT_2 road
  12's own centerline vertices are held by earlier road 0's roadbed band 0.47-0.66 m above road 12's plan, 38-44 m
  from the nearest junction pin, past its 13.29 arm reach. Incidental roadbed overlap away from any compiled junction:
  by c3b18778 and this decision it shares nothing and gets no support, so it is a separate cause (independent roads
  overlapping outside a junction's reach, D-020's rule), not Q-028's.
- **Archetype 3 seed 162** fails at `profile_conflict` where it failed at `pin_landing`: the same crossing area, its
  refusal order changed with the profiles.

Before the own-pass rule was found the first filtered run showed 13 new obstruction-crossing failures; that run is
superseded by the table above.

### Census r15 (same 1400-world corpus, after Q-028)

Phases 9 and 10 are unchanged: **448** and **98**. **Phase 11: 1 of 98 solves** (archetype 1 seed 93, after one
reroute). **Phase 12: 0 pass, 1 fails** (that world, on the deck-end ulp above). No native world passes Phase 12, so the
in-game smoke test was not possible and was not attempted.

**Phase 11 failures, all 98 worlds (mutually exclusive).** r14 predates the D-029 follow-up (1fb67210), whose full census
was not run, so the column mixes both changes.

| r15 | r14 | Failure |
| --- | --- | --- |
| 50 | 50 | road crossing: neither junction nor overpass |
| 19 | 22 | junction refused: `pin_landing` |
| 16 | 15 | junction or overpass refused: `profile_conflict` |
| 10 | 10 | obstruction crossing |
| 1 | 0 | `realization`: ordinary roadbed on the grid (archetype 5 seed 64: incidental overlap, above) |
| 1 | 1 | `realization`: ground-only profile (archetype 1 seed 80, its own 6.008 cut) |
| **1** | 0 | **solves Phase 11** |

The 51-crossing / 50 road-crossing failures were not touched. 37 worlds rejected solutions before their final failure.

**Status: implemented; filtered and full census reported. Stopped for direction before the next behavioural change.**

## D-029 - Hybrid rewrite: one canonical vertical-constraint record set, compiled in Phase 11 and consumed by Phase 12 (2026-10-04)

**The mismatch (D-028's finding).** Phase 11 and Phase 12 judged junction heights differently:
- **Phase 11** judged an at-grade or topology junction at the ground's height, one junction at a time
  (`At_Grade_Is_Legal`, `Topology_Junction_Is_Legal`).
- **Phase 12** held the later road at the earlier road's solved surface, with every junction and other fixed height on
  that road acting together.

D-027's post-pass re-ran Phase 12's whole plan inside Phase 11. It caught the conflict only as a generic plan refusal
(`realization`: "Phase 12's plan cannot be realized ... forward_propagation_empty"), with a reroute request and no
provenance. In r13 that was 19 of r11's 34 worlds.

**Decision (user, 2026-10-04).**
- **Where the representation lives.** One canonical junction-height / vertical-constraint representation. Phase 11
  produces and evaluates it; Phase 12 consumes it unchanged. It is not a second approximation of Phase 12.
- **Height authority.** The earlier road is the vertical authority. The junction height is its solved surface at the
  junction station. There is no averaging, no bare-terrain fallback, no clamping, and Phase 12 never moves a junction.
- **Reach.** `Hybrid_Junction_Arm_Reach = max(4W, class turn radius)` is kept. Incidental overlap creates no pins.
- **One profile problem per road.** All of a road's fixed constraints are evaluated together, in station order, with no
  distance threshold:
  - compiled junctions;
  - topology junctions;
  - structural pins;
  - deck and bridge endpoints;
  - other fixed heights.
- **Infeasible.** An infeasible profile is a Phase-11 failure or reroute. Nothing is averaged, softened, dropped,
  converted or repaired.
- **Diagnostics.** A rejection names the road, both constraints (kind, source, interaction, station, Z, owning road),
  their separation and dz, and the required against the allowed grade or cut/fill.
- **Frozen.** D-027 rules, the c3b18778 narrowing, the arm reach, class geometry, bridge-kind eligibility, overpass
  policy, Phases 9-10, reroute budgets and the census corpus. The 51 crossing failures are not touched.

**Implementation.**

*The record.* `HybridVerticalConstraint` (hybridinteractions.h) has these fields:
- `StableId`;
- `RoadEdgeId` (the physical road's first logical edge);
- `InteractionId` (the source);
- `OwnerEdgeId` (the earlier road, for junctions and gores);
- `Kind`;
- `From`/`To` (the station range);
- `Level` or carried `StartZ`/`EndZ` with `EndsFixed`;
- `Reach`.

It is frozen on `HybridInteractionSet::Constraints` with `ConstraintsCompiled`, and both are part of the set checksum.

The kinds:
- `STRUCTURE`: decks and bores;
- `LANDING`: D-027;
- `JUNCTION`: the earlier road's solved surface over a compiled at-grade or topology junction's footprint, sample by
  sample through the junction point;
- `LOCAL_JUNCTION`: `z_L`;
- `RAMP_GORE`: the solved highway;
- `OVERPASS_LOWER`.

*Phase 11.* `Hybrid_Compile_Vertical_Constraints` (hybridrealization.cpp) is the compile half of the one plan builder,
`Build_Plan`:
1. It solves the physical roads in routing order.
2. For each road it gathers that road's records and solves the whole profile once (`Hybrid_Solve_Road_Stretch`). A
   junction is owned by the earliest road through it.
3. Where two records overlap, they are compared over the overlap:
   - the same height within 2Q is held once, and only the non-overlapping rest is added;
   - any other height is an `OVERLAP` conflict.
4. When a profile fails, `Diagnose_Profile` re-runs the forward propagation with each bound's source sample. It maps
   both samples to their records and classifies the conflict:
   - `GRADE`: two fixed heights;
   - `CUT_FILL`: one fixed height against the ground interval;
   - `GROUND`: no fixed height.

Phase 11's last step, `Compile_Realization` (replacing D-027's `Compile_Pin_Landings`), runs this always, even with no
terrain grid. On the native grid it then applies D-027's rasterization judgement to the plan those records make, and
recompiles after each landing.

On a conflict, `Profile_Conflict` rejects the later-compiled solution among the two constraints' sources through D-027's
exclusion order. The `SolutionKey` now carries the problem: `profile_conflict`, or `pin_landing` when it was the
solution's own landing. The phase is then re-solved. There are three other outcomes:
- a conflict among interchange elements fails the phase;
- a ground-only conflict asks for the road's next envelope;
- every rejection is kept on `HybridInteractionResult::Rejections` / `RejectedSolutions`. The census prints them as
  `P11REJECT`.

*Phase 12.* `Hybrid_Plan_Realization` is the consume half of the same builder:
- it takes each road's records exactly as frozen and derives none;
- it refuses a set without `ConstraintsCompiled`;
- `HybridRealizationPlan::Constraints` is the same list, field for field.

Topology junctions get the same semantics: they are `AT_GRADE_JUNCTION` records carried by `NodeId`.

**Behaviour change found while testing.** D-028's code dropped a junction's whole shared stretch whenever it touched a
landing or structure. D-029 keeps the consistent rest. On D-027's quarter-cell-shifted mound, that drop was the only
reason the landed local junction failed on the grid: the roads leaving `L_s` fell away from the earlier road's surface
and fought it for the pin's vertices. With the surface held, the junction realizes at every grid shift from 0 to 15/16
of a cell.

That D-027 test now asserts the realization. The original assertion, a landed pin that still fails on the grid and is
then refused, has no controlled fixture left. A pit beside `L_s` is absorbed by the roadbed fill, and a pond would change
the routing. The refusal path itself is unchanged.

**Tests (`Check_Hybrid_Profile_Parity`, plus the c3b18778 regression kept).**
- `single_junction_surface_parity`. A 3 m spike is under the junction. The earlier road solves to 1.599 over ground
  3.000. The later road is held at 1.599, and Phase 12's records equal Phase 11's.
- `two_compatible_junction_constraints`. One later road crosses two earlier roads at 0.000 and 2.000, 60 apart. Both are
  held in one profile, and the records are identical.
- `two_incompatible_junction_constraints`. The earlier roads stand at 0 and 8, 24 apart, and each is legal alone.
  Phase 11 rejects the later junction, naming both junction constraints, both earlier roads, and the grade
  (0.4610 needed, 0.2126 allowed). It then rejects the overpass alternative the same way. Whatever Phase 11 accepts,
  Phase 12 realizes.
- `same_earlier_road_two_junctions`. One earlier road is crossed twice on a sloped ground. Each junction is held at its
  own station's surface (-2.250 / 2.250) as two records with the same owner.
- `junction_bridge_endpoint_conflict`. A trunk crosses a bridge road in a 4 m trench outside its approach. The
  rejection names both constraints:
  - the bridge's `STRUCTURE` at the deck end, z 3.000;
  - the `JUNCTION` at z -4.000, 30.38 apart (grade 0.2304 needed, 0.2126 allowed).
- `topology_junction_height_parity`. A shared-node junction is held at the earlier road's solved surface, with the same
  record semantics, and the records are identical.

The suite is 237/237 (client and FDS).

### Census r14 (same 1400-world corpus)

Phases 9 and 10 are unchanged: **448** and **98**.

**Phase 11: 0 of 98 solve.** Phase 12 is reached by 0 worlds and fails in 0. No native world solves Phase 11, so the
in-game smoke test was not possible and was not attempted.

**Failure categories, all 98 worlds (mutually exclusive):**

| r14 | r13 | Failure |
| --- | --- | --- |
| 50 | 51 | road crossing: neither junction nor overpass |
| 22 | 9 | junction refused: `pin_landing` (D-027 grid refusals, and D-029 conflicts blamed on a solution's own landing) |
| 15 | 0 | junction or overpass refused: `profile_conflict` (D-029 combined profile) |
| 10 | 10 | obstruction crossing |
| 1 | 25 | `realization`: profile infeasible with no fixed height involved (ground alone; `GROUND`) |
| 0 | 3 | `realization`: ordinary roadbed grade on the grid |

**r11's 34 solved worlds:**

| r14 | r13 | Failure |
| --- | --- | --- |
| 19 | 8 | `pin_landing` |
| 11 | 0 | `profile_conflict` |
| 3 | 4 | road crossing |
| 1 | 19 | ground-only profile (`realization`) |
| 0 | 3 | ordinary roadbed grade |

**The former 19 (r13 Phase-12 plan refusals among r11's 34):**
- 11 now fail as `profile_conflict`:
  - 5 are topology junctions;
  - 6 are crossings whose junction was refused and whose overpass alternative was then refused for its own reason:
    `method_unavailable` 3, `turn_too_tight` 2, `grade_too_steep` 1.
- 7 now fail as `pin_landing`. The cause is either a landed pin that fails on the grid, or a D-029 conflict blamed on
  the solution's own landing:
  - 6 are topology junctions;
  - 1 is a crossing whose junction was refused (`profile_conflict`) and whose overpass was refused (`pin_landing`).
- 1 (archetype 1, seed 80) is still a profile with no fixed height involved: the road's own cut/fill bound, 6.008 deep.

None now reaches the old generic refusal with a fixed height involved. Every one names its constraints.

**The D-029 rejections.** 37 worlds rejected 101 solutions before their final failure: 1 rejection in 17 worlds, and
up to 9 in one.

By kind:

| Count | Kind |
| --- | --- |
| 53 | `OVERLAP` (two holds on one stretch) |
| 26 | `GRADE` (two fixed heights too close) |
| 6 | `CUT_FILL` (6.07-7.59 needed, 6.0 allowed) |
| 16 | D-027 grid refusals of a landed pin (the refusal path the controlled fixture no longer reaches) |

The 53 overlaps, by height difference:

| Count | Height difference |
| --- | --- |
| 2 | 0.01 m or less |
| 13 | 0.05 m or less |
| 12 | 0.25 m or less |
| 8 | 1 m or less |
| 18 | more than 1 m |

The 53 overlaps, by the constraints involved:

| Count | Constraints |
| --- | --- |
| 21 | junction and junction, two earlier roads |
| 10 | landing and landing |
| 7 | junction and landing of the **same** interaction |
| 5 | junction and junction, one earlier road |
| 5 | junction and structure |
| 3 | junction and another interaction's landing |
| 2 | junction and overpass lower surface |

**Findings for the user's direction (nothing changed for them).**
1. **A junction's own landing against its own shared surface: 7 rejections.**
   - D-027's landing holds every ground road through the junction pin level at the pin's elevation for `L_pin`.
   - D-029's shared surface holds the later road at the earlier road's solved surface, which slopes away from the pin
     beyond the earlier road's own landing.
   - D-028's code hid this by dropping the whole shared stretch wherever it touched a landing. D-029 forbids the drop,
     so the two frozen rules now contradict each other on the same stretch.
2. **Overlaps of millimetres to centimetres: 15 of 53 are 0.05 m or less.**
   - Two holds on one stretch must agree within 2Q (2 mm; the solver itself pins at plus or minus Q/2).
   - The holds are two junction stretches, or two landings, a few millimetres to centimetres apart. They are genuine
     contradictions under the semantics, but physically small.
3. **The 51 road-crossing failures are untouched.** The census counters below are not modified.

**Road-crossing and overpass counters (reported, not modified).**

The final failing attempt now differs per world, because rejections re-solve:

| Counter | r14 | r13 |
| --- | --- | --- |
| Road meetings | 586 shared-node, 17 obligation, 253 incidental | 667, 17, 585 |
| Failed meetings | 25 shared-node, 17 obligation, 46 incidental | 8, 17, 62 |

Junction refusals:

| Reason | r14 | r13 |
| --- | --- | --- |
| `approach_does_not_fit` | 9 | 28 |
| `turn_too_tight` | 22 | 29 |
| `reserve_conflict` | 3 | 3 |
| `pin_landing` | 21 | 10 |
| `profile_conflict` | 16 | (new) |

Overpass refusals:

| Reason | r14 | r13 |
| --- | --- | --- |
| `approach_does_not_fit` | 3 | 2 |
| `grade_too_steep` | 5 | 2 |
| `turn_too_tight` | 57 | 53 |
| `method_unavailable` | 59 | 101 |
| `pin_landing` | 1 | 0 |
| `profile_conflict` | 1 | 0 |

Reroutes: 251 requested, 97 exhausted (r13: 336, 97). Phase-11 failure codes: 74 `no_solution`,
23 `obligation_lost`, 1 `realization` (r13: 47 / 23 / 28).

**Status: implemented. Census r14 reported. Stopped for direction before any other behavioural change.**

### Follow-up after the user's "go" (2026-10-04): finding 1 was an implementation defect, not a rule conflict

**Root cause of the 7 same-interaction landing/junction overlaps.**
- `Add_Landings` decided which roads pass through a landed pin by distance: centerline within 2Q of the pin.
- A topology junction's pin is the node point, and a trunk through a node need not pass through it. Probed in the
  census, the earlier road's centerline missed its own junction's pin by 0.019 to 27 m.
- So the landing held only the later road level. The earlier road kept its slope, and between landing rounds it moved,
  leaving the landing's Z stale against the shared surface built from it.

D-027's rule (every ground road through the junction pin is held level for `L_pin`) and D-029's shared surface do not
conflict. The proxy did.

**Fixes (no rule changed).**
1. **Landing membership by junction record.** A junction's landing applies to every road its record carries
   (`Junction_Carries`: either edge, or the road through its node), held level about that road's station nearest the
   pin. Other pins (deck ends, overpass crossings) keep the 2Q test.
2. **Junction pieces cut at held ends.** A junction stretch is split at every end of another held structure inside
   it, so its sampled chords never interpolate across a landing's or deck's boundary.

Test `junction_landing_off_centerline`: a trunk running 1.5 off a topology node on a 0.05 grade receives the
junction's landing and is held level about its nearest station (0.0000 off; 0 landings before fix 1).

**Measured on r11's 34 solved worlds (filtered census).**

| After | r14 | Failure |
| --- | --- | --- |
| 16 | 19 | `pin_landing` |
| 11 | 11 | `profile_conflict` |
| 3 | 3 | road crossing |
| 3 | 0 | ordinary roadbed grade on the grid |
| 1 | 1 | ground-only profile |

- Same-interaction landing/junction overlaps: 7 to 0.
- Three worlds (archetype 5 seeds 64 and 95, archetype 1 seed 93) now pass the plan and fail later, on the D-026 grid
  judgement of ordinary roadbed grade. That judgement and its cause are Q-028.

## D-028 - Hybrid rewrite: the shared junction surface is a compiled junction's, nothing else (2026-10-04)

**Decision (user, 2026-10-04).** Narrow the shared-surface rule to compiled Phase-11 junctions; this supersedes D-025
reading 2.
- **When a surface is shared.** A later road uses an earlier road's surface only when both roads take part in the same
  explicit Phase-11 compiled junction record.
- **What never creates a vertical pin.** Incidental paved-footprint overlap, nearby or parallel roads, adjacent roads,
  and roads passing through another road's margin.
- **A compiled junction's surface:**
  - it uses the junction's frozen elevation and profile;
  - it applies only over that junction's compiled reach/patch;
  - it is processed once, under the earlier road;
  - outside that patch, each road follows its own Phase-12 vertical solution.
- **Phase consistency.** Phase 11 and Phase 12 use the same junction footprint and elevation constraints, and Phase 12
  adds no vertical pin Phase 11 never validated. D-027's structural-pin and overpass rules are unchanged.

**Implementation.**
- **The footprint.** `Hybrid_Junction_Arm_Reach` (`max(4W, class turn radius)`) is the arm Phase 11's at-grade and
  topology junction tests reserve and grade each road over. It is now one shared function. Phase 12 holds a road at
  an earlier road's surface only for an at-grade junction record both roads carry (`EdgeId`, `OtherEdgeId` or `NodeId`).
  It holds it only within that road's arm reach either side of the junction point, and only on the one stretch of its
  centerline over the earlier roadbed (plus the 1.5-cell grid margin) that passes through the junction point. Any other
  overlap inside that window is incidental and holds nothing.
- **The elevation.** It is the earlier road's own solved surface there: the junction's frozen pin and profile (D-026).
- **Unchanged.** Interchange local junctions (`z_L`), gores and overpass lower surfaces keep their compiled holds.
- **Validation.** Phase 11 builds this same plan in its D-027 judgement, so every hold Phase 12 applies is one Phase 11
  validated.

**Regression test.** It runs on two parallel `PRIMARY_2` roads with no junction record:
- **Overlapping:** paved footprints overlap, 6.48 apart.
- **Nearby:** footprints apart, the later centerline inside the old 1.5-cell margin, 9.29 apart.

In both cases each road keeps exactly the profile it solves to alone (0.324 / 0.465 apart in z), with no pin. With a
compiled junction record between the overlapping roads, the later road is held at the earlier surface within its
16.61 arm reach (2Q), and its own profile elsewhere. Reinstating the old any-overlap rule fails both no-junction cases.

### Census r13 (1400 worlds; Phase 12 now runs in the census on every Phase-11-solved world)

**448 / 98 / 0** in Phase 11; Phase 12: 0 reached, 0 failed. Phases 9 and 10 are unchanged.

**Phase-11 failures, all 98 worlds:**

| Count | Failure |
| --- | --- |
| 51 | road crossing: neither junction nor overpass (overpass refusals `turn_too_tight` 53, `method_unavailable` 101; r11: 25 / 59) |
| 25 | `realization`: Phase 12's profile plan refused |
| 10 | obstruction crossing |
| 9 | junction refused: `pin_landing` |
| 3 | `realization`: ordinary roadbed grade on the grid |

**r11's 34 solved worlds:**

| Count | Failure |
| --- | --- |
| 19 | Phase-12 plan refused |
| 8 | `pin_landing` |
| 4 | road crossing |
| 3 | ordinary roadbed grade |

r12 had 18 plan refusals among them; the narrowing removed the overlap holds, and the 19 left are all compiled-junction
holds.

**The 19 plan refusals, measured:**

| Count | Cause |
| --- | --- |
| 12 | No adjacent pair of holds is over grade: a compiled junction's held surface lies outside the later road's cut/fill interval |
| 4 | Two compiled junctions with two different earlier roads, 0.8-4.6 m apart, whose surfaces differ by more than the grade allows (e.g. 1.11 m in 0.79 m) |
| 1 | Two junctions on the same earlier road |
| 1 | A deck end against a junction hold |
| 1 | Another hold |

**Finding, not yet acted on.** The junction footprint is now shared. The junction elevation is not:
- **Phase 11** judges an at-grade junction at the ground's height (`At_Grade_Is_Legal`, `Topology_Junction_Is_Legal`),
  one junction at a time.
- **Phase 12** holds the later road at the earlier road's solved surface, and junctions close together each hold it.

D-027's post-pass catches the conflict in Phase 11 (`realization`, with a reroute request), but the junction judgement
itself does not yet use Phase 12's elevation. Reported for the user's direction before any further change.

*Resolved by D-029 (user, 2026-10-04):* one canonical vertical-constraint record set, compiled by Phase 11 with every
fixed height on a road together, consumed unchanged by Phase 12. Census r14 is in D-029.

**Status: implemented. Census r13 reported.**

## D-027 - Hybrid rewrite Q-027: structural pin landings, overpass deck reach, the bridge-end support footprint (2026-10-04)

**Decision (user, 2026-10-04).** Q-027: pins option (A) and overpasses option (C), each refined.

- **Pins.**
  - `L_pin = native_terrain_cell * sqrt(2)`. Phase 11 first tests whether the planned surface rasterizes directly,
    meeting the pin at 2Q, the class grade and cross slope, and cut/fill. If it does, the profile stays as it is.
  - When the coarse grid cannot hit the frozen pin, Phase 11 compiles a `STRUCTURAL_PIN_LANDING`. It sits at the pin's
    frozen elevation, exactly `L_pin` along each attached ground road away from the pin. Phase 11 includes it in
    vertical feasibility, and Phase 12 realizes exactly it.
  - A landing that makes grade or cut/fill impossible rejects the solution. Only the existing fallback, reroute and fail
    rules follow. Phase 12 never discovers the contradiction first.
- **Bridge deck ends.** A river or canyon deck is never lengthened to make a landing. The road-side terrain just outside
  the deck is the landing, and the terrain under the end support is adjusted as already implemented.
- **Overpasses.**
  - On each side, the deck ends at least `upper_outer_blend + L_pin` past where the upper road leaves the lower road's
    actual paved roadbed polygon (asymmetric and widened edges included, shoulders counted once).
  - Phase 11 uses the extended deck for span, `L_ramp`, grade, cut/fill, clearance, approach reserve and gore/structure
    conflicts, and rejects or reroutes there.
- **Bridge-end support footprint.**
  - **Across:** `max(actual bridge/abutment width, attached road's actual paved roadbed at the endpoint)`, shoulders
    and asymmetric/widened edges included, never the outer blend.
  - **Along:** at most 1.5 native cells along the deck.
  - **Untouched:** water geometry and the open river outside it.

### Implementation

1. **Records.**
   - `HybridPinLanding` is `HybridInteractionSet::Landings`, in the stage-11 checksum.
   - The pin kinds move to `hybridinteractions.h`.
   - The Phase-12 plan holds each landing as a level structure on every ground road through its pin, exactly `L_pin`
     away from the pin on each ground side. It never overlays a deck or bore, so at a deck end it is the ground road
     outside the deck.
2. **One judgement, Phase 12's own criteria (`Hybrid_Plan_Rasterization`).** Phase 11's last step builds the very plan
   Phase 12 will realize (`Hybrid_Plan_Realization` over the compiled records). It then runs Phase 12's own
   deformation and pin realization over the native grid (`TerrainCellSize`/`TerrainOrigin` on the Phase-9 and
   Phase-11 params). Over that realized surface it checks Section 12.3's criteria, through limits shared with Phase 12
   (`Hybrid_Realized_*`):
   - every pin at 2Q, and the ground its correction moved inside cut/fill;
   - every ground sample's surface within the ordinary tolerance and the cross slope;
   - every sample swept with each envelope the road requires, placed and judged as Section 12.3 places it.
3. **Outcomes.**
   - A failure within `L_pin` + 2 cells of a pin compiles that pin's landing, and the plan is judged again.
   - A pin that still fails, or a landing that leaves its road no profile, rejects the solution the pin belongs to.
     Phase 11 is solved again with that solution excluded (`SolutionKey`), so the solution's own order picks the next
     station, the swap, the next method or a reroute.
   - An interchange element is never rerouted: its rejection fails Phase 11 (`interchange`).
4. **Ordinary roadbed (reading, for review).**
   - **The finding.** On native cells, a road climbing at the class maximum across a grade change can realize a vehicle
     grade a hair over the limit (0.2143 against 0.2140 on the falling-bank world). This is away from every pin.
   - **The reading.** The same judgement catches it in Phase 11 and asks for that road's one more envelope (the
     existing fallback) or fails there. Phase 12 never meets an ordinary-roadbed failure first either.
   - **Limits.** The vehicle grade limit is Section 12.1 step 4's: class maximum + 0.001 rad, + 2Q over the base.
     Before this the Phase-12 sweep lacked the angular quantum and refused profiles the solver itself accepts.
5. **Overpass reach (`Overpass_Reach`).**
   - **Exit.** Where the whole deck width leaves the lower road's actual paved roadbed, read at the exit with
     `Paved_Edges_At`: a ramp's effective edges, or a HIGHWAY_4's merge/diverge `E(x)`. Then
     `Hybrid_Overpass_Deck_Allowance` = outer blend + `L_pin` is added.
   - **The deck width counts.** The exit is taken at the deck edges rather than the centerline, as the old formula's
     `w_deck * cot` term did. On a square crossing the two are the same.
   - **Span.** The registered kind must span the whole extended deck.
   - **Phase 9.** `d_Lov` is `E_hwy + allowance + L_ramp_plan + l_arm`, so Phase 9's DIAMOND geometry and Phase 11's
     deck agree.
6. **Support footprint.**
   - **Across.** Measured on each side, looking into the deck: `max(deck half width, the attached road's paved edge)`
     (`Hybrid_Bridge_End_Support_Extents`).
   - **Along.** 1.5 cells along the deck.
   - **On the grid.** A vertex belongs when one of its cells overlaps that footprint and it is on the paved roadbed's
     whole-cell reach.
   - **Claims.** Every water vertex graded there is a `BRIDGE_END_SUPPORT` claim of that assembly.

### Tests (controlled worlds, Phases 10-12 on one native 3.75 grid; `terrain_hybridrealization`)

- **Local junction needs a landing:** a 1.5 m mound on `L_s` compiles one landing at `z_L` over 5.303. The bare plan
  misses the pin, every road through `L_s` holds `z_L` within `L_pin`, and Section 12.3 is green.
- **Exact gores:** no landing, and no judgement issue on them.
- **Bridge endpoints need landings:** the river bridge's two deck ends are landed on the ground road outside a deck
  that is exactly as compiled, and the bare plan misses both pins. Section 12.3 is green.
- **Impossible landings:**
  - **Bridge (falling bank, fill 4.0).** Every one of 64 stations is refused for `pin_landing`, then
    `obligation_lost`, in Phase 11. The same world compiles the bridge with no grid.
  - **Local junction (the mound, grid shifted a quarter cell).** The interchange is refused in Phase 11.
- **DIAMOND overpass:** the deck reaches `E_hwy + blend + L_pin` = 22.19 each side (44.380). Both abutments are
  landed and Section 12.3 is green. With every kind's span cut below the extended deck, Phase 11 refuses the
  interchange.
- **Support footprint:** the bridge-end support keeps the attached asymmetric paved roadbed (3.0 left) wider than the
  deck (1.5). It leaves the river beyond the roadbed and under its blend, and the span, untouched, with 22 claims all
  the assembly's.

Generator identity stays 16 (no released native world).

### Census r12 (1400 worlds, native 3.75 grid in Phases 9 and 11), against r11

**448 / 98 / 0** (r11: 448 / 98 / 34). Phase 9 and Phase 10 are unchanged. All 34 r11-solved worlds now fail
Phase 11:

| Count | Cause |
| --- | --- |
| 18 | Phase 12's own profile plan refuses them (`forward_propagation_empty`) |
| 9 | A topology/incidental junction whose pin still fails with its landing (`pin_landing`) |
| 3 | Ordinary roadbed vehicle grade on the grid (0.142, light envelope) |
| 3 | An overpass whose extended deck no longer fits its straight run (`turn_too_tight`) |
| 1 | An incidental crossing |

**Isolation** (same 34 worlds, r11's Phase 11 without the grid judgement, then Phase 12's plan):
- 31 still solve Phase 11. The 3 lost are the extended overpass decks.
- Phase 12's plan, unchanged since `863d6553`, refuses 18 of those 31: 15 at any cell size, 3 only with the native
  1.5-cell shared-surface margin.

Those 18 worlds would have failed Phase 12 in-game before D-027. D-027 only moves the failure into Phase 11, so
Phase 12 no longer meets it first.

**Cause of the 18, found here.** D-025 reading 2 holds a later road at an earlier road's surface wherever its centerline
runs over that road's roadbed plus 1.5 terrain cells. That applies at any roadbed overlap, not only at compiled
junctions. A road passing near, alongside or between other roads is pinned to their surfaces, metres apart: 3.2 m
within 8.3 m, 12.6 m within 36.5 m, past every class grade. Phase 11 imposes none of this, so Phase 12 is stricter than
Phase 11. Excluding earlier roads' deck and bore samples changes nothing (still 13 of 31 plannable). Plan 12.2 step 5
speaks of "a road-road junction['s] ... already compiled shared junction surface", which the overlap rule exceeds.
Recorded for the user's direction; not changed here.

**Status: implemented and tested. The census shift is reported, not tuned.**


## D-026 - Hybrid rewrite: Section 12.3 post-realization verification on the runtime objects; the realized grid (2026-10-04)

**Directive (user, 2026-10-03).** Verify the runtime objects exactly:
- **Swept/collision query** at `min(W/2, A/3)`: light on every road, heavy on heavy-capable routes, Harvester on economy
  routes. It covers ordinary roads, ramps, parallel intervals, local junctions, widened highway sections, bridge
  approaches and decks, and overpasses.
- **Every `BridgeAssembly`** against its compiled span, clear width, deck elevation, attachment at both ends and
  vehicle clearance.

A discrepancy fails the attempt through the existing teardown. Nothing is repaired after realization.

**Implementation readings** (no constraint weakened):
1. **The canonical swept query** (`HybridRoadRealizer::Verify_Physical`). Roads carry no collision; vehicles drive on
   the terrain and on decks. The query therefore reads exactly those:
   - `WorldTerrainSystem` heights;
   - `BridgeSystem::Conform_Point` decks and their definition's deck thickness;
   - each bridge's support and decoration sections as posts;
   - the runtime `RoadSystem` roads' paved edges (carriageway, shoulder and edge profile).

   At every plan sample, the envelope's 3 x 3 footprint (its length along the runtime road's own frame, its width
   across it) must:
   - stand on paved runtime surface;
   - stand on the solved surface within the realization tolerance (reading 3);
   - stay within the class grade and cross slope;
   - stand under no deck closer than its height plus Section 12.3's safety;
   - meet no post.
2. **Placement and ownership.**
   - The envelope stands on the roadbed midpoint. Where that roadbed alone is narrower than the envelope, it stands
     flush with either paved edge. This is the ramp's 2W gore transition, where D-024 lets the combined highway and
     auxiliary-lane surface carry it.
   - Where roadbeds overlap (a junction or a gore), the surface is the earliest road's, as solved (12.2 step 5).
3. **The realization tolerance.** The ground is bilinear between vertices that hold the solved surfaces. Between
   them it may depart from a surface by that surface's class slope over one cell diagonal:
   `2Q + tan(class max grade) * cell * sqrt(2)`. This is the case at a grade break inside a cell, such as a deck end
   or a junction's level patch, and where two roads' surfaces meet. The tilt this puts under a vehicle must still keep
   the class grade and cross slope. Measured on the native-cell (3.75) controlled worlds:
   - 0.196 at a river-bridge deck end (the attachment point);
   - 0.1385 at the heavy interchange's local junction point;
   - 0.687 at a heavy envelope's corner on a roadbed next to that junction;
   - no envelope tilted past its class limits.

   Superseded for structural points by the 2026-10-04 ruling below.
4. **Bridges.** Each runtime bridge is checked against both its assembly and the compiled record:
   - span against the compiled `DeckReachA + DeckReachB`;
   - every deck section at least Section 12.4's clear width for the carried road's vehicles;
   - the deck surface along its whole length and width at the compiled elevation (2Q);
   - the deck bottom over a river no lower than compiled, and an overpass's clear gap over the realized road beneath
     (and that road at its compiled `LowerSurfaceZ`);
   - a runtime road bound to each end, stopping on the deck end at its surface;
   - traversable, and the definition's own stated clearance.
5. **The ramp list (12.6.13), from the realized records and the runtime objects.**
   - **Ends.** The runtime ramp lies on its realized centerline (2Q). Its `L_s` end is an arm of a canonical
     `RoadSystem` junction whose ground stands at `z_L`. Its gore end is `ROAD_ENDPOINT_ATTACHED` to the realized
     highway, at `d_aux` and at the highway's surface.
   - **Geometry at the gore.** The tangent is within 0.001 rad, measured on the realized centerline's arc samples as
     Phase 10 measures it. The cross section is inner 2.375W and outer 3.825W, with no inner shoulder and the 0.45W
     outer one.
   - **Plan form.** There is no turn under `R_r`. The convergence leg is 15 degrees, with its straight through the
     midpoint.
   - **Merge/diverge.** The record's lengths and subdivisions hold, and every sample matches the Q-026 section. The
     runtime highway's ramp-side carriageway and roadbed equal the record over the whole interval.
   - **Junctions and capabilities.** No `RoadSystem` junction has the ramp as an arm except at `L_s`. The ramp carries
     its capabilities.
6. **Live gate.** The manager runs the physical half both after realization and in the live gate before road
   geometry is built. Terrain changed by any later stage therefore fails the attempt.

**Defects found at native resolution and fixed at the owner (`Hybrid_Deform_Terrain`):**
- **Whole cells.** The deformation graded only vertices within the paved edge. With 3.75 cells as wide as a ramp's
  roadbed, the bilinear ground under the paved width took blend vertices, which tilted a ramp's light envelope 12.5
  degrees (class 8). A vertex now holds the road when any of its four cells reaches the paved roadbed
  (`distance <= edge + cell * (|n.x| + |n.y|)`). Those vertices are the road's own and protected from later roads.
  The smoothstep still runs from the paved edge.
- **Abutment seat over water.** A bank inside the cell of a deck end left the seat vertex in water, so it was
  skipped, and the road's last 1.4 m sagged 3.2 m to the river bed. An abutment seat's own roadbed vertices (at most
  1.5 cells under the deck end) are now graded even over water. The span, and the channel beside the seat's
  roadbed, stay untouched.

A mutation that turns whole cells off fails Section 12.3 on both native-cell worlds.

**Status (2026-10-04):** `ef679952` accepted. Readings 2 and 3 are amended below by the user's ruling.

### Ruling (user, 2026-10-04)

**Bridge-end terrain: accepted, as structural support grading, not road deformation into water.** The water-skip
rule yields only under a bridge end's support footprint:
- it reaches at most 1.5 native terrain cells along the deck;
- it never touches the open river under the unsupported span, the water beside the approach, the water surface or
  the WaterSystem polygon;
- every vertex it changes is recorded as `BRIDGE_END_SUPPORT` terrain owned by that `BridgeAssembly`.

**Height tolerance: split.**
- **Ordinary road surface.** `2Q + tan(max_grade) * cell * sqrt(2)` applies only to ordinary roadbed between
  structural points, and only while grade, cross slope, the vehicle sweep and continuous paving still pass.
- **Structural pins.** At every frozen point the authoritative drivable surface matches the plan within `2Q`, never
  relaxed by the terrain cell. The pins are:
  - local junctions;
  - ramp gores;
  - bridge deck endpoints;
  - bridge/road attachments;
  - overpass fixed crossing elevations;
  - shared junction surfaces.
- Terrain beneath a bridge end may differ from the deck. The deck itself must match its compiled profile tightly.
- The 0.69 at the heavy interchange's local junction is not accepted merely because the vehicle's grade and cross
  slope pass.

### Implementation (working tree, uncommitted pending Q-027)

1. **The authoritative drivable surface.**
   - Off a deck it is the terrain. `RoadSystem` conforms every road point to the terrain (`Conform_Road`, no grading),
     and road meshes carry no collision ("the ground under it already does").
   - There is no exact `RoadSystem` surface over hidden terrain, so the second resolution option does not apply.
   - At a deck end it is the deck, and the attached road ends on it.
2. **Pins (`HybridRealizationPlan::Pins`, in the stage checksum).** Every compiled at-grade junction is a pin, at the
   compiled junction point:
   - a local junction at `z_L`;
   - a gore at the realized highway's surface;
   - any other junction at the surface of the earliest road through it.

   Every overpass crossing is a pin at its lower road's `LowerSurfaceZ`, and every deck end at its compiled height.
3. **Pins realized.** The terrain is two triangles per cell (`HeightfieldClass::Sample_Cell`, mirrored exactly by
   `Hybrid_Grid_Height_At`). Each pin moves the three vertices of its triangle by the least change that hits it,
   repeated over pins sharing vertices (Kaczmarz, at most 64 passes). Water moves only inside a deck end's own
   support footprint, and nothing moves under a bridge's span.
4. **The support footprint (reading, for review in Q-027).**
   - **Across.** As wide as the abutment: the wider of the deck and the attached road's paved roadbed there, since
     the abutment carries the whole road.
   - **Along.** 1.5 cells along the deck.
   - **On the grid.** A vertex belongs when one of its four cells overlaps that rectangle.

   With the deck's clear width alone, the attached shoulder beside water sagged 1.43.

   Claims are checked by `Verify_Physical`. In the hand-made fixture they are 22 vertices, all owned by the deck.
5. **Verification.** `Verify_Physical` checks every pin at `2Q`:
   - the terrain at the point;
   - the deck at a deck end;
   - every runtime road end there.

   The local-junction, gore, attachment and overpass checks in the ramp and bridge verifiers are `2Q` as well. The
   coarse tolerance remains only for ordinary roadbed.

### Measured on native 3.75 cells (Q-027)

- **Before correction**, the terrain misses the pins by:
  - `L_s`: 0.1385;
  - river-bridge deck ends: 0.196 and 0.180;
  - gores: 0.
- **On a 0.15 field** the misses are 0.0017 and 0.048. The misses are the grid's.
- **After correction**, every pin is exact, but the vehicle grade beside two of them exceeds the class limit:
  - ramp out of `L_s`: 0.2132;
  - bridge approach: 0.2247;
  - limit: 0.2126.

  Both roads climb at the class maximum into the grade break.
- **Grid-support landings** (±cell·√2) fix the bridge. Applied in Phase 12 only, they refuse the heavy `SIDE_ACCESS`
  world.
- **Overpasses (found here, already in committed code).** Overpass deck ends stand on the lower road's paved edge,
  because Phase 11 sizes the deck to the lower roadbed exactly. The DIAMOND deck end is 5.15 / 5.414 above the
  terrain (3.75 / 0.25 cells), and the record-half `Verify` fails at both resolutions.

**Status: accepted with the 2026-10-04 ruling; the grid conflicts it left are resolved by D-027 (Q-027).**



## D-025 - Hybrid rewrite: Phase 12 goes live on the canonical owners; tunnels deferred; migration adapters (2026-10-03)

**Decision (user, 2026-10-03).**
- **Phase 12 ownership.** On the native hybrid path, Phase 12 is the sole runtime realization owner of roads,
  bridge/overpass decks (through `BridgeAssembly`), junction surfaces and accepted road cut/fill. The old hybrid-path
  road and bridge generators neither instantiate, deform, reroute, repair nor duplicate them. Authored/legacy
  generation is unchanged.
- **Adapters.** Downstream systems read Phase 12's records through temporary read-only migration adapters, which may
  translate records and ids but never alter geometry or create runtime objects. Phase-12 records stay authoritative.
- **Tunnels deferred.** No Phase-12 tunnel realization, no tunnel adapter, and tunnels are not part of the Phase-12
  gate. Hybrid planning behaves as `tunnel_available = false`.
- **Bridges.** A bridge is `CompiledBridge -> BridgeAssembly -> BridgePiece[]`, with one `WHOLE` piece in this
  version.

**Implementation readings** (no constraint weakened):
1. **Tunnels.** `Hybrid_Native_Tunnels_Available` returns false for Phase 6's window flags, Phase 9's window method
   choice and Phase 11's tunnel fallback. The census pipeline mirrors it. Phase 11's own tunnel tests keep their
   explicit fixtures.
2. **The shared junction patch, processed once under the earlier road (12.2 step 5).** *Superseded by D-028:* the
   shared surface applies only within a compiled Phase-11 junction's arm reach, between roads carrying that record.
   As first read here, it applied wherever a later road's centerline ran over an earlier road's roadbed (plus 1.5
   terrain cells), and that pinned nearby and parallel roads Phase 11 never judged held. The deformation keeps the
   earlier roadbed.

   A level patch at a fixed reach was tried first and rejected. It made Phase 12 stricter than Phase 11: a
   `DIAMOND_ACCESS` local junction sits on the crossing's climb to its deck and cannot be level. The earlier-road rule
   is the plan's own wording and needs no new geometry.
3. **Ramp gore pin.** It is the realized highway's surface at the gore, which is 12.6.10's "highway profile at the gore
   station". It equals the compiled `JunctionZ` whenever Phase 12 grades the ground Phase 11 compiled on. `z_L` stays
   the compiled value at every road through `L_s`.
4. **Abutment seats.** One and a half grid cells inside each deck end are graded with the road, so the road's last
   point stands on the deck end's own height. The rest of the span stays untouched (12.2 step 4).
5. **Adapter graph.** A new `STRATEGIC_NODE_ROUTE_POINT` type carries road nodes with no gameplay role. River/canyon
   bridges become bank-to-bank deck edges, so navigation's bridge portals stand at the two banks. Overpass decks are
   Phase-12 objects but are not published as bridges. The base pair is Phase 8's frozen one
   (`Adopt_Selection`); no second selection algorithm runs.
6. **Gore attachment.** A ramp's gore end is `ROAD_ENDPOINT_ATTACHED` to its highway road, a new canonical endpoint type
   that RoadSystem's junction discovery skips. The gore lies exactly at RoadSystem's tee reach (2.875W).

**Defects fixed at their owners:**
- **Section 12.1 sampler.** It placed whole multiples of the spacing, then the exact end. A structure end landing just
  past a multiple left a 0.0166 step whose quantized heights read as a 0.24 rad grade, refusing feasible profiles in
  Phase 11's crossing judgements as well as Phase 12. It now uses the fewest equal steps within the spacing.
- **Deformation distance.** The roadbed test measured distance to an end segment's infinite line, grading a band
  behind a road's end. It now uses the true distance.

**Census r11** (1400 worlds; tunnels off and the sampler fix together, against r10): 448 / 98 / 34, unchanged in
total, with the solved set moved.

| World | r10 | r11 |
| --- | --- | --- |
| archetype 3 seed 106 | failed | solved |
| archetype 3 seed 110 | failed | solved |
| archetype 1 seed 24 | solved | fails on a `PRIMARY_2 x HIGHWAY_4` crossing |
| archetype 5 seed 182 | solved | fails a bridge sweep cross-slope after reroutes |

Each change was isolated on these four worlds:
- **The tunnel switch** changes none of them.
- **The sampler fix** accounts for all four:
  - Archetype 3 seeds 106 and 110 now pass grading where the false sliver grade refused them.
  - Archetype 1 seed 24 and archetype 5 seed 182 now accept a different crossing station first, which a later
    Phase-11 check (overpass turn, bridge sweep) refuses.

The second pair is Phase 11's existing station/sweep order, not a sampler defect.

**Status: accepted (user, 2026-10-03).**



## D-024 - Hybrid rewrite: Phase-11 interchange compilation readings; the effective ramp roadbed (2026-10-03)

**Context.** Plan 12.6.10 is implemented as written. Three of its terms need a concrete reading. These are the
readings taken; none weakens a constraint.

1. **Gore reserve, laterally.** "Covers the highway to `E_aux`" is read as the whole highway cross section on that
   side of the chain: from the far roadbed edge `-E_hwy` to `E_aux`, over `Hybrid_Gore_Reserve_Extent` along the side
   frame. It also covers the independent ramp's roadbed over its `2W` gore shoulder transition, using the ramp's own
   edge offsets. A structure on the highway therefore cannot stand inside a gore reserve on either carriageway.
2. **Merge/diverge sample frame.** A `HybridMergeDiverge` sample's station is in the same frame as the frozen
   `HybridRampInterval`: chain stations relative to `X`, with the highway point at `X + a * station`. `d`, `h`
   and `E` are the 12.6.7/12.6.11 values there, and `z` is the highway's Section-12.1 surface. The merge/diverge
   record's `JunctionZ` is that surface at the gore, and a local junction's is `z_L`.
3. **Ramp swept proxy.** Superseded by the resolution below.

**Resolution (user, 2026-10-03).** Items 1 and 2 are accepted.

Item 3 changes: a vehicle a ramp's capabilities require must fit inside the actual drivable paved roadbed. Its
outside edge standing on generic placeable terrain never satisfies ramp validation.

- **Cross section.** The nominal `HIGHWAY_RAMP` stays `1.50W`. Its effective independent roadbed is
  `max(1.50W, RequiredSweptVehicleWidth)`, widened equally either side of `RampLaneCenterline` by
  `E = max(0, (RequiredSweptVehicleWidth - 1.50W) / 2)`. The inner edge is `-(0.70W + E)` and the outer edge
  `+(0.80W + E)`.
- **Light-only ramps.** `E = 0`, and the Q-026/D-022 geometry is unchanged. Harvester ramps also have `E = 0`
  (`4.5142 < 1.50W`).
- **Heavy ramps.** The measured heavy reference is `6.801906 = 2.0470W`, so `E = 0.2735W` (`0.9090`).
- **Unchanged.** The lane centre, `R_r = 5W`, the gore, `d_aux = 2.875W` and the `16W`/`12W` intervals do not move.
- **At the gore,** the required vehicle is validated against the combined `HIGHWAY_4` + auxiliary-lane paved
  surface on the ramp side, not the nominal ramp strip.

Implementation:
- Phase 9 computes `E` from the ramp's capabilities, which are LIGHT plus every access-side arrival's, using the
  preflight vehicle widths now in `HybridTopologyParams`. It freezes `E` on each ramp edge
  (`HybridTopologyEdge::RampRoadbedExtra`, topology checksum) and judges test 5's ramp edges and the local-junction
  disc on the effective roadbed. `Check_Interchanges` verifies that the frozen `E` matches the capabilities.
- Phase 10 copies `E` to the ramp route (route checksum) and re-validates on the effective edges. Later roads keep
  their Q-025 clearance from the effective edge.
- Phase 11 uses `Hybrid_Edge_Roadbed_Width` wherever an edge's roadbed matters: junction arms, at-grade surfaces,
  overpass decks.
- **The ramp fit.** At every ramp sample, the vehicle must fit the paved surface:
  - the ramp's effective roadbed;
  - or, where the ramp's inner edge has joined the highway's ramp-side roadbed (the gore shoulder transition and the
    gore), the combined surface from the highway's median edge to the ramp's outer edge.

  Over each parallel interval it must fit the combined ramp-side surface `E(x) - median/2`.
- **Gore transition.** `E` blends out over D-022's `2W` gore shoulder transition with the same `f`, so the edges
  reach the combined surface's `2.375W`/`3.825W` exactly at the gore and no width step exists there
  (`Hybrid_Ramp_Effective_Edges`).

Two defects surfaced and were fixed at their owner (Phase 10):
- **OFF_RAMP edges swapped.** Its samples are stored `L_s -> gore`, against its traffic, so its `0.70W`/`0.80W` edges
  had been applied on the wrong sides. The inner edge is now left of the ramp's traffic, as in Phase 9 and D-022.
- **Own-highway allowance.** It was a fixed lateral bound (`DSep`) on the nominal roadbed. It is now the converged
  gore wedge itself: the ramp may touch its own highway's roadbed only over the run of samples from the gore in which
  its effective inner edge is still joined to it, and never again after separating.

**Status: accepted.**



## D-023 - Hybrid rewrite: no arrival-angle prefilter on highway interchanges; local-junction reachability (2026-10-03)

**Decision (user, 2026-10-03).** The step-15 rule that rejected an anchor when any arrival lay within 30 degrees of
the `HIGHWAY_4` axis is removed. Arrival direction does not decide interchange legality; the geometry that must be
driven does.

`SIDE_ACCESS` imposes no minimum arrival-vs-highway angle. It requires only:
- that the arrival can reach its local junction;
- turn room at the local junction;
- that the shared 5W ramp constructor reaches the gore;
- every 12.6.8 exclusion.

For `DIAMOND_ACCESS` the angle rules apply to the crossing edge itself:
- `60..120` degrees when the crossing is grade-separated (the step-12 overpass tests, already on the crossing
  segment);
- `30..150` degrees when it is at grade (now an explicit check in `At_Grade_Crossing_Is_Feasible`).

Each arrival still has to reach its local junction.

**Plan 12.6.4 step 3 and 12.6.8 test 7, as implemented.** Side is the sign of `cross(a, delta)`, ties to `RIGHT`.
Test 7 keeps turn room: arms at least 30 degrees apart.

Test 7's 60-degree stem cone was the same arrival-angle prefilter measured from `L_s`, so it is replaced by
reachability:
- the stem must leave `L_s` outside the footprint wedge between the `OFF` and `ON` ramp arms, which Phase 10 makes
  hard-infinite;
- the arrival's intent leg from `L_s` must not cross either ramp (beyond the shared arms within `l_arm`), the
  highway's straight extent or the diamond crossing.

A nearly parallel feeder is legal when that geometry works. No 12.6.8 constant changed.

**Census** (1400 worlds, identity 16, the same seed set):

| | value |
| --- | --- |
| worlds with anchors | 124 |
| anchors: `SIDE_ACCESS` | 112 |
| anchors: `DIAMOND_ACCESS` | 12 |
| accepted `SIDE_ACCESS` | 3 (archetype 5 seeds 16 and 199, archetype 3 seed 129), each at `k = 2` |
| accepted `DIAMOND_ACCESS` | 0 |
| frozen checksum vs rebuilt | identical in every world |
| anchor rejections | 121, all `no_candidate` |

Candidate refusals by 12.6.8 test, over all 32 candidates of every anchor:

| test | refusals |
| --- | --- |
| 1, straight extent and pose | 2605 |
| 2, structure reserves | 81 |
| 5, exclusions | 984 |
| 7, local junction | 208 |
| 3, 4 and 6 | 0 |

The natural-world gate holds: three generated worlds freeze a `HIGHWAY_INTERCHANGE`, and it survives the full
Phase-9 gate and checksum rebuild. The census line now also splits test 1 into chain reach, pose before X and pose
after X, and records each anchor's chain turn and the neighbour distances along the axis.

Phase 10 does not yet construct ramps, so archetype 5 seed 16, which reached Phase 11 before, now fails Phase 10
on its ramp edge. The Phase-10 count is 97 instead of 98. The next work item, Plan 12.6.9, removes that failure.

**Status: accepted.**



## D-022 - Hybrid rewrite Q-026: merge/diverge is an explicit auxiliary lane; generator 16 (2026-10-03)

**Decision (user, 2026-10-03, option (b)).** An entrance or exit ramp becomes a real additional highway lane
before it merges or after it diverges. Sliding the whole `1.50W` independent ramp from `3.575W` to the outer lane
centre made a long diagonal roadbed, not an acceleration or deceleration lane. The single smoothstep over the whole
`16W` / `12W` interval in Plan Sections 12.6.7 and 12.6.11 is superseded.

**Geometry (Plan 12.6.6, 12.6.7, 12.6.11, 12.6.13, 12.6.15).**
- `ON_RAMP`: independent `HIGHWAY_RAMP` -> gore -> `10W` full-width auxiliary lane -> `6W` merge taper -> normal
  `HIGHWAY_4`.
- `OFF_RAMP`: normal `HIGHWAY_4` -> `6W` diverge taper -> `6W` full-width auxiliary lane -> gore -> independent
  ramp.

| | value |
| --- | --- |
| auxiliary-lane centre `d_aux` | `d_join + W = 2.875W` |
| ramp-side roadbed edge `E_aux` | `E_hwy + W = 3.825W` |
| highway width | `5.65W -> 6.65W` |

The highway centerline and median never move. The taper uses `f = 3u^2 - 2u^3`, `u = clamp(s/6W, 0, 1)`:
- merge: `d = 2.875W - W f`, `E = 3.825W - W f`;
- diverge: `d = 1.875W + W f`, `E = 2.825W + W f`.

Throughout, `h = d + 0.5W` and `E = d + 0.95W`.

**The gore.** The gore is at `d_aux`. The independent ramp finishes its lateral transition before it, and its
shoulders blend into the widened roadbed over the `2W` gore shoulder-transition interval (`GoreReserveMargin`).
`d_sep = 3.575W` and `4.325W` are not gore or highway cross-section values.

**Construction constants.** The construction keeps its constants, with `d_aux` in place of `d_sep`:
- `d_Lmin(k) = (8.3926380 + 0.2588190k)W`;
- `SIDE G(k) = (9.0029197 + 0.9659258k)W`, unchanged.

**Recorded alongside.** The new constants (`TaperLength`, `MergeFullLength`, `DivergeFullLength`, `d_aux`, `h_aux`,
`E_aux`), the subinterval fields, the gore reserves' lateral extent, the Phase-11 record samples, the Phase-12 gore
attachment, the Phase-19A check, and the checksum fields.

**Revision (user, 2026-10-03): the ramp reference is its travel lane.** For `HIGHWAY_RAMP` only, the authoritative
spline is `RampLaneCenterline`, the centre of the `W` travel lane. It is used for turn radius, routing, gore
alignment, merge/diverge offsets, grade sampling and the swept vehicle path, and it is never shifted by the unequal
shoulders.

Relative to it the independent roadbed spans `-0.70W` to `+0.80W`, so the derived `RoadbedMidpointOffset` is
`+0.05W`: `2.925W` when the lane is at `2.875W`. The midpoint is never a routing centerline.

The `2W` gore shoulder transition changes only the roadbed edges: `inner_shoulder = 0.20W(1-f)` and
`outer_shoulder = 0.30W + 0.15Wf`, with the edges taken from the fixed lane centre.

The gore invariant:
- lane `2.875W`, tangent parallel;
- edges `2.375W` / `3.825W`;
- shoulders `0` / `0.45W`;
- continuous XY and Z.

Every other class keeps its centerline semantics.

**The `d_Lmin` value.** The instruction also gave `d_Lmin(0) = 8.4426380W` with `G(k)` unchanged. That figure came
from my earlier message, which wrongly assumed the construction still referenced the roadbed midpoint. With
12.6.7's construction on `RampLaneCenterline` and the gore at the lane offset `2.875W`:
- `d_Lmin(0) = 2.875 + l_conv(0) sin 15 + (l_arm + T30) sin 45 = 8.3926380W`;
- `G(0) = 9.0029197W`, unchanged.

Forcing `8.4426380W` with the lane still at `2.875W` would lengthen the `L_s -> V1` leg and move `G(k)` by `+0.05W`,
contradicting "keep G(k) unchanged". The plan and code use `8.3926380W`; the local junction `L_s` is a
`RampLaneCenterline` point.

**Generator identity 16.** It is bumped in the first ramp implementation commit and is the first identity containing
the finalized post-stabilization behaviour plus the `HIGHWAY_RAMP` records. Intermediate identity-15 development
commits after `5384130c` (notably `48f7d5ab`) changed generated output without a bump. They are not compatibility
fixtures, and no version number is spent repairing them.

**Status: accepted.** `Q-026` closes on this record.



## D-021 - Hybrid rewrite: ordinary guides are strategic anchors; structural-transition defects fixed; generator 15 (2026-10-02)

**Decision (user, 2026-10-02).** An ordinary route-family guide is a strategic routing anchor. The Phase-10 A* chain
must pass through the guide's analysis cell (the gate checks it on the chain). The final continuous class-radius
centerline is not required to intersect that cell, and a valid road is never distorted to drag its curve back
through it. Structural waypoints stay exact: crossing obligation, ramp pass window, overpass obligation, base
entrance or field access point where the corridor ends exactly, and the future highway-ramp gore, merge and
diverge points keep their position, axis, straight approach and departure reserve, and turn-radius transition.

**Ordinary guides in the corner construction.** The centerline-in-cell check is gone. A guide still gives the
corridor polyline a corner (`RouteVertex::Guide`); at a trunk joint it stands on the waypoint's own frozen
position. It is held against ordinary corridor corners but dropped when a structure's lead needs its room. Phase
9's Q-023 connector (`Hybrid_Road_Poses_Connect`) builds the same corner. Both alternatives were measured over the
1400 worlds and refused:
- *Held absolutely* (the old pinned corner): it refused crossing lead-ins the terrain and the Q-023 connector both
  allow. This was a genuine Q-023 / Phase-10 mismatch: Phase 9 judged the logical edges' ends as free poses, and
  Q-025 made them fixed interior corners.
- *Released entirely* (plain chain corners): the corner fell back to chain cell centres inside the field reserves
  guides stand beside, and fillet crowding dropped it. Phase 10's yield fell from 93 to 54.

**Logical slicing.** The boundary between two logical edges at an ordinary guide is found in two steps:
1. Take the centerline sample nearest the centre of the guide's analysis cell, searching from the previous
   boundary, by quantized distance, ties to the earlier sample.
2. Take the exact closest point on the segment either side of that sample, clamped and quantized to `Q`.

A boundary inside a structural interval moves to that interval's nearer end. Boundaries are strictly ordered and
no logical edge has zero length. The boundary point is shared by both slices and lies on the polyline, so slicing
never changes the road. The first version had an off-by-one in the slice's structural-span indices, which failed
Phase 10's crossing-alignment gate on about 51 worlds. It was found by the census and fixed before anything was
recorded.

**Structural-transition census.** Every refusal is classified by exact cause (`HybridStructuralCause`):
- `lead_before_approach` and `departure_after_structure`: the searched chain leaves the lead or departure no room
  while the ideal transition is legal;
- `wrong_side`;
- hard obstacle, envelope, grade or cross slope on the ideal `2R` straight transition;
- `sequence_too_close`;
- `q023_phase10_mismatch`: Q-023's empty-plane connector refuses what Phase 9 accepted;
- `unplanned_crossing`;
- `far_bank_approach`.

Classifying the 91 refusals at `e99f8654` exposed five more Phase-10 implementation defects. Each is fixed at its
owner:
1. *Rivers between cell centres.* Rivers are 2.5 to 6 units wide and analysis cells 8 to 16, and the hard-infinite
   cell list judges cells by their centres. So the search stepped across rivers between two dry centres, and the
   centerline's point-and-cell test let the road follow. Search moves (`Move_Crosses_Channel`, cached per cell and
   direction) and centerline steps (`Segment_Meets_River`) are now judged where they meet a river. Phase 11 never
   saw these crossings, so those roads would have driven through water with no structure. This fix accounts for
   most of the drop between some intermediate censuses.
2. *Own planned window re-crossed.* Its obstruction was open to every leg of the road. A leg could reach the near
   bank from the far one by crossing the same window outside its frozen interval: an unrecorded crossing, refused
   later as a frozen corner. The window is now closed to the legs that approach or leave it
   (`RouteBuilder::ClosedWindow`).
3. *Unplanned crossings never aligned.* The plan allows a crossing at a window the road was not sent to, at the
   `20A` window penalty, but nothing lined the road up with it. When the centerline is refused at a corner that
   belongs to such a crossing, Phase 10 inserts the window's approach points as stops of that leg and searches the
   logical edge again in the same envelope. If the promoted crossing still cannot be lined up, its stops are
   removed and the road may not cross that window (`ForbiddenWindows`). Both repairs count against
   `retry.max_local_retries_per_stage` for that envelope.
4. *Departure room.* The leg leaving a crossing took the frozen straight as run behind it and could turn in the
   cell after the egress. `Departure_Fits_Crossing` now mirrors `Arrival_Fits_Crossing`.
5. *Stale lead decision.* Dropping a corridor vertex beside a frozen point now lays the leads again. Before, a
   point whose neighbour had stood on its axis kept "no lead needed" after that neighbour was dropped, and was
   refused as a frozen point left at a corner.

**Census** (`renegade -terrainselfcheck hybridinteractionsearch`, the unchanged 200 seeds x 7 archetypes):

| | `e99f8654` | ordinary guide only | final |
| --- | --- | --- | --- |
| pass Phase 9 | 448 | 448 | 448 |
| pass Phase 10 | 69 | 93 | 107 |
| pass Phase 11 | 24 | 30 | 35 |
| Phase-10 centerline refusals | 297 | 263 | 194 |
| structural transitions | 91 | 102 | 20 |
| reroutes requested / resolved / exhausted | 130 / 0 / 44 | 171 / 1 / 60 | 196 / 2 / 71 |

The ordinary-guide-only column is the guide change and slicing alone, before the five defects above were fixed. Its
structural count rose because roads that had stopped at the guide boundary now reached their crossings. No Phase-10
gate failure remains: the one world that hit the per-attempt A* expansion ceiling in an intermediate run passes.
The seed set is the same in every column; no fixture was replaced.

**The 55 former ordinary-guide refusals.** The `guide_boundary` category is gone, and so is the class. Their next
owning failure: 6 solved by Phase 11, 10 refused in Phase 11 (no solution), 3 refused in Phase 11 (obligation lost), 18 Phase-10
ground rules on the centerline, 13 no path (turn/approach pose frontier), 1 no path (Tiberium), 1 no path (river
outside a window), 1 structural obstacle, 1 `lead_before_approach` and 1 `unplanned_crossing`. None of the 55 now
fails on the guide itself.

**Structural transitions remaining.** 20, down from 91:
- 11 `departure_after_structure` and 2 `lead_before_approach`: the ideal straight transition is legal and Q-023's
  connector agrees, but the terrain-routed chain leaves the turn no room;
- 3 `unplanned_crossing`: the local repair budget is spent, or the window was refused and the alternative also
  fails;
- 2 `far_bank_approach`;
- 1 hard obstacle;
- 1 `sequence_too_close`.

None is `wrong_side`, envelope, grade, cross slope or `q023_phase10_mismatch`. They stay strict failures. No
repeatable Phase-9 / Phase-10 rule conflict remains, so no new question is filed.

**Not changed.** The river-bridge quota stays mandatory. An open stabilization item records why valid Phase-6
bridge windows so often have no heavy node on the opposite bank. In the census, 3095 cross-link candidates were
refused `no_bank_pair` and 2839 fallback candidates lay outside every family corridor. The item is to find which
owner is responsible: Phase-3 intent placement, Phase-6 window placement, Phase-9 cross-link endpoint selection,
or the quota procedure itself. No rule changed.

**Generator identity 15.** It is recorded in the four shipped profiles, the self-check's mirrored profiles and
`NetworkingAndPersistence.md`.

**Tests.**
- `Check_Hybrid_Ordinary_Guide`:
  - a hemmed 135-degree turn at an ordinary guide is accepted with the chain in the cell and the centerline
    outside it;
  - the slices are contiguous, non-overlapping and cover the trunk, and the boundary is the nearest point;
  - the same geometry with a crossing obligation keeps the exact crossing pose.
- `Check_Hybrid_Structural_Poses` gains a guide beside a crossing's lead.
- `Check_Hybrid_Crossing_Discipline`:
  - own window closed, and narrow river between cell centres: both checked to fail without their fix;
  - departure room, and an unplanned crossing aligned and recorded once: positive checks only. Both pass without
    their fix too, because in open controlled terrain the search already turns late and meets the window on its
    axis. The census is what measures these two rules.

**Status: accepted.**



**Revision (user, 2026-10-03): accepted, with the repair bounded and authorized.** Checkpoint `5384130c` is
accepted. The plan now carries every rule the code depends on: Section 8 Phase 10 and Section 17 for the repair;
Phase 10 for segment-vs-river testing, symmetric transition room and the rebuild of stale structural leads.

`Phase10NaturalCrossingAlignmentRepair` was brought to the user's bounds:
- Each promotion and each withdrawal costs one local retry of the edge's envelope. The budget is
  `retry.max_local_retries_per_stage`, shared with the step-5 insertions; a rebuilt centerline receives only what
  is left.
- A withdrawn window is unavailable to that edge only for the rest of the envelope.
- Advancing the envelope removes the edge's promotions and unavailable windows and gives it a fresh budget.
- The repair acts only on a corner refusal that involves the crossing exactly. The nearest-structure reading is
  kept for the census alone.

Arrival and departure turn room are one measure (`Transition_Room`). The centerline step test is exactly "a river
only inside the road's own accepted crossing stretch". Before, it accepted any window's footprint.

That exact test exposed one more owner defect, now fixed: `Note_Unplanned_Crossings` decided whether a chain crossed
a window by comparing the run's two end cells about the window's centre. It missed crossings the chain made near a
footprint end, so the search allowed water crossings that were never recorded or built. It now asks the chain's
moves with the search's own segment test.

The two far-bank cases are now classified only when the chain actually meets the river.

Census on the unchanged seed set: 448 / 98 / 33. Structural transitions: 22, with no `q023_phase10_mismatch`. The
drop from `5384130c`'s 448 / 107 / 35 is the exact river rule. The 20 centerline river refusals are class-radius
arcs that touch a river beside a window, outside the road's own crossing. Under the looser test those roads would
have driven through water with no structure.

Generator identity stays 15 by instruction. These are corrections inside the same accepted stabilization, and the
HIGHWAY_RAMP bump is the next increment.



## D-020 - Hybrid rewrite Q-025: one physical trunk per heavy family; independent roads keep apart (2026-10-02)

**Decision (user, 2026-10-02, option (a)).** Each heavy route family is one continuous physical trunk road through
its ordered family waypoints. The logical topology edges are not collapsed. They stay authoritative for strategic
topology, `StableId`s, Phase-9 ownership, crossing-obligation ownership, reroute envelope ownership, fairness,
max-flow, diagnostics and checksums. Physical continuity and logical segmentation are separate.

**Phase 10 (`Route_Road`).** A family's trunk edges are routed in travel order, each with its own envelope,
Phase-11 floor and expansion budget. The heading a leg ends on is offered to the next across an ordinary family
waypoint, and a leg arriving at that waypoint is first asked to arrive toward the next edge's first stop, each
falling back to the existing free search. The chains are concatenated and the centerline is built once over the
whole family. An ordinary waypoint is a position-only pinned corner standing on its own frozen position; the
class-radius arc rounds it and must stay in its cell. *(Superseded by D-021: the guide is a strategic anchor whose
cell the chain passes; the centerline need not intersect it, and its corner yields to a structure's lead.)* Structural waypoints keep their exact Q-022 pose. A failure
is charged to the logical edge it happened on, which alone moves to its next envelope; a Phase-11 reroute raises
only that edge's floor and the whole trunk is laid again. Each logical edge's route is the exact slice of the one
centerline its interval covers. `HybridRouteSet::Trunks` holds the `HybridTrunkRoad`: family, class, capability,
logical edges and intervals, nodes, junction nodes, centerline, crossing intervals. No mixed-class trunk exists:
Phase 9 assigns one class per family.

**Independent roads.** The `-0.25d` reuse modifier is revoked (`reuse_modifier = 0`). One common rule replaces
hugging. A later-routed independent road may not run within 30 degrees of an existing road, closer than
`ParallelCenterlineClearance(A,B) = 0.5*RoadbedWidth(A) + 0.5*RoadbedWidth(B) + fairness.min_vehicle_lane_clearance`,
for a continuous `>6W`, outside the junction-owned interval `JunctionReach(class) = max(4W, R)` measured along each
road from a node the two share. `First_Parallel_Overlap` checks it exactly on the centerline; the search applies a
conservative version. No shared-road / merge semantics (option (b)) were implemented.

**Phase 11.** Every shared topology node is the topology's intended junction: one `NodeId` junction record, never
an incidental crossing, judged by the shared surface's reserve, cut/fill and cross-slope rules on every road arm
there. A heavy family's own consecutive edges are one road and need no junction. Grading is per physical road
(`Hybrid_Realized_Physical_Road`): a trunk is graded once by Section 12.1 with every structure on any of its edges.

**Owner defects found and fixed in the same pass.**
- *Approach reserves.* The approach-reserve test used the reserve's length as a radius around the whole straight
  and refused junctions beyond a bridge's approach. It is now the straight from `ApproachA` to `ApproachB` at the
  road's own roadbed width.
- *4W separation.* The gate's `4W` separation now treats solutions as related when they share a road, including
  every road through a junction's node.
- *Entrance corridors.* A base entrance corridor overlapping a frozen Tiberium reserve could not be reserved
  against it (step 6), so step 5 now skips it. Before: 148 of 512 corridors over 420 worlds overlapped. After: 0.
- *Centerline field exemption.* The centerline pass keeps its road-end field exemption.
- *Phase 6.* Approach legality uses the exact `BASE_PROTECTED` and Tiberium discs Phase 10 refuses, and approach
  turn room is the departure fan (straight on, or a quarter turn either way at the class radius).
- *Phase 9.* Preferred bridge windows bind to the family whose corridor made them preferred. Crossing obligations
  are no longer road endpoints.

**Tests.** `Check_Hybrid_Continuous_Trunk`, plus native trunk checks in `Check_Hybrid_Routes`, covering:
- one road through ordinary waypoints with no corner and no second road object;
- several guides in order;
- a branch at a trunk node compiled as one `NodeId` junction with no incidental record;
- a service road that may not ride the trunk past `JunctionReach`;
- a legal parallel road;
- the logical graph and max-flow unchanged;
- a single-edge reroute that moves only that edge's floor and lays the trunk again in one piece.

The structural-waypoint pose case is the existing Q-022 alignment worlds, run through the same router.

**Status: accepted.** `Q-025` closes on this record.



## D-019 - Hybrid rewrite Q-024: the Section-5 overpass number is a target; generator 14 (2026-10-02)

**Decision (user, 2026-10-02, option (a)).** Section 5's highway road-over-road overpass numbers (0 / 1 / 2) are
deterministic generation targets, not minimum quotas. Steps 12 and 13 enumerate every candidate and apply every
hard-valid rule before ranking: the `60..120` degree angle, the `10W` arms, the exclusions, `12W` spacing, slope,
Q-023 pose and the registered structure. Candidates are taken in order until the target is met or none remains.
Phase 9 freezes `RequestedOverpassTarget` and `ReservedOverpassCount`. `OVERPASS_TARGET_SHORTAGE` is a diagnostic,
not an attempt failure. The Phase-9 gate checks that every reserved overpass is hard-valid, pose-realizable and
shared by both edges, never more than the target. Phase 11 stays strict: every reserved overpass compiles or
follows the swap / reroute / fail order, and the quota check is `compiled reserved overpasses ==
ReservedOverpassCount`. Phase 19's quota check is written the same way in the plan. River and canyon/ravine bridge
quotas stay mandatory. A highway on-ramp or off-ramp is not an overpass.

**The rule Q-024 exposed.** The `60..120` degree test never refused anything: it compared the 0..90 line angle
against both ends of the directed band. Angle and arms were also measured on each edge's endpoint-to-endpoint
line instead of the intent sub-segment the road is driven along. Both are fixed; in the 1400-world census they are
now the dominant overpass refusals (natural: 309 angle, 65 arm; synthesized: 1171 angle, 519 arm).

**Generator identity 14.** `CURRENT_GENERATOR_VERSION` is 14: the four shipped profiles, the self-check's
mirrored profiles and `NetworkingAndPersistence.md`.

**The stabilization census** (`renegade -terrainselfcheck hybridinteractionsearch`, 200 seeds x 7 archetypes, 1400
attempts, final tree):

| | before (`0c8328fa`) | after |
| --- | --- | --- |
| pass Phase 9 | 172 | 448 |
| pass Phase 10 | 32 | 69 |
| pass Phase 11 | 6 | 24 |
| reroutes requested / resolved / exhausted | 77 / 0 / 26 | 130 / 0 / 44 |

What still fails, by owner:
- *Phase 9: 332 failures.* 225 bridge quota (river 182, canyon/ravine 43) and 107 obligation unbound. Of the river
  quota's refused candidates, 3095 have no heavy node on one bank and 2839 lie outside every family's corridor.
  Those windows are real but far from the families, and the plan's procedure cannot bind them.
- *Phase 10: 379 failures.* 66 no path, the frontier dominated by turn/approach pose. 297 centerline refusals,
  classified by geometric condition: 149 ground rules (Tiberium 49, grade 48, cross slope 21, others), 91
  structural transitions (a frozen crossing or overpass straight that the terrain-routed chain cannot line up
  with), 55 ordinary-guide boundaries (the class-radius arc at a trunk joint leaves the guide's cell because the
  family's intent turns sharply there) and 2 control/fillet constructions. None is a quantization or rounding
  artifact.
- *Phase 11: 45 failures.* 19 no solution (shallow incidental road x road crossings), 18 obligation lost (reserved
  overpasses whose roads cannot fit the ramps, or obligations that cannot cross) and 8 bridge sweep (cross slope).

**Status: accepted.** `Q-024` closes on this record.



## D-018 - Hybrid rewrite Q-023: Phase-9 structural bindings must be realizable road poses; generator 13 (2026-10-02)

**Decision (user, 2026-10-02, option (a)):** Phase-9 structural bindings must be geometrically realizable as
oriented road poses before the topology is frozen. Q-022 is not reverted and Phase 10's crossing-axis rules
are not weakened; the tests were not reseeded to find rare worlds that happen not to expose an invalid
topology. The owner was fixed, then the census re-measured.

**The ownership chain** this settles, each phase owning exactly one question:

| Phase | Owns |
| --- | --- |
| 6 | local crossing capability per road class: `HybridCrossingWindow::ApproachClasses`, one bit per class -- the class's own roadbed half width on legal ground over both straight approaches, and the class's own turn radius of room past each approach point. The existence test is unchanged (`PRIMARY_2`, approach only). |
| 9 | structural-pose realizability: which windows and overpass points an edge of a given class can drive, in order |
| 10 | the exact terrain route through them (Q-022) |
| 11 | the exact compile of each crossing on that route (Q-022) |
| 12 | realization only |

**Selection order** for every crossing candidate -- obligation, quota fill, synthesized cross-link:
capability bit -> corridor -> `>= 12W` -> wrong side -> pose -> ranking / `StableId`. An impossible candidate
is skipped and the next ranked one considered; the phase fails only when none remains, never because an
impossible window happened to rank first.

- **Binding class.** A heavy-family binding made before highway assignment is judged as `HIGHWAY_4`, because
  that family may still become a highway; a synthesized cross-link as `SECONDARY_2`; anything else as its
  actual class. A window is never discarded globally because a wider class cannot use it.
- **Wrong side.** With `A` the window's axis oriented from the previous structural point to the next,
  `dot(incoming, A) >= 0` and `dot(outgoing, A) >= 0`.
- **Pose.** A window is a `HybridRoadPose` (point, axis, `8W` straight either side); an overpass point is one
  with `max(10W, half deck)` either side, the half deck being `Hybrid_Overpass_Half_Deck`, the measure Phase 10
  keeps straight. Consecutive poses on an edge must connect in the empty plane by `Hybrid_Road_Poses_Connect`,
  which is Phase 10's own arcs-and-tangents corner construction at the class radius -- no terrain A*, no
  second geometry algorithm, no distance formula. Overpass obligations take the same pose rule on both edges
  on top of the unchanged `60..120` degrees, `10W`, exclusions, spacing and slope tests.
- `Check_Gate` re-checks every frozen chain, and `HybridTopologyResult` carries the attempt's census
  (`RejectedApproachClass`, `RejectedWrongSide`, `RejectedPose`, `RecoveredByNextRank`) whether or not the
  attempt succeeded.

**The reroute fixture.** No native seed in the 1400-world census leaves a Phase-11 refusal that the next
envelope then resolves: a wider envelope only helps where the narrower one forced a road onto a crossing it
would otherwise avoid, and Phase 9 no longer freezes those. As the decision allowed, the one-reroute fixture
is a **controlled owner-level world** (`Check_Controlled_Reroute`), not a claim that a native one exists: real
Phase 10 routes a `SECONDARY_2` across a river by its window, real Phase 11 refuses the crossing and names the
edge, the real owner raises the envelope floor, real Phase 10 reroutes round the river's end in envelope 1,
and real Phase 11 succeeds -- Phase 10 runs exactly twice and the accepted state reproduces its checksum. The
two-level (three-run) reroute and the late-edge byte-identity proof stay on native worlds.

**Generator identity 13.** Phases 9 to 11 and the 12.1 solver now build different topologies, roads and
crossings from the same seed: `CURRENT_GENERATOR_VERSION` is 13, the four shipped profiles and the
self-check's three mirrored representative profiles say `generator_version: 13`, and the versioning table in
`NetworkingAndPersistence.md` is current.

**The census, as the decision required it** (`renegade -terrainselfcheck hybridinteractionsearch`, 200 seeds x
7 archetypes, 1400 attempts, on the final tree):

| measure | count |
| --- | --- |
| Phase-9 candidates rejected for the approach-class bit | 7834 |
| Phase-9 candidates rejected on the wrong side | 6619 |
| Phase-9 candidates rejected for pose (spacing / turn) | 3322 |
| Phase-9 bindings recovered by a lower-ranked window | 1146 |
| worlds passing Phase 9 | 172 |
| worlds passing Phase 10 | 32 |
| worlds passing Phase 11 | 6 |
| reroute requests | 77 |
| reroutes resolved at envelope +1 | 0 |
| reroutes resolved at envelope +2 | 0 |
| reroute exhaustions | 26 |

Phase 9's realizability rule is doing the work it was asked to: 1146 bindings that would have frozen an
undrivable sequence went to a lower-ranked window instead. The yield is still low and is **reported, not
acted on** here: before Q-023 the same tool passed Phase 10 in 8 of 420 worlds (1.9%) and now in 32 of 1400
(2.3%); 6 worlds solve (0.4%). What stops the rest is upstream of this correction -- Phase-9 quota and
obligation refusals (overpass quota short, river bridge quota short, obligations with no realizable window),
Phase-10 hard-invalid ground and no-path refusals, and Phase-11 road x road crossings that no junction or
overpass can compile (`turn_too_tight`, `approach_does_not_fit`) on synthesized cross-links. Two worlds that
solved mid-way (`highland_pass` 24 and 171) are now refused, by the whole-road overpass grading below: their
overpass decks could not be climbed to by the road beyond the ramp feet. Seed choice for the gates: the
Phase-9 and Phase-10 walks keep their original seeds and add the census's first canyon seeds that pass
(18, 20, 23; 23, 98); the Phase-11 walk is re-seeded with the census's own Phase-11 worlds, solved and
refused, because the old eight seeds now build no Phase-10 world -- every one of those refusals is a named
owner refusal, not a weakened check.

**Status: accepted.** `terrain_hybridtopology` (with the seven Q-023 pose tests: collinear crossings, close
right-angle crossings, wrong side, next candidate, highway safety, secondary cross-link, overpass pose),
`terrain_hybridroutes`, `terrain_hybridinteractions`, `terrain_hybridrealization` and their `fds_` twins pass;
237/237 ctest on client and dedicated server; state validator green. `Q-023` closes on this record.



## D-017 - Hybrid rewrite Q-022: Phase 10 owns crossing alignment; Phase 11 compiles attached crossings (2026-10-02)

**Decision (user, 2026-10-02, option (a)).** D-009's crossing behaviour is revised: Phase 10 owns the
horizontal road geometry necessary to reach every frozen structural crossing correctly. Phase 11 may select
or move a crossing station and compile the structure; it is never responsible for repairing a road that
arrives at its crossing in the wrong direction. The previous behaviour -- keep the waypoint and leave a corner
there for Phase 11 -- is **revoked** (see D-009's 2026-10-02 revision).

**Phase 10.** Every frozen structural crossing (obligation, bridge-required window, accepted crossing or
ramp-pass window used as a waypoint) is a constrained waypoint with a position and a crossing-axis tangent,
`normalize(approach_b - approach_a)`. The road enters and leaves along that axis, straight from one frozen
approach point to the other, with every class-radius turn finished before the straight approach; Phase 10
guarantees `8W` of it on each side. A* arrives and departs on the axis (judged at move generation with the
run the class's turn still needs); the approach points are frozen vertices with axis leads; a corner that
would sit on or crowd a frozen point refuses the candidate; a structural exemption covers only its own span;
the open road between spans is graded by Section 12.1's interval propagation. A road that cannot is a normal
envelope failure, and after the third the edge fails.

**Phase 11.** The station list is anchor-first: candidate 0 is the problem's own anchor projected onto the
obstruction, then the range, both ends included, at most 64 unique stations in all (superseding the literal
"exactly 64 uniformly spaced"; the performance ceiling is unchanged). Stations are measured on the obstruction.
A moved station is reached by a connector built with Phase 10's own corner construction
(`Hybrid_Build_Crossing_Path`): class-radius turns, then the complete straight approach of
`max(8W, kind.RequiredApproachLength)`, never shortened -- a kind that does not fit is rejected. Connectors are
authoritative compiled geometry (`HybridInteraction::Path`, spliced by `Hybrid_Realized_Route`); a crossing
not attached to its realized road at both ends fails the gate (`HYBRID_INTERACTION_DETACHED`). Abutment seats
are the bridge definition's own start/end cap section lengths, from the bridge owner -- no invented "one
lane". A river deck is level and extends along its axis until the bank is within `max_fill_height` of it; a
canyon or ravine deck is carried between the two road profiles. An overpass deck is a registered bridge kind
or nothing (`method_unavailable`); it is straight under the deck, and its ramps follow the upper centerline.

**Whole-road grading**, found by the restored gate rather than assumed: every compiled structure -- bridges,
tunnels and now overpasses, whose ramps had been graded alone with free feet -- is judged by Section 12.1 over
the whole realized road with every structure already on it. And the 12.1 solver had a fixed-point defect: a
compiled pin is a height the solver itself produced and quantized, but propagation treated it as an exact
real, so a deck graded at the class maximum could not be re-solved from its own pins (measured: a ravine deck
at grade 0.140525 against a maximum of 0.140541, the climb beyond it 3e-5 short). A pin now propagates as its
quantization cell `FixedZ +/- Q/2` and is returned exactly; the final grade check is unchanged.
`Check_Hybrid_Realization` pins a profile to its own solution and requires it back. An overpass `L_ramp` is
recorded at the quantum at or above it, since rounding to nearest put a floor-sized ramp under the `10W` floor.

**Section-13 tests**, on controlled worlds through the real owners: straight crossing (accepted unbent),
sideways arrival (turns on its own bank and enters aligned), opposite arrival (swings round or is refused --
this world is refused), insufficient turn room (refused in every envelope; the same world with room routes),
asset approach longer than `8W` (the shipped kinds need 30 against 26.6 and compile at 30 on a straight road;
a 12W kind is refused with `approach_does_not_fit` and asks for the next envelope), moved station (43.8 along
the river, connector 57.4 long, tightest turn 16.6 = the class radius) and the 64-station cap (anchor first,
both ends kept, unique).

**Section-14 probe.** The detached-crossing measurement (81 of 81 compiled crossings off their road before
Q-022) is now an owner gate: Phase 11 refuses a detached crossing, and the interactions gate grades every road
of every solved world as realized, with its structures, by Section 12.1.

**Status: accepted** with D-018. `Q-022` closes on this record.



## D-016 - Hybrid rewrite Q-021: the cliff talus range is `1W..5.25W`; generator 12 (2026-09-30)

`Q-021` asked whether a native explicit cliff should be crossable on the surface at all, and the answer is
the user's: yes, some of them -- a shallow, broad cliff may expose a legal ramp window while a tall or
narrow one stays a hard wall -- by widening exactly one range. Section 10.2's rock/talus transition width is
now **`1W..5.25W`** (was `1W..3W`), in `hybridlandforms.cpp`'s `CLIFF_TALUS_MAX_W`, in the plan text, and in
`HybridLandforms.md`. Nothing else moved: cliff height stays `tier_step * 1.00..2.00`, `tier_step` 18,
grading 6/6, D-004's raised band, the method orders, the class grades and roadbeds, and D-015's same-way
lean. The talus is still drawn through the ordinary keyed range; nothing is redrawn or widened to make a
road fit, and there is no ramp quota.

**Generator identity.** The same seed now draws different cliff widths, so every cliff, window and route
downstream differs: `CURRENT_GENERATOR_VERSION` is **12** (was 11), the four shipped profiles and the
self-check's three mirrored representative profiles say `generator_version: 12`, and the versioning table
in `NetworkingAndPersistence.md` is brought current (it had lagged at 9/10).

**Where the threshold actually is.** The decision derived `H / F <= 0.926` from
`half_roadbed * 1.5 * H / F <= max_cut_depth`. Measured on the builder with the real fixture geometry, a
`tier_step`-high band with the broadest edge is accepted from **`4.0W` of talus (`H / F = 1.14`)** and
refused at `3.8W` (`1.19`): the ramp's profile is the straight grade line, which sits above the smoothstep
face on its lower half where the uphill edge is highest, so less of the face slope lands on that edge than
the peak-slope derivation assumed. `5.25W` therefore carries a margin beyond the derivation, and the four
boundary tests in `Check_Hybrid_Local_Pass` pin it: old maximum `3W` refused (cross slope), new maximum
`5.25W` at `tier_step` accepted, `5.25W` at `2 * tier_step` refused (cross slope), `1W` refused (a 3.3-wide
shelf fails the swept-width test before any profile is built).

**Three builder corrections the boundary tests forced**, all in `hybridlocalpass.cpp`:

- each ramp climbs its *face* -- toe to shelf edge -- not toe to shelf centreline. Aiming at the centreline
  left the grade line still climbing where it crossed the shelf edge, several units below ground on a
  native shelf, all of it landing on the uphill roadbed edge as cut; every band to `8W` of talus refused on
  cross slope for it;
- the shelf traverse runs crest to crest, diagonally across the shelf while continuing the lean, not via a
  dogleg onto a centreline: two right-angle corners on a leg half a shelf long need more fillet than any
  shelf has;
- the traverse's along-cliff run is solved for its own two corners -- a longer run makes both turns smaller
  and the leg longer -- as a bounded monotone fixed point from the pure-bend estimate.

**The real-pipeline census, reported rather than acted on.** 200 seeds of each of the seven archetypes
(1400 attempts, 136 worlds through Phase 10, 59 solved) accepted **408 explicit cliffs**:

| dimension | distribution |
| --- | --- |
| height | 18..22.5: 113, ..27: 109, ..31.5: 103, ..36: 83 |
| talus | 1..1.85W: 70, ..2.7W: 108, ..3.55W: 80, ..4.4W: 77, ..5.25W: 73 |
| `H / F` (rising face `e + t`) | min 1.143, max 6.278; `<= 0.93`: 0, `<= 1.14`: 0, `<= 1.5`: 66, `<= 2`: 112, `<= 3`: 118, `> 3`: 112 |
| pass-capable cliff windows | **1** (`coastal_peninsula` seed 137: cliff 2, `H 24.0`, talus `4.66W`, `H / F 1.435`, length 281) |
| routes crossing them | 0 |
| `ramp_pass` selected | 0 |

The new range occurs through normal generation -- 73 cliffs drew a talus in the top fifth -- and native
geometry *does* produce a pass-capable cliff window, so the requirement the decision set ("native geometry
can legitimately produce one when its generated dimensions satisfy the existing physical validators") is
met. The window on seed 137 is on real provisional terrain whose base slope evidently helps, since its
`H / F` is past the fixture threshold. What did not occur in 1400 seeds is a routed road crossing such a
window: the shallow-and-broad corner is roughly one cliff in four hundred, and a route crossing any given
cliff window was about one world in four when every cliff carried a window. A native routed `ramp_pass`
fixture is therefore expected at the order of several thousand seeds, which the tool can walk but this
census did not. No further constant is changed on that account; the search tool now prints the cliff
distribution and each pass-capable cliff (`RAMPWINDOW`) so the next walk needs no new instrumentation.

**Status: accepted.** `terrain_hybridlocalpass`, `terrain_hybridlandforms` (its range assertion updated),
`terrain_hybridhydrology`, `terrain_hybridroutes`, `terrain_hybridinteractions` and their `fds_` twins
pass; `shader_perfbudgets` and its twin pass with the mirrored profiles at generator 12; 235/235 ctest on
client and dedicated server; state validator green. `Q-021` closes on this record.

## D-015 - Hybrid rewrite Q-020: the three ground methods, and what a native cliff can carry (2026-09-30)

`Q-020` was decided per branch: `road_cut` and `surface_pass` accepted as unreachable on the native
configuration and covered by owner-level fixtures on the real solver; `ramp_pass` to be made to understand
D-004's actual raised-band cliff. All three now execute through the real Phase-11 solver, and the native
constants, the native bridge registry, D-004 and every method order are unchanged.

**The band ramp.** A D-004 cliff has both approaches on one bench, so its pass is the floor pass inverted:
ascend the near talus face on the diagonal, run along the shelf, descend the far face. Both ramps and the
traverse lean the **same** way along the cliff. The decision asked for opposite leans "so the route returns
across the original crossing station"; measured on the geometry, that cannot be built. A native face is
`e + t = 4.15..12.47` wide and `18..36` high, so at 12 degrees each ramp spends about 84 along the cliff for
a dozen across it and runs nearly parallel to the face; ramps leaning opposite ways would meet the shelf at
close to 180 degrees on a shelf `t = 3.3..10` wide, a hairpin no class radius drives and the decision's own
"no switchbacks" rule forbids. Leaning the same way, every corner is a few degrees except the two at the
toes, which are ordinary class-radius corners on legs that can host them. The crossing therefore exits some
distance along the cliff from where it entered (110.9 on the fixture); that distance is bounded by the
cliff's directional arc length exactly as any lean is, the far approach is frozen with the geometry, and
Phase 11 compiles the frozen ends rather than the point opposite the station.

**Two validators judged the ground instead of the road.** Both were corrected under D-012's rule that a pass
candidate is planned road geometry:

- corners are judged as Phase 10 builds them (D-009) -- an arc of the class minimum radius consuming
  `R tan(theta/2)` of each leg, legal when both legs can host their share. The circumradius through three
  controls was refusing floor-pass corners a vehicle drives easily, and once it stopped, the honest
  refusals underneath surfaced: canyon floors `1.5W..3W = 5..10` wide against the heavy envelope's `8.80`,
  and runs the class grade genuinely needs;
- cross slope is Section 12.2 step 5's: "zero on ordinary straight roadbed", the *compiled profile* inside
  the class maximum. The test is therefore whether a level roadbed at the profile height is realizable --
  both roadbed edges inside their own `RoadVerticalInterval`, banking up to the class maximum -- and not
  whether the untouched face happens to be flat across the road.

**What that measures about native cliffs.** A `PRIMARY_2` roadbed benched along a face has the face's slope
across it, so its uphill edge needs `half_roadbed * slope` of cut; with D-004's smoothstep the steepest
slope is `1.5 * H / F`, so the cut is `4.32 * 1.5 * H / F` and fits inside `max_cut_depth = 6` only when
`H / F <= 0.93`. The broadest, shallowest native band -- `EdgeWidth 1.5W`, `TalusWidth 3W`, `tier_step`
high -- is `H / F = 18 / 12.47 = 1.44` and needs 8.98. So **no native explicit cliff can carry a ramp pass
under native grading**, the native census carries no cliff window at all (0 in 135 worlds, 1400 seeds
attempted), and `Check_Hybrid_Local_Pass` records the broadest native band as the cross-slope refusal it is.
The native `ramp_pass` real-pipeline fixture the decision asked for therefore does not exist, for the same
kind of reason `road_cut`'s does not: the constants, not the algorithm. Reported here rather than worked
around; changing `tier_step`, the cliff height range, the talus range, the grading limits or the roadbed
would each make one, and each is the user's call.

**The three executable fixtures**, all on `Hybrid_Build_Interactions` with real road-class data, real
`CutClasses`, the real method orders, and the real cut and pass validators, on a one-edge world whose only
feature is the obstruction:

- `road_cut`: a 4-high, 6-wide band. Cut is first in the cliff order and wins with no fallback, 4.00 of cut;
  the plan's four conditions -- cut and fill limits, `RoadDeformationHalfWidth`, the outer blend on legal
  ground -- are recomputed from the compiled record;
- `surface_pass`: an 80-unit canyon with a fixture bridge kind of 30 maximum span. `Compile_Bridge` refuses
  it as `span_too_long`, the order falls through to the pass, and Phase 11 revalidates the frozen Phase-6
  geometry; the same canyon with the native-sized registry takes the bridge, as the order requires;
- `ramp_pass`: a 12-high band over a 16.6 face, `H / F = 0.72`. Phase 6 plans no cut for any class (the
  straight profile needs 12), the order falls through to the pass, and the frozen band geometry compiles.

`HybridInteraction::FallbackFrom` records the first method's refusal when a later method compiled
(`NONE` when the first won), so the fixed orders are provable from the record, and it is in the checksum.

**Status: accepted.** `terrain_hybridlocalpass`, `terrain_hybridhydrology`, `terrain_hybridroutes`,
`terrain_hybridinteractions` and their `fds_` twins pass; 235/235 ctest on client and dedicated server;
state validator green. `Q-020` closes on this record with one item carried: whether a native cliff should
be crossable on the surface at all is a constants question, filed as `Q-021`.

## D-014 - Hybrid rewrite I12: reroute lifecycle, edge capabilities, and honest window metadata (2026-09-29)

**Reroute.** Section 8 Phase 11's last method-order step -- "rerun this edge once in its next Phase-10
corridor envelope" -- is owned by `Hybrid_Solve_Interactions_With_Reroute`. Phase 10 runs once on the
normal path and Phase 11 consumes that frozen set; Phase 10 is re-entered only after an actual reroute
request, and then **whole**, not spliced: a route is routed to avoid the routes before it, so splicing one
changed spline into avoidance state derived from the old set is exactly the half-updated state
transactionality forbids. The accepted state is always one Phase-10 run paired with one Phase-11 run over
it. The next envelope is strictly after the one that produced the refused route; floors
(`HybridRouteParams::EnvelopeFloors`) only rise and never pass 2, which is the termination proof, so the
bound is per edge and there is no invented per-world cap. `next > 2` fails with the interaction failure
Phase 11 named. Proved by the gate: `Phase10Runs == reroutes + 1` on every attempt; a no-reroute world
keeps its frozen route checksum; `plateau_basin` seed 193 spends exactly one reroute, runs Phase 10 twice,
moves its edge to the next envelope only, and solves reproducibly; an unspannable-bridge fixture goes
0 -> 1 -> 2 and fails as `obligation_lost` after exactly three runs. The user accepted the whole-pass
re-run on the condition that unaffected edges reproduce byte-identically; every route routed before the
rerouted edge is asserted identical (on seed 193 the rerouted edge is a first-priority trunk, so the
guarantee there covers the routes of every earlier group, which is none).

**Required navigation capabilities** are Section 12.1's per-road field, set at Phase-9 edge creation from
the edge's role and copied unchanged into the route (both checksums). The mapping is Phase 12's own
swept-check list: family trunks and step 11's optional heavy cross-links carry heavy; home roads and
contested/remote field branches carry the Harvester; quota service links, trails and the cross-links
synthesized for bridge and overpass quotas carry light only. Road class is never read. Phase 11's ground
roadbed test is `required_swept_width <= RoadbedWidth(class)`; `+ 2 * clearance` stays only where the
plan names clear width -- the pass floor, Section 12.4's bridge deck and Section 12.5's tunnel bore -- and
all of them now use the edge's own envelope. The audit table is in `HybridTopology.md`.

**Window metadata means what it says.** Three Phase-6 flags were true of stations that could not use them:

- `cut_capable` was set for every surviving cliff station. It is now `CutClasses`, one bit per road class,
  set by running Phase 11's own cut test on the provisional height -- the class grade through the approach
  intervals, cut and fill on the straight profile at `min(W/2, A/3)`, and the blend at
  `RoadDeformationHalfWidth(class)` plus the outer blend landing clear of bases, Tiberium reserves and other
  walls. `HYBRID_WINDOW_METHOD_CUT` is set exactly when a bit is; Phase 11 tries a cut only for the edge's
  own class and still revalidates on the realized terrain;
- cliff stations carried `bridge_capable`. Step 18 gives cliff stations pass/tunnel/cut metadata only and
  the cliff method order has no bridge, so the bit served nothing -- but it kept the window alive, and a
  window is the only hole in an immutable wall. 55 routed cliff crossings went through such windows and
  every one ended in an exhausted reroute. The bridge bit is now river, canyon and ravine only;
- the self-check pipeline fixture ran Phase 6 on stand-alone defaults that promised tunnels the content
  registers no module for. `Build_Hybrid_Pipeline` now takes the preflight metric cache whenever Phase 11
  will run, exactly as the manager fills Phase 6.

With those corrected, the native configuration leaves every explicit cliff a sealed wall and puts no
routed road on a pass-capable, bridge-incapable canyon window. Whether that is acceptable is `Q-020`.

## D-013 - Hybrid rewrite I12: what `plateau_basin`'s rim actually is (2026-09-29)

Asked whether the missing road-cut coverage should come from a `plateau_basin` CLIFF crossing obligation,
the answer traced from the code is **no**, and the reason is worth recording because it looks like one.

`HYBRID_INTENT_BASIN_GUIDE` (`hybridmapidentity.cpp:655`) carries a centre and a radius drawn in
`0.22*D .. 0.32*D`. It is consumed in exactly two places:

- `hybridcoarsematrix.cpp:930` stamps a `HYBRID_FLAG_PLATEAU` disc of `radius + band` and then a
  `HYBRID_FLAG_BASIN` disc of `radius` that clears the plateau flag again. The rim is therefore **one
  coarse ring of a semantic flag** -- macro terrain, realized by Phase 7's height expression;
- `hybridhydrology.cpp:2319` offers the basin centre as a fallback lake site for step 14's lake quota.

No `HybridLandform` is created from it. Phase 5 goes the other way and *rejects* landform placement in
`HYBRID_FLAG_BASIN` cells (`hybridlandforms.cpp:2174`), so the rim is never converted into an explicit
cliff run. Step 18 creates crossing windows for rivers, explicit cliffs, canyons and ravines only, so the
basin rim creates no window and can carry no Phase-11 interaction.

The two `HYBRID_INTENT_BASIN_ENTRY` intents at `(0, +-0.35*s)` are **not** dropped: Phase 3
(`hybridstrategicgraph.cpp:766`) treats them exactly as it treats a `ROUTE_FAMILY_CROSSING` guide, so each
replaces its family's default guide and becomes that family's own `ROUTE_WAYPOINT` site. Family 0 and
family 1 therefore enter the basin at the two intended regions, which is the archetype's actual
requirement -- two independent heavy descents into the basin -- and it is met by ordinary graded routing
through macro terrain. They carry neither `BRIDGE_REQUIRED` nor `DISTINCT_CROSSING_REQUIRED`, and
`hybridstrategicgraph.cpp:809` creates a `CROSSING_OBLIGATION_INTENT` only for a guide that carries one of
those two flags, so no obligation exists and none should be invented.
`Archetype_Obstruction(PLATEAU_BASIN) = CLIFF` (line 627) is consequently dead code for this archetype: it
only labels an obligation that something else created.

**Decision.** `plateau_basin` is the wrong archetype to use as the road-cut fixture, and its basin rim is
not to be reinterpreted as a cliff. Road cut belongs to the explicit Phase-5 cliff runs, which exist in
quantity on many archetypes -- 36 to 67 cut-capable cliff windows per `canyon_crossroads` world -- so the
fixture is a matter of finding a routed road that crosses one, not of adding an obligation.

## D-012 - Hybrid rewrite I12: what a local pass is measured against (2026-09-29)

Section 8 Phase 6 step 18 asks for a "sampled `PRIMARY_2` swept path across the local floor/ramp" and
Section 8 Phase 11 offers a surface pass and a `RAMP_PASS_WINDOW`, but neither says what the path *is*.
Three separate readings of that were wrong in ways that made both methods unreachable on every generated
world, and all three are decided here. The shared owner is `Code/wwphys/hybridlocalpass.{h,cpp}`; Phase 6
proves and freezes a candidate, Phase 11 revalidates the frozen one, Phase 12 realizes it.

**1. The along-obstruction run a station may spend is directional, and it is arc length.**

A ramp buys grade with run along the obstruction, so the run available is a property of the obstruction,
not of the road. It is the arc distance along that obstruction's own sampled centerline: `stations[index]`
back toward its start and the remainder toward its end. The two are rarely equal, because a station sits
somewhere along the feature rather than at its middle, and a nominal `Length` is not the run a particular
station has. A floor pass spends run *both* ways -- the descent leans one way, the ascent the other -- so
each ramp is judged against its own direction. Nothing is subtracted from what is left: a margin would be
a tuning value. A ramp may not buy run past either end, because past the end it is no longer this window's
local pass but ordinary routing around the obstacle.

There are no switchbacks in this generator version. One descent, a floor where there is one, and one
ascent; a station that cannot fit that shape in either of the two deterministic orientations is not
pass-capable, and says so as a `RUN` refusal rather than an unexplained `GRADE`.

**2. A cliff ramp's profile is the grade line, not the ground under it.**

The ramp's intermediate control elevations are the grade-limited line between the two approach elevations,
spread over the plan distance the ramp travels. Sampling the face at the ramp feet instead judges the
cliff rather than the road, and that is what it did: measured on `canyon_crossroads` seed 31337, a band
whose two benches both stand at 0 and whose face rises 31.7 between them produced a "profile" of
0, 31.7, 0, 0 and a 47-degree leg. 69 of 76 stations on that world refused for it. Whether the grade line
can be built out of this ground is the `RoadVerticalInterval` test, which is where a band needing more cut
than the class allows is refused -- and refused *as* cut, so a surface ramp never silently becomes a road
cut.

**3. A floor pass descends the wall, not the approach.**

The candidate carries the two rim points, so the graded legs are rim-to-floor and floor-to-rim and the
approach legs sit on the ground where the approach is. Starting the grade line out at the approach point
spreads the same drop over flat ground as well, which leaves the profile buried under the rim and hanging
above the floor -- cut and fill in the shape of the road rather than of the ground. That artifact alone
refused most canyon and ravine stations on `RoadVerticalInterval`. This is the six-control candidate the
record was always sized for: approach, rim, floor entry, floor exit, rim, approach.

The run and the foot depend on each other, because the foot lands `run` along the obstruction where the
floor may stand lower than it does at the station. So the run is *solved* rather than estimated: each step
lengthens it to what the foot it just reached asks for, which is monotone and therefore terminates. It is
not a search -- there is one candidate at every step -- and the two ways it can end are distinguished,
because they are different defects: too little run on the first step means the obstruction is too short
(`RUN`), while a run that grew and then ran out means the floor falls away faster than a ramp gains on it,
which is the floor's own grade (`FLOOR_GRADE`).

**Phase 11 consumes the frozen geometry.** `Compile_Pass` validated the straight line between the two
approach points, which is the geometry a road crossing a canyon never uses. It now revalidates
`HybridCrossingWindow::Pass` through `Hybrid_Validate_Local_Pass`, re-reading every control elevation from
the realized macro terrain and re-measuring grade, corners, cross slope, floor and each sample's
`RoadVerticalInterval` for itself at `min(W/2, A/3)` -- a `pass_capable` flag is not permission to skip
validation -- and it does not draw a second centerline, because two phases each inventing geometry for one
crossing is how they come to disagree. The frozen shape also decides the method: a floor pass is a surface
pass and a cliff ramp is a ramp pass, so a canyon can never be reported as a `RAMP_PASS_WINDOW`.

**What was measured, and what it rules out.** The refusal census now carries the distribution, not just
the count, because "the parameter space cannot build a pass" and "passes fit and later validation rejects
them" are different defects. On the two worlds that solve, the run fits at 75 of 76 and 82 of 87 stations,
median ratio at or under 0.5. So the along-obstruction run is **not** what makes these methods unreachable
and the branches are not impossible; `0.5 * Length` was never conservative, it was simply not the run a
particular station has. No cut or fill limit, quota, cap or approach tolerance was changed.

**Status: accepted.** `terrain_hybridlocalpass` and `fds_terrain_hybridlocalpass` cover both shapes,
both orientations, directional exhaustion in each direction and each refusal reason; the whole 235-test
ctest suite passes on client and dedicated server. `canyon_crossroads` seed 31337 is the first generated
world to carry pass-capable windows out of Phase 6 -- two of them -- and seed 51 routes structural spans
across two more. Phase 11 still selects no pass, because the plan's own method order for canyon and ravine
is bridge first and those windows are bridge-capable; that is a coverage question about which world to
freeze as the fixture, recorded in `WIP_features.md`, not a licence to reorder the methods.

## D-011 - Hybrid rewrite Q-019: Phase 5 owns Section 5's canyon/ravine bridge quota (2026-09-29)

**Decision: option (a).** Section 5 asks every map for 1/1/2 canyon/ravine bridges and forbids reducing a
quota to rescue a seed, but Phase 9 can only bind a bridge to an obstruction a heavy route actually
crosses. Phase 5 placed its generic canyons and ravines without regard to where the heavy routes run, so
most seeds had nothing bindable -- measured on `open_frontier`, every seed produced bridge-capable ravine
windows and on every one of them all four heavy sites stood on the same side of the crossing axis. The
archetypes carrying their own principal canyon do not have this problem precisely because their intent
guides them onto the strategic axis the routes follow.

So Phase 5 now satisfies the quota itself. `Place_Quota_Crossing_Systems` runs before the density fill --
its systems count toward `TargetSystems` rather than adding to it, and `TargetSystems` is raised to the
quota because a Section-5 quota is a floor on the world. The guide is a point on a heavy family's own
corridor and the landform axis is perpendicular to it, so a route following that corridor has to cut
through rather than run beside; stations walk out from each corridor's midpoint in eighths, edges in
record order. Every candidate still goes through the ordinary required-placement test and the ordinary
Section-6 rejection rules. Distinct `SystemId`s are counted, so one accepted system satisfies one quota
slot.

Option (b) -- keep Phase 9's nearest-chain binding and accept the attempt-failure rate -- is rejected: a
quota that cannot be filled on most seeds is not a quota. Phase 9's binding rule is unchanged.

### Four defects fixed at their owners to make this work

1. **`StableId` ownership (Phase 5).** A landform's id keys every `Landform_Hash` draw, so it must exist
   before candidate positions are probed and stay the same across all of them. What must not happen is the
   id being *spent* when the feature is rejected outright. `LandformIdReservation` returns the id on
   refusal, so probing consumes nothing and the authoritative collection stays in contiguous creation
   order. Every existing caller aborts the phase on refusal, so nothing changed for them.

2. **The line-feature control frame (Phase 5, shared owner).** `Build_Geometry` rotated the four controls
   by the raw `angle` and then stored `Orientation` as `Hybrid_Quantize_Angle(angle)` -- two descriptions
   of one frame differing by up to the 0.001 rad angle quantum. Harmless at the centre, not at the ends,
   where the lever arm is half the feature's length: measured on a 116-unit ravine, end controls sat 0.029
   off an axis the record said they were on, three times the 0.01 the plan allows. The stored
   `Orientation` is now the authoritative frame and the controls are laid in it. This is the shared helper
   for cliffs, canyons, ridges and ravines, so it is fixed once for all of them. The tolerance was not
   touched.

3. **The `SURFACE_SADDLE` crossing obligation (Phase 3).** Phase 3 turned the saddle into a crossing
   obligation with `Obstruction = RIDGE, RequiredMethod = PASS`. Section 6.10 realizes a saddle by
   multiplying the principal ridge's amplitude down around the saddle point, and Phase 5 widens that flat
   until it holds a `PRIMARY_2` roadbed plus two lane clearances -- the road crosses on ordinary drivable
   ground. Step 18 creates windows for rivers, cliffs, canyons and ravines only, never ridges, so the
   obligation could never bind.

4. **`highland_pass`'s alternate family guide (Phase 2).** It was tagged
   `DISTINCT_CROSSING_REQUIRED`, which raised a second unbindable `RIDGE` obligation. Section 6.10's table
   writes those tags where it means them -- `ravine_network` tags one guide `BRIDGE_REQUIRED` and the other
   `DISTINCT_CROSSING_REQUIRED` in as many words -- and for `highland_pass` it lists only "alternate family
   guide `(0,+0.45*s)`". Untagged, as the table has it.

Both (3) and (4) were diagnosed from the same census: the obligation refusal now reports how many windows
exist and why each was passed over, and it read "no qualifying window among 59: 0 already used, 59 wrong
obstruction" -- no ridge window existed, nor could one.

**Status: accepted.** Full suite 233/233 on client and dedicated server.

## D-010 - Hybrid rewrite I12: the overpass ramp reservation carries the smoothstep's peak slope (2026-09-28)

Section 8 Phase 11 asks for two things about an overpass ramp at once:

```text
L_ramp = max(10W, abs(delta_z) / max(tan(upper_class.max_grade),Q))
...one smoothstep ramp over exactly L_ramp on each side...
Sample at min(W/3,A/4) and require max grade, cross-slope, cut/fill interval and clearance at every sample.
```

For `y = h * smoothstep(x/L)` the slope is steepest at the midpoint, where it is `1.5 h / L`. A ramp
measured out by `abs(delta_z) / tan(g)` therefore *averages* `g` and *peaks* at `1.5 g`, so the per-sample
requirement refuses the very shape the same paragraph asks for. Measured on the fixture seeds this is
exactly what happened: a reserved `OVERPASS_OBLIGATION` between edges 6 and 20 was refused
`grade_too_steep`, and a reserved overpass may not downgrade.

**Decision:** the grade limit does not move; the reservation does. `L_ramp` keeps the plan's formula as its
floor and carries the smoothstep's exact peak-slope factor:

```text
L_ramp = max(10W, 1.5 * abs(delta_z) / max(tan(upper_class.max_grade),Q))
```

The plan's literal term is preserved as a **lower bound**: `1.5 * x >= x`, so every `L_ramp` this produces
is at least the reservation the original formula asked for, and the `max(10W, ...)` floor is untouched. The
`1.5` is not a tuning constant -- it is the exact peak derivative of `smoothstep`, `d/ds [s^2(3-2s)] = 6s(1-s)`,
which is `1.5` at `s = 0.5` -- so it is the smallest factor that can satisfy the per-sample constraint for
the shape the plan specifies. **The sampled-grade validation remains authoritative:** the reservation only
decides what geometry is offered, and a ramp whose samples exceed the class maximum grade is still
refused.

Of the two statements the per-sample limit is the stricter, and `L_ramp` is already written as a
`max(...)` reservation, so extending it satisfies both rather than weakening either. The ramp is also
measured against the rise it actually makes -- from the road's own elevation at each ramp foot up to the
deck, not from the ground under the crossing -- and the foot elevation is resolved iteratively because the
foot moves as the ramp lengthens. The compiled profile interpolates foot to deck through the smoothstep; it
does not follow the terrain, which would put the ground's own slope into the road's grade.

Two smaller corrections were made under the same evidence and are part of this decision:

- **Section 12.4 is a question about what carries the vehicle, not about the marked carriageway.** The
  common station tests asked the carriageway to exceed the swept vehicle envelope, which refuses every
  crossing a widened deck exists to make -- 13 of 64 stations on a `PRIMARY_2` canyon crossing, whose 6.65
  carriageway can never hold an 8.80 envelope and never needed to, because Section 12.4 sizes the deck to
  `max(carriageway, swept + 2*clearance)` and blends both approaches up to it. A structural method now asks
  its own asset; a ford, pass or cut asks its graded roadbed.
- **A refused Phase 11 leaves no compiled solution behind**, matching every other phase's contract, while
  its issue records are kept because they are what a refused attempt is read from.

**Status: accepted.** `terrain_hybridinteractions` and `fds_terrain_hybridinteractions` pass, as does the
whole 233-test ctest suite on client and dedicated server.

## D-009 - Hybrid rewrite I11: HIGHWAY_4 curvature resolution (2026-09-28)

**Decision (user, 2026-09-28):** retain the canonical global analysis spacing
`A = clamp(world.chunk_size/4, 8, 16)` and retain the Section-7 road minimum-turn-radius requirements,
`HIGHWAY_4 = 8W` included. Phase-10 spline conversion now deterministically thins non-mandatory
direction-change controls for wide-radius road classes before centripetal Catmull-Rom construction.
Full envelope, obstacle, curvature, grade and cross-slope validation remains mandatory. This resolves
the mismatch between raster A* turn density and `HIGHWAY_4`'s `8W` continuous-curve requirement without
changing global planning resolution and without deferring horizontal route validity to Phase 12.

Explicitly rejected: deriving `A` from the widest road class (it is shared analysis resolution for
terrain, water, crossings and sampling, so one road class may not redefine it), and reading `8W` as a
Phase-12 open-road target (Phase 12 consumes frozen Phase-10 splines and must not become a second
routing phase).

How it is implemented in `Code/wwphys/hybridroutes.cpp`:

1. The raw control set is built exactly as before: the leg ends plus every direction-change cell.
2. `Thin_Controls` drops ordinary interior controls until the retained ones are at least
   `ceil(4*Rmin/A)` analysis cells apart along the original chain. The factor 4 follows the measured
   centripetal Catmull-Rom behaviour at a 45-degree turn -- the rounded corner comes out at roughly a
   quarter of the control spacing.
3. Never thinned, and never thinned *across*: the leg ends and the cell just inside each of them (which
   is what fixes the departure tangent), every frozen waypoint, every mandatory crossing or overpass
   obligation, every chain cell inside an accepted crossing window, and every control step 5 inserted.
4. `4*Rmin` is a starting spacing only. The sampled spline is still checked against the envelope, the
   hard obstacles, the class minimum radius, vertical feasibility, grade and cross slope; a curvature
   failure widens the spacing by one analysis cell and rebuilds, bounded by
   `retry.max_local_retries_per_stage` within the envelope.
5. A non-curvature failure takes step 5's original-A*-cell insertion instead, and the result is
   rechecked against everything including the radius. Every attempted control set is recorded by digest
   so no set is evaluated twice and the two repairs cannot oscillate.

Two related conversion rules were corrected by the same evidence and are part of this decision: a
crossing is entered through the window's own frozen approach endpoints rather than a cell centre (a
curve through a single pinned point bows out of a corridor barely a roadbed wide and lands on the wall
it was crossing), and the spline is anchored on the frozen endpoint positions rather than on cell
centres.

### Revision (2026-09-28): the centerline is arcs and tangents, not a thinned control polygon

**Thinning was implemented as decided above and did not close the gate.** Measured on the fixture seeds
it moved the shortfall rather than removing it -- 25.7, then 24.2, then 21.9, then 8.3 on a `PRIMARY_2` --
and each repair exposed a different corner. The census that was required before any further change said
why, and the answer is structural rather than tuning:

> A curve drawn *through the cell centres of an 8-way A\* chain* cannot honour a minimum turn radius wider
> than the analysis cell except by accident. The chain's only turn granularity is 45 degrees at a cell
> vertex; on this grid `A = 15` while a `HIGHWAY_4` needs `8W = 26.6`. The curvature of any spline through
> those controls is therefore decided by which cells the search happened to pick, and thinning, turn
> spacing and search margins are all ways of steering an accident.

**So Phase 10 now reads the A\* chain as what it is -- a corridor -- and builds the centerline as geometry
that satisfies the radius by construction:**

1. The chain becomes a corridor polyline: the frozen endpoints, every direction-change cell, every frozen
   waypoint, every step-5 insertion. A crossing contributes its window's own two frozen approach points
   and nothing between them.
2. Every frozen interior point is given a lead-in and a lead-out along its own axis, so the polyline runs
   *straight* through it. An arc that rounds a corner does not pass through the corner, and a frozen
   position must be interpolated exactly; this is what makes both possible at once. For a crossing that
   axis is the line between its two approach points, which is also what makes the crossing straight.
3. Every remaining corner is filleted with a circular arc of the class minimum turn radius. A turn of
   `theta` consumes `R tan(theta/2)` of straight on each side; where two corners cannot share the leg
   between them, the shallower -- corridor, never frozen -- is dropped and the pass restarts. Two *frozen*
   corners crowding each other is reported as what it is, which points at the phase that froze them.
4. The centerline is sampled at `min(W,A/2)` along those straights and arcs, and the complete validator
   then runs unchanged: envelope, hard obstacles, the exact Section-7 radius, grade, cross slope.

**The `8W` requirement is not weakened; it is strengthened.** It is no longer a test a candidate has to
pass by luck -- the arcs *are* the radius -- and it is now measured twice: by the router on every
candidate, and independently in `Check_Hybrid_Routes`, which recomputes the circumradius over a
class-radius baseline at every sample of open road. The only tolerance either allows is the project
position quantum `Q = 0.001`, which covers writing a coordinate down and nothing else. Arcs are built at
`1.05 * Rmin` so that quantum cannot decide the outcome; a curvature refusal raises that construction
margin by one step, which can only flatten the curve, and never moves the limit.

Section 8 Phase 10's wording -- "centripetal Catmull-Rom through those controls" -- no longer describes
the road centerline, and that is a deliberate, recorded deviation on the grounds above: the plan's own
Section 0.5 `A` and Section 7 radius table are in direct arithmetic conflict for the wide classes on an
8-way grid, and Section 7 is the one that describes something a vehicle drives. The shared sampler
`Hybrid_Sample_Centripetal_Catmull_Rom` is unchanged and still serves Phases 5, 6 and 10's other curves.

Six defects in the surrounding rules were found by the same census and fixed at their owners:

- a crossing is now **known from the leg that built it** rather than deduced from the finished chain --
  Phase 10 routes a crossing as its own leg between the two frozen approach points, so a route that
  merely clips a window's corridor is no longer mistaken for a crossing, and a real crossing whose bank
  cell centre falls outside the corridor is no longer missed;
- a leg's stops are ordered by the whole journey, not by the stop behind them, so a crossing is no longer
  entered from the far bank and crossed back;
- standing water inside an accepted window is legal ground for the road that crosses it, which it has to
  be: the two approach points stand on the two banks and every cell between them is wet;
- a leg boundary carries the previous leg's heading and its trailing straight, so a turn crowded against a
  frozen waypoint is measured like any other turn;
- same-way turns (an arc of 45-degree steps) and opposite-way turns (an S, each corner rounded alone) no
  longer share one spacing number, which had them the wrong way round;
- vertices the road would have to double back to reach are dropped wherever they appear, including after
  a fillet drop creates one.

**Status: accepted.** All eight fixture seeds route every mandatory edge including their `HIGHWAY_4`
trunks; every accepted sample of open road measures at or above its class minimum radius under the
independent check; `terrain_hybridroutes` and `fds_terrain_hybridroutes` pass, as does the whole 231-test
ctest suite on client and dedicated server; the second routing pass reproduces the checksum. The
temporary routing diagnostics are gone -- `Hybrid_Route_Census` remains in `hybridroutes.h` as a
deliberate tool, called by nothing in a normal build.

### Revision (2026-10-02): crossings are aligned by Phase 10, never left as corners (Q-022, D-017)

The crossing behaviour above is revised by D-017. *When Phase 10 cannot run straight through the frozen
crossing approach, preserve the waypoint but leave a corner there for Phase 11 to resolve* is **revoked**.
Every frozen structural crossing is a constrained Phase-10 waypoint with both a position and a crossing-axis
tangent: the accepted road's tangent where it enters the structural approach is collinear with the frozen
axis, the turn onto it is finished before the straight approach, and a road that cannot do that is an
envelope failure and, after the third envelope, an edge failure. A spline that merely touches the waypoint
while arriving sideways or backwards is not accepted. The arcs-and-tangents construction of the 2026-09-28
revision stays the only turn solver: Phase 11's connectors are built with it, and Phase 9 uses it in the empty
plane to judge whether a structural sequence is drivable at all (D-018).
