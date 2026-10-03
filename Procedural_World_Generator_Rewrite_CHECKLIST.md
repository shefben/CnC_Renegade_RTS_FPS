# Procedural World Generator Rewrite — Working Checklist

Source of truth: `Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md` (referred to below as **Plan**). Section numbers (`§`) and runtime phases (`P#`) point into the Plan. If this checklist and the Plan disagree, the Plan wins.

Order rule (Plan §19, §24): work items I0 → I19 strictly in order. Do not start an item until the previous item's gate is checked. Every item must leave the project buildable. Old/new records coexist through adapters; adapters are deleted only in item 22.

**Highway access ramp extension (Plan §12.6, added 2026-10-02).** The extension was specified after I1, I2, I10, I11 and I12 were completed. Their highway-access items below are marked **REOPENED FOR HIGHWAY ACCESS RAMP EXTENSION**; the historical completion records stay as written and are not re-claimed for the new work. Do the reopened items in order I1 → I2 → I10 → I11 → I12, re-pass each item's gate, then continue I13 with its §12.6 items included. Revisit only what the extension needs and keep already-passing behavior, apart from the one `CURRENT_GENERATOR_VERSION` increment §12.6.15 prescribes for the whole extension.

Standing rules for every item:

- [ ] Reuse canonical owner (§0.1, §1); on name mismatch record it in `docs/procedural_world/<owner>.md` (fallback `GenerationPipeline.md`) and continue.
- [ ] No duplicate terrain/road/water/bridge/tunnel/pathfind/minimap/shroud/save-load/Commander subsystem.
- [ ] All draws via `WorldSeedService` named domains + `Hash01`/`PickIndex`/`PickSign`/`Range`/`RoundHalfUp` (§0.3); no stateful RNG, no midpoint/hand-tuned values for ranges.
- [ ] Explicit stable sort before every authoritative selection (§0.3 order: hard-valid, cost/score, shorter distance, `StableId`, X, Y, Z).
- [ ] Quantize position/height/distance 0.001, angles 0.001 rad, normalized 0.000001; never hash raw float bits.
- [ ] No city/settlement work (§0.7).
- [ ] Authored/legacy (plan-disabled) path unchanged.

---

## I0 — Runtime Phase 0: repository audit and baseline (§8 P0)

Done 2026-09-26 — findings in `docs/procedural_world/HybridRewriteAudit.md`.

No generator behavior change in this item.

- [x] Map canonical owners from §1: `WorldTerrainSystem`, `WaterSystem`, `RoadSystem`, `BridgeSystem`, `TunnelSystem`, `EscarpmentSystem`, `FoliageSystem`, `PathfindClass`/nav capability owner, minimap, shroud, generated-world manifest, network sync, save/load, Commander, logistics, `WorldSeedService`, descriptor registries.
- [x] Map planning layers to be rewritten: `MapPlanGenerator`, strategic-site planning, `HydrologyGenerator`, `RoadGraphGenerator`, `BridgePlanner`, base/resource ranking, `TiberiumFieldGenerator`, biome/decor planning, `SettlementGenerator` (to be bypassed).
- [x] Record current seed domains (incl. `map_plan`, terrain, hydrology, roads, bridges, tunnels, Tiberium, biomes, decoration) and `CURRENT_SCHEMA_VERSION` / `CURRENT_GENERATOR_VERSION`.
- [x] Record procedural profiles (`data/procedural_world/*.json`) and menu / listen-server / dedicated-server entry points.
- [x] Locate Humvee, heavy reference vehicle, Harvester collision width/height/length sources (expected Humvee `H=2.556`, `W=3.3228`).
- [x] Locate bridge definitions + `BridgePlannerParams` metrics (clear width, max span, structural depth, water clearance, approach length) and tunnel definitions.
- [x] Locate `V_REF` (BaseSelectionPlanner fairness vehicle speed), `I_REF` (CombatSpaceValidator infantry speed), `HarvesterLoadAmount`.
- [x] Locate existing quantization helper, if any.
- [x] Locate canonical vehicle/water fordability query (needed only if `allow_fords=true`).
- [x] Locate starting-building footprint proxies, `ProceduralBaseLayoutService`, `CommanderPlacementValidator`, `RuntimeBuildingFactory`, `GeneratedWorldIntegrator`, `BiomeDecorationGenerator`, `CombatSpaceValidator`, `WorldBoundsService`, `Road_Graph_Class_To_Road_Class`.
- [x] Search every consumer of the existing planner public records.
- [x] Run existing procedural self-checks; save baseline build/test results.
- [x] **Gate:** baseline builds/tests recorded; canonical owners mapped.

## I1 — Runtime Phase 1: data model, profile migration, version bump (§8 P1, §4, §6, §7)

Done 2026-09-26 -- records in `Code/wwphys/hybridplanrecords.{h,cpp}`, plan block in `proceduralworldprofile`, schema 12 / generator 11, gate self-check `terrain_hybridplan`.

Do not change generation output yet. *(User directive 2026-09-26: finish the whole phase, so legacy settlements/lane width are now inert on the hybrid path; native output changes accordingly.)*

- [x] Semantic matrix structures (SoA layers of §2.2 + composable bit flags; not one mutually exclusive number).
- [x] Strategic graph records: site roles and edge/intent types of §2.1.
- [x] Vector feature records (§2.3) with `StableId`, type, owner, bounds, priority, required flag, geometry params, source edge/site.
- [x] Compiled interaction/crossing records (§2.4).
- [x] Temporary refinement-patch records (§2.5 spacing table).
- [x] Native hybrid `plan` block exactly as §4 (symmetry, archetype, biome_mode, biome, secondary_biome, mixed_biome_probability, tier_count, tier_step, detail_weight, base_radius_cells, flank_lanes, water_fraction, boundary_margin_cells, road_density, island_chance, route_redundancy, allow_fords) with §4 legal ranges.
- [x] `route_family_count = clamp(max(route_redundancy, 1+flank_lanes), 2, 3)`.
- [x] Biome enums/pairs (§4.1–4.3), archetype list in stable order (§6), road classes + cross-section table (§7).
- [x] Legacy `lane_width_cells` and settlement/city fields: parseable, inert on hybrid path, one deprecation/debug message, not in hybrid hash.
- [x] Native profiles drop active settlement/city controls.
- [x] `CURRENT_SCHEMA_VERSION` +1 and `CURRENT_GENERATOR_VERSION` +1 — exactly once for the whole rewrite.
- [x] **Gate:** profile round-trip deterministic; old profiles still load; native profile selects hybrid generator.

**REOPENED FOR HIGHWAY ACCESS RAMP EXTENSION (2026-10-02)** — records only (§7, §12.1, §12.6.1, §12.6.10, §12.6.15):

- [x] `HIGHWAY_RAMP` semantic class appended after `TRAIL_1` (existing ordinals, `HYBRID_ROAD_CLASS_COUNT`-sized arrays and the Phase-6 `approach_classes` bitmask keep their values; the `HIGHWAY_RAMP` window bit is always false); cross section `1` lane, `W`, inner `0.20W` / outer `0.30W` shoulders, roadbed `1.50W`, 12° grade, 8° cross slope, `5W` radius; normal paved tier mapping and its speed hint. *(2026-10-03: `HybridRoadClassSpec::OuterShoulderW` carries the asymmetric shoulder; `Hybrid_Road_Class_Is_Routed` excludes the ramp from window, bridge and corridor loops.)*
- [x] `HIGHWAY_INTERCHANGE` record: `StableId`, form (`DIAMOND_ACCESS`/`SIDE_ACCESS`), status (`ACTIVE`/`WITHDRAWN`), anchor node, highway edge ids, axis, access sides, crossing mode, selected `k`, `d_L`, `G`, `S_I`, retarget list with fallback endpoints, capability mask.
- [x] Topology kinds appended: node `INTERCHANGE_LOCAL_JUNCTION`; edges `HIGHWAY_RAMP` and `INTERCHANGE_CROSSING`; ramp fields (role `ON_RAMP`/`OFF_RAMP`, side, gore station, merge/diverge interval with its full-auxiliary-lane and taper subintervals, owning interchange).
- [x] Compiled junction roles `HIGHWAY_RAMP_MERGE`, `HIGHWAY_RAMP_DIVERGE`, `HIGHWAY_RAMP_LOCAL_JUNCTION` alongside the existing incidental / shared-node / overpass-obligation roles; merge/diverge solution record (§12.6.10 item 5). *(roles derived for existing records by `Hybrid_Interaction_Junction_Role`; ramp roles set where compiled.)*
- [x] Road record fields of §12.1 (junction role per junction record, ramp metadata, highway widened-interval list). *(Phase-10 `HybridRoute` carries them; the Phase-12 road record consumes them in I13.)*
- [x] **Gate (re-pass):** records round-trip; existing self-checks unchanged except the new fields. *(`Check_Hybrid_Ramp_Records`; full ctest green; generator identity 16.)*

## I2 — Shared prerequisites (§0.3, §0.5, §0.8, §2.3, §7.1)

Done 2026-09-27 -- helpers in `Code/wwphys/hybridplanning.{h,cpp}`, preflight and identity in `Code/Combat/hybridpreflight.{h,cpp}`, request/manifest/network integration in `ProceduralWorldManager`, `WorldGenerationManifest` and `WorldSyncHandshakeClass` (handshake version 2). Gate self-checks `terrain_hybridplanning` / `terrain_hybridpreflight` and their `fds_` twins. Evidence in `docs/procedural_world/HybridPlanningPrerequisites.md`; the measured reference vehicles are in `docs/procedural_world/VehicleCollisionMetrics.{md,json}`.

- [x] Deterministic helpers: `Hash01`, `PickIndex`, `PickSign`, `Range`, `RoundHalfUp`, shared quantizers.
- [x] `HybridPlanningInputHash` (§0.3 field list; excludes wall clock, watchdog, worker count, inert legacy fields, render-only cosmetic availability; includes fairness + retry limits).
- [x] Constants table once: `H`, `C`, `W`, `A`, `S`, `Q`, `D`, `R_BASE`, `R_EXPAND`, `R_BASE_PROTECTED` (§0.5).
- [x] Size class derived from `D` (§0.6); reject before P2 when width/height >2400, area >5,760,000, or ≤0. Oversize only via explicit developer/offline path.
- [x] Hybrid feature validation (§5): rivers, lakes, highways, tiberium_fields all true, else `HYBRID_REQUIRED_FEATURE_DISABLED`.
- [x] Immutable metric-cache preflight (§0.8, §16.4) with exact failures:
  - [x] `MISSING_HUMVEE_COLLISION_METRIC`; missing heavy/Harvester metrics reject.
  - [x] No bridge def able to carry heavy class → reject.
  - [x] Tunnels enabled but unavailable → `tunnel_available=false`.
  - [x] `allow_fords=true` without canonical fordability query → `FORD_CAPABILITY_UNAVAILABLE`.
  - [x] Missing starting-building proxy → reject.
  - [x] `MISSING_FAIRNESS_REFERENCE_SPEED` (extract existing literal unchanged if inline).
  - [x] `MISSING_HARVESTER_LOAD_AMOUNT`.
  - [x] Biome material slot availability per §0.8 (lower material `StableId` on multi-match).
  - [x] Missing decoration/lamp model only omits that type.
- [x] Section 7.1 helpers: `RoadbedWidth`, `PreferredCorridorHalfWidth`, `MaxCorridorHalfWidth`, `RoadVerticalInterval`, `RoadOuterBlendWidth`, `RoadDeformationHalfWidth`, grade-compatibility test.
- [x] `ProceduralPlanningSpatialHash`, bucket `B_HASH = max(4A, 2S)`, insert by quantized AABB, bucket results sorted by `StableId`.
- [x] Scratch arena / pool (§16.3) and work counters (§16.1 list).
- [x] Unit/self-checks for each helper, preflight and spatial hash.
- [x] **Gate:** self-checks pass.

**REOPENED FOR HIGHWAY ACCESS RAMP EXTENSION (2026-10-02)** (§7.1, §12.6.6–12.6.7, §16.1):

- [x] `RoadbedWidth(HIGHWAY_RAMP)=1.50W`; corridor helpers undefined for `HIGHWAY_RAMP`; deformation half width from the shared formula. *(D-022: roadbed `-0.70W..+0.80W` of the `RampLaneCenterline` via `Hybrid_Road_Inner/Outer_Edge_Offset`, `RoadbedMidpointOffset=+0.05W`; Phase-12 deformation must use the asymmetric edges, carried to I13.)*
- [x] Interchange constants once: `MergeLength=16W`, `DivergeLength=12W`, `TaperLength=6W`, `MergeFullLength=10W`, `DivergeFullLength=6W` (Q-026), `GoreReserveMargin=2W`, `InterchangeGoreSpacing=24W`, `K_GORE=32`, `theta_c=15°`, `psi=45°`, `l_arm=5W`, `T15`, `T30`, `d_sep=3.575W`, `d_join=1.875W`, `d_aux=2.875W`, `h_aux=3.375W`, `E_aux=3.825W`, `h_hwy=2.375W`, `E_hwy=2.825W`, `HighwayJunctionReserve=8W` — derived from the §7 table, not retyped. *(`Hybrid_Interchange_Constants`.)*
- [x] Preflight metric-cache entries `BridgeApproachMax(class)` and `OverpassDepthMax(class)` from existing bridge definitions / `BridgePlannerParams`. *(hashed into the planning input.)*
- [x] Counters `highway ramp candidates evaluated`, interchanges by form, ramps by role, withdrawals, anchor rejections by reason. *(on `HybridTopology`, landed with I1.)*
- [x] Self-check: §12.6.7 construction reproduces `d_Lmin(0)=8.3926380W` (gore at `d_aux` on the `RampLaneCenterline`, D-022), `SIDE G(0)=9.0029197W`; the shared Phase-10 corner construction (`Hybrid_Ramp_Centerline`) ends at the gore at `d_aux` with zero heading error and radius `R_r`; merge `10W` full + `6W` taper, diverge `6W` taper + `6W` full; highway `5.65W -> 6.65W`; `h`/`E` continuous; gore invariant (edges `2.375W`/`3.825W`, shoulders `0`/`0.45W`). *(`Check_Hybrid_Interchange_Construction`.)*

- [x] **Gate (re-pass):** self-checks pass.

## I3 — Runtime Phase 2: MapIdentity (§8 P2, §4, §5, §6.1, §6.10)

Done 2026-09-27 -- records, strategic frame, resolver and checksum in
`Code/wwphys/hybridmapidentity.{h,cpp}`, named-key helper `Hybrid_Key` alongside the other §0.3
helpers in `Code/wwphys/hybridplanning.{h,cpp}`, per-attempt adapter
`ProceduralWorldManager::Build_Hybrid_Map_Identity`. Gate self-checks `terrain_hybridmapidentity`
and `fds_terrain_hybridmapidentity`. Decisions and ownership in
`docs/procedural_world/HybridMapIdentity.md`.

- [x] Build in exact order: size class → symmetry → archetype (auto: drop `coastal_peninsula` if `water.coastline=false`, `PickIndex`) → strategic axis (`mirror_x`=0, else `Range(0,pi)`) → biome mode → primary biome → secondary biome (mixed only; reject `snow+desert`) → island slot bits (`Hash01(map_plan,"island",i) < island_chance`; slots 0/1/2) → `ArchetypeIntentSet`.
- [x] Nod guide per symmetry; `s = PickSign(Hash01(map_plan,"flank_sign"))`.
- [x] `ArchetypeIntentSet` per §6.10 table for all 8 archetypes.
- [x] Guided-river spline (`GUIDED_RIVER` / `COASTAL_GUIDED_RIVER`) constructed now: sample every `A`, sinusoidal meander with keyed amp/wave/phase; river_valley `u` clamp to ±0.10; coastal amp ×0.5, end at sea boundary; quantized and frozen.
- [x] Highland `SURFACE_SADDLE` intent record; mixed biome divider parameters (§4.3 steps 4–9) recorded for P4 rasterization.
- [x] No terrain touched; no unlisted RNG.
- [x] **Gate:** identical seed/profile → identical `MapIdentity + ArchetypeIntentSet` + checksum.

## I4 — Runtime Phase 3: strategic gameplay graph (§8 P3, §6.1, §6.10)

Done 2026-09-27 -- roles, repair, families, obligations, intents, gate and checksum in
`Code/wwphys/hybridstrategicgraph.{h,cpp}`; `HybridStrategicSite::RouteFamily` and
`HybridStrategicEdge::RoadClass` added to the I1 records; per-attempt adapter
`ProceduralWorldManager::Build_Hybrid_Strategic_Graph`. Gate self-checks
`terrain_hybridstrategicgraph` and `fds_terrain_hybridstrategicgraph`. Decisions, the frozen-intent
rule and the `coastal_peninsula` attempt-failure mode in
`docs/procedural_world/HybridStrategicGraph.md`.

- [x] Role creation order: GDI_BASE, NOD_BASE, HOME_TIBERIUM_GDI, HOME_TIBERIUM_NOD, FRONT_CENTER, contested (1/3/5), remote-rich (1/1/2), route-family waypoints.
- [x] Guide positions: §6.1 + P3 non-home resource table; `river_valley` first contested via river-adjacent algorithm (§6.10: offset `0.5*river_width + FieldRadiusMax + 7W`, left then right, midpoint outward).
- [x] Hard-validity rules for BASE / RESOURCE / ROUTE waypoint; guided-river influence band (`centerline + 0.5*river_width + 2W`) unavailable to base/resource.
- [x] Boundary margin: `boundary_margin_cells*chunk_size + reserve radius` (base=`R_BASE_PROTECTED`, resource=`FieldRadiusMax`, waypoint=0).
- [x] Legality repair: rings `A,2A,...≤0.10D`, 16 stations from +u CCW; never moves accepted earlier role; no legal → attempt fails.
- [x] `route_family_count` families with default guides, archetype guide replacement, `CROSSING_OBLIGATION_INTENT` records `(family_id, obstruction_kind, guide, method_or_ANY)`.
- [x] Families share only base nodes.
- [x] Resource intents: base→home; contested→nearest family waypoint; remote→nearest same-`v`-half waypoint (fallback global); `SERVICE_DIRT_2` branch intent per non-home field.
- [x] No trails / optional cross-links here.
- [x] **Gate:** exact role counts, `route_family_count` complete GDI→Nod intents, resource intents attached, ≥1 cycle (tree fails).

## I5 — Runtime Phase 4: coarse semantic matrices (§8 P4, §9)

Done 2026-09-27 -- layers, reservations, conflict repair, biome ownership, gate and checksum in
`Code/wwphys/hybridcoarsematrix.{h,cpp}`; `HybridSemanticMatrix` rewritten onto the §16.3 scratch
arena in `Code/wwphys/hybridplanrecords.{h,cpp}`; §6.1 ring search shared as `Hybrid_Repair_Ring_Search`
(`hybridmapidentity`) and Phase 3's `Hybrid_Role_Is_Hard_Valid` exported so the repair rule has one
owner; per-attempt adapter, arena and work counters in `ProceduralWorldManager::Build_Hybrid_Coarse_Matrices`.
Gate self-checks `terrain_hybridcoarsematrix` and `fds_terrain_hybridcoarsematrix`. Decisions in
`docs/procedural_world/HybridCoarseMatrix.md`.

- [x] Dimensions `ceil(width/S) x ceil(height/S)`, row-major `(y,x)`, in scratch arena.
- [x] Populate in exact 10-step order (boundary via `WorldBoundsService`; middle tier; archetype macro zones; `BASE_PROTECTED` + `BASE_CORE` with forced middle tier / zero bias; Tiberium discs; heavy corridors `max(6S,0.08D)`; service/trail corridors `max(4S,0.05D)`; `open_frontier` `NO_IMMUTABLE_BARRIER`; biome ownership incl. mixed divider rasterize, 35% rule, ≤6 shifts of 0.05, fallback `u=0`, transition `max(4S,8W)`; `NO_DECORATION`).
- [x] No crossing windows yet.
- [x] Reservation priority comparator (§8 P4 list) implemented and reusable for later window insertion; conflicts only move lower-priority role via §6.1 repair.
- [x] `open_frontier` discs placed per §6.9 (95% placeable rule, repair search).
- [x] Matrix checksum + conflict self-checks.
- [x] **Gate:** every required role center in legal reserved region; `BASE_PROTECTED` clear of water/cliff hard intent; no unresolved hard conflict.

## I6 — Runtime Phase 5: vector landforms (§8 P5, §10)

Done 2026-09-27 -- targets, creation order, the six-step candidate process, the Section-10.3/10.4
fields, plateau conversion, gate and checksum in `Code/wwphys/hybridlandforms.{h,cpp}`; the shared
centripetal Catmull-Rom sampler added to `Code/wwphys/hybridplanning.{h,cpp}`; `LandformMask` and
`RoughnessClass` now written by this phase into the Phase-4 matrix; per-attempt adapter
`ProceduralWorldManager::Build_Hybrid_Landforms`. Gate self-checks `terrain_hybridlandforms` and
`fds_terrain_hybridlandforms`. Decisions in `docs/procedural_world/HybridLandforms.md` and
`DECISIONS.md` D-004. Open: **Q-018**, the LARGE hill quota against the 25% same-type overlap cap,
which Phase 5 reports as the named attempt failure Section 5 prescribes.

- [x] Target counts: hills 3/6/10 + `RoundHalfUp(hill_density*(2/4/6))`; ridges 1/2/3 + mountain_density term; cliffs = quota(1/2/4) + interior_cliff_density term; ravine systems = quota(1/1/2) + valley_density term.
- [x] Creation order: archetype principal features → remaining canyon/ravine → cliffs → ridges → hills → plateau conversion.
- [x] Geometry ranges from P5 table, clamped to elevation limits.
- [x] Generic candidate process (6 steps): keyed cell ordering, ≤`max_alternate_candidates_per_feature`, ±0.25S jitter, dimensions drawn once, rejection rules, ≤25% same-type overlap (ravine parent/child exempt).
- [x] Line features: 4 controls (−0.5L, ±0.1666667L, +0.5L), interior lateral ±0.10L, centripetal Catmull-Rom, sample `A/2`; cliff upper-side sign keyed.
- [x] Archetype features obey §6 axis/crossing constraints; `ravine_network` branches 2/3/5 = one system.
- [x] Canyon/ravine profile per §10.3; branch joins = min contribution + `A`-wide smooth union.
- [x] Highland saddle amplitude multiplier (0.15 / lerp / 1.0) widened to fit PRIMARY_2 + shoulders + 2×lane clearance.
- [x] Plateau eligibility + `Hash01(terrain,"plateau",id) < plateau_chance`; upper 35% flattened.
- [x] No crossing windows yet.
- [x] **Gate:** counts + §5 qualifications pass; no immutable landform in `BASE_PROTECTED` or `OPEN_MANEUVER`.

## I7 — Runtime Phase 6: vector hydrology (§8 P6, §11)

Done 2026-09-27 -- the whole twenty-one-step sequence in `Code/wwphys/hybridhydrology.{h,cpp}`:
provisional surface with the `+4*tier_step` base barrier, Priority-Flood with the
`(filled_height,row_major_index)` key, D8 with the `E,NE,N,NW,W,SW,S,SE` tie order, accumulation,
the channel threshold with its three `0.80` reductions, the guided river as selected system 0, the
D8 centerlines with their accept-or-fall-back smoothing, the Section-11 widths, basin and guide
lakes, the water-fraction basins, the clipped coast, the mouth binding, the optional islands and
every crossing/ramp-pass window in one pass after the water was fixed.  Phase 7's `water_carve`,
standing-water and island-bias terms are `Hybrid_Water_Carve_Depth`,
`Hybrid_Point_In_Standing_Water` and `Hybrid_Island_Height_Bias`, so no later phase re-derives a
carve profile.  Per-attempt adapter `ProceduralWorldManager::Build_Hybrid_Hydrology`.  Gate
self-checks `terrain_hybridhydrology` and `fds_terrain_hybridhydrology`.  Decisions in
`docs/procedural_world/HybridHydrology.md` and `DECISIONS.md` D-005.

- [x] Provisional height = tier + P5 landforms (no detail noise); `+4*tier_step` on `BASE_PROTECTED` cells.
- [x] Priority-Flood from exterior, key `(filled_height,row_major_index)`, `drain_parent`, `flood_rank`, retain basin labels.
- [x] D8 with tie order `E,NE,N,NW,W,SW,S,SE`; strictly-lower rule else `drain_parent`.
- [x] Accumulation in descending `(filled_height,flood_rank)`.
- [x] `river_threshold = max(1, RoundHalfUp(river_accumulation_chunks*(chunk_size/A)^2))`; channels, headwaters, trace/merge, rank systems.
- [x] Insert guided river as system 0 (direction rule; synthetic accumulation threshold→4×threshold).
- [x] Select remaining D8 systems with source separation ≥0.18D; up to 3 threshold reductions ×0.80; fail after.
- [x] D8 river centerline: endpoints + direction changes, one 0.25/0.5/0.25 smoothing pass, centripetal Catmull-Rom, accept only if within `A` and monotonic, else raw polyline.
- [x] River width formula (stream→river lerp); `WaterDefinitionClass::BedDepth` only carve source.
- [x] Lake candidates from basins: legality rules, rank `(area desc, StableId)`; required fallback = 8-radial irregular polygon, shrink 10%/retry ≤`max_local_retries_per_stage`; water elevation = spill height clamped below shoreline by `Q`; simplify at `A/2` (≤5% area change).
- [x] Optional basins toward `water_fraction` (stop when next moves farther).
- [x] Coast only when required; move outward by `S` per retry.
- [x] River mouth binding to lake/coast/legal exterior sink.
- [x] Optional islands (decorative only): host selection, `6W` from shore, 8-sample polygon at min area, subtract from water, `island_macro_height_bias`, `2W` blend; rejection not a failure.
- [x] Crossing/pass windows in one pass after all water fixed (sample every `A`, `8W` approaches, method flags `bridge/pass/tunnel/ford_light/ford_harvester`, `preferred` flag); merge `<4W`; stamp `NO_DECORATION`; lakes/coast get no windows.
- [x] Insert all water vectors + windows into `ProceduralPlanningSpatialHash`.
- [x] Adapt `WaterSystem` interface only enough to consume accepted vectors later.
- [x] **Gate:** river/lake quotas; acyclic source→sink flow; no water in `BASE_PROTECTED`/`OPEN_MANEUVER`; no lake/coast blocks heavy corridor bypass; every obstructed required corridor has a window; stable records.

## I8 — Runtime Phase 7: macro terrain realization (§8 P7, §10.1)

Done 2026-09-27 -- the height expression, the plan's `HybridValueNoise2D`, the
`detail_protection_factor` rules, the island shoreline blend, the six-step river longitudinal walk,
the bank blend and the phase gate in `Code/wwphys/hybridmacroterrain.{h,cpp}`. The heightfield stays
`WorldTerrainSystem`'s and the single write stays `MacroTerrainGenerator::Generate`, which now takes
the Phase 4-6 records (`MacroTerrainParams::UseHybrid`) and replaces its four noise-geography
channels with `Hybrid_Macro_Sample_Height`; the authored/legacy and D-003 plan paths are untouched
and mutually exclusive with it. Ordinary-slope relaxation now skips explicit wall and water profiles
(`Hybrid_Macro_Is_Protected_Profile`); the chunk registry is still built exactly once and no road
cut/fill happens here. Per-attempt adapter and gate invocation in
`ProceduralWorldManager::Build_Macro_Terrain`, record at `Get_Hybrid_Macro_Terrain`. Gate self-checks
`terrain_hybridmacroterrain` and `fds_terrain_hybridmacroterrain`. Decisions in
`docs/procedural_world/HybridMacroTerrain.md` and `DECISIONS.md` D-006.

- [x] `HybridValueNoise2D` exactly as specified (hash-grid value noise, smoothstep, bilinear).
- [x] Height formula: tier height + macro bias + landforms − water carve + protected detail; clamp to elevation limits.
- [x] `detail_protection_factor` rules (min when overlapping).
- [x] Island bias blended across `2W` shoreline.
- [x] River longitudinal consistency (6-step surface walk; fail if bed above surface).
- [x] Bank blend half-width `max(river_width,2W)`.
- [x] Ordinary-slope enforcement only outside explicit wall/water profiles; chunk registry rebuilt once.
- [x] Replace native macro geography noise channels; keep authored/legacy path intact.
- [x] Write through `WorldTerrainSystem` exactly once; no road cut/fill.
- [x] **Gate:** heightfield exists; elevation + ordinary-slope checks pass; river surfaces non-increasing; no bed above surface; every P5/P6 feature still represented.

## I9 — Runtime Phase 8: validate/freeze base and Tiberium anchors (§8 P8, §0.10)

Done 2026-09-27 -- the base hard requirements, the sixteen exit sectors, the resource-anchor legality
rules, the Section-0.10 provisional fairness gates, the freeze and the phase gate in
`Code/wwphys/hybridanchors.{h,cpp}`; Section 0.10's `PlanningDistance` added once to the graph owner
as `Hybrid_Planning_Distance`. Nothing is moved here: a hard requirement the realized ground fails is
a named attempt failure. Frozen record at `ProceduralWorldManager::Get_Hybrid_Anchors`, built by
`Build_Hybrid_Anchors` straight after Phase 7 and sampling `WorldTerrainSystem` for realized height
and slope. The base-to-base minimum/maximum time gates are deliberately absent -- they need routed
distance and belong to Phase 19. Gate self-checks `terrain_hybridanchors` and
`fds_terrain_hybridanchors`. Decisions in `docs/procedural_world/HybridAnchors.md` and
`DECISIONS.md` D-007.

- [x] Do not move centers; failure fails the attempt.
- [x] Base checks: placeable disc, no hard overlap, buildable slope over proxy area, expansion area ≥ `min_base_expansion_area`, viable exit sectors ≥ `max(2, min_vehicle_approach_count)`.
- [x] 16 exit sectors (ordinal 0 = +u, CCW 22.5°), swept rectangle `R_BASE_PROTECTED → +6W`, PRIMARY_2 grade; freeze sorted viable ordinals.
- [x] Provisional fairness with `PlanningDistance`: home travel diff, base-to-front diff, non-zero base-to-base distance. No base-to-base min/max here.
- [x] Resource anchor legality (dry/placeable, outside walls/base, ≥1 approach direction).
- [x] **Gate:** all centers valid; geometric + provisional fairness pass; centers and `BASE_PROTECTED` frozen.

## I10 — Runtime Phase 9: transportation topology (§8 P9)

Done 2026-09-27 -- all fourteen construction steps in `Code/wwphys/hybridtopology.{h,cpp}`: base
entrance assignment from the frozen Phase-8 sectors with its reserved corridor, the family chains with
every Phase-3 crossing obligation bound to one Phase-6 window, both Section-5 bridge quotas, the
highway assignment by trunk length, the field access points with their 22.5-degree rotation search, the
home roads and field service branches, the service and trail quota fills, the optional cross-links with
the plan's exact benefit score (mean shortest-path reduction, max-flow gain, open-frontier term) and
both overpass passes with one shared `OVERPASS_OBLIGATION` waypoint per crossing. Section-12.2's
unit-capacity max-flow with `BASE_INTERNAL` contracted is exported for Phase 19. Replaces the native MST
topology on the hybrid path; `RoadGraphGenerator` still owns the authored/legacy one. Two precision
fixes this item needed: Phase 5 now rejects an immutable wall inside a reserve *disc* and Phase 6 keeps
water out of a Tiberium reserve disc, which is what makes Phase 8's dry-field requirement satisfiable.
Per-attempt adapter `ProceduralWorldManager::Build_Hybrid_Topology`. Gate self-checks
`terrain_hybridtopology` and `fds_terrain_hybridtopology`. Decisions in
`docs/procedural_world/HybridTopology.md` and `DECISIONS.md` D-008; **Q-019** open on the attempt-0
pass rate for archetypes with no canyon obligation.

- [x] Base entrance assignment per family (ranked viable sectors, reuse only when exhausted, entrance point at `R_BASE_PROTECTED + W`, corridor capsule, reserve against Tiberium/decoration).
- [x] `BASE_INTERNAL` zero-cost edges (never realized, contracted in max-flow).
- [x] Steps 1–14 in exact order: family chains; crossing obligations; river-bridge quota fill (1/2/3); canyon-bridge quota fill (1/1/2); highway assignment by trunk length (0/1/2); `FIELD_ACCESS_POINT` with 16-station rotation; home roads (PRIMARY_2, or SECONDARY_2 if `<12W`); service branches; quota-filling SERVICE_DIRT_2 (2/4/8); TRAIL_1 shortcuts (1/2/4, ≥8W); optional heavy cross-links (`extra_links = RoundHalfUp(road_density*cap)`, caps 2/5/10) with benefit score; natural overpasses (60–120°, 10W arms, 12W spacing, slope rule, `overpass_cost`); synthesized SECONDARY_2 overpass cross-links; shared `OVERPASS_OBLIGATION` waypoints.
- [x] Windows selected `>=12W` apart; one physical window satisfies one category only.
- [x] Routed-edge count ≤ R (18/32/52) else fail.
- [x] Replace native MST topology; topology self-check before any A*.
- [x] **Gate:** families present, bridge obligations bound to distinct windows, overpass waypoints shared, road/highway/dirt/trail counts pass, cycle exists, no shared required bottleneck.

**OPEN STABILIZATION ITEM (2026-10-02)** — the river-bridge quota stays mandatory; this is an investigation, not a rule change:

- [ ] Determine why valid Phase-6 bridge windows frequently cannot be connected to the heavy topology because no suitable heavy-family node exists on the opposite bank (1400-world census: 3095 cross-link candidates refused `no_bank_pair`, 2839 fallback candidates outside every family corridor, in 182 quota-short worlds). Distinguish: Phase-3 intent placement failing to straddle rivers; Phase-6 windows outside usable corridors; Phase-9 synthesized cross-link endpoint selection too restrictive; the quota procedure internally inconsistent. Do not change the rule; file the next `Q-###` if the investigation finds a spec conflict. Not blocking the HIGHWAY_RAMP extension; **must be resolved before final release acceptance** (current yield is far below the §17 seed-batch targets).

**REOPENED FOR HIGHWAY ACCESS RAMP EXTENSION (2026-10-02)** — new step 15 after step 14; steps 1–14 untouched (§8 P9 step 15, §12.6.2–12.6.8):

- [x] Highway-access topology selection: anchors = `HIGHWAY_4` pass-through `FAMILY_WAYPOINT`s with ≥1 `PRIMARY_2`/`SECONDARY_2` arrival; never a highway terminus; processing order `(family, chain index, StableId)`; only existing graph connections, never proximity.
- [x] Frame and sides: bisector axis `a`, right-hand-traffic `tau_s`/`n_s`; side by `cross(a,delta)`; reject on `|c|<0.5` or a lower edge already served by another interchange; `DIAMOND_ACCESS` (two access sides) vs `SIDE_ACCESS` (one).
- [x] DIAMOND crossing: class rule; grade-separated via step-12 overpass tests (one `OVERPASS_OBLIGATION` anchored at node `X`), else at-grade via Phase-11 at-grade conditions (crossing split at `X`), else reject.
- [x] Gore candidate selection: `k=0..31` ascending, first hard-valid by §12.6.8; `d_L(k)`, `G(k)`, `S_I(k)` per §12.6.7; `highway ramp candidates evaluated` counter.
- [x] Pose realizability: interchange pose `(X, a, S_I)` connects to neighbouring structural poses by the `HIGHWAY_4` radius construction (Q-023 rule); chain extends `S_I + 8W` before a base entrance.
- [x] Structure reserves: crossing-window, overpass (highway-upper planning reserve) and conventional-junction reserves; own-crossing and own-junction reserves against gore reserves.
- [x] Interchange spacing: gores of unrelated interchanges `≥24W` apart along the chain; gore reserves and straight extents disjoint; same-carriageway intervals non-overlapping.
- [x] Exclusions (no window exception), grade feasibility at 12°, local-junction arm cone (60°) and separation (30°).
- [x] Retargeting of all access-side arrivals (incl. `SERVICE_DIRT_2`/`TRAIL_1`) to `L_s`; non-access-side arrivals keep `X`; Section-5 qualification recheck; routed-edge cap `R`.
- [x] Records in fixed `StableId` order; ramp graph length; frozen fallback variant; capabilities = `LIGHT` OR all retargeted arrivals' capabilities.
- [x] Rejected anchor keeps its conventional junction (never a phase failure); quotas unchanged.
- [x] **Gate (re-pass):** §8 P9 gate including the interchange clause; stage-9 checksum includes interchange records. *(2026-10-03: `Build_Interchanges` / `Hybrid_Build_Interchanges`, gate clause `Check_Interchanges` re-tests every frozen candidate; `Check_Hybrid_Interchange_Topology`: SIDE_ACCESS, grade-separated DIAMOND_ACCESS, ramp-across-river refusal, spacing refusal, determinism; ramps never get their own bridge/tunnel -- a candidate whose ramp, gore band, local junction or crossing meets water, a hard wall, a base, a corridor or a Tiberium reserve is refused in Phase 9.)*

## I11 — Runtime Phase 10: bounded heading-aware A* and splines (§8 P10, §16.2)

- [x] Routing priority order (8 groups) then `StableId`; subroutes through frozen obligation/window waypoints.
- [x] State `(cell, incoming_direction)`, expansion order `E,NE,N,NW,W,SW,S,SE`.
- [x] Three envelopes: preferred, +S, +2S (= max).
- [x] Hard-infinite cells list (boundary, `BASE_PROTECTED` except own entrance corridor, Tiberium reserve except own endpoint, lake/coast, river/wall outside windows, `OPEN_MANEUVER` violation).
- [x] Move grade via vertical intervals; cost formula with `Ks/Kt/Kc` table, reuse modifier, heavy-family collinear rule (`W`, `>6W`), optional cross-link collinear rule, window penalty `20A`.
- [x] Heuristic `0.75*Euclidean`; open-set key `(f,g,cell,dir)`; reopen only on strictly lower quantized `g`.
- [x] Expansion ceiling `8N` per route, `R*8N` per attempt.
- [x] Path-to-spline: dedupe, direction-change controls, sample `min(W,A/2)`, curvature/grade/cross-slope checks, insert earliest failing nearest path cell, ≤`max_local_retries_per_stage` insertions, then next envelope. **The centerline is straights and circular arcs of the class radius, not a Catmull-Rom through cell centres** — see `DECISIONS.md` D-009 revision for the measured conflict between `A = clamp(chunk/4,8,16)` and the Section-7 radii on an 8-way grid, and for why `8W` is thereby satisfied by construction and verified independently.
- [x] Crossing intervals remain unresolved records.
- [x] **Gate:** every mandatory edge has one legal spline with stable checksum.

**STABILIZATION (D-021, 2026-10-02)** (§8 P10, §17):

- [x] Ordinary strategic guide vs structural waypoint: the A* chain passes the guide's cell; the final centerline need not; the guide corner stays at its frozen position, is not pushed aside by ordinary smoothing, yields to a structural straight approach; Phase 9 Q-023 uses the same corner geometry; logical slicing at the nearest centerline point.
- [x] Continuous segment-vs-river testing for every search move and every centerline segment; a road crosses its own window only through its accepted crossing stretch.
- [x] `Phase10NaturalCrossingAlignmentRepair`: promote the earliest failing naturally selected crossing's frozen approach pair to constrained stops, rerun in the same envelope, 1 local retry each (shared with step-5 insertions), window unavailable to the edge for the rest of the envelope when still unusable, reset only by advancing to the next envelope.
- [x] Symmetric structural arrival/departure turn-room validation by one class-radius owner.
- [x] Structural leads rebuilt whenever adjacent control geometry changes; the gate validates only the final rebuilt geometry.

**REOPENED FOR HIGHWAY ACCESS RAMP EXTENSION (2026-10-02)** (§8 P10, §12.6.9):

- [x] Highway tangent alignment: both highway edges at an active anchor routed through stops `a_I`/`b_I`, straight along `a` over `[X-S_I, X+S_I]` (Q-022 alignment rule).
- [x] Routing group 1b after group 1: crossing edge(s), then ramps `RIGHT`→`LEFT`, `OFF_RAMP`→`ON_RAMP`; route `HIGHWAY_RAMP` centerlines by the exact §12.6.7 construction (arcs of `5W`, `45°` departure, `15°` convergence leg, parallel at the gore), never A*.
- [x] Re-validate every §12.6.8 test on routed geometry plus no intersection with other group-1 roadbeds.
- [x] Withdrawal (only fallback, only before group 2): third-envelope failure of an aligned highway edge, or a failed group-1b re-validation; restore frozen Phase-9 variant; `WITHDRAWN` status in checksum.
- [x] Footprint polygons hard-infinite for edges routed after group 1b (own `L_s` cell excepted).
- [x] Lower-road attachment routing: retargeted edges route to `L_s` under ordinary rules.
- [x] **Gate (re-pass):** §8 P10 gate including straight-extent, construction and withdrawal clauses; stage-10 checksum includes ramp splines and statuses. *(2026-10-03: aligned stops `a_I`/X/`b_I` with an open-road aligned straight; group 1b `Construct_Interchanges` with `Hybrid_Ramp_Centerline` re-validated against the realized world, the gore exactly as frozen and every group-1 roadbed (12.6.8 tests 2/3/4/7 depend only on frozen topology positions, which Phase 10 never moves); withdrawal (a) and (b) to the fallback variant (`HybridRouteSet::WithdrawnInterchanges`, `Hybrid_Route_Edge_End`); footprint keep-out; gate `Check_Interchanges_Gate`; `Check_Hybrid_Interchange_Routes`.)*

## I12 — Runtime Phase 11: interaction solver (§8 P11, §12.3–12.5)

- [x] Query only via spatial hash; process by road `StableId` then along-road distance; no runtime objects.
- [x] Station search: `half_length = max(8W,4S)`, spacing `min(W,A/2)`, cap 64 stations, quantize + dedupe; mandatory obligations centered on frozen waypoint.
- [x] Common hard-valid station tests (8 conditions).
- [x] Road×river: bridge → ford (all conditions; never HIGHWAY_4/PRIMARY_2; never quota) → next envelope once → fail.
- [x] Road×canyon/ravine: bridge → pass window → tunnel → next envelope → fail; mandatory bridge skips pass/tunnel.
- [x] Road×cliff/ridge: cut → ramp/pass window → tunnel → next envelope → fail.
- [x] Road×road: at-grade junction conditions → overpass → reroute lower priority → fail; reserved overpass never downgrades.
- [x] Bridge asset selection: smallest sufficient max span, then lower kind `StableId`; metrics from definition then `BridgePlannerParams`.
- [x] Bridge scoring `4*span + 2*elevation + 3*grade + 2*curve + detour`; approach reserve `max(8W, required_approach_length)`.
- [x] Bridge vertical geometry (§12.3) and width (§12.4); overpass clearance with heavy vehicle height + safety.
- [x] Overpass construction: assignment order, `L_ramp`, smoothstep ramps, sample `min(W/3,A/4)`, one swap only, then reroute.
- [x] Tunnel rules (§12.5); shortest sufficient definition.
- [x] All bridges/overpasses `PRISTINE`; old random damage disabled on hybrid path.
- [x] CrossingProblems per edge ≤12.
- [x] Swept proxy validation over every compiled crossing interval.
- [x] **Gate:** one solution per conflict; obligations keep method; bridge/overpass quotas; proxy sweeps pass; limits respected.

**REOPENED FOR HIGHWAY ACCESS RAMP EXTENSION (2026-10-02)** (§8 P11, §12.6.10):

- [x] Compile merge/diverge interactions: one `HIGHWAY_RAMP_MERGE`/`DIVERGE` record per ramp (gore station, interval, `d(u)`, `h`, `E`, `z` at `min(W/3,A/4)`); one `HIGHWAY_RAMP_LOCAL_JUNCTION` per `L_s` via the existing class-radius junction construction.
- [x] Distinguish intended ramp junctions (merge, diverge, local junction, DIAMOND crossing at `X`) from incidental `ROAD_X_ROAD` crossings; incidental at-grade only outside footprint + `RoadOuterBlendWidth`; the interchange element is never the rerouted road.
- [x] DIAMOND crossing: reserved overpass via existing rule with the highway continuous through `X` (or intended at-grade junction); swap rule checked against gore reserves; constructed edges have no envelope, so exhaustion fails the phase.
- [x] Structural conflict validation: new common station test (no approach / portal / overpass ramp interval in an active gore reserve).
- [x] `z_L` per §12.6.10 item 3; whole-ramp 12.1 solve with gore, parallel-interval and `L_s` pins.
- [x] Swept proxy validation of ramps, parallel intervals and local junctions with ramp capabilities.
- [x] No withdrawal in Phase 11.
- [x] **Gate (re-pass):** §8 P11 gate including the interchange clause; stage-11 checksum includes merge/diverge and junction-role records. *(2026-10-03: `Collect_Interchanges`, `Compile_Interchange_Crossings`, local-junction role, `Compile_Interchange_Elevations` (z_L, whole-ramp solves, merge/diverge records and samples), `Sweep_Interchanges`, `Check_Interchanges_Gate`; station test 9 and overpass-assignment gore checks; footprint + `RoadOuterBlendWidth` at-grade rule; `Hybrid_Route_Priority_Rank`; Phase 11 reads `Hybrid_Route_Edge_End`; `Check_Hybrid_Interchange_Interactions`.)*

## I13 — Runtime Phase 12: final roads, crossings, cut/fill (§8 P12)

- [ ] No reroute/method change; failure fails attempt.
- [ ] Vertical-profile solver exactly as §12.1 (forward/backward propagation, clamp walk, quantize, revalidate, no invented banking).
- [ ] Terrain deformation 9 steps (roadbed, shoulder, `outer_blend` smoothstep, protected voids, shared junction patch once, existing ditch helper only if present, commit via `WorldTerrainSystem`, realize once via Road/Bridge/Tunnel/junction APIs, reverse-order cleanup list).
- [ ] Semantic class → existing physical road tier mapping (§7); explicit width passed or hybrid defs created once at init.
- [ ] Refinement patches released after commit.
- [ ] Post-realization swept verification (Humvee all roads, heavy on heavy edges, Harvester on economy edges) + crossing object verification.
- [ ] No global nav rebuild triggered.
- [ ] Highway access (§8 P12 §12.2a, §12.6.11–12.6.13), processed in routing order (highway, then group 1b, then later roads):
  - [ ] Realize ramp roadbed: one canonical `RoadSystem` road per ramp, `L_s`→gore, normal paved tier, width `W`, symmetric shoulder `0.25W` (exact `1.50W` footprint).
  - [ ] Realize merge/diverge widening on the highway road: optional per-sample asymmetric edge profile added to `RoadSplineClass`/`RoadPointClass` (default zero, existing callers unchanged) used by ribbon, road mask and grade stamping; `h`/`E` from the compiled record.
  - [ ] Gore attachment = compiled merge/diverge record; assert no `RoadSystem`-discovered junction inside a parallel interval or gore reserve.
  - [ ] Canonical RoadSystem instantiation of local junctions at `z_L` and the DIAMOND crossing through the existing overpass/junction path; runtime ids on the cleanup list.
  - [ ] Terrain deformation: union roadbed; ramp outer blend never modifies the highway roadbed/shoulder (gore wedge).
  - [ ] Swept validation: ramps, parallel intervals, local junctions with heavy/Harvester envelopes per capability; §12.6.13 ramp list.
  - [ ] No new topology, no downgrade to an ordinary intersection; discrepancy fails the attempt.
- [ ] **Gate:** every mandatory object realized once, matches frozen records, swept tests pass, no detached crossing, every active interchange realized and attached at both ends.

## I14 — Runtime Phase 13: Tiberium fields (§8 P13, §13)

- [ ] Realize via canonical `TiberiumFieldGenerator` from frozen anchors.
- [ ] Radius attempt list from `K = max_local_retries_per_stage` (Max→Min, quantized, deduped).
- [ ] Core hard-invalid list; transition/contamination masks.
- [ ] Harvest apron from `FIELD_ACCESS_POINT` (no road mesh); Harvester OBB checks at `min(W/2,A/3)`; turning disc `1.5*Harvester length`; a Harvester route through an interchange is checked along its ramps and parallel intervals too.
- [ ] Contestability: ≥1 contested field with `|GDI−Nod| ≤ max_base_to_front_travel_time_difference_seconds`.
- [ ] REMOTE_RICH yield multiplier 1.25 only if field member already exists.
- [ ] **Gate:** counts; home Harvester routes valid; contestability; legal footprints/access.

## I15 — Runtime Phase 14: base layout planning (§8 P14)

- [ ] Split `ProceduralBaseLayoutService` into `Plan_Initial_Base_Layout` / `Commit_Planned_Base_Layout` (no second algorithm).
- [ ] Use authored `base.gdi_initial_buildings` / `base.nod_initial_buildings` order; existing rings + `CommanderPlacementValidator`; vehicle exits face nearest `BASE_ENTRANCE_POINT`; proxies only (no `RuntimeBuildingFactory`).
- [ ] Freeze + checksum transforms, production exits, refinery/Harvester corridor, spawn clearances, expansion reserve, entrance connectors.
- [ ] Exit sectors `max(2, min_vehicle_approach_count)` still viable.
- [ ] **Gate:** every descriptor has legal frozen transform; exits clear; spawn clearances; sectors viable.

## I16 — Runtime Phase 15: tactical refinement (§8 P15, §15)

- [ ] Immutable exclusion envelopes built first (ramps, widened merge/diverge roadbed and gore reserves included).
- [ ] Run existing `CombatSpaceValidator` on listed targets with profile thresholds only.
- [ ] Repair order: berm (≤4W radius, ≤0.75W height, buildable slope) → rock/ruin props → vegetation → fail; stations `max(4W, preferred_cover_interval)` midpoint outward; ≤`max_alternate_candidates_per_feature` per method; keyed yaw.
- [ ] Bridge approach cover pocket within `6W..12W` unless already under thresholds.
- [ ] Local re-check after each repair; LOS ray caps 512/2048/4096; repair candidates ≤ 3×alternate limit.
- [ ] **Gate:** combat-space hard checks pass; repairs preserve structure.

## I17 — Runtime Phases 16–17: materials, biome, decoration (§8 P16–17, §14)

- [ ] Material priority (road, Tiberium core, contamination, wet shore, exposed rock, biome background/transition).
- [ ] Single-biome rules per table; mixed blend only in transition band; no elevation-driven biome.
- [ ] `BiomeDecorationGenerator` unchanged as producer; theme multipliers 1.00/0.55/0.20/0.15; `accepted_budget` per category/region; cluster-center separation `2W`, query radius `max(3W,2A)`.
- [ ] `forests=false` skips tree/bush/grass, keeps rocks.
- [ ] Decoration exclusion list.
- [ ] Street lamps: HIGHWAY_4 every 12W alternating; PRIMARY_2 10W near base/bridge; none otherwise (none on `HIGHWAY_RAMP`); offset 0.5 beyond roadbed, using the widened edge `E` on merge/diverge intervals; skip a station whose lamp lands in a ramp roadbed, gore reserve or clear zone.
- [ ] Cities remain disabled.
- [ ] **Gate:** no decoration in structural envelopes; only decoration/material checksum affected.

## I18 — Runtime Phase 18: single integration/navigation build (§8 P18, §16.5)

- [ ] Terrain collision rebuild once; register existing structural/Tiberium/decoration ids (no second instantiation).
- [ ] Split `GeneratedWorldIntegrator` creation vs finalization if mixed.
- [ ] Nav/`PathfindClass` build exactly once; bridge/tunnel/overpass links from frozen records; freeze capability graph.
- [ ] Final navigation links through ramps: every ramp, parallel interval and local junction is a two-way capability link (§12.6.5) with its required capabilities; planned interchange connectivity == capability graph.
- [ ] Minimap, shroud, Commander chokepoint/expansion metadata, logistics (only if enabled); ramps and widened intervals drawn as connected road geometry, never detached splines.
- [ ] Discard planning spatial hash; compute provisional checksums.
- [ ] Verify no earlier phase triggers a global rebuild.
- [ ] **Gate:** owners report success; planned connectivity == capability graph connectivity.

## I19 — Runtime Phase 19: validation, retries, manifest, activation (§8 P19, §17, §20, §21)

- [ ] 19A read-only checks 1–21 (heavy connectivity, Edmonds-Karp ≥ `route_family_count` per §12.2 contraction rules incl. ramps as ordinary unit-capacity edges, home access, home fairness, income fairness, front fairness, base separation min/max, single-bridge dependency, vehicle approaches, infantry approaches, expansion, exposure, road geometry incl. `HIGHWAY_RAMP`, crossings, obstacle binding, quotas, base layout, nav agreement, biome invariants, checksum recompute, highway access interchanges).
- [ ] Check 21 final attachment/geometry/capability validation: both ramp ends attached (`L_s`, highway gore), gore metadata vs realized geometry, `5W` radius, 12° grade, 8° cross slope, gore tangent alignment (0 within 0.001 rad) and 15° convergence leg, merge `16W` / diverge `12W` along the highway, swept clearance per capability, `h`/`E` continuity, no accidental intersection, required capability through the interchange; withdrawn interchanges have their fallback junction.
- [ ] Named diagnostic per failure; no repair.
- [ ] Attempts 0..2 with `effective_seed = Normalize_Seed(requested + attempt_index)`; full reverse-order teardown; watchdog never resets (15/30/60 s).
- [ ] 19B commit: manifest fields (requested seed, attempt index, effective seed, `HybridPlanningInputHash`, versions, content revision), stage checksums in §20 order (19 stages, absent/zero for disabled), commit-not-active, server `Commit_Planned_Base_Layout` + spawn pool, rollback on failure, then `WORLD_ACTIVE`.
- [ ] Exhaustion/watchdog: no commit, no `WORLD_ACTIVE`, existing failure path, full log line.
- [ ] Pre-`WORLD_ACTIVE` lockout: no spawn, timer, AI, economy, Harvesters, Commander, runtime terrain mutation (§3).
- [ ] Client uses server-accepted effective seed; checksum match before gameplay; save/load stores identity (§21).
- [ ] **Gate:** validator, retry, manifest and activation behavior verified.

---

## 21 — Acceptance gates (§18, §3.1, §16)

- [ ] Cold batches: 256 MEDIUM, 64 SMALL, 64 LARGE, 32 dedicated-server MEDIUM; record per-seed metrics (§18.1); cache disabled.
- [ ] Quality targets: ≥95% MEDIUM pass attempt 0, ≥99% by attempt 1, ≤1 exhausted of 256.
- [ ] Performance targets (normal/P95/watchdog/memory) per size on reference machine.
- [ ] Determinism: 1 vs 4 workers, client vs server, repeat, manifest reconstruction, save/load.
- [ ] §18.3 automatic gameplay/geometry checks on every batch map.
- [ ] §18.4 live visual/driving smoke seeds documented: temperate, snow, desert, tiberium-wasteland, mixed, coastal_peninsula, highland_pass; Humvee, heavy, Harvester, infantry.
- [ ] First milestone: gray battlefield with all features coherent, traversable, alternate routes, within budget — before biome/foliage/lamps count as done.
- [ ] Highway access acceptance (§12.6, §18.2, §18.4):
  - [ ] Smoke seeds with an active `SIDE_ACCESS` and an active `DIAMOND_ACCESS` found in the deterministic seed sequence and recorded.
  - [ ] On-ramp driving smoke: Humvee enters `HIGHWAY_4` through an `ON_RAMP`.
  - [ ] Off-ramp driving smoke: Humvee exits `HIGHWAY_4` through an `OFF_RAMP`.
  - [ ] Heavy reference vehicle drives a heavy-capable ramp; Harvester traverses a ramp whenever a Harvester-capable strategic route uses one on a smoke seed.
  - [ ] Determinism test: interchange records/checksums identical for 1 vs 4 workers, repeat runs and manifest reconstruction.
  - [ ] Client/FDS checksum agreement including interchange records.
  - [ ] Merge/diverge visual inspection: no width step, gap or overlapping surface; ramp attached to highway and local junction; Commander/minimap shows connected ramps.
  - [ ] Diagnostics recorded per batch seed: candidates evaluated, interchanges by form, withdrawals.

## 22 — Remove obsolete native hybrid branches

- [ ] Only after all gates pass; call-site search proves unused; delete migration adapters; keep authored/legacy plan-disabled paths.

## 23 — Documentation reconciliation (§22)

- [ ] Update `docs/procedural_world/*` to hybrid pipeline; remove claims of MST roads, noise-driven geography, active cities, multi-patch biomes, unspecified budgets, post-commit bridge discovery, last-incomplete-world activation.
- [ ] Document cities/settlements as future feature needing asset set + generator-version bump.
- [ ] Document highway access interchanges in `HybridTopology.md`, `HybridRoutes.md`, `HybridInteractions.md`, `RoadsBridgesTunnels.md`; document elevated-highway viaducts as a future feature needing its own specification + generator-version bump.
- [ ] Record source-name mismatches found during §0.1 adaptation.

## Definition of done (§23)

- [ ] Every §23 bullet true (including the highway-access bullet); project lifecycle files (`completed_features.md`, `WIP_features.md`, `unstarted_features.md`, `DECISIONS.md`) reconciled per `CLAUDE.md`.
