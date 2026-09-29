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
