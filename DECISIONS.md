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
