# Command & Conquer Renegade FPS/RTS Procedural World Generator Rewrite Plan

**Status:** audited authoritative implementation specification. This file supersedes every older procedural-generation planning document where the two disagree.

**Goal:** rewrite the procedural map generator so a procedural map requested by a client/listen server/dedicated server is generated during map startup, finishes inside a bounded loading-time budget, and produces coherent Command & Conquer-style battlefields with deliberate roads, rivers, lakes, cliffs, ravines, canyons, bridges, highways, dirt roads, hills, Tiberium fields, islands and foliage instead of disconnected random terrain features.

**Primary architectural decision:** use a **hybrid semantic-raster + vector-feature + adaptive-local-refinement compiler**. The semantic raster plans the world cheaply. Continuous vector features represent roads, rivers, cliffs, ravines and canyons. A deterministic compiler resolves interactions such as road/water or road/canyon crossings before final geometry is created. High-resolution work is performed only near accepted gameplay-critical features.

**Important scope decision:** procedural cities/settlements are **disabled for this rewrite** because the required civilian/abandoned-city building asset library does not yet exist. Do not generate city regions, parcel grids, city streets, civilian structures or industrial settlement districts. The rewrite must not block a future city system, but no city-generation work belongs in the implementation defined here.

---

## Audit closure note

This revision closes the ambiguity/dependency defects found in the previous hybrid plan: size class is derived rather than randomly chosen; quantization and deterministic range selection are explicit; hybrid feature toggles and quota semantics are explicit; legacy plan-field units are preserved; strategic route-family construction is explicit; landform counts/dimensions are bounded; hydrology uses one specified Priority-Flood+D8 algorithm; road A* state/cost/corridor limits are specified; crossing search/scoring/asset selection are specified; bridges start pristine; Phase 12 versus Phase 18 object ownership is no longer duplicated; Tiberium role counts and contestability are specified; tactical repair cannot modify structural topology; performance work ceilings are fixed rather than calibrated by agent judgment; three-attempt retry/exhaustion behavior is explicit; and coding order now mirrors runtime dependencies one-for-one.

# 0. Non-negotiable execution contract

## 0.1 Agent behavior

The implementation agent MUST complete this specification without asking the user to make design decisions. If source names differ from this document:

1. search the checked-out tree for the named concept and its call sites;
2. identify the current canonical owner by responsibility;
3. reuse that owner where possible;
4. adapt names to the real source tree;
5. preserve non-procedural callers;
6. record the mismatch in the existing `docs/procedural_world/<owning-subsystem>.md`; when no owning document exists, append it to `docs/procedural_world/GenerationPipeline.md`;
7. continue.

Do not create a duplicate terrain, road, water, bridge, tunnel, pathfinding, minimap, shroud, save/load or Commander subsystem merely because rewriting a caller looks easier.

## 0.2 Instruction precedence

When instructions conflict, use this order:

1. engine safety/correctness invariants;
2. this section and exact constants/tables in this file;
3. the ordered generation pipeline;
4. detailed subsystem rules later in this file;
5. old procedural-world documentation.

Old documentation is historical input. Update it when it conflicts with this specification.

## 0.3 Determinism

Authoritative generated output MUST depend only on:

```text
requested seed
effective attempt seed
profile
procedural generator version
content/descriptor revision
```

It MUST NOT depend on:

```text
wall clock
thread scheduling
worker count
pointer values
unordered container iteration order
frame rate
client GPU
machine speed
```

Use the repository's `WorldSeedService` and named seed domains. The hybrid planner uses the existing `map_plan` domain for map identity/strategic choices and the existing subsystem domains for terrain, hydrology, roads, bridges, tunnels, Tiberium, biomes and decoration. Do not introduce a stateful shared RNG stream.

Every deterministic draw in this document uses these exact helper semantics:

```text
Hash01(domain,key...) = existing deterministic repository hash converted to [0,1)
PickIndex(n,h)        = min(n-1, floor(h*n))                     // n > 0
PickSign(h)           = -1 when h < 0.5, otherwise +1
Range(a,b,h)          = Quantize(a + (b-a)*h)                    // a <= b
RoundHalfUp(x)        = floor(x + 0.5)                           // x >= 0
```

Whenever this document writes a range such as `0.18..0.25`, consume exactly one keyed `Hash01` draw and use `Range(lower,upper,h)`. Do not choose a midpoint, a random library RNG or a hand-tuned value.

Stable candidate selection order is:

1. hard-valid before hard-invalid;
2. lower scalar cost / higher scalar score according to the owning rule;
3. shorter quantized path/distance;
4. lower `StableId`;
5. lower quantized X;
6. lower quantized Y;
7. lower quantized Z.

Sort candidate collections explicitly before authoritative selection. Any associative container used during planning is converted to a stable sorted vector before a result is selected or hashed.

Quantization used by this rewrite is fixed to the repository's existing generated-world convention:

```text
position/height/distance: 0.001 world unit
angles:                   0.001 radian
normalized 0..1 scalars:  0.000001
```

If the source tree exposes a helper already implementing these exact quantizers, reuse it. If it does not, implement one shared helper and use it everywhere in this rewrite. Raw floating-point bit patterns are never used for generated-world checksums.

### Hybrid planning-input hash

The native hybrid path adds one canonical `HybridPlanningInputHash`. It is computed from a stable field-order serialization of **every input that can change candidate selection, repair, acceptance or authoritative world state**:

```text
world dimensions/chunk size/sea level
boundary settings
all active hybrid `plan` fields
terrain settings consumed by this rewrite
water settings consumed by this rewrite
active world-feature toggles
resource/Tiberium settings consumed by this rewrite
road grading limits derived/used by this rewrite
all fairness fields consumed by generation/validation
retry.max_local_retries_per_stage
retry.max_alternate_candidates_per_feature
procedural generator/schema version
content/descriptor revision and structural asset availability ids
```

Explicitly exclude wall clock, watchdog elapsed time, worker count, inert legacy city/settlement fields and render-only non-colliding cosmetic availability. The hybrid manifest/network identity stores this hash in addition to any legacy repository profile hash. For the hybrid path, this rule **supersedes older documentation that marked fairness or retry limits as non-checksum-affecting**, because those values can change which candidate or effective seed is accepted. Legacy/authored generation keeps its existing hash behavior.

## 0.4 Units and strategic frame

Unless a field explicitly says otherwise:

- XY is horizontal and Z is vertical;
- distances are Renegade world units;
- use `WorldMetricsClass::Get_World_Units_Per_Meter()` when physical metres are required;
- normalized strategic coordinates use `(u,v)` where `u=-1` is the GDI side, `u=+1` is the Nod side, `v=-1/+1` are lateral map edges and `(0,0)` is the playable center;
- `u` follows the seeded GDI-to-Nod strategic axis;
- `v` is its +90-degree perpendicular.

## 0.5 Canonical reusable constants

Define these once in the generator parameter table:

```text
H = actual lateral collision width of the shipped GDI Humvee
C = max(0.35 * WorldMetricsClass::Get_World_Units_Per_Meter(), H * 0.15)
W = max(H + 2*C, H * 1.30)
A = clamp(world.chunk_size / 4, 8.0, 16.0)            // analysis-grid spacing
S = clamp(world.chunk_size / 2, 16.0, 32.0)           // coarse semantic-matrix spacing
Q = 0.001                                              // geometry quantization, world units
D = min(world.width, world.height)
R_BASE = plan.base_radius_cells * world.chunk_size
R_EXPAND = sqrt(fairness.min_base_expansion_area / pi)
R_BASE_PROTECTED = max(R_BASE,R_EXPAND)
```

For the shipped Renegade Humvee content the expected `H` is `2.556` world units and the expected `W` is `3.3228` world units. Resolve `H` from the canonical collision/model metric cache during preflight. If that metric cannot be resolved, fail procedural-generation preflight with `MISSING_HUMVEE_COLLISION_METRIC`; do not guess another vehicle width.

For a 60-unit chunk:

```text
A = 15
S = 30
```

Therefore a 1200x1200 MEDIUM world uses:

```text
40 x 40 coarse semantic cells
80 x 80 analysis cells
```

and a 2400x2400 LARGE world uses:

```text
80 x 80 coarse semantic cells
160 x 160 analysis cells
```

Do not create a full-resolution planning grid at one-world-unit spacing.

## 0.6 Supported interactive size envelope

Size class is **derived from configured world dimensions**. It is never randomly selected.

Let `D = min(world.width, world.height)`:

| Class | Rule |
|---|---|
| SMALL | `D < 900` |
| MEDIUM | `900 <= D < 1800` |
| LARGE | `1800 <= D <= 2400` |

The shipped reference dimensions remain 600x600, 1200x1200 and 2400x2400.

Interactive procedural generation MUST reject the request before Phase 0 when any of these are true:

```text
world.width  > 2400
world.height > 2400
world.width * world.height > 5,760,000
world.width  <= 0
world.height <= 0
```

Oversize generation is allowed only through an explicit developer/offline command-line or tool path. It is never reachable from the normal procedural-map menu or normal dedicated-server procedural-map selection.

## 0.7 City/settlement exclusion

For this rewrite:

- do not generate cities;
- do not generate towns/villages;
- do not generate civilian building parcels;
- do not generate industrial settlement districts;
- do not route roads because of a city/settlement target;
- do not reserve city/settlement quotas;
- do not fail generation because city assets are absent.

If legacy profiles contain settlement/city fields, keep the parser source-compatible but ignore those fields on the native hybrid generator path and emit one concise deprecation/debug message. Do not carry their values into the world checksum because they have no effect in this generator version.

A future city feature may consume the semantic/vector interfaces defined here, but it must be implemented under a later generator-version bump.

---

## 0.8 No-inference preflight and missing-data rules

Before runtime Phase 2, perform one preflight that resolves every content/runtime metric required by later phases. The agent does not invent missing numbers.

Required preflight data:

```text
Humvee collision width/height
heavy reference vehicle collision width/height
Harvester collision width/height
registered bridge definitions plus existing `BridgePlannerParams`/BridgeSystem metrics for clear width, maximum span, structural depth, water-clearance margin and required approach length
registered tunnel definitions and their clear width/height/portal data when tunnels are enabled
starting-building footprint proxies from the configured starting-base descriptors
canonical world-units-per-meter value
existing `BaseSelectionPlanner` fairness-reference vehicle speed value/accessor
existing `CombatSpaceValidator` fairness-reference infantry speed value/accessor
existing economy `HarvesterLoadAmount` value/accessor used by Tiberium income projection
terrain min/max elevation and max ordinary slope from the profile
all fairness thresholds used by validators
```

Preflight behavior is exact:

- missing Humvee, heavy-vehicle or Harvester metrics -> reject the procedural request;
- no registered bridge definition/BridgePlanner parameter set capable of carrying the heavy route class -> reject the procedural request because river/canyon bridges are mandatory;
- tunnels enabled but no tunnel definition/runtime tunnel geometry -> set `tunnel_available=false`; tunnel remains an optional crossing method and is skipped;
- `plan.allow_fords=true` requires an existing canonical vehicle/water traversability query that returns a maximum legal water depth/fordability result for the route's required vehicle capability; cache that query/result in preflight. If no such canonical capability exists, reject that profile with `FORD_CAPABILITY_UNAVAILABLE`; do not invent a depth;
- missing starting-building footprint proxy -> reject the procedural request;
- if a fairness-reference speed is currently an inline literal rather than a reusable symbol, Phase 0 extracts that **same existing numeric value unchanged** into the shared metric cache; if no current value can be found in the owning validator, reject preflight with `MISSING_FAIRNESS_REFERENCE_SPEED` rather than inventing one;
- if `HarvesterLoadAmount` cannot be resolved from the existing economy/Tiberium owner, reject preflight with `MISSING_HARVESTER_LOAD_AMOUNT`;
- missing decorative foliage/lamp model -> omit only that decorative type; never fail structural generation;
- missing city/settlement models -> irrelevant because city/settlement generation is disabled;
- unsupported explicit biome asset set -> reject that explicit biome profile; `biome=auto` filters that biome out before selection. A biome is "available" only when the current terrain-material registry/profile can resolve all semantic slots required below: `temperate={grass_or_soil, exposed_rock}`, `desert={sand_or_dry_soil, exposed_rock}`, `snow={snow_ground, exposed_rock}`, `tiberium_wasteland={barren_or_cracked_ground, exposed_rock}`. If more than one existing material matches a semantic slot, select lower material `StableId`; do not create or rename texture assets in this rewrite.

No phase is allowed to stop and ask what value to use.

## 0.9 Phase ownership, locks and object lifecycle

The runtime phases in Section 8 are a strict dependency chain. A phase starts only after the previous phase's gate passes.

Each successful phase freezes the decisions named by its gate. Later phases may add derived data but may not reopen an earlier topology decision. The only exceptions are the explicit local-repair operations listed in the phase that owns the affected feature.

Lock points:

```text
Phase 2  locks map identity
Phase 3  locks strategic roles and route-family intent
Phase 4  locks hard semantic reservations
Phase 5  locks landform vectors
Phase 6  locks water vectors/polygons
Phase 7  locks macro terrain before roads
Phase 8  locks base/resource anchors
Phase 9  locks transportation topology/road classes
Phase 10 locks preliminary road splines
Phase 11 locks crossing/junction solution types and locations
Phase 12 locks structural road/crossing geometry and road cut/fill
Phase 13 locks Tiberium footprints/access envelopes
Phase 14 locks starting-base footprint/production envelopes
Phase 15 locks final gameplay terrain geometry
Phase 16 locks material masks only
Phase 17 locks decoration placement only
Phase 18 builds final integration/navigation from the already locked world
Phase 19A validates read-only; Phase 19B performs only the transactional activation/commit actions explicitly listed in Phase 19 and never changes generated-plan decisions
```

Heavy/runtime object lifecycle is also fixed:

- Phases 2-11 use lightweight records, splines, polygons, bounds and proxy metrics only.
- Phase 7 is allowed to create/update the canonical terrain heightfield because later routing requires real terrain heights.
- Phase 12 realizes accepted roads, bridges, overpasses and tunnels through their canonical owners exactly once.
- Phase 13 realizes Tiberium field masks/data exactly once.
- Phase 15 may make only the explicitly bounded terrain/cover changes described there.
- Phase 17 may instantiate only decoration that passed proxy collision/exclusion checks.
- Phase 18 does **not** instantiate roads/bridges/tunnels a second time. It finalizes collision/spatial registration for already accepted structural objects, instantiates remaining accepted non-gameplay static proxies if needed by the current integrator, and performs the single global navigation build.
- Phase 19A never repairs or mutates generated-world data. After 19A passes, Phase 19B may instantiate only the already-frozen starting buildings/spawn state as a commit transaction; it performs no placement search, terrain edit, route edit or topology change. A 19A validation failure ends the attempt and enters Section 17 retry handling; a 19B commit failure rolls the attempt back and returns bootstrap failure as specified in Phase 19.

## 0.10 Canonical fairness semantics

No phase may reinterpret a fairness field. Resolve two existing legacy fairness speeds during preflight:

```text
V_REF = exact existing BaseSelectionPlanner fairness-reference vehicle speed
I_REF = exact existing CombatSpaceValidator fairness-reference infantry speed
```

Use those values unchanged for this generator version. Do not substitute a vehicle preset's theoretical top speed and do not tune either value during this rewrite.

Define these helpers:

```text
PlanningDistance(a,b) = shortest path distance on the currently frozen strategic/topology graph,
                        using quantized straight-line edge lengths before exact A* routing exists
FinalDistance(capability,a,b) = shortest distance on the Phase-18 authoritative capability graph
VehicleTime(d) = d / V_REF
InfantryTime(d) = d / I_REF
PercentDifference(a,b) = 100 * abs(a-b) / max(max(abs(a),abs(b)), Q)
```

The fields mean exactly this on the hybrid path:

| Fairness field | Exact hybrid meaning | Hard gate |
|---|---|---|
| `max_initial_resource_travel_time_difference_seconds` | absolute difference between GDI and Nod one-way times from base to their own HOME field, using the owning economy route and `V_REF` | yes, provisional in Phase 8 and authoritative in Phase 19 |
| `max_projected_initial_income_difference_percent` | `PercentDifference(IPM_GDI,IPM_NOD)`, where `IPM = HarvesterLoadAmount * 60 / max(2*home_field_one_way_time,Q)` | yes in Phase 19 |
| `max_base_to_front_travel_time_difference_seconds` | absolute difference between each base's shortest `V_REF` time to any CONTESTED field | yes, provisional in Phase 8 and authoritative in Phase 19 |
| `max_base_to_base_travel_time_difference_seconds` | legacy name retained; on this generator it is the **maximum allowed one-way GDI-to-Nod heavy-route time**, not a difference of two symmetric measurements | yes in Phase 19 |
| `min_base_to_base_travel_time_seconds` | minimum allowed one-way GDI-to-Nod heavy-route time | yes in Phase 19 |
| `min_vehicle_approach_count` | minimum distinct heavy-capable base exit sectors/corridors | yes |
| `min_infantry_approach_count` | minimum distinct infantry-capable base approach corridors after Phase 18 | yes |
| `min_base_expansion_area` | minimum clear Commander expansion area at each base, using the existing owner's area calculation | yes |
| `allow_single_bridge_dependency` | when false, removing any one physical bridge from the final heavy graph must not disconnect GDI from Nod | yes |
| `max_mandatory_exposed_infantry_run_distance` | maximum contiguous mandatory infantry-route distance reported exposed by `CombatSpaceValidator` | yes |
| `max_mandatory_exposed_infantry_run_time_seconds` | the same exposed segment distance divided by `I_REF` | yes |
| `preferred_cover_interval` | target spacing used to place/test Phase-15 cover candidates; it is not by itself a final hard failure if both exposed-distance/time hard gates pass | soft placement target only |
| `min_vehicle_lane_clearance` | clearance added on both sides of swept vehicle envelopes everywhere this plan names it | yes |

Phase-8 provisional fairness uses `PlanningDistance`; Phase-19 authoritative fairness uses `FinalDistance`. A Phase-8 provisional failure rejects that candidate anchor pair before roads are built. Phase 19 never substitutes the provisional value for the final value. Because the Section-5 quotas guarantee at least one contested field, the front-time check is never skipped on a valid hybrid map.

# 1. Preserve existing engine/runtime value

The rewrite changes planning and compilation. It does not create replacement runtime owners.

**Preserve and reuse these canonical realization/runtime owners:**

- `WorldTerrainSystem` for authoritative terrain heights, patches, collision and chunk registry;
- `WaterSystem` for water masks, carve/shape and water geometry;
- `RoadSystem` for road ribbons, grading hooks, road masks and junction realization;
- `BridgeSystem` for bridge geometry/collision/damage/traversability;
- `TunnelSystem` for tunnel geometry/collision/traversability when enabled;
- `EscarpmentSystem` for explicit cliff/escarpment realization;
- `FoliageSystem` for clustered foliage/prop instances;
- `PathfindClass` / current canonical navigation-capability owner;
- current minimap/shroud/generated-world manifest/network/save-load/Commander/logistics owners;
- existing deterministic seed service and descriptor registries.

**Rewrite or substantially adapt these planning layers on the native hybrid path:**

- `MapPlanGenerator` -> semantic-matrix planner described here;
- strategic-site planning -> Section 8 Phase 3 graph;
- `HydrologyGenerator` planning -> vector/basin plan while still realizing through `WaterSystem`;
- `RoadGraphGenerator` topology/routing -> cyclic route families + bounded heading-aware A*;
- `BridgePlanner` selection -> unified interaction solver while still realizing through `BridgeSystem`;
- base/resource candidate ranking -> consume semantic reservations and realized terrain;
- Tiberium placement/access -> role-based rules in Phase 13;
- biome/decor planning -> global-theme rules in Phases 16-17.

`SettlementGenerator` is not called by the native hybrid path in this generator version.

Public runtime APIs used by authored maps and non-procedural callers are preserved. Do not rename/remove an existing public API merely to make the hybrid planner cleaner; add an adapter at the planner boundary when needed.

The target flow is exactly:

```text
intent -> semantic plan -> vector features -> interaction solving -> structural geometry -> tactical refinement -> materials/decoration -> one navigation build -> validation -> world activation
```

# 2. Core architecture

The generator is a small deterministic world compiler with five representations.

## 2.1 Representation A: strategic graph

The strategic graph contains gameplay meaning, not geometry.

Required site/role types:

```text
GDI_BASE
NOD_BASE
FRONT_CENTER
HOME_TIBERIUM_GDI
HOME_TIBERIUM_NOD
CONTESTED_TIBERIUM
REMOTE_TIBERIUM
ROUTE_WAYPOINT
ROAD_JUNCTION          // derived in Phase 9
CROSSING_OBLIGATION    // derived in Phase 9
```

Required edge/intent types:

```text
MAIN_ROUTE
FLANK_ROUTE
RESOURCE_ROUTE
OPTIONAL_CROSS_LINK
INFANTRY_SHORTCUT
```

The graph records exactly:

```text
required connectivity
required separation/bottleneck independence
required alternate-route relationships
resource-role ownership and attachment intent
```

It does not contain final terrain vertices or final road meshes.

## 2.2 Representation B: coarse semantic matrices

Use cache-friendly structure-of-arrays matrices at spacing `S`.

Required layers:

```text
base_elevation_tier[cell]
macro_height_bias[cell]
land_water_intent[cell]
landform_mask[cell]
reservation_mask[cell]
transport_influence[cell]
resource_influence[cell]
biome_id[cell]
roughness_class[cell]
no_decoration_mask[cell]
```

Required composable bit flags include:

```text
LAND
WATER_BASIN
COAST
RIDGE
PLATEAU
HILL
CLIFF_BAND
CANYON_BAND
RAVINE_BAND
BASE_CORE
BASE_EXPANSION
TIBERIUM_RESERVE
MAJOR_TRANSPORT_CORRIDOR
SECONDARY_TRANSPORT_CORRIDOR
CROSSING_WINDOW
RAMP_PASS_WINDOW
NO_DECORATION
```

Do **not** encode the whole cell as one mutually exclusive number. A cell can simultaneously be snow-themed, inside a road corridor and adjacent to a cliff band. Keep separate layers/bitmasks.

The semantic matrix is the fast macro planning surface. It is not the final mesh.

## 2.3 Representation C: vector feature graph

Continuous features are vectors, not raster staircases.

Use splines/polylines/polygons for:

```text
RiverSpline
RoadSpline
HighwaySpline
CanyonSpline
RavineSpline
CliffSpline
RidgeSpline
LakePolygon
CoastPolygon
TiberiumPolygon
BaseReservePolygon
CrossingWindow
```

Each vector feature has:

```text
StableId
feature type
semantic owner
bounds
priority
required/optional flag
geometry parameters
source strategic edge/site when applicable
```

Store every planning vector feature in one dedicated `ProceduralPlanningSpatialHash`. This is compiler scratch data, not a second runtime world-spatial subsystem. Bucket size is fixed to:

```text
B_HASH = max(4A, 2S)
```

`B_HASH` is used only by `ProceduralPlanningSpatialHash`. It is deliberately not named `H`; `H` is reserved globally for Humvee collision width in Section 0.5.

Each feature is inserted into every bucket touched by its quantized AABB. Bucket contents are sorted by `StableId` before queries return results. Phase 18 discards this planning hash after final validation data is built. Never compare every road against every river/cliff/road globally.

## 2.4 Representation D: compiled interaction graph

Before heavy geometry is created, detect semantic/vector interactions.

Examples:

```text
ROAD x RIVER   -> bridge/ford/reroute crossing problem
ROAD x CANYON  -> bridge/pass/descent/tunnel/reroute problem
ROAD x CLIFF   -> cut/pass/tunnel/reroute problem
ROAD x ROAD    -> junction/overpass problem
RIVER x BASIN  -> lake widening candidate
ROAD x BASE    -> protected entrance/exit alignment problem
ROAD x TIBERIUM -> service-access edge but no carriageway through field core
```

Every required conflict is solved here and becomes a first-class compiled record. No later renderer/terrain pass is allowed to casually discover a required crossing for the first time.

## 2.5 Representation E: adaptive refinement patches

Do not maintain an enormous globally high-resolution planning map.

Create temporary local refinement patches only around accepted complex features:

```text
road corridors
bridge approaches
road intersections
river banks
lake shores
cliff edges
canyon/ravine walls
base cores/exits
Tiberium harvest approaches
```

Patch spacing:

| Region | Planning/refinement sample spacing |
|---|---:|
| ordinary open terrain | `A` |
| ridge/hill general area | `A/2` |
| river/lake shore | `min(W, A/2)` |
| road corridor | `min(W/2, A/3)` |
| bridge/tunnel/overpass approach | `min(W/3, A/4)` |
| base entrance/production corridor | `min(W/2, A/3)` |

These are temporary compiler samples. Final terrain remains the existing `WorldTerrainSystem` rectangular heightfield/patch topology. This rewrite does not replace that renderer/collision topology with a quadtree mesh.

Release each temporary patch after its accepted deformation has been committed.

---

# 3. Map-start runtime contract

Procedural generation happens while a new procedural map is loading.

State sequence:

```text
PROCEDURAL_REQUESTED
 -> ordinary map/bootstrap preparation
 -> PROCEDURAL_GENERATING
 -> PROCEDURAL_VALIDATING
 -> manifest commit
 -> WORLD_ACTIVE
 -> gameplay begins
```

Before `WORLD_ACTIVE`:

- players do not spawn into the battlefield;
- match timer does not run;
- AI does not run;
- economy does not tick;
- Harvesters do not run;
- Commander gameplay does not run;
- runtime terrain mutations do not execute.

Client loading UI must remain responsive. Dedicated server logs bounded progress.

## 3.1 Performance targets

| Size | Normal target | P95 target | Hard watchdog | Max transient generation memory |
|---|---:|---:|---:|---:|
| SMALL | <=3 s | <=5 s | 15 s | 128 MiB |
| MEDIUM | <=8 s | <=12 s | 30 s | 256 MiB |
| LARGE | <=20 s | <=30 s | 60 s | 512 MiB |

The watchdog covers the entire request, including whole-world retries. It does not reset per retry.

Wall-clock timeout may abort/report generation, but MUST NOT influence authoritative candidate selection or produce a different map for the same seed on slower hardware.

## 3.2 Concurrency

Use at most four expensive generation workers.

Parallel jobs are allowed only when:

- their inputs are immutable or partitioned;
- each job writes private output;
- authoritative merge order is deterministic;
- one-worker and max-worker checksums are identical.

Never create one thread per chunk, spline, candidate or route.

## 3.3 Resource rules

Forbidden during normal interactive generation:

- subprocesses;
- external tools;
- network downloads;
- temporary intermediate files;
- GPU compute requirement;
- thousands of erosion iterations;
- full-resolution one-unit planning grids;
- all-pairs feature intersection scans;
- repeated full navigation rebuilds during planning;
- instantiating render/physics objects merely to score candidates.

Cache immutable model/definition metrics as lightweight records before generation begins.

---

# 4. Native profile and randomization controls

The native hybrid profile uses this exact active `plan` block:

```json
"plan": {
  "enabled": true,
  "symmetry": "rotational",
  "archetype": "auto",
  "biome_mode": "auto",
  "biome": "auto",
  "secondary_biome": "auto",
  "mixed_biome_probability": 0.125,
  "tier_count": 3,
  "tier_step": 18.0,
  "detail_weight": 0.35,
  "base_radius_cells": 3,
  "flank_lanes": 1,
  "water_fraction": 0.08,
  "boundary_margin_cells": 2.0,
  "road_density": 0.65,
  "island_chance": 0.20,
  "route_redundancy": 2,
  "allow_fords": false
}
```

Unrelated existing world/terrain/water/fairness/resource settings remain in the profile and retain their documented meanings unless this file explicitly replaces their procedural-planner interpretation.

Active hybrid-plan field semantics are exact:

| Field | Hybrid rule |
|---|---|
| `symmetry` | legal values `rotational`, `mirror_x`, `none`; semantics in Section 6.1 |
| `archetype` | exact legal list in Section 6; `auto` is uniform across the filtered legal list |
| `biome_mode` | `single`, `mixed`, `auto` as below |
| `biome` | primary biome or `auto` |
| `secondary_biome` | secondary mixed biome or `auto` |
| `mixed_biome_probability` | `[0,1]`; one `map_plan` hash draw decides `auto` mode |
| `tier_count` | integer `2..5`; number of semantic elevation tiers |
| `tier_step` | `>0`; exact elevation difference between adjacent semantic tiers |
| `detail_weight` | `[0,1]`; multiplies only Phase-7 continuous detail amplitude; it never changes feature topology |
| `base_radius_cells` | integer `>=1`; units are **world chunks**, preserving the legacy field meaning; base hard-reserve radius is `base_radius_cells * world.chunk_size` |
| `flank_lanes` | integer `0..2`; contributes to route-family count by the formula below |
| `water_fraction` | hybrid-native range `0.03..0.20`; target fraction for inland standing-water area, excluding sea and narrow river channels; archetype-required water may exceed it |
| `boundary_margin_cells` | `>=0`; units are **world chunks**; hard role centers must remain this many chunks inside the placeable boundary after their own reserve radius is included |
| `road_density` | `[0,1]`; controls only optional cross-links using Section 5's formula |
| `island_chance` | `[0,1]`; independent deterministic Bernoulli test for each size-class island slot |
| `route_redundancy` | integer `2..3`; minimum heavy strategic route-family count |
| `allow_fords` | bool; default false; even when true, a ford never satisfies a bridge quota and `HIGHWAY_4` never uses a ford |

Legacy `plan.lane_width_cells` remains parseable for old schema/profile files but is inert on the hybrid path because road widths are derived from actual vehicle metrics. It remains part of an old profile hash according to the repository's schema compatibility rules; the native hybrid profile does not write it.

Route-family count is fixed to:

```text
route_family_count = clamp(max(route_redundancy, 1 + flank_lanes), 2, 3)
```

## 4.1 Biome mode

Allowed values:

```text
auto
single
mixed
```

`auto` uses exactly one draw:

```text
r = Hash01(map_plan, "biome_mode")
mode = mixed when r < mixed_biome_probability, otherwise single
```

## 4.2 Single-biome maps

Allowed primary biome values:

```text
auto
temperate
desert
snow
tiberium_wasteland
```

For `biome=auto`, the selectable primary list is exactly `[temperate, desert, snow, tiberium_wasteland]` in that order; the literal token `auto` is **never** an element of the draw list. Remove any entry whose required material set failed preflight. When `biome_mode` resolves to `mixed`, also remove any primary entry that has no available compatible secondary under Section 4.3. Select `PickIndex(count, Hash01(map_plan,"primary_biome"))`. If the resulting list is empty, reject preflight.

One selected biome owns all ordinary playable land. Only these semantic material overrides can replace it locally:

```text
road/pavement
water/wet shore
exposed cliff rock
Tiberium core/contamination
required base-specific surface owned by an existing starting-building descriptor
```

## 4.3 Mixed-biome maps

Allowed automatic pairs are exactly:

```text
temperate + snow
temperate + desert
temperate + tiberium_wasteland
desert + tiberium_wasteland
snow + tiberium_wasteland
```

`snow + desert` is rejected by this generator version even when supplied explicitly. Adding it requires a later generator-version bump.

The pair table is **unordered** for compatibility. For example, `tiberium_wasteland + temperate` is the same legal pair as `temperate + tiberium_wasteland`; selection/order still records one biome as primary and one as secondary. Primary and secondary must be different.

Selection rules:

1. resolve primary biome by Section 4.2;
2. if `secondary_biome != auto`, require that secondary asset set to be available, require it differ from primary, and require the unordered pair to occur in the table above; otherwise reject the profile before generation;
3. if `secondary_biome == auto`, form the stable ordered compatible-secondary list by scanning `[temperate, desert, snow, tiberium_wasteland]`, excluding primary, unavailable assets and illegal unordered pairs; select exactly one with `PickIndex(count, Hash01(map_plan,"secondary_biome"))`; an empty list is a preflight failure;
4. create one cubic Bezier divider running from `v=-1` to `v=+1` in strategic coordinates;
5. endpoint `u` values are `Range(-0.15,+0.15,hash)` and control-point `u` values are `Range(-0.30,+0.30,hash)`, each from its own named keyed draw;
6. rasterize the divider to the coarse semantic matrix;
7. each biome must own at least 35% of playable land; if it does not, translate all divider control/end `u` values toward zero by `0.05` per deterministic iteration, at most six iterations;
8. if the 35% condition still fails, replace the divider with the straight line `u=0`;
9. transition width is exactly `max(4S,8W)` centered on the divider.

Tiberium contamination remains an overlay and never counts as the second biome.

# 5. Mandatory feature enablement and quotas

The native hybrid generator is intentionally opinionated because the requested C&C battlefield language requires water, bridges, roads, Tiberium and hard terrain divisions.

For `plan.enabled=true` on the native hybrid path, profile validation requires:

```text
water.rivers = true
water.lakes = true
world_features.highways = true
world_features.tiberium_fields = true
```

If any is false, reject the profile with `HYBRID_REQUIRED_FEATURE_DISABLED`. Do not silently remove the corresponding quota.

Optional toggles behave as follows:

```text
water.coastline=false      -> exclude coastal_peninsula from auto archetype list; no exterior sea
water.ponds                -> parsed for compatibility; no mandatory pond quota in this rewrite
world_features.tunnels     -> enables/disables tunnel as a crossing fallback
world_features.forests     -> enables/disables tree/bush/grass placement; rock props remain legal
world_features.settlements -> ignored on hybrid path under Section 0.7
```

Minimum required feature counts:

| Feature | SMALL | MEDIUM | LARGE |
|---|---:|---:|---:|
| main bases | 2 | 2 | 2 |
| home Tiberium fields | 2 | 2 | 2 |
| contested fields | 1 | 3 | 5 |
| remote-rich fields | 1 | 1 | 2 |
| river systems | 1 | 1 | 2 |
| true lakes | 1 | 1 | 2 |
| explicit interior cliff runs | 1 | 2 | 4 |
| ravine/canyon systems | 1 | 1 | 2 |
| independent heavy strategic route families | 2 | 2 | 2 |
| river bridges | 1 | 2 | 3 |
| canyon/ravine bridges | 1 | 1 | 2 |
| highway corridors | 0 | 1 | 2 |
| highway road-over-road overpasses | 0 | 1 | 2 |
| dirt/service-road branches | 2 | 4 | 8 |
| `TRAIL_1` shortcuts | 1 | 2 | 4 |

A counted feature must meet these minimum qualifications:

```text
river system       = one distinct source-to-sink channel network; tributaries do not count separately
true lake          = one non-sea closed polygon with area >= max(4*A*A, 0.002 * world area)
cliff run          = continuous explicit cliff spline length >= max(8W, 0.12D)
canyon/ravine      = centerline length >= max(12W, 0.20D) and depth >= 0.75*tier_step
highway corridor   = connected HIGHWAY_4 spline length >= 0.30D
river bridge       = distinct bridge whose primary obstruction is RIVER
canyon bridge      = distinct bridge whose primary obstruction is CANYON or RAVINE
overpass           = distinct grade-separated ROAD_X_ROAD crossing
service branch     = SERVICE_DIRT_2 edge length >= 6W
trail shortcut     = TRAIL_1 edge length >= 8W
```

A physical bridge has exactly one primary obstruction and can satisfy only one bridge-category quota.

Optional cross-links are exact:

```text
SMALL cap  = 2
MEDIUM cap = 5
LARGE cap  = 10
extra_links = RoundHalfUp(road_density * cap)
```

Optional island slots are exact:

```text
SMALL slots  = 0
MEDIUM slots = 1
LARGE slots  = 2
```

For each slot `i`, create an island only when `Hash01(map_plan,"island",i) < island_chance`. An accepted island must have dry area `>= max((16W)^2, 0.01*world area)`. Optional-island slots create decorative geography only in this generator version: no base, Tiberium field or mandatory route role is assigned to an optional island, so access is not required.

A mandatory quota failure is a named attempt failure after the exact local-repair sequence for its owning phase is exhausted. Do not reduce quotas to rescue a seed.

# 6. Archetypes

Allowed archetypes, in stable selection order:

```text
river_valley
twin_lakes
canyon_crossroads
highland_pass
coastal_peninsula
plateau_basin
ravine_network
open_frontier
```

For `archetype=auto`, start with that exact ordered list, remove `coastal_peninsula` when `water.coastline=false`, then select uniformly with `PickIndex`. No other archetype is filtered by tunnels because every tunnel use has a non-tunnel fallback.

All normalized coordinates below are in the strategic `(u,v)` frame. Any numeric range uses Section 0.3 `Range()` with a unique key derived from archetype name + feature `StableId` + parameter name.

## 6.1 Common strategic anchors and symmetry

Base guides are always opposite sides of the map:

```text
GDI base guide = (-0.72, 0.00)
```

Nod guide depends on `symmetry`:

```text
rotational -> (+0.72, 0.00), and paired role (u,v) maps to (-u,-v)
mirror_x   -> (+0.72, 0.00), and paired role (u,v) maps to (-u,+v)
none       -> (+0.72, Range(-0.18,+0.18,Hash01(map_plan,"nod_v_offset")))
```

The strategic axis world angle is:

```text
mirror_x   -> 0 radians in world XY so the mirror name retains literal X-axis meaning
otherwise  -> Range(0, pi, Hash01(map_plan,"strategic_axis_angle"))
```

Angles differing by `pi` are equivalent because team sides already provide direction.

Other guide positions:

```text
GDI home field = (-0.48, +0.18*s)
Nod home field = (+0.48, -0.18*s)
front center   = ( 0.00,  0.00)
flank A        = ( 0.00, +0.45*s)
flank B        = ( 0.00, -0.45*s)
```

where `s = PickSign(Hash01(map_plan,"flank_sign"))`.

A guide center must remain inside the placeable boundary by:

```text
boundary_margin_cells * world.chunk_size + that role's reserve radius
```

Role reserve radius is exact: base=`R_BASE_PROTECTED`, resource=`resource.FieldRadiusMax`, route/analysis waypoint=`0` for boundary testing. These values are known before Phase 3; do not defer base-reserve sizing to later terrain phases.

A guide may move during plan-stage legality repair by at most `0.10D` from its original world position. Candidate offsets are tested in concentric rings at radii `A,2A,3A,...` up to `0.10D`, with 16 angular stations per ring beginning at strategic +u and rotating counter-clockwise. First hard-valid candidate in stable score order wins. No legal candidate means the attempt fails.

## 6.2 `river_valley`

- create one principal river connecting the two `v` map boundaries;
- its endpoint-to-endpoint direction differs from the strategic `v` axis by at most 20 degrees;
- its mean centerline `u` remains within `[-0.10,+0.10]`;
- meander amplitude is `0.05D..0.12D` and wavelength is `0.25D..0.45D`;
- route family 0 crosses within normalized distance `0.10` of `(0,0)`;
- route family 1 crosses at `|v|>=0.35` with opposite sign from the nearest family-0 crossing offset;
- the nearest edge of the first contested Tiberium field reserve lies `4W..10W` from the principal river bank; its center therefore lies `resource.FieldRadiusMax + 4W..10W` from that bank.

## 6.3 `twin_lakes`

- create two required lake guide centers `(u_jitter,-0.38)` and `(u_jitter,+0.38)`, each `u_jitter` independently in `-0.05..+0.05`;
- each target effective radius is `0.18D..0.25D`;
- the final polygons must preserve a dry central corridor of width `>=10W` between their nearest shores;
- route family 0 uses that central corridor;
- route family 1 passes outside one lake and shares no lake-edge bottleneck with family 0.

## 6.4 `canyon_crossroads`

- one principal canyon crosses the strategic axis with its closest point to `(0,0)` within `0.10D`;
- route family 0 uses a canyon bridge at `|v|=0.25..0.45`;
- route family 1 uses a second physical crossing on the opposite `v` sign;
- the second crossing method follows the exact Phase-11 canyon/ravine method order, but it cannot be the same physical bridge/pass/tunnel object as family 0.

## 6.5 `highland_pass`

- one principal ridge crosses the strategic axis with its closest point within `0.12D` of center;
- family 0 uses a surface pass with `|v|<=0.15`;
- family 1 uses a distinct corridor with `|v|=0.35..0.55`;
- both bases remain on opposite sides of the ridge signed-distance field;
- the alternate corridor uses the exact Phase-11 cliff/steep-ridge method order after ordinary surface routing is tested.

## 6.6 `coastal_peninsula`

This archetype exists only when `water.coastline=true`.

- choose sea side `v_sign = PickSign(Hash01(map_plan,"coast_side"))`;
- exterior sea begins at `|v|=0.55..0.70` on that side and remains connected to the world exterior;
- both bases remain on mainland;
- SMALL creates one peninsula neck `>=10W` wide plus a separate inland heavy route family;
- MEDIUM/LARGE create one peninsula; a successful optional island slot may add one separate decorative sea island, but the required peninsula remains land-connected;
- one required river terminates in the sea.

## 6.7 `plateau_basin`

- basin center is within normalized distance `0.10` of `(0,0)`;
- basin effective radius is `0.22D..0.32D`;
- each base is on a separate plateau at the basin-rim tier or one tier above it;
- at least two heavy-vehicle descents/crossings enter the basin and are independent under Section 12.2.

## 6.8 `ravine_network`

- create one main ravine;
- create exactly `2/3/5` secondary branches for SMALL/MEDIUM/LARGE; the main ravine plus all of these attached branches is **one** ravine system for Section-5 quota/count purposes; any additional `terrain.valley_density` ravines are separate systems;
- branch endpoints may join the main ravine but no ravine polygon may intersect `BASE_PROTECTED`;
- route family 0 crosses the main ravine by bridge;
- route family 1 uses a separate physical crossing or a legal ravine-floor/pass corridor;
- a resource field may occupy a shelf only when its entire Harvester maneuver envelope remains outside the ravine floor/walls.

## 6.9 `open_frontier`

- no single hard barrier may intersect more than 60% of the playable `v` span measured at analysis spacing `A`;
- normal river/lake/cliff/ravine quotas still apply;
- after Phase 9 has computed straight-line strategic trunk lengths, the longest heavy route-family trunk is assigned `HIGHWAY_4` before any other heavy family trunk; ties use lower family id;
- optional cross-link score receives the Section-9 fixed open-frontier term;
- on MEDIUM/LARGE, create exactly three `OPEN_MANEUVER` reservation discs centered at strategic `(0,0)`, `(0,+0.42)` and `(0,-0.42)`, each radius `9W`; these discs may contain roads, Tiberium and ordinary hills/foliage, but Phase 5 may not place an immutable cliff/canyon/ravine wall through them and Phase 6 may not place lake/coast water through them; river channels may cross a disc only when they also provide a legal crossing window inside that disc;
- SMALL creates only the center `OPEN_MANEUVER` disc at `(0,0)` with radius `9W`;
- an `OPEN_MANEUVER` disc is accepted only if at least 95% of its sampled area at spacing `A` lies inside the placeable boundary; otherwise move its center with the Section-6.1 legality-repair search; failure to place a required disc fails the attempt.

## 6.10 Archetype intent records consumed by later phases

Archetype prose above is not advisory. At the end of MapIdentity resolution, build these exact immutable intent records; later phases consume them rather than reinterpreting the archetype name. Coordinates are strategic `(u,v)` before world rotation.

| Archetype | Required immutable intents |
|---|---|
| `river_valley` | `GUIDED_RIVER` from `(0,-0.90)` to `(0,+0.90)` with Section-6.2 meander parameters; route-family crossing guides `(0,0)` and `(0,-0.45*s)`; `RIVER_ADJACENT_CONTESTED` flag |
| `twin_lakes` | two `LAKE_GUIDE` records from Section 6.3; family-0 guide `(0,0)`; family-1 guide `(0,0.78*s)` so it is outside the nearer lake |
| `canyon_crossroads` | one `PRINCIPAL_CANYON` guide centered `(0,0)` with centerline approximately along strategic `v`; mandatory bridge guides `(0,+0.35*s)` and `(0,-0.35*s)`, first tagged `BRIDGE_REQUIRED`, second tagged `DISTINCT_CROSSING_REQUIRED` |
| `highland_pass` | one `PRINCIPAL_RIDGE` guide centered `(0,0)` with centerline approximately along strategic `v`; `SURFACE_SADDLE` at `(0,0)`; alternate family guide `(0,+0.45*s)` |
| `coastal_peninsula` | `COAST_GUIDE` on selected `v_sign`; `PENINSULA_NECK` centered `(0,0.35*v_sign)` with minimum dry width `10W`; `COASTAL_GUIDED_RIVER` from `(0,-0.35*v_sign)` toward the selected sea boundary |
| `plateau_basin` | `BASIN_GUIDE` centered `(0,0)`; two independent basin-entry guide points `(0,+0.35*s)` and `(0,-0.35*s)` |
| `ravine_network` | `MAIN_RAVINE_GUIDE` centered `(0,0)` along strategic `v`; family-0 crossing `(0,+0.32*s)` tagged `BRIDGE_REQUIRED`; family-1 crossing `(0,-0.32*s)` tagged `DISTINCT_CROSSING_REQUIRED` |
| `open_frontier` | the Section-6.9 `OPEN_MANEUVER` reservation centers; no extra barrier guide |

"Approximately along strategic `v`" in the table is resolved exactly as: use the generic Phase-5 line-feature control construction with overall endpoint vector parallel to strategic `v`; only the keyed interior lateral offsets create curvature. The endpoints themselves remain on that axis.

### Guided-river construction

`GUIDED_RIVER` and `COASTAL_GUIDED_RIVER` centerline intents are constructed immediately with `MapIdentity` in Phase 2, before role placement, and later promoted to mandatory hydrology systems rather than suggestions to the D8 selector. Sample the straight start-to-end axis every `A`, then apply lateral meander:

```text
lateral(t) = amplitude * sin(2*pi*(axis_distance(t)/wavelength) + phase)
amplitude  = Range(0.05D,0.12D,Hash01(hydrology,archetype,"river_amp"))
wavelength = Range(0.25D,0.45D,Hash01(hydrology,archetype,"river_wave"))
phase      = Range(0,2*pi,Hash01(hydrology,archetype,"river_phase"))
```

For `river_valley`, clamp the lateral position so strategic `u` stays within `[-0.10,+0.10]` after conversion to normalized coordinates. For `COASTAL_GUIDED_RIVER`, multiply amplitude by `0.5` and force the final point to the selected sea boundary. The Phase-2 guided-river intent is **not moved around bases later**. Instead Phase 3 treats its river influence band (centerline plus `0.5*water.river_width + 2W`) as a hard unavailable region for base/resource role placement; role-guide repair moves the role, preserving the archetype river.

Phase 2 stores the resulting quantized spline in `ArchetypeIntentSet`. The hydrology phase promotes that same frozen spline **before** selecting D8-derived additional river systems. A guided system counts toward the Section-5 river quota. D8 systems then fill only the remaining quota and must satisfy the normal source-separation rule from the guided river's source as well.

### Highland surface saddle

When realizing the `PRINCIPAL_RIDGE` field, compute along-ridge distance `d_pass` from the `SURFACE_SADDLE` point. Multiply ridge amplitude by:

```text
0.15                                         when d_pass <= 4W
lerp(0.15,1,smoothstep((d_pass-4W)/(4W)))   when 4W < d_pass < 8W
1.0                                          when d_pass >= 8W
```

The saddle footprint must remain wide enough for `PRIMARY_2` roadbed plus both shoulders and `2*fairness.min_vehicle_lane_clearance`. If not, widen the `0.15` zone symmetrically until it is. This is the only mandatory surface saddle; the alternate family corridor is solved normally by routing/crossing logic.

### River-adjacent contested field

For `RIVER_ADJACENT_CONTESTED`, place the first CONTESTED role during Phase 3 from the already-frozen `GUIDED_RIVER` intent, so no resource center moves after semantic reservations exist. Sample guided-river intent stations from its midpoint outward at spacing `A`, tie negative along-river offset before positive. At each station:

1. compute the local river tangent and its left/right normals;
2. test left then right;
3. provisional bank offset is `0.5*water.river_width`;
4. candidate center offset from river centerline is `0.5*water.river_width + resource.FieldRadiusMax + 7W`; therefore the reserve edge is exactly `7W` from the provisional bank, inside the Section-6.2 `4W..10W` requirement;
5. candidate must be inside the placeable boundary by its full reserve radius, outside both base reserves, outside every non-river archetype hard intent and at least `2*resource.FieldRadiusMax` from another resource center;
6. first hard-valid candidate wins.

If no candidate exists along the frozen guided-river intent, the attempt fails in Phase 3. Phase 6 does not relocate the field later.


# 7. Road classes

Use these semantic cross sections:

| Class | Lanes | Lane width | Shoulder | Median | Max grade | Max cross slope | Minimum turn radius |
|---|---:|---:|---:|---:|---:|---:|---:|
| `HIGHWAY_4` | 4 | `W` | `0.45W` each | `0.75W` | 8 deg | 6 deg | `8W` |
| `PRIMARY_2` | 2 | `W` | `0.30W` each | 0 | 12 deg | 8 deg | `5W` |
| `SECONDARY_2` | 2 | `W` | `0.20W` each | 0 | 14 deg | 10 deg | `4W` |
| `SERVICE_DIRT_2` | 2 | `W` | `0.20W` each | 0 | 18 deg | 12 deg | `3W` |
| `TRAIL_1` | 1 | `W` | `0.10W` each | 0 | 22 deg | 14 deg | `2W` |

Exact total roadbed widths before outer terrain blend are:

```text
HIGHWAY_4       = 4W + 2(0.45W) + 0.75W = 5.65W
PRIMARY_2       = 2W + 2(0.30W)         = 2.60W
SECONDARY_2     = 2W + 2(0.20W)         = 2.40W
SERVICE_DIRT_2  = 2W + 2(0.20W)         = 2.40W
TRAIL_1         = 1W + 2(0.10W)         = 1.20W
```

The existing three physical/render road tiers remain the realization owner. Map semantic classes by capability, not by inventing new rendering systems:

```text
HIGHWAY_4                    -> existing widest/highway physical tier
PRIMARY_2, SECONDARY_2      -> existing normal paved physical tier
SERVICE_DIRT_2, TRAIL_1     -> existing dirt/track physical tier
```

If the source enum names differ, identify the tiers through the existing `Road_Graph_Class_To_Road_Class` mapping/registered widths and preserve those enum names. The semantic width above is still passed as the actual spline/ribbon width when the canonical RoadSystem API supports explicit width. If the RoadSystem stores width in its road definition instead, create/update the hybrid definitions once during init; do not approximate the width with a different lane count.

`TRAIL_1` never satisfies heavy-vehicle route redundancy. `SERVICE_DIRT_2` may carry a Harvester only after the actual Harvester swept-envelope validator passes it.

## 7.1 Shared road-planning geometry helpers

These helpers are defined before runtime generation and are used by hydrology, routing and crossing compilation; no earlier phase needs to wait for Phase 10/11/12 code to decide their values.

```text
RoadbedWidth(HIGHWAY_4)      = 5.65W
RoadbedWidth(PRIMARY_2)      = 2.60W
RoadbedWidth(SECONDARY_2)    = 2.40W
RoadbedWidth(SERVICE_DIRT_2) = 2.40W
RoadbedWidth(TRAIL_1)        = 1.20W

PreferredCorridorHalfWidth(HIGHWAY_4 or PRIMARY_2) = max(6S,0.08D)
PreferredCorridorHalfWidth(SECONDARY_2)            = max(5S,0.06D)
PreferredCorridorHalfWidth(SERVICE_DIRT_2)         = max(4S,0.05D)
PreferredCorridorHalfWidth(TRAIL_1)                = max(3S,0.04D)
MaxCorridorHalfWidth(class) = PreferredCorridorHalfWidth(class) + 2S

RoadVerticalInterval(z_terrain) =
    [z_terrain-grading.max_cut_depth, z_terrain+grading.max_fill_height]

RoadOuterBlendWidth = max(W,A/2)
RoadDeformationHalfWidth(class) = 0.5*Section7TotalRoadbedWidth(class) + RoadOuterBlendWidth
```

For two route samples separated by horizontal distance `d`, intervals `I0` and `I1` are grade-compatible for class `c` iff:

```text
gap = max(0, I0.low-I1.high, I1.low-I0.high)
atan(gap/max(d,Q)) <= c.max_grade
```

This is only a planning feasibility test; the exact deterministic vertical-profile realization algorithm is specified in Phase 12.

# 8. Exact generation pipeline

**Phase 0 and Phase 1 below are one-time implementation/migration phases and never execute when a player/server starts a map. Runtime map-start generation begins at Phase 2.** Section 19 gives the coding sequence that implements all phases.

Runtime Phase 2 through Phase 19 consume only outputs from earlier runtime phases. Do not skip forward and return later to change topology. A later phase may perform a bounded local repair only when this document explicitly allows it.

## Phase 0 - repository audit and baseline

Before changing code:

1. identify canonical owners listed in Section 1;
2. identify current seed domains/version constants;
3. identify current procedural profiles and menu/server entry points;
4. identify real Humvee/heavy-vehicle/Harvester collision dimensions;
5. identify bridge/tunnel model metrics already available;
6. run existing procedural self-checks and save baseline results;
7. search all consumers of the existing planners before changing public records.

No generator behavior changes occur in this phase.

**Gate:** baseline builds/tests are recorded and canonical owners are mapped.

## Phase 1 - data model, profile migration and version bump

Implement:

- semantic matrix structures;
- strategic graph records;
- vector feature records;
- compiled interaction/crossing records;
- temporary refinement-patch records;
- biome-mode fields from Section 4;
- archetype list from Section 6;
- road semantic classes from Section 7.

Increment the checked-out `CURRENT_SCHEMA_VERSION` by exactly 1 and `CURRENT_GENERATOR_VERSION` by exactly 1 for this rewrite. Do not perform a second version bump while implementing later phases in this same specification.

Native profile removes active settlement/city controls. Legacy fields remain parser-compatible but inert under Section 0.7.

**Gate:** profile round-trip is deterministic; old profiles still load; native profile selects the hybrid generator.

## Phase 2 - choose map identity

Build one immutable `MapIdentity` record in this exact order:

```text
1 size class        = derive from Section 0.6; no random draw
2 symmetry          = exact profile value
3 archetype         = resolve explicit value or Section-6 auto selection
4 strategic axis    = Section 6.1 rule
5 biome mode        = Section 4.1 rule
6 primary biome     = Section 4.2 rule
7 secondary biome   = Section 4.3 rule only when mixed
8 island slot bits  = Section 5 Bernoulli tests
9 ArchetypeIntentSet  = exact Section-6.10 immutable intent records, including any guided-river spline and archetype guide parameters
```

Do not touch terrain geometry and do not consume any unlisted stateful RNG.

**Gate:** identical seed/profile produces identical `MapIdentity + ArchetypeIntentSet` records and checksum.

## Phase 3 - generate strategic gameplay graph

Create gameplay roles before geography. Use the guides and legality-repair search from Section 6.1. A role candidate is hard-valid only under these rules:

```text
BASE role:
  full R_BASE_PROTECTED disc is placeable
  does not intersect guided-river/coast/lake/canyon/ravine/cliff hard intent
  does not intersect the other base's R_BASE_PROTECTED disc

RESOURCE role:
  full resource.FieldRadiusMax disc is placeable
  does not intersect either R_BASE_PROTECTED disc
  does not intersect guided-river water influence, lake/coast hard intent, canyon/ravine/cliff hard intent
  does not overlap another resource reserve; minimum center separation = 2*resource.FieldRadiusMax

ROUTE waypoint:
  center is placeable or lies on its own explicit archetype crossing/pass intent
  center is outside unrelated Tiberium reserves
```

Test roles in the creation order below. When a guide is invalid, Section-6.1 bounded legality repair tests candidate offsets against every already accepted higher-priority role plus `ArchetypeIntentSet`; it never moves an already accepted earlier role.

Required site roles are created in this exact order:

```text
1 GDI_BASE
2 NOD_BASE
3 HOME_TIBERIUM_GDI
4 HOME_TIBERIUM_NOD
5 FRONT_CENTER analysis node
6 contested fields, count from Section 5
7 remote-rich fields, count from Section 5
8 route-family waypoint nodes
```

Non-home resource guide placement is fixed:

```text
SMALL contested      -> (0.00, 0.00)
MEDIUM contested     -> (0.00,0.00), (0.00,+0.32*s), (0.00,-0.32*s)
LARGE contested      -> MEDIUM set plus (0.00,+0.56*s), (0.00,-0.56*s)
first remote-rich    -> (Range(-0.20,+0.20,key), +0.70*s)
second remote-rich   -> (Range(-0.20,+0.20,key), -0.70*s) // LARGE only
```

`river_valley` replaces only the **first** contested guide with the exact Section-6.10 river-adjacent placement algorithm. All other resource guides use the table above.

Before accepting any role guide, treat the frozen `ArchetypeIntentSet` hard water/cliff/canyon/ridge/core polygons as unavailable according to their semantic type. If a table guide overlaps such a hard intent, move that guide with the Section-6.1 legality search while testing the full role reserve radius; do not delete the role.

Create `route_family_count` heavy strategic families. Default family guide bands are:

```text
family 0 -> one guide at (0, 0.00)
family 1 -> one guide at (0,+0.45*s)
family 2 -> one guide at (0,-0.45*s)   // only when route_family_count=3
```

Apply the selected Section-6.10 archetype route/crossing intents now:

- when an archetype specifies a guide for family 0 or 1, replace that family's default guide with the archetype guide;
- when it specifies two guides for one family, insert both in increasing source-to-destination `u` order;
- family 2 keeps its default guide unless Section 6.10 explicitly names it;
- create a `CROSSING_OBLIGATION_INTENT` record for every archetype guide tagged bridge/distinct-crossing/pass; store `(family_id, obstruction_kind, strategic_guide_position, required_method_or_ANY)`;
- the guide is still a strategic waypoint, not a final bridge location. Its bounded corridor is what causes the later obstruction-window selector to prefer the correct part of the river/canyon/ridge.

Each family is one complete GDI-to-Nod intent through its ordered guides. Two families may share only their source/sink base nodes at this stage; no intermediate waypoint object is shared between families. Therefore `route_family_count>=2` creates a graph cycle by construction.

Create resource intents:

- each base -> its home field;
- every contested field -> nearest heavy route-family waypoint by `(quantized Euclidean distance,family_id,waypoint StableId)`;
- every remote-rich field -> nearest heavy-route waypoint on the same signed-`v` half; if none exists, use the globally nearest under the same tie order;
- every non-home field receives one `SERVICE_DIRT_2` branch intent from that attachment point.

Trail shortcuts and optional cross-links are **not** chosen at this strategic-role stage; the transportation-topology stage constructs them only after anchors and obstruction windows exist.

The graph must contain the expected cycle from the distinct heavy families before the phase passes; a tree fails this phase.

**Gate:** exact role counts exist, `route_family_count` complete GDI-to-Nod intents exist, resource intents are attached, and at least one graph cycle exists.

## Phase 4 - create coarse semantic matrices

Allocate Section-2.2 arrays at spacing `S` in the reusable scratch arena. Matrix dimensions are `ceil(world.width/S)` by `ceil(world.height/S)` and iteration is row-major `(y,x)`.

Populate in this exact order:

1. playable/placeable boundary classification from `WorldBoundsService`;
2. initialize every playable cell to middle tier `floor((tier_count-1)/2)`;
3. rasterize archetype macro intent zones (basin/ridge/water/coast bands) without final geometry;
4. reserve each `BASE_PROTECTED` disc at radius `R_BASE_PROTECTED`; mark its inner `R_BASE` disc as `BASE_CORE`; throughout `BASE_PROTECTED` force `base_elevation_tier=middle` and `macro_height_bias=0` so later terrain honors the buildable/expansion platform intent;
5. reserve each home/non-home Tiberium role as a disc of radius `resource.FieldRadiusMax`; this field is part of the existing resource settings and its schema default applies when an older profile omits it;
6. reserve each heavy route-family corridor as a capsule with preferred half-width `max(6S,0.08D)`;
7. reserve each service/trail intent corridor as a capsule with preferred half-width `max(4S,0.05D)`;
8. when the archetype is `open_frontier`, rasterize the Section-6.9 `OPEN_MANEUVER` discs into a dedicated `NO_IMMUTABLE_BARRIER` bit layer;
9. rasterize single/mixed biome ownership;
10. stamp `NO_DECORATION` into every `BASE_PROTECTED` disc and provisional Tiberium field core.

No crossing window exists at this stage. `CROSSING_WINDOW` and `RAMP_PASS_WINDOW` records are created only after both landform and water vectors are fixed; their later insertion uses the same priority comparator below. Do not reserve guessed crossing positions now.

Hard-reservation conflicts are resolved only by moving the lower-priority role with Section-6.1 legality repair. The same priority comparator is reused when crossing-window reservations are inserted after water/landform planning:

```text
BASE_PROTECTED
required heavy route corridor
home Tiberium reserve
required crossing window
contested/remote Tiberium reserve
OPEN_MANEUVER no-immutable-barrier reservation
optional route corridor
optional island
```

`OPEN_MANEUVER` is not exclusive with roads/resources; its priority applies only to immutable landform/water conflicts. Unrelated bit layers are never erased merely because a higher-priority layer exists.

**Gate:** every required role center is inside a legal reserved region, `BASE_PROTECTED` discs do not overlap water/cliff hard-intent zones, and no unresolved hard-reservation conflict exists.

## Phase 5 - plan vector landforms

Create vector landform intent before any final terrain is built.

Target feature counts are deterministic. Archetype-required features count toward these totals:

```text
base hills   SMALL/MEDIUM/LARGE = 3 / 6 / 10
extra hills  = RoundHalfUp(terrain.hill_density * (2 / 4 / 6))
base ridges  = 1 / 2 / 3
extra ridges = RoundHalfUp(terrain.mountain_density * (1 / 2 / 3))
cliff target = Section-5 cliff quota + RoundHalfUp(terrain.interior_cliff_density * (1 / 2 / 4))
ravine target= Section-5 ravine quota + RoundHalfUp(terrain.valley_density * (0 / 1 / 2))
```

Create in this order:

```text
1 archetype principal ridge/canyon/ravine/basin/coast features
2 remaining required canyon/ravine systems
3 explicit cliff runs
4 remaining ridges
5 hills
6 optional plateau conversion of eligible hills/ridges
```

Geometry ranges:

```text
hill radius                    = 0.08D .. 0.16D
hill amplitude                 = tier_step * (0.75 .. 1.50) * (0.50 + 0.50*terrain.macro_relief)
ridge centerline length        = 0.25D .. 0.55D
ridge influence half-width     = 0.04D .. 0.09D
ridge amplitude                = tier_step * (1.00 .. 2.00) * (0.50 + 0.50*terrain.macro_relief)
cliff run length               = max(8W,0.12D) .. 0.45D
cliff height difference        = tier_step * (1.00 .. 2.00)
canyon centerline length       = 0.35D .. 0.75D
canyon floor half-width        = 1.5W .. 3.0W
canyon rim half-width          = floor_half_width + (4W .. 8W)
canyon depth                   = tier_step * (1.00 .. 2.50)
ravine floor half-width        = 1.0W .. 2.0W
ravine rim half-width          = floor_half_width + (2W .. 5W)
ravine depth                   = tier_step * (0.75 .. 1.50)
```

All values are clamped so final provisional heights stay inside `terrain.min_elevation..terrain.max_elevation` relative to sea level.

### Exact generic landform candidate construction

Archetype-required principal features use the positional/orientation constraints in Section 6. Those constraints replace only the generic center/orientation choice below; their width/depth/amplitude and curve construction still use this Phase-5 algorithm unless Section 6 explicitly fixes a value. Remaining features use this exact candidate process:

1. build a row-major vector of coarse-cell centers whose cell is placeable and whose center is outside base/Tiberium hard reserves;
2. for feature `(type,StableId)`, assign every candidate cell the key `Hash01(terrain,type,StableId,"cell",row_major_index)`, sort ascending by `(key,row_major_index)`, and test at most `retry.max_alternate_candidates_per_feature` cells;
3. candidate center is that cell center plus independently keyed X/Y jitter in `[-0.25S,+0.25S]`;
4. draw every size/amplitude/angle parameter exactly once from its Section-0.3 range using keys `(terrain,type,StableId,parameter_name)`; retries move the feature but do not redraw its dimensions;
5. reject a candidate if its full influence/protected polygon leaves the placeable boundary, intersects `BASE_PROTECTED`, intersects an incompatible Tiberium hard reserve, violates an `OPEN_MANEUVER` `NO_IMMUTABLE_BARRIER` bit for an immutable cliff/canyon/ravine wall, or reduces a required heavy-corridor dry/passable width below `4W`;
6. for two accepted independent features of the same type, overlap area of their full influence polygons must be <=25% of the smaller polygon; archetype parent/child ravine branches are exempt from this overlap rule.

Geometry construction is fixed:

```text
hill:
  ellipse aspect ratio = 0.75..1.25
  orientation = 0..pi
  influence = compact-support smooth radial ellipse

ridge/cliff/canyon/ravine line feature:
  local longitudinal control positions = -0.50L, -0.1666667L, +0.1666667L, +0.50L
  endpoint lateral offsets = 0
  interior lateral offsets = independently -0.10L..+0.10L
  curve = centripetal Catmull-Rom through those four controls
  validation/sample spacing = A/2

cliff only:
  upper-side sign = PickSign(Hash01(terrain,"cliff_upper_side",StableId))
```

For a required archetype line feature, rotate/translate this local curve so its centerline obeys the archetype crossing/axis constraint; retries use the Section-6.1 bounded legality offsets and never silently discard the required feature.

`ravine target` counts **independent ravine/canyon systems**, not child branches. In `ravine_network`, the main ravine and all Section-6.8 attached branches are one system; density-created additional ravines are separate systems.

A hill/ridge is plateau-eligible only when its proposed upper-35%-influence plateau footprint intersects none of: `BASE_PROTECTED`, Tiberium hard reserve, the center `2W` of a required heavy-route corridor, required basin/water macro intent, or `OPEN_MANEUVER` disc boundary in a way that would create an immutable wall. For each eligible hill/ridge, convert it to a plateau when `Hash01(terrain,"plateau",StableId) < terrain.plateau_chance`; plateau top uses the upper 35% of that feature's influence profile flattened with a smoothstep transition.

Do **not** create crossing/pass windows while only landforms exist. Water must be fixed first; the immediately following hydrology phase creates and validates all river/cliff/canyon/ravine crossing/pass windows in one place.

**Gate:** target cliff/ridge/hill/ravine-system counts exist, all Section-5 minimum qualifications pass, every accepted vector satisfies the exact candidate rules above, and no immutable landform intersects `BASE_PROTECTED` or violates an `OPEN_MANEUVER` no-immutable-barrier reservation.

## Phase 6 - plan hydrology as vectors and basins

Hydrology planning runs at analysis spacing `A` and uses no iterative erosion.

Exact sequence:

1. evaluate provisional height from semantic tier + Phase-5 landforms only; omit all Phase-7 detail noise;
2. add an artificial `+4*tier_step` barrier to `BASE_PROTECTED` analysis cells so drainage never routes through a base reserve;
3. run deterministic Priority-Flood from all exterior analysis-grid cells. Priority-queue key is `(filled_height,row_major_index)`. When an unvisited neighbor is first discovered, set `filled_height[neighbor]=max(original_height[neighbor],filled_height[current])`, set `drain_parent[neighbor]=current`, and assign an incrementing `flood_rank`; exterior seeds have no parent; retain the pre-fill depression/basin labels;
4. assign D8 flow direction with neighbor tie order `E,NE,N,NW,W,SW,S,SE`: choose the neighbor with the lowest **strictly lower** filled height; when no strictly lower neighbor exists, use `drain_parent`; because every equal-height parent has lower `flood_rank`, flat regions are acyclic and require no implementer-selected flat solver; exterior cells with no parent are sinks;
5. process cells in descending `(filled_height,flood_rank)` and accumulate flow into their D8 destination;
6. convert existing `water.river_accumulation_chunks` to analysis-cell threshold: `river_threshold = max(1, RoundHalfUp(water.river_accumulation_chunks * (world.chunk_size/A)^2))`;
7. mark cells with accumulation >= threshold as channel cells;
8. identify channel headwaters as channel cells with no upstream channel neighbor;
9. trace every headwater through D8 destinations to a sink, merge shared downstream chains by analysis-cell id, and rank distinct D8 source-to-sink systems by descending maximum accumulation, then descending quantized chain length, then `StableId`;
10. if Section 6.10 supplied `GUIDED_RIVER` or `COASTAL_GUIDED_RIVER`, construct that exact vector now and insert it as selected river system 0. Determine its source/sink direction as follows: coastal guide always flows toward the sea; non-coastal guide flows from the endpoint with higher provisional Phase-6 height toward the lower endpoint, tie broken from negative strategic `v` endpoint to positive. Its per-sample synthetic accumulation increases linearly from `river_threshold` at source to `4*river_threshold` at sink so the ordinary width formula remains usable;
11. let `remaining = Section5_required_river_count - guided_selected_count`. Enforce D8 source separation `>=0.18D` from every already selected source and other selected D8 source, then select `remaining` D8 systems. If too few exist, multiply `river_threshold` by exactly `0.80`, apply `max(1,RoundHalfUp(...))`, rebuild steps 7-9, preserve the mandatory guided river, and repeat selection; perform at most three threshold reductions after the initial attempt; failure after the third reduction fails the phase;
12. for each selected **D8-derived** river, construct its vector centerline from the ordered D8 cell-center chain: retain endpoints and every direction-change cell, apply exactly one interior three-point smoothing pass `p_i'=0.25*p_{i-1}+0.50*p_i+0.25*p_{i+1}`, then build a centripetal Catmull-Rom spline through the resulting points; accept the smoothing only when every spline sample at spacing `A/2` stays within `A` of the original D8 polyline and projected along-chain distance never decreases; otherwise use the unsmoothed D8 polyline as the vector centerline. Guided rivers already have their Section-6.10 spline and skip this conversion;
13. create lake candidates from retained Priority-Flood basins. A basin is legal only when its closed polygon is non-self-intersecting, its area meets the Section-5 true-lake minimum, it does not intersect `BASE_PROTECTED` or `OPEN_MANEUVER` no-water reservation, and every heavy-route corridor it intersects retains a connected dry bypass at least `heavy_vehicle_width + 2*fairness.min_vehicle_lane_clearance` wide inside `MaxCorridorHalfWidth(PRIMARY_2)` around that heavy corridor; rank legal basins by descending quantized area then `StableId`;
14. select required lakes from that ranking. If the required count is short, instantiate the archetype/semantic required basin guides as deterministic eight-radial-sample carved polygons. Start at the archetype radius; on conflict shrink radius by exactly 10% per local retry, never below the Section-5 qualified area, for at most `retry.max_local_retries_per_stage` retries. If a required basin still cannot satisfy the legal-basin rules, fail the phase;
15. create exterior coast water only when the selected archetype/profile requires it. Coast water is legal only when it leaves every mandatory heavy corridor a dry bypass meeting the same width rule as step 13. On a required coastal conflict, move the coast boundary outward by exactly `S` per local retry, at most `retry.max_local_retries_per_stage`; failure after that fails the phase;
16. bind river mouths to a selected lake/coast when the D8 sink chain intersects that sink polygon; otherwise the mouth ends at its legal exterior D8 sink. A `legal exterior sink` is an exterior analysis-grid cell outside `BASE_PROTECTED` and inside the `WorldBoundsService` hydrology-permitted boundary;
17. realize each successful optional-island slot as decorative geography: select exterior sea when present, otherwise the largest selected true lake with water area at least `4 * max((16W)^2,0.01*world area)`; choose the first stable interior candidate point at least `6W` from the host shoreline; build an eight-sample irregular dry polygon at the Section-5 minimum island area using radius multipliers `0.85..1.15`; subtract it from the host water polygon; mark it `LAND`; suppress water carve inside it; store `island_macro_height_bias = max(0.5*tier_step, host_water_level + 0.5*tier_step - provisional_land_height_at_center)` with a `2W` shoreline blend for Phase 7 to consume. Reject only that island slot if no such polygon fits; optional-island rejection is not a generation failure;
18. after **all** river/lake/coast/island vectors are fixed, create every crossing/pass window in one pass. For each selected river and each explicit Phase-5 cliff/canyon/ravine, sample the obstruction centerline every `A`. Keep a station only when both approach sides provide at least `8W` of continuous non-water, non-hard-wall approach, neither approach enters `BASE_PROTECTED`/Tiberium hard reserve, and the provisional endpoint elevations can be connected within `PRIMARY_2` max grade. For each retained station compute method flags immediately from preflight metrics: `bridge_capable=true` only when at least one registered bridge kind can cover local obstruction span/clear width and its straight approach reserve fits; `pass_capable=true` only when a sampled `PRIMARY_2` swept path across the local floor/ramp obeys grade/turn/cross-slope; `tunnel_capable=true` only when tunnels are enabled/available and a registered tunnel can clear the cached heavy envelope at legal portals; `ford_capable_light`/`ford_capable_harvester` are true only when `allow_fords=true` and the preflight canonical fordability query approves the entire straight swept crossing for that cached capability. River stations are `CROSSING_WINDOW` and remain only when `bridge_capable` or at least one ford-capability flag is true; cliff stations are `RAMP_PASS_WINDOW` with pass/tunnel/cut metadata; canyon/ravine stations are `CROSSING_WINDOW` with bridge/pass/tunnel metadata. Mark `preferred=true` when the station intersects a required heavy-route corridor or lies within `2S` of its archetype `CROSSING_OBLIGATION_INTENT` guide;
19. merge same-obstruction windows whose along-obstruction distance is `<4W`, keeping `preferred=true` over false, then lower `StableId`; stamp accepted window approach/reserve bounds into the semantic overlays as `NO_DECORATION`;
20. lakes and coast are **not bridgeable road obstacles in this generator version**. They receive no crossing windows. If one blocks an entire mandatory heavy corridor's `MaxCorridorHalfWidth(PRIMARY_2)` envelope, its water candidate was illegal under steps 13/15 and must already have been rejected or repaired;
21. insert accepted river/lake/coast/island vectors and all crossing/pass windows into `ProceduralPlanningSpatialHash`.

River width at an analysis sample is exact:

```text
t = clamp((accumulation - river_threshold) / (3*river_threshold), 0, 1)
width = lerp(water.stream_width, water.river_width, t)
```

`WaterDefinitionClass::BedDepth` remains the only carve-depth source for the selected water type.

`water_fraction` is a target for total inland standing-water polygon area excluding rivers and exterior sea. Required archetype lake sizes take precedence. After required lakes exist, optional basins are considered only if they pass step-13 legal-basin rules; add them in descending `(area,StableId)` order until adding the next basin would move the standing-water fraction farther from `water_fraction` than stopping. Do not distort a required archetype lake merely to hit the fraction.

**Gate:** Section-5 river/lake quotas pass; every selected river has continuous acyclic source-to-sink flow; no water polygon intersects `BASE_PROTECTED` or required `OPEN_MANEUVER` no-water area; no lake/coast blocks a mandatory heavy corridor's maximum dry bypass; every required route corridor intersected by a river/cliff/canyon/ravine has at least one accepted crossing/pass window; and every required water/obstruction feature has a stable vector/polygon/window record.

## Phase 7 - realize macro terrain once

Create/update the canonical `WorldTerrainSystem` heightfield exactly once before road routing.

Use one exact hybrid detail-noise primitive so the agent never chooses among unrelated legacy noise functions. `HybridValueNoise2D(p,L,octave)` is hash-grid value noise:

```text
q = p / L
i = floor(q)
f = frac(q)
u = f*f*(3-2*f)                    // per axis
corner(ix,iy) = 2*Hash01(terrain,"detail_noise",octave,ix,iy)-1
return bilinear(corner values, u.x, u.y)
```

For each terrain sample `p`:

```text
middle = floor((tier_count-1)/2)
tier_height = world.sea_level + (base_elevation_tier(p)-middle) * tier_step
landform = sum(accepted Phase-5 vector fields at p)
water_carve = accepted Phase-6 river/lake/coast carve at p
n0 = HybridValueNoise2D(p, max(terrain.feature_scale, 8S), 0)
n1 = HybridValueNoise2D(p, max(terrain.feature_scale/3, 4S), 1)
detail_raw = detail_weight * tier_step * 0.35 * (0.70*n0 + 0.30*n1)
detail = detail_raw * detail_protection_factor(p)
height = tier_height + macro_height_bias(p) + landform - water_carve + detail
height = clamp(height, sea_level+terrain.min_elevation, sea_level+terrain.max_elevation)
```

`macro_height_bias(p)` is zero except for explicit Phase-4/5 macro intent and accepted Phase-6 `island_macro_height_bias`; the island bias is blended from full value inside the island to zero across its exact `2W` shoreline band.

`detail_protection_factor(p)` is exact:

```text
0 inside a base-core inner reserve
0.25 inside a provisional Tiberium reserve
0 inside lake/sea water polygons
0 on the river bed from centerline to half current river width
smoothstep(0->1) across each river bank blend width max(river_width,2W)
smoothstep(0->1) across 2W outward from a lake/sea shoreline
1 elsewhere
```

When multiple protection rules overlap, use the minimum factor. This rewrite stops using independent macro/ridge/hill/valley noise channels to decide major geography. Noise above is detail only; explicit Phase-5 vectors own ridges, hills, cliffs, canyons and ravines.

After the first height evaluation, enforce river longitudinal consistency without changing river topology. For each accepted river in source-to-sink order:

1. provisional water-surface sample is the Phase-6 filled-drainage height for a D8-derived river; for a guided river it is the Phase-6 provisional terrain height sampled at that vector point before water carve; quantize either value to `Q`;
2. if the river terminates in a lake/sea, the final mouth sample equals that sink water level;
3. walk downstream and set `surface[i] = min(provisional_surface[i], surface[i-1])`;
4. then walk upstream from the fixed mouth and set `surface[i] = max(surface[i], surface[i+1])`; this preserves non-increasing downstream order while meeting the sink level;
5. bed target is `surface[i] - WaterDefinitionClass::BedDepth`; clamp only by the global terrain min elevation; if clamping would place bed above its water surface, fail Phase 7;
6. apply bed/bank carve from these final samples.

Flat river reaches are allowed; an uphill downstream sample is not.

Within base-core reserves, detail is already suppressed by the factor above; preserve the existing base flattening/ordinary slope rules. Do not apply final road cut/fill in this phase.

After setting heights, run the canonical ordinary-slope enforcement only on terrain not tagged as explicit cliff/canyon/ravine wall and outside protected water-bed/shore profiles, then rebuild the terrain chunk registry once.

**Gate:** authoritative macro heightfield exists; min/max elevation constraints pass; ordinary-slope validation passes outside explicit steep/water profiles; every selected river surface is non-increasing from source to sink; no lake/river bed protrudes above its water surface; and every Phase-5/6 feature remains represented by its frozen semantic/vector record.

## Phase 8 - validate and freeze base/Tiberium anchors

Validate the already selected Phase-3 role centers against realized Phase-7 terrain. **Do not move a center in this phase.** Phase 3 chose it and Phase 4 reserved it before geology/hydrology; if the realized world violates a hard requirement, fail the attempt rather than moving into unreserved terrain.

Base hard requirements:

```text
entire `BASE_PROTECTED` disc inside placeable land
no WATER/CLIFF/CANYON/RAVINE hard overlap
slope <= terrain.max_buildable_slope_degrees across required starting-building proxy area
available expansion area >= fairness.min_base_expansion_area
possible heavy exit sectors >= max(2, fairness.min_vehicle_approach_count)
```

Reuse the already defined Section-0.5 constants `R_BASE`, `R_EXPAND` and `R_BASE_PROTECTED`. Freeze the Phase-4 `BASE_PROTECTED` reservation at each selected base center without changing its center or radius. Every later initial-building proxy must lie inside this protected region unless its existing descriptor explicitly requires an exit apron extending outward; such an apron is allowed only into a viable exit sector and is added to the protected reservation when base layout is frozen. Tiberium fields and unrelated roads cannot enter `BASE_PROTECTED`.

Exit sectors are measured as 16 equal angular sectors around `BASE_PROTECTED`; sector ordinal 0 points along strategic `+u` and ordinals increase counter-clockwise by 22.5 degrees. A sector is viable when a heavy swept rectangle of width `heavy_vehicle_width + 2*fairness.min_vehicle_lane_clearance` can travel radially from distance `R_BASE_PROTECTED` to distance `R_BASE_PROTECTED + 6W` from the base center without water/immutable cliff/canyon-wall collision and with slope <= `PRIMARY_2` max grade. Freeze the sorted viable-sector ordinal list for each base in Phase 8.

Phase-8 fairness uses only the exact **provisional** calculations in Section 0.10. Compute `PlanningDistance` on the frozen Phase-3 strategic graph after substituting these candidate base/resource centers. Reject the candidate pair when the provisional home-field travel-time difference or provisional base-to-front travel-time difference exceeds its named threshold, when the expansion-area/exit-sector hard gates fail, or when the straight strategic base-to-base planning distance is zero. Do **not** apply the final base-to-base maximum/minimum time gate here because exact routed road distance does not exist until Phase 18; Phase 19 applies that gate authoritatively. Do not invent any additional fairness threshold.

Resource roles are fixed to Section 5 counts:

```text
HOME_GDI
HOME_NOD
CONTESTED x size count
REMOTE_RICH x size count
```

A field anchor is legal only when its full `resource.FieldRadiusMax` reserved footprint is dry/placeable, outside immutable walls/BASE_PROTECTED, and has at least one route-corridor approach direction from Phase 3 that can reach the reserve edge without immutable obstruction. Phase 13 may shrink the eventual crystal/core radius inside this already-protected maximum reserve, but Phase 8 does not move the center.

**Gate:** both preselected base centers and every mandatory resource-role center remain valid; all Phase-8 geometric gates and Section-0.10 provisional home/front fairness gates pass using `PlanningDistance`; all selected centers and both `BASE_PROTECTED` reservations are frozen for later phases.

## Phase 9 - build transportation topology

Convert Phase-3 intents and Phase-8 frozen centers/viable-exit sectors into one explicit cyclic transportation graph. This phase chooses graph edges/classes/entry points but no exact terrain path.

### Base entrance assignment

Before constructing topology edges, assign one `BASE_ENTRANCE_POINT` per heavy route family at each base:

1. for each family in ascending family id, compute desired outgoing direction from that base center to that family's first non-base guide (for Nod, use its last non-base guide);
2. rank that base's viable Phase-8 sector ordinals by `(absolute wrapped angular difference from desired direction, sector ordinal)`;
3. choose the first sector not already assigned to another family; if all viable sectors are already used, choose the first ranked sector even though it is reused;
4. entrance point is at radial distance `R_BASE_PROTECTED + W` on that sector centerline; entrance corridor is the capsule from radius `R_BASE_PROTECTED-W` to `R_BASE_PROTECTED+6W` with half-width `0.5*RoadbedWidth(PRIMARY_2) + fairness.min_vehicle_lane_clearance`;
5. the entrance corridor must remain inside the Phase-8 viable swept sector. If it does not, try the next ranked viable sector. If no sector fits, fail Phase 9;
6. store `(base,family_id,sector_ordinal,entrance_point,entrance_corridor)` and reserve it against unrelated Tiberium/decoration.

Heavy world-road routing begins/ends at these entrance points. The logical base node connects to all of its entrance points by zero-cost **BASE_INTERNAL** graph edges used only for strategic connectivity; `BASE_INTERNAL` is never realized by `RoadSystem`, never counts as an independent route edge and is contracted by Section-12.2 max-flow. The later base-layout stage connects actual production/building exits to these already frozen entrances inside `BASE_PROTECTED`.

Construction order is fixed:

1. create one waypoint chain for each heavy route family from that family's GDI `BASE_ENTRANCE_POINT` to its Nod `BASE_ENTRANCE_POINT` through its archetype guide; graph-level trunk length is the sum of quantized Euclidean distances between consecutive frozen waypoints;
2. resolve archetype `CROSSING_OBLIGATION_INTENT`s first, in `(family_id,StableId)` order. For an intent tagged `BRIDGE_REQUIRED`, consider only `bridge_capable=true` windows on the intended obstruction; for `DISTINCT_CROSSING_REQUIRED`, consider windows with at least one legal method flag. Rank by `(quantized distance from window center to strategic guide, StableId)`, require the selected window to lie inside that family's `MaxCorridorHalfWidth(PRIMARY_2)` envelope, require it be `>=12W` along the obstruction from a previously selected window, then insert it as a mandatory family waypoint and bind the intent to that physical window. `BRIDGE_REQUIRED` bindings immediately count toward that obstruction's bridge quota. If no qualifying window exists, fail Phase 9;
3. fill the remaining Section-5 river-bridge quota from unused Phase-6 river windows with `bridge_capable=true`. First rank preferred windows by `(family_id, distance from that family's nearest guide, StableId)` and insert them into that family. If quota remains, select the lowest-`StableId` unused non-preferred bridge-capable river window and add one `SECONDARY_2` cross-link between the nearest heavy-family waypoints on opposite banks through it. Every selected physical window is `>=12W` along the obstruction from another selected window; multiple bridges may cross one river system only at distinct windows;
4. fill the remaining canyon/ravine-bridge quota by the same procedure using only unused `bridge_capable=true` canyon/ravine windows. Synthesized fallback cross-links are `SECONDARY_2`; a physical window consumed by one category/intent cannot satisfy another. If either bridge quota is still short after candidates are exhausted, fail Phase 9 before pathfinding;
5. family 0 is the main trunk. For a size with highway quota, sort all heavy family trunks by descending graph-level trunk length then ascending family id and assign the first required count to `HIGHWAY_4`; all other heavy-family trunk segments are `PRIMARY_2`; there is no additional highway-eligibility judgment;
6. for every resource role, create a frozen `FIELD_ACCESS_POINT` before adding its road: let `a` be the chosen route/base attachment point and `c` the frozen field center; `dir=normalize(a-c)`; set access point `p = c + dir*(resource.FieldRadiusMax + fairness.min_vehicle_lane_clearance)`. If `a==c`, use the strategic vector from map center toward `c`; if that is also zero, use strategic `+v`. The access point is hard-valid only when: it is dry/placeable and outside immutable walls/`BASE_PROTECTED`; the straight radial segment from `p` to `c + dir*resource.FieldRadiusMax` stays dry, outside immutable walls/other Tiberium cores/`BASE_PROTECTED`, and has provisional longitudinal grade <= `SERVICE_DIRT_2.max_grade`; and a disc of radius `1.5*Harvester collision length` centered on the reserve-edge endpoint is free of water/immutable walls/other hard reserves. If the first direction fails, rotate `dir` around `c` in 16 stations at 22.5-degree increments, testing `+22.5,-22.5,+45,-45,...,+180` degrees, and choose the first hard-valid point. If none exists, fail Phase 9. The road always terminates at `p`, never at the Tiberium center;
7. attach each home field's `FIELD_ACCESS_POINT` to the owning base's `BASE_ENTRANCE_POINT` having minimum quantized Euclidean distance to that field access point by `PRIMARY_2` unless the quantized Euclidean distance from that entrance point to the field access point is `<12W`, in which case use `SECONDARY_2`;
8. attach each contested/remote `FIELD_ACCESS_POINT` to the Phase-3 chosen route-family attachment point by `SERVICE_DIRT_2`; these field branches count toward the Section-5 dirt/service-road quota;
9. if the dirt/service-road quota exceeds the number of non-home field branches, enumerate pairs of heavy-family non-base waypoints that are not already adjacent; sort by `(quantized Euclidean distance, lower endpoint StableId, higher endpoint StableId)` and add the exact required number as `SERVICE_DIRT_2`; skip a pair already consumed by another newly added service edge; insufficient pairs fail Phase 9;
10. add exactly the Section-5 `TRAIL_1` shortcuts. Candidate pairs are non-base waypoints on different heavy families, not already adjacent, with straight distance `>=8W`; sort by `(distance, lower endpoint StableId, higher endpoint StableId)` and accept in order while forbidding duplicate endpoint pairs; insufficient candidates fail Phase 9;
11. generate optional heavy cross-link candidates between non-base waypoints on different heavy families; reject already-adjacent pairs and pairs with straight distance `<8W`; compute the exact benefit score below, sort by descending score then shorter candidate distance then endpoint StableIds, and accept exactly `extra_links` unless fewer hard-valid candidates exist, in which case accept all available candidates; optional-link shortage alone is not a phase failure;
12. reserve road-over-road overpass obligations after highway assignment. Treat every current topology edge as a straight intent segment between its frozen endpoints. First enumerate geometric intersections between `HIGHWAY_4` intent segments and `PRIMARY_2`/`SECONDARY_2` intent segments. A candidate intersection is hard-valid only when: crossing angle is `60..120` degrees; both intent segments have at least `10W` of segment length on each side of the intersection; the intersection is outside base/Tiberium/crossing-window protected bounds; it is `>=12W` from an already selected overpass; and the Phase-7 terrain disc of radius `2W` around it has maximum sampled absolute slope <= `SECONDARY_2.max_grade`. Rank valid natural intersections by `(overpass_cost, highway edge StableId, other edge StableId)`. `overpass_cost = clamp(local_max_slope/SECONDARY_2.max_grade,0,1) + clamp((distance_to_nearest_edge_endpoint)/(0.5D),0,1)`. Select in order until quota is met;
13. if overpass quota is still short, enumerate candidate `SECONDARY_2` cross-links between non-base waypoints on different heavy families whose straight intent segment intersects a `HIGHWAY_4` intent segment at `60..120` degrees and satisfies the same `10W`, exclusion, spacing and slope tests. Rank by `overpass_cost + candidate_crosslink_length/D`, then endpoint StableIds. Add the lowest-ranked candidate, insert one shared `OVERPASS_OBLIGATION` waypoint at the exact quantized intersection into both the highway edge and the new cross-link, then repeat enumeration until quota is met. If no valid candidate remains before quota is met, fail Phase 9;
14. for every selected natural or synthesized overpass, insert one shared `OVERPASS_OBLIGATION` waypoint into both participating topology edges at the exact quantized intersection. Phase 10 therefore routes both roads through the same point; it does not have to rediscover the crossing.

Optional heavy cross-link benefit is computed on the graph **before** adding that candidate:

```text
M_before = mean finite shortest-path distance over all unordered strategic-role/node pairs
M_after  = same mean after temporarily adding candidate with straight-line edge weight
path_reduction = clamp((M_before-M_after) / max(M_before,Q), 0, 1)

F_before = deterministic unit-capacity GDI-to-Nod edge-disjoint max-flow before candidate
F_after  = same max-flow after candidate
flow_gain = clamp((F_after-F_before) / max(route_family_count,1), 0, 1)

frontier_term = 1 when archetype==open_frontier, otherwise 0
score = 0.50*path_reduction + 0.30*flow_gain + 0.20*frontier_term
```

Shortest-path adjacency and max-flow adjacency are sorted by edge `StableId`; disconnected pair distances are excluded from the mean in both `M_before` and `M_after`, and if no finite unordered pair exists the candidate score is zero. The open-frontier term is fixed and does not alter candidate legality.

**Gate:** graph contains `route_family_count` heavy GDI-to-Nod families, all river/canyon bridge obligations are assigned to distinct Phase-6 hard-valid windows and all overpass obligations have shared Phase-9 crossing waypoints, Section-5 road/highway/dirt/trail intent counts pass, at least one cycle exists, and no family pair is intentionally assigned the same required bottleneck.

## Phase 10 - route roads on bounded analysis grid

Route topology edges in this stable priority order, then by edge `StableId`:

```text
1 heavy route-family trunks
2 mandatory synthesized river/canyon bridge cross-links
3 home-field roads
4 contested/remote service branches
5 quota-filling SERVICE_DIRT_2 cross-links
6 mandatory/synthesized overpass SECONDARY_2 cross-links
7 optional heavy cross-links
8 TRAIL_1 shortcuts
```

A topology edge containing one or more frozen `CROSSING_OBLIGATION`, `RAMP_PASS_WINDOW` or `OVERPASS_OBLIGATION` waypoints is routed as consecutive subroutes through those waypoints in source-to-destination order. A waypoint maps to the analysis cell containing its quantized XY position; if it lies exactly on a cell boundary, choose the lower row-major cell id. The resulting concatenated path contains that cell exactly.

Use an 8-neighbor heading-aware A* on analysis spacing `A`. State is `(cell,incoming_direction)`, where `incoming_direction` is one of the eight neighbor directions plus one `START` value. Neighbor expansion order is exactly `E,NE,N,NW,W,SW,S,SE`.

Preferred corridor half-width is `PreferredCorridorHalfWidth(road_class)` from Section 7.1. One edge gets at most three path-search envelopes: preferred width, preferred width `+S`, preferred width `+2S` (ending exactly at `MaxCorridorHalfWidth(road_class)`). Search the next envelope only after the previous one finds no path or produces a spline that fails the deterministic smoothing/constraint procedure below. No search may leave the third envelope.

Hard-infinite analysis cells are:

```text
outside placeable boundary
`BASE_PROTECTED` except cells touched by that edge's frozen entrance corridor; world-road A* starts/ends at the outer entrance point and may use only that corridor portion when crossing the protected boundary
Tiberium hard reserve except the edge's own resource-service endpoint
lake/coast water
river bed/bank outside an accepted river CROSSING_WINDOW
cliff/canyon/ravine hard wall outside an accepted CROSSING_WINDOW/RAMP_PASS_WINDOW
OPEN_MANEUVER immutable-barrier violation
```

Starting-building transforms do not exist yet and are therefore not routing inputs. `BASE_PROTECTED` plus the frozen Phase-9 entrance corridors are the complete base-side routing constraints at this stage.

For an ordinary candidate move from cell `a` to neighbor `b`:

```text
d = A for cardinal move, A*sqrt(2) for diagonal move
z_a = Phase-7 terrain height at a center
z_b = Phase-7 terrain height at b center
I_a = RoadVerticalInterval(z_a)
I_b = RoadVerticalInterval(z_b)
gap = max(0, I_a.low-I_b.high, I_b.low-I_a.high)
move_grade = atan(gap / d)
```

The move is hard-invalid when `move_grade > road_class.max_grade`. For a bridge/tunnel/overpass obligation cell, this phase only routes through the frozen XY cell using the ordinary feasibility interval. Exact structural endpoint elevations are not needed to choose the horizontal path and are compiled in the immediately following interaction phase.

For each legal move:

```text
heading_change = 0 for START, otherwise the smaller absolute angle between the two 8-way directions
corridor_excess = max(0, distance_from_intent_corridor_center-half_width)/S

base_distance = d
reuse_modifier = 0

if edge is SERVICE_DIRT_2 or TRAIL_1 and move follows an already-routed compatible road center corridor:
    reuse_modifier = -0.25*d

if edge belongs to one heavy route family and move would run collinear within W of a different heavy-family centerline for >6W outside either `BASE_PROTECTED` contraction zone:
    move is hard-invalid

if edge is an optional heavy cross-link and move would be collinear within W of an existing road for >6W:
    move is hard-invalid

window_penalty = 20A when entering a non-mandatory water/cliff/canyon/ravine crossing window, otherwise 0

cost = base_distance
     + d * Ks * (move_grade/max(road_class.max_grade,Q))^4
     + A * Kt * (heading_change/pi)^2
     + A * Kc * corridor_excess^2
     + window_penalty
     + reuse_modifier
```

Class constants:

| class | `Ks` | `Kt` | `Kc` |
|---|---:|---:|---:|
| `HIGHWAY_4` | 10 | 8 | 3.0 |
| `PRIMARY_2` | 8 | 6 | 2.5 |
| `SECONDARY_2` | 6 | 4 | 2.0 |
| `SERVICE_DIRT_2` | 4 | 3 | 1.5 |
| `TRAIL_1` | 3 | 2 | 1.0 |

A* heuristic is exactly `0.75 * EuclideanDistance(current,goal)`, which remains admissible because the only negative modifier reduces the base distance to `0.75*d`. Open-set priority key is `(f,g,row_major_cell_id,incoming_direction_ordinal)` with lower values first. Closed-state reopening is allowed only when a strictly lower quantized `g` is found.

### Deterministic path-to-spline conversion

For one accepted A* cell chain:

1. remove consecutive duplicate cells;
2. keep endpoints plus every cell whose outgoing 8-way direction differs from the incoming direction; this is control set 0;
3. build a **centripetal Catmull-Rom** spline (`alpha=0.5`) through that control set; duplicate the first/last control mathematically for endpoint tangent evaluation rather than adding random endpoint handles;
4. sample at `min(W,A/2)`; a sample set passes only if every sample is inside the current search envelope, outside all hard obstacles, its horizontal curvature radius is >= the Section-7 class minimum, and the local vertical feasibility intervals admit the class max grade/cross-slope;
5. when a sample fails, identify the earliest-along-route failing sample; find the nearest original A* path cell not already a control point, tie by earliest path index, insert it in path order, and rebuild;
6. perform at most `retry.max_local_retries_per_stage` control insertions for that search envelope;
7. if no legal Catmull-Rom result exists, try the next wider search envelope and rerun A*; after the third envelope fails, the topology edge fails Phase 10.

Do not use an unspecified smoothing/tension parameter, hand-edit a control point, or accept the raw staircase as final road geometry.

**Gate:** every mandatory topology edge has one legal preliminary spline that passes the exact Phase-10 envelope/curvature/grade-feasibility checks and every spline has a stable checksum.

## Phase 11 - solve feature interactions before terrain road deformation

Query only nearby vector features through `ProceduralPlanningSpatialHash`. Interactions are processed by road edge `StableId`, then increasing along-road distance. Phase 11 does not instantiate runtime road/bridge/tunnel objects.

For a non-obligatory road-feature intersection, nominal station search along the obstruction is:

```text
half_length = max(8W,4S)
station_spacing = min(W,A/2)
```

Center that interval on the preliminary road/obstruction intersection. Mandatory Phase-9 crossing/overpass obligations center the interval on their frozen waypoint. Generate stations from `-half_length` through `+half_length` inclusive at `station_spacing`; if this produces more than 64 stations, replace it with exactly 64 uniformly spaced stations including both endpoints. Quantize every station to `Q` and deduplicate by quantized XY before scoring.

### Common hard-valid crossing-station tests

A bridge/canyon-pass/tunnel station is hard-valid only when all applicable tests pass:

```text
station belongs to the intended obstruction
both road-side approach centerlines remain inside placeable bounds
straight approach reserve on each side is available
no approach reserve intersects unrelated `BASE_PROTECTED` or unrelated Tiberium core
connected road can meet the station within its max grade and minimum turn radius
required swept vehicle width/height clearance passes
selected structural asset span/width/height limits pass when an asset is required
station is >=4W from an unrelated already compiled crossing unless the topology explicitly shares a road junction
```

If a mandatory frozen crossing waypoint has no hard-valid station inside its search window, attempt only the method-specific fallback sequence below. Phase 11 never silently moves that obligation to a different obstruction.

### Road x river

Method order is fixed:

1. evaluate all hard-valid bridge stations and select the lowest Bridge Candidate Scoring value below;
2. ford only when **all** are true: `plan.allow_fords=true`, the crossing is not a mandatory bridge obligation, road class is `SECONDARY_2`, `SERVICE_DIRT_2` or `TRAIL_1`, the preflight canonical fordability query returns true for the required route capability at every swept sample, approach grade/turn rules pass, and water current/depth checks exposed by that same canonical query pass;
3. rerun this edge once in its next Phase-10 corridor envelope if an unused envelope remains;
4. fail the phase.

`HIGHWAY_4` and `PRIMARY_2` never ford. A ford never satisfies any bridge quota.

### Road x canyon/ravine

Method order is fixed:

1. bridge using the lowest-score hard-valid bridge station;
2. preplanned surface pass/descent only at an accepted Phase-6 pass-capable window; sample its full swept route at `min(W/2,A/3)` and require road-class max grade, turn radius, cross-slope and vehicle-clearance rules at every sample;
3. tunnel only when `world_features.tunnels=true` and preflight set `tunnel_available=true`;
4. rerun this edge once in its next Phase-10 corridor envelope if one remains;
5. fail.

A mandatory canyon/ravine **bridge** obligation skips methods 2 and 3: it must compile as a bridge or the edge/attempt fails.

### Road x cliff/ridge

Method order is fixed:

1. road cut when the minimum centerline correction needed to cross the explicit face is <= `grading.max_cut_depth`, no sample requires fill above `grading.max_fill_height`, the cut is entirely inside `RoadDeformationHalfWidth(road_class)` from Section 7.1, and the resulting outer blend reaches untouched terrain without intersecting another immutable wall/base/Tiberium core;
2. preplanned `RAMP_PASS_WINDOW` when its full sampled swept route passes the road-class constraints;
3. tunnel only when `world_features.tunnels=true` and preflight set `tunnel_available=true`;
4. rerun this edge once in its next Phase-10 corridor envelope if one remains;
5. fail.

### Road x road

A non-reserved crossing becomes an at-grade junction only when:

```text
crossing angle >=30 degrees and <=150 degrees
intersection is outside every bridge/tunnel approach reserve
for each road, at least max(4W, its minimum turn radius) of centerline exists on both sides
required local vertical correction fits grading.max_cut_depth/max_fill_height
combined junction surface can satisfy the larger of the two road cross-slope limits
```

If any condition fails, try an overpass using the rule below. If an overpass cannot be compiled, reroute the lower-priority road through its remaining Phase-10 envelope; if none remains, fail. Reserved Phase-9 `OVERPASS_OBLIGATION`s always compile as overpasses and never downgrade to at-grade junctions.

### Bridge asset selection

For each bridge station, form the registered bridge-kind list capable of the connected road's physical tier. Resolve metrics from the existing bridge definition first and `BridgePlannerParams` second (`MaxSpanLength`, water-clearance/deck-depth/approach values already owned by the planner). Reject kinds whose clear width is below Section 12.4, whose maximum span cannot cover the measured obstruction plus both canonical abutment seats, or whose required approach reserve does not fit. Select the candidate with the **smallest sufficient maximum span**, then lower bridge-kind `StableId`. If neither source supplies a required metric, preflight must already have failed; do not substitute a new constant here.

### Bridge candidate scoring

For each hard-valid station calculate:

```text
span_term      = span / selected_bridge.max_supported_span
elevation_term = abs(bank_z_a-bank_z_b) / max(tier_step,Q)
grade_term     = max(approach_grade_a,approach_grade_b) / max(road_class.max_grade,Q)
curve_term     = max(0, road_class.min_turn_radius/measured_turn_radius - 1)
detour_term    = added_route_length / max(8W,Q)
score = 4*span_term + 2*elevation_term + 3*grade_term + 2*curve_term + detour_term
```

`added_route_length` is the candidate road length through the two approach-reserve endpoints minus their straight endpoint distance; it is never negative. Lower score wins; ties use Section 0.3 stable selection order.

Straight bridge approach reserve on each end is exactly:

```text
max(8W, selected_bridge.required_approach_length)
```

### Overpass construction rule

Try upper/lower assignments in this exact order: higher-priority road stays lower; if classes tie, lower `StableId` stays lower. The lower road retains its frozen horizontal spline and Section-7.1 vertical feasibility intervals. Let `delta_z` be the extra upper-road deck elevation needed by Section 12.3 after structural depth. On each side reserve:

```text
L_ramp = max(10W, abs(delta_z) / max(tan(upper_class.max_grade),Q))
```

The upper target elevation uses one smoothstep ramp over exactly `L_ramp` on each side and a level/linearly graded deck through the crossing. Sample at `min(W/3,A/4)` and require max grade, cross-slope, cut/fill interval and clearance at every sample. If the first assignment fails, swap upper/lower exactly once. If the swapped assignment fails, reroute the lower-priority road through its next Phase-10 envelope; equal-class tie reroutes the higher `StableId`. No third assignment or improvised vertical profile is allowed.

Every generated bridge/overpass starts in `PRISTINE` damage state. Initial random broken/damaged bridge state from older planners is disabled on this hybrid startup path.

Before the phase gate, run lightweight swept proxy validation over every compiled bridge/pass/tunnel/overpass interval using the cached collision envelopes. This is the last phase allowed to change crossing method/location or invoke a remaining Phase-10 corridor envelope.

**Gate:** every road-feature conflict has exactly one compiled solution record; mandatory bridge/overpass obligations retain their required method; Section-5 bridge/overpass quotas pass; all compiled proxy swept checks pass; and no accepted solution exceeds asset, vehicle, grade, turn, cut/fill or clearance limits.

## Phase 12 - compile final roads and local terrain cut/fill

Phase 12 consumes frozen Phase-10 splines and Phase-11 compiled interactions. It does **not** reroute a road, move a crossing, change a crossing method or reopen Phases 9-11. A failure here fails the whole attempt.

Process roads in the exact Phase-10 routing priority and `StableId` order. This phase is the single runtime realization point for roads and structural crossings.

### 12.1 Deterministic vertical-profile solver

Sample each non-bridge/non-tunnel road interval at spacing `ds = min(W/2,A/3)`, including exact endpoints/junction boundaries. For sample `i`:

```text
t_i = Phase-7/current terrain height before this road is deformed
lo_i = t_i - grading.max_cut_depth
hi_i = t_i + grading.max_fill_height
g = tan(road_class.max_grade)
```

A compiled crossing/junction endpoint with a fixed required deck/portal/junction elevation sets `lo_i=hi_i=fixed_z`. Otherwise keep the interval above. Solve feasibility deterministically:

1. forward interval propagation: `lo_i=max(lo_i,lo_{i-1}-g*ds_i)` and `hi_i=min(hi_i,hi_{i-1}+g*ds_i)`; fail if `lo_i>hi_i`;
2. backward propagation: `lo_i=max(lo_i,lo_{i+1}-g*ds_{i+1})` and `hi_i=min(hi_i,hi_{i+1}+g*ds_{i+1})`; fail if `lo_i>hi_i`;
3. choose first sample `z_0=clamp(t_0,lo_0,hi_0)`; for each later sample choose `z_i=clamp(t_i,max(lo_i,z_{i-1}-g*ds_i),min(hi_i,z_{i-1}+g*ds_i))`; if the clamp interval is empty, fail;
4. quantize every `z_i` to `Q`, revalidate adjacent grade after quantization, and fail if the max grade is exceeded by more than angular quantization `0.001 rad`;
5. compute the road cross-section from Section 7. Cross slope is zero on ordinary straight roadbed unless an existing RoadSystem banking API already consumes an explicit bank value; this rewrite does not invent banking. At intersections/ramps the compiled profile must remain <= the Section-7 max cross slope.

This algorithm is the only road centerline vertical-profile solver on the hybrid startup path.

### 12.2 Terrain deformation and realization

For each accepted road interval:

1. roadbed follows the solved centerline/cross-section;
2. shoulder follows the roadbed edge elevation;
3. `outer_blend = max(W,A/2)`; blend from shoulder edge to untouched terrain with `smoothstep` across exactly `outer_blend`;
4. do not modify terrain inside a bridge deck span, tunnel bore, water/canyon void under a bridge, or an unrelated earlier road's protected junction surface;
5. a road-road junction uses the already compiled shared junction surface; process that shared patch once under the lower junction `StableId`;
6. apply an existing canonical ditch/embankment helper only when the current `RoadSystem` already exposes it; absence of such a helper causes no extra geometry and is not a failure;
7. commit the deformation through `WorldTerrainSystem`;
8. realize the road/crossing exactly once through canonical `RoadSystem`, `BridgeSystem`, `TunnelSystem` and junction APIs;
9. append every created runtime id to the attempt-owned reverse-order cleanup list.

Temporary refinement patches use Section 2.5 spacing and are released immediately after their accepted deformation is committed.

### 12.3 Post-realization physical verification

Sample at `min(W/2,A/3)` and run the canonical swept/collision query with:

```text
Humvee/light reference -> every road
heavy reference         -> every heavy-capable strategic edge
Harvester envelope      -> every home/resource economy edge
```

Verify bridge/tunnel/overpass width/height/span/attachment against the realized runtime object and verify every realized road sample against its solved centerline/grade/cross-slope. A discrepancy is a hard Phase-12 attempt failure. Do **not** jump backward and repair it after runtime objects have been committed; teardown/retry keeps the pipeline one-way and prevents half-undone bridge/road state.

**Gate:** every mandatory road/crossing object is realized exactly once; realized geometry matches the frozen compiled records; all required swept-envelope tests pass; and no crossing is detached from either connected approach.

## Phase 13 - realize Tiberium fields

Resolve each frozen Phase-8 resource anchor using the existing `resource` profile settings and canonical `TiberiumFieldGenerator` realization path.

Role rules:

```text
HOME_GDI / HOME_NOD -> owned early fields; one per team
CONTESTED            -> neutral; attached to central/flank route network
REMOTE_RICH          -> neutral; farther from both bases than the nearest contested field
```

For each field, let `K = retry.max_local_retries_per_stage`. Radius attempts are exactly:

```text
K==1: radius[0] = resource.FieldRadiusMax
K>1 : radius[j] = resource.FieldRadiusMax
                  - (resource.FieldRadiusMax-resource.FieldRadiusMin) * j/(K-1), j=0..K-1
```

Quantize each radius to `Q`, deduplicate equal radii while preserving order, and accept the first hard-valid radius. A field that exhausts this list fails Phase 13; do not invent another center because Phase 8 froze the anchor.

A footprint is hard-invalid when its core polygon intersects:

```text
`BASE_PROTECTED`
paved carriageway/roadbed
bridge/tunnel/overpass approach reserve
water
immutable cliff/canyon/ravine wall
another Tiberium core
```

The field transition/contamination perimeter may overlap a road shoulder/material mask but may not place collision-bearing crystals inside the road/vehicle swept envelope.

Harvester access begins on the already-realized resource road at the frozen Phase-9 `FIELD_ACCESS_POINT`. Connect that point to the accepted field edge by one straight **off-road harvest apron** along the radial line toward the field center; apron length is therefore `resource.FieldRadiusMax + fairness.min_vehicle_lane_clearance - accepted_field_radius`. The apron creates no road mesh/material and is reserved only for traversal/decoration exclusion. Validate the cached Harvester OBB along the road and apron at spacing `min(W/2,A/3)` and require:

```text
clear width >= Harvester width + 2*fairness.min_vehicle_lane_clearance
road samples obey that road class's max grade and min turn radius
apron longitudinal grade <= SERVICE_DIRT_2.max_grade and cross slope <= SERVICE_DIRT_2.max_cross_slope
bridge/tunnel/overpass samples carry the Harvester capability and clear its OBB
field-edge turning disc radius >= 1.5 * Harvester collision length
turning disc and harvest apron contain no water, immutable wall, BASE_PROTECTED/building reserve or unrelated Tiberium core
```

No separate guessed Harvester grade/turn capability is introduced; the realized road class plus canonical swept collision is authoritative for this startup generator.

For each CONTESTED field, compute GDI and Nod shortest vehicle-capable route times with `V_REF`. At least one contested field must satisfy:

```text
abs(GDI_route_time - Nod_route_time)
<= fairness.max_base_to_front_travel_time_difference_seconds
```

Additional contested fields may exceed that difference if all accessibility rules pass.

`REMOTE_RICH` always records the semantic role. If the already-existing resource field record has a field-specific yield multiplier member, write exactly `1.25`; otherwise leave economy yield unchanged. Do not add a new economy field/API solely for the label.

Tiberium contamination is a biome overlay and does not change `MapIdentity.biome`.

**Gate:** exact Section-5 field counts exist, both home fields have Harvester-valid routes, at least one contested field satisfies the contestability formula, and every field has a legal footprint/access envelope.

## Phase 14 - base layout planning

Adapt the existing `ProceduralBaseLayoutService` into a planning/commit split; do not create a second base-layout algorithm.

`Plan_Initial_Base_Layout` runs here using the exact existing authored order from:

```text
base.gdi_initial_buildings
base.nod_initial_buildings
```

Placement rules remain those already implemented by the service/`CommanderPlacementValidator`:

1. first descriptor is the anchor at the selected base site;
2. later buildings test the existing deterministic rings beginning behind the base;
3. all buildings face the enemy base unless their descriptor has a vehicle-exit semantic anchor; a vehicle-production building is rotated so its exit faces the nearest frozen `BASE_ENTRANCE_POINT` by quantized Euclidean distance; tie by lower family id;
4. candidate validation uses bounds, slope, water, cliff/canyon, roads, crossings, Tiberium/Harvester corridors, chokepoints, overlap and production-exit rules through the existing placement validator;
5. use descriptor/collision proxies only; do not call `RuntimeBuildingFactory` in this phase.

Freeze and checksum:

```text
planned building descriptor ids + transforms
production/factory exit corridors
refinery/Harvester corridor
spawn candidate clearances
Commander build expansion reserve
frozen base entrance points/corridors and their internal building-exit connectors
```

Required heavy-vehicle exit sectors are `max(2, fairness.min_vehicle_approach_count)` and that exact count must remain viable after all proxy placements.

`Commit_Planned_Base_Layout` is called only by the final transactional activation step after all read-only world validation has passed and before `WORLD_ACTIVE`; it uses the frozen transforms and the existing `RuntimeBuildingFactory::Create_Constructing_Building` + immediate `Activate_Completed_Building` path. It does not rerun placement search.

**Gate:** every configured starting-building descriptor has one legal frozen transform, all production/Harvester exits are clear, spawn candidate clearances exist, and required vehicle exit sectors remain viable.

## Phase 15 - tactical FPS/RTS refinement

This is the final phase allowed to modify gameplay terrain geometry. It cannot move a road, bridge, tunnel, water body, base center, Tiberium center or starting-building reserve.

Build immutable exclusion envelopes before any repair:

```text
all road swept envelopes + shoulder
bridge/tunnel/overpass approaches
base starting-building footprints and production corridors
Harvester routes/field turning discs
water polygons
cliff/canyon/ravine hard faces
```

Run the existing `CombatSpaceValidator`/sightline owner against:

```text
base exits
mandatory infantry approaches
mandatory vehicle approaches
bridge/overpass/tunnel mouths
Tiberium approaches
paved road segments longer than fairness.max_mandatory_exposed_infantry_run_distance
```

Use the profile's exact fairness thresholds; do not create separate tactical thresholds.

For one failing exposed infantry segment, try repair methods in this exact order:

1. terrain berm/fold outside every exclusion envelope; maximum footprint radius `4W`, maximum height `0.75W`, and slope <= `terrain.max_buildable_slope_degrees`;
2. collision-bearing registered rock/ruin-cover props from the existing biome/prop registry, sorted by `(footprint area,StableId)` and tested smallest first;
3. collision-bearing vegetation already tagged legal for the selected biome, sorted by `(footprint area,StableId)` and tested smallest first;
4. fail the attempt.

An empty method-2 or method-3 asset list simply advances to the next method; it never prompts for an asset substitute. Candidate stations are spaced by `max(4W,fairness.preferred_cover_interval)` along the failing segment, ordered from segment midpoint outward by increasing absolute along-segment offset and then negative offset before positive offset. Test at most `retry.max_alternate_candidates_per_feature` stations per repair method. Prop/vegetation orientation uses `Hash01(biomes,"cover_yaw",segmentStableId,stationIndex,assetStableId)` mapped to `[0,2*pi)` and is part of the tactical stage checksum.

Each mandatory bridge approach must have one infantry-cover pocket within `6W..12W` of at least one abutment unless the existing `CombatSpaceValidator` already reports the complete approach below both exposed-distance and exposed-time thresholds. A cover pocket is a point from which an infantry-height LOS ray to the opposite approach is blocked while a `2W`-wide infantry movement corridor remains open.

After every accepted terrain repair, re-run only the local road/Harvester/base-clearance checks intersecting that repair AABB. Any failure rejects that repair immediately.

**Gate:** all existing combat-space hard checks pass and every accepted tactical repair preserves structural traversal/reservations.

## Phase 16 - material and biome compilation

Apply the exact Section-14.2 priority:

1. road/pavement;
2. Tiberium core;
3. Tiberium contamination transition;
4. wet shoreline;
5. exposed cliff/canyon/ravine rock;
6. biome background/transition.

Single-biome background rules:

| Biome | Broad land material behavior |
|---|---|
| `temperate` | grass/soil; rock on steep exposed surfaces |
| `desert` | sand/dry soil; rock on steep exposed surfaces |
| `snow` | snow-covered soil/rock across ordinary land; steep faces expose rock |
| `tiberium_wasteland` | barren/cracked/contaminated ground with sparse ordinary vegetation |

Mixed maps use the two broad region masks from Phase 4 and blend only inside the configured transition width.

Do not decide biome from elevation after this phase. Elevation can affect local exposed rock/snow depth, but the selected biome remains the map's theme.

## Phase 17 - foliage and decorative props

Reuse `FoliageSystem` and deterministic clustered placement. Decoration never changes terrain geometry or structural topology.

Theme density multipliers applied to the existing `BiomeDecorationGenerator` base density are:

```text
temperate           1.00
snow                0.55
desert              0.20
tiberium_wasteland  0.15
```

When `world_features.forests=false`, tree/bush/grass candidates are skipped; rock/boulder candidates keep the existing `BiomeDecorationGenerator` rock-candidate count/density algorithm unchanged and are filtered only by the Phase-17 exclusion rules.

Every candidate instance is rejected when its collision/placement bounds intersect:

```text
road roadbed + shoulder + W clear zone
bridge/tunnel/overpass approach reserve
`BASE_PROTECTED` or production exit
starting-building footprint
Harvester route or field turning disc
water
explicit cliff face
Tiberium core
```

Do not invent a second scatter algorithm. `BiomeDecorationGenerator` remains the candidate producer and its existing deterministic candidate ordering/hash placement is reused unchanged. Apply the selected theme density multiplier by computing `accepted_budget = RoundHalfUp(existing_candidate_budget * theme_multiplier)` separately for each existing decoration category/region; examine the existing generator's candidates in its canonical order and stop after `accepted_budget` candidates have survived this plan's exclusion checks. In addition, reject a cluster center whose XY distance from an already accepted cluster center is `<2W`. Use `cluster_radius = max(3W,2A)` only as the bounds for this extra cluster-center separation/exclusion query. Existing `FoliageSystem` per-type spacing remains authoritative for member instances. This plan adds filters/budgets; it does not choose a new Poisson sampler.

Street lamps are deterministic:

```text
HIGHWAY_4   -> station spacing 12W; every station; alternate side
PRIMARY_2   -> station spacing 10W only within 12W of a `BASE_PROTECTED` boundary or an abutment of a river/canyon bridge or overpass carrying PRIMARY_2/HIGHWAY_4
SECONDARY_2 -> no lamps on the native hybrid path in this version
SERVICE_DIRT_2 / TRAIL_1 -> no lamps
```

Lamp offset from road edge is `0.5` world units beyond the full roadbed width, preserving the existing foliage-lamp placement convention. Missing lamp art omits lamps without affecting checksums for authoritative topology.

**Gate:** no decoration instance intersects any structural exclusion envelope; decoration does not alter any Phase-15 checksum except the dedicated decoration/material stage checksum.

## Phase 18 - one integration/navigation build

At entry, terrain, water, roads, structural crossings, Tiberium, base-layout reservations and tactical terrain are frozen.

Perform exactly:

1. finalize/rebuild canonical terrain collision once for all Phase-15 height edits;
2. finalize canonical collision/static-spatial registration for the already realized Phase-12 structural objects, Phase-13 Tiberium data and Phase-17 collision-bearing decoration; do not instantiate any road/bridge/tunnel/Tiberium/foliage object a second time;
3. call only the non-instantiating/finalization path of `GeneratedWorldIntegrator`. If the current integrator mixes creation with finalization, split it so the hybrid path can register/finalize existing ids without duplicating them;
4. build the authoritative navigation capability map / `PathfindClass` sectors exactly once;
5. add/verify bridge/tunnel/overpass links from the frozen Phase-11/12 records, then freeze the capability graph;
6. build the minimap base layer from frozen world records;
7. partition shroud;
8. build Commander chokepoint/expansion metadata from the final capability graph and Phase-14 reservations;
9. build the existing logistics graph only when its existing profile/runtime enable flag is true; otherwise record the repository-standard disabled/absent state;
10. compute Phase-18 provisional checksums.

No active starting building, AI, Harvester, player object or economy actor is spawned in Phase 18.

Any failure in terrain collision, structural registration or navigation is a hard attempt failure. Do not retry only the nav build and do not mutate geometry here to make nav pass.

**Gate:** canonical collision/navigation/integration owners report success and planned connectivity equals actual capability-graph connectivity for all mandatory routes.

## Phase 19 - final validation, manifest commit and activation

### Phase 19A - read-only final validation

Phase 19A is read-only with respect to generated-world geometry/topology and performs no local repair. Re-derive the following from authoritative Phase-18 collision/navigation/runtime records rather than trusting planner booleans:

1. **Heavy connectivity:** GDI and Nod `BASE_PROTECTED` regions are connected by heavy capability.
2. **Independent routes:** Section-12.2 deterministic max-flow is `>= route_family_count`.
3. **Home economy access:** each base reaches its own HOME Tiberium field through a Harvester-capable road + harvest apron.
4. **Home travel fairness:** using `FinalDistance` and `V_REF`, absolute GDI/Nod home-field one-way time difference is `<= fairness.max_initial_resource_travel_time_difference_seconds`.
5. **Projected initial income fairness:** calculate `IPM = HarvesterLoadAmount * 60 / max(2*home_one_way_time,Q)` for each team and require Section-0.10 `PercentDifference(IPM_GDI,IPM_NOD) <= fairness.max_projected_initial_income_difference_percent`.
6. **Front fairness:** each base's shortest vehicle-capable time to any CONTESTED field uses `FinalDistance/V_REF`; require the absolute difference `<= fairness.max_base_to_front_travel_time_difference_seconds`.
7. **Base separation:** shortest heavy GDI-to-Nod one-way time is `>= fairness.min_base_to_base_travel_time_seconds` and `<= fairness.max_base_to_base_travel_time_difference_seconds` under the legacy hybrid semantics in Section 0.10.
8. **Bridge dependency:** when `fairness.allow_single_bridge_dependency=false`, remove each physical bridge one at a time in ascending bridge `StableId`; GDI-to-Nod heavy connectivity must survive every single removal. Restore the edge before testing the next bridge.
9. **Vehicle approaches:** each base retains at least `fairness.min_vehicle_approach_count` distinct heavy-capable frozen entrance-sector ordinals connected from `BASE_PROTECTED` to the final graph.
10. **Infantry approaches:** each base has at least `fairness.min_infantry_approach_count` distinct infantry-capable approaches. Distinct means their final capability paths leave `BASE_PROTECTED` through different one-of-16 Phase-8 sectors; multiple paths through the same sector count once.
11. **Expansion:** each base still has clear expansion area `>= fairness.min_base_expansion_area` by the existing Commander expansion-area calculator.
12. **Infantry exposure:** every mandatory segment passes both `fairness.max_mandatory_exposed_infantry_run_distance` and `fairness.max_mandatory_exposed_infantry_run_time_seconds` using `I_REF`; `preferred_cover_interval` remains a soft placement target and is not promoted to a failure threshold.
13. **Road geometry:** every realized road remains within its Section-7 max grade/cross-slope/min turn radius and swept-clearance rules.
14. **Crossings:** every bridge/tunnel/overpass is attached at both ends, within registered span/width/height/approach limits, and has the expected navigation link.
15. **Obstacle binding:** no realized road intersects river water, lake/coast water, cliff, canyon or ravine hard geometry without its exact Phase-11 compiled solution record. Lakes/coast have no road crossing solution in this generator version, so any road intersection with them fails.
16. **Quotas:** every Section-5 mandatory feature quota is still satisfied by distinct qualified final objects.
17. **Base layout:** every Phase-14 starting-building proxy transform, production exit, refinery/Harvester corridor and spawn clearance remains legal against final Phase-15/17 world state.
18. **Navigation agreement:** every mandatory planned connection exists with the required capability in the Phase-18 graph; no required capability exists only in planner metadata.
19. **Biome/material invariants:** single/mixed ownership and Section-14.2 priority masks match MapIdentity.
20. **Determinism:** immediately recompute every authoritative stage checksum from frozen records in Section-20 order; every value must match the value captured when that stage locked.

Any failure rejects the attempt with a named diagnostic. Do not modify the map to rescue Phase 19A.

### Phase 19B - transactional commit/activation

Phase 19B runs only after every 19A check passes. It may instantiate the already-frozen starting buildings/spawn bootstrap state, but it cannot choose new positions or modify generated terrain/topology.

Whole-world attempt behavior is exact:

```text
requested_seed = user/server seed
attempt_index  = 0,1,2
effective_seed = Normalize_Seed(requested_seed + attempt_index)
MAX_WORLD_GENERATION_ATTEMPTS = 3
```

A failed attempt is completely torn down in reverse owner order before the next effective seed starts. The global watchdog from Section 3 continues running and never resets.

When an attempt passes:

1. manifest stores requested seed, attempt index, effective seed, `HybridPlanningInputHash`, generator/schema version and content/descriptor revision;
2. write authoritative stage checksums in Section-20 order, including the frozen Phase-14 base-layout transforms;
3. mark generated world committed but not active;
4. on the server, call `ProceduralBaseLayoutService::Commit_Planned_Base_Layout` using the frozen transforms and then build the spawn pool from the committed buildings/Phase-14 clearances; clients receive those normal runtime building objects through the existing multiplayer path;
5. if base-layout commit fails, destroy the committed attempt and return a named map-bootstrap failure; do not search new building positions and do not set `WORLD_ACTIVE`;
6. set `WORLD_ACTIVE`;
7. only then allow player spawn, starting units, economy, AI and Commander gameplay.

When all three attempts fail or the global watchdog fires:

- do not commit the last incomplete world;
- do not set `WORLD_ACTIVE`;
- return failure through the existing map-load/request failure path;
- log the exact requested seed, all attempted effective seeds, `HybridPlanningInputHash`, content/descriptor revision and final named failure reason.

This intentionally replaces the older behavior that allowed the last incomplete world to stand after retry exhaustion.

# 9. Semantic-matrix compilation rules

The user's proposed numeric-layer concept is adopted here as a planning representation, with one critical improvement: layers remain separate until semantic compilation instead of later layers blindly replacing earlier numbers.

## 9.1 Layer order

Populate conceptual layers in this order:

```text
1 land/playable mask
2 elevation tiers and macro height bias
3 water intent
4 ridges/plateaus/hills
5 cliffs/canyons/ravines
6 base/resource reservations
7 transportation corridor influence
8 biome ownership
9 no-decoration/protected masks
```

The order controls dependency, not destructive overwrite.

Example:

```text
road influence + river influence
```

means `CrossingProblem(ROAD,RIVER)`, not "road wins" or "river wins".

Likewise:

```text
road + cliff
```

means cut/pass/tunnel/reroute evaluation.

## 9.2 Coarse-to-fine rule

The coarse matrix decides only broad intent. Fine positions come from vectors and local refinement.

Never raster-walk a road from one semantic cell to the next and use those cell edges as final road geometry. Instead:

```text
coarse corridor -> bounded A* -> smooth vector spline -> final road geometry
```

Likewise:

```text
coarse river corridor -> flow path -> river spline -> local river-bank geometry
```

This prevents stair-step roads/rivers and reduces memory/work.

---

# 10. Terrain and geology rules

Section 8 Phases 5 and 7 are the authoritative algorithms. This section records their invariants so later code cannot reinterpret them.

## 10.1 Terrain source formula

Final pre-road height is exactly the Phase-7 formula. `accepted road cut/fill` is added only in Phase 12 and tactical berm/fold corrections only in Phase 15. No later phase changes terrain height.

## 10.2 Explicit cliffs

A cliff record contains:

```text
StableId
centerline spline
upper-side sign
height difference
edge width = 0.5W..1.5W
rock/talus transition width = 1W..5.25W
allowed crossing/pass windows
```

A cliff is counted only if it meets Section-5 minimum length. Ordinary detail noise is clamped so it cannot independently create terrain steeper than `terrain.max_buildable_slope_degrees`; slopes beyond that cap must belong to an explicit cliff/canyon/ravine semantic feature.

## 10.3 Canyons and ravines

Use signed distance to the accepted centerline and a smooth profile:

```text
floor: |d| <= floor_half_width                 -> full depth
wall : floor_half_width < |d| < rim_half_width -> smoothstep from full depth to zero
outside: |d| >= rim_half_width                 -> zero canyon/ravine contribution
```

Branch joins use the minimum (deepest) signed height contribution inside the overlap followed by one `A`-wide smooth union zone. Do not simply sum overlapping depths.

A canyon/ravine floor is vehicle-routable only when its continuous legal floor width is at least the required swept vehicle width + `2*fairness.min_vehicle_lane_clearance`, its longitudinal grade is within the road class limit and both entry/exit ramps pass the same swept validator.

## 10.4 Hills/ridges/plateaus

Hill/ridge counts and ranges are exactly Section 8 Phase 5. Use compact-support smooth radial/spline fields that become exactly zero at their influence radius so neighboring features compose deterministically.

Plateau conversion uses the Phase-5 hash test and affects only the owning hill/ridge field. It cannot flatten across a road, base, water or resource reservation not owned by that feature.

# 11. Hydrology rules

Section 8 Phase 6 owns all hydrology planning. `WaterSystem` remains the only realization owner.

## 11.1 Rivers

Each accepted river record stores:

```text
StableId
source analysis cell
sink type/id
ordered flow-cell chain
smoothed centerline spline
accumulation samples
width samples from the Phase-6 formula
bed/water definition id
bank/floodplain influence bounds
```

River centerline smoothing is accepted only when every smoothed sample remains within `A` of the original D8 cell-chain polyline and never reverses downstream order.

River terrain carve includes bed plus two-bank blend. Bank blend half-width is `max(river_width,2W)` on each side. Floodplain/material influence may extend another `2*river_width`, but that outer influence does not flatten terrain.

## 11.2 Lakes

A true lake is a closed non-self-intersecting polygon from a retained basin or required archetype basin. Its water elevation is the basin spill elevation quantized to `Q`, clamped below the lowest dry shoreline sample by at least `Q`.

Minimum qualified area is Section 5. Lake polygons are simplified with tolerance `A/2`; simplification is rejected if it changes area by more than 5% or self-intersects.

No circular stamped fallback is allowed. Required synthetic/archetype basins are created by an irregular closed spline whose radial samples use eight equally spaced angles and radius multipliers `0.85..1.15` from keyed hashes, then are smoothed once with a circular three-point average.

## 11.3 Coast and islands

Exterior sea is one connected water polygon touching the world exterior. Islands are dry closed polygons fully surrounded by a selected lake or sea. Island slot creation and minimum area are fixed in Section 5.

Optional islands in this generator version are decorative geography only, are marked as LAND holes inside their host water polygon under Phase 6, and receive no base, Tiberium or mandatory-route role. They therefore require no access. A later generator version may add strategic islands with an explicit access contract.

# 12. Transportation and crossing rules

## 12.1 Roads are first-class logical objects

Every road record stores:

```text
StableId
semantic class
source strategic edge
spline
exact Section-7 cross section
speed hint
required navigation capabilities
junction records
crossing records
attempt-owned runtime road ids after Phase 12
```

Road geometry is never inferred from a texture mask.

## 12.2 Route independence

Heavy route independence is measured on the final transportation graph after Phase 18 navigation exists.

Before max-flow:

1. contract all graph edges/vertices lying inside each `BASE_PROTECTED` disc into one source/sink vertex per base;
2. replace every physical bridge, tunnel, canyon pass and road-over-road one-at-a-time crossing by one unit-capacity edge;
3. all ordinary heavy road edges also have unit capacity;
4. `TRAIL_1` edges and any edge failing heavy capability are removed.

Run deterministic Edmonds-Karp with adjacency lists sorted by edge `StableId`. Required max flow from GDI contracted source to Nod contracted sink is `>= route_family_count`.

Two splines that diverge and then share one bridge therefore count as one path through that bottleneck.

## 12.3 Bridge/overpass vertical geometry

River bridge:

```text
abutment_z = max(bank_a_road_z, bank_b_road_z)
deck_bottom_z >= water_surface_z + selected_bridge.water_clearance_margin
deck_top_z = max(road approach profile, deck_bottom_z + selected_bridge.structural_depth)
```

Approach profiles are extended backward until their grade is <= road-class max grade. If required approach length exceeds the reserved/searchable corridor, reject that bridge candidate.

Canyon/ravine bridge endpoints equal the connected road profiles at the two abutments. The deck is the minimum-grade profile between those endpoint elevations; it does not descend toward canyon floor.

Road-over-road clearance uses the heavy reference vehicle:

```text
safety = max(0.35 * world_units_per_meter, 0.10 * heavy_vehicle_height)
minimum_clear_vertical_gap = heavy_vehicle_height + safety
upper_deck_bottom_z - lower_road_surface_z >= minimum_clear_vertical_gap
```

Add the selected bridge/overpass structural depth above that bottom plane.

## 12.4 Bridge width

Minimum clear bridge width is:

```text
max(connected road carriageway width,
    required swept vehicle width + 2*fairness.min_vehicle_lane_clearance)
```

Both approach roads are widened/blended to the bridge clear width over the reserved straight approach when the bridge is wider. A bridge narrower than this value is rejected.

## 12.5 Tunnels

Tunnel is never mandatory in this generator version. It is a fallback only when:

```text
world_features.tunnels=true
tunnel_available=true
portal grades pass the connected road class
clear width >= required swept width + 2*fairness.min_vehicle_lane_clearance
clear height >= required vehicle height + safety from Section 12.3
both portals clear water/base/Tiberium/bridge-approach reserves
```

Select the shortest sufficient registered tunnel definition/module sequence; tie break by StableId. If none fits, continue to reroute/fail according to Phase 11. Do not invent tunnel geometry outside the existing canonical TunnelSystem capabilities.

# 13. Tiberium rules

Tiberium roles/counts/placement/access are fixed by Section 5 and Phase 13.

The semantic distinction is:

```text
HOME_*      = early economy, owned by one team
CONTESTED   = neutral strategic resource; at least one passes the explicit equal-access threshold
REMOTE_RICH = neutral remote objective; normal yield unless the existing schema supports the Phase-13 1.25 multiplier
```

No field may be placed by independent random scatter after Phase 13. Crystal scatter inside an accepted field remains deterministic and uses the existing `TiberiumFieldGenerator` algorithm.

Harvester validation uses real swept bounds and checks width, grade, turn radius, crossing clearance, base/refinery exit corridor and the field-edge turning disc. Point-path reachability alone is insufficient.

Field masks are:

```text
core
transition
contamination perimeter
```

Only `core` is a hard vehicle/resource footprint exclusion for unrelated roads. Transition/contamination affect materials/decoration but do not create a new biome.

# 14. Biome/material/foliage rules

## 14.1 Biome ownership

`single` mode gives 100% of ordinary playable land to one biome before semantic overrides. `mixed` mode gives every ordinary land sample exactly one primary biome ownership weight plus the Section-4.3 transition blend; no third biome can appear.

## 14.2 Material priority

Highest priority wins at a terrain sample:

```text
1 road/pavement
2 Tiberium core
3 Tiberium contamination transition
4 wet shore
5 exposed cliff/canyon/ravine rock
6 biome background/biome transition
```

A semantic material mask never modifies geometry.

Snow behavior: ordinary land is snow material regardless of elevation; only steep exposed rock and semantic overrides remove snow. Desert behavior: ordinary land is sand/dry soil regardless of elevation. Temperate behavior: ordinary land is grass/soil. Tiberium-wasteland behavior: ordinary land is barren/cracked/contaminated.

## 14.3 Foliage/props

Phase 17 is authoritative. Foliage uses clustered deterministic sampling through `FoliageSystem`, not independent uniform points. Biome palette controls which registered foliage types are eligible; an ineligible/missing type is omitted rather than substituted with an unrelated asset.

Collision-bearing decoration participates in the decoration stage checksum and must pass Phase-17 structural exclusion tests before it is instantiated.

# 15. Tactical FPS/RTS coexistence

The map must satisfy both commander-scale movement and first-person combat. No separate subjective tuning pass is allowed to override the quantitative validators.

- Vehicle maneuverability is enforced by swept-envelope paths, open-maneuver polygon requirements and route-independence validation.
- Infantry exposure is enforced by the existing fairness distance/time thresholds and Phase-15 repair algorithm.
- Chokepoints are explicit compiled objects/regions: bridge, mountain pass, canyon crossing, tunnel or narrow corridor.
- No generated route family pair is allowed to share every chokepoint; Section-12.2 max-flow is the final authority.
- Open areas are not decorated to a target object count; decoration stops whenever the legal cluster-placement process runs out of valid candidates.

# 16. Performance implementation rules

## 16.1 Deterministic work ceilings

Wall-clock time is not used to decide authoritative candidates. The hard watchdog may abort the request, but deterministic work ceilings below bound pathological seeds identically on every machine.

Let:

```text
N = analysis_grid_width * analysis_grid_height
R = max routed-edge count by size
R_SMALL=18, R_MEDIUM=32, R_LARGE=52
```

Mandatory ceilings:

```text
A* expansions per route = 8*N
A* expansions per attempt = R * 8*N
crossing stations per CrossingProblem = 64
CrossingProblems per routed edge = 12
LOS rays SMALL/MEDIUM/LARGE = 512 / 2048 / 4096
local tactical repair candidates per failed segment =
    3 * retry.max_alternate_candidates_per_feature
whole-world attempts = 3
workers = 4 max
```

If an edge exceeds its A* expansion ceiling, that route attempt fails and uses the documented local reroute sequence; do not raise the ceiling at runtime.

Routed-edge creation itself may not exceed `R`. Phase 9 must fail if mandatory graph requirements would exceed that cap; the default native profile/quotas are designed to remain below it.

Expose counters for:

```text
semantic cells processed
analysis cells processed
A* state expansions
routed edges
crossing problems
crossing candidate stations
LOS rays
local refinement samples
whole-world attempt count
peak scratch bytes
```

No P99-derived tuning step is required to finish the implementation. Future generator versions may change these constants only with explicit benchmark evidence and a version bump.

## 16.2 Search bounds

- road A* uses only the Phase-10 corridor and at most two `+S` expansions;
- bridge/crossing search uses the Phase-11 window and 64-station cap;
- vector intersections use `ProceduralPlanningSpatialHash`;
- local refinement uses only the accepted feature AABB expanded by its blend width;
- tactical LOS checks only the Phase-15 listed mandatory corridors;
- no all-pairs visibility graph or all-feature intersection scan.

## 16.3 Memory

Use one generation scratch arena or reusable pool set. Peak attempt-owned scratch memory must remain below:

```text
SMALL  128 MiB
MEDIUM 256 MiB
LARGE  512 MiB
```

The arena is reused for semantic matrices, analysis buffers, A* state/open lists, candidate arrays, hydrology buffers, refinement patches and validation scratch. Rejected heavy/runtime objects are destroyed immediately and never retained across attempts.

## 16.4 Content lookup

Build the immutable metric cache once before Phase 2 from definitions/collision descriptors. Do not load full render assets merely to inspect a metric already available from definition/collision metadata.

## 16.5 Navigation

The global authoritative navigation build occurs once in Phase 18. Earlier phases use analysis grids, splines and swept proxy tests only. Runtime owners may build small local road/bridge geometry data required for their own realization in Phase 12, but they must not trigger a full global `PathfindClass` rebuild.

# 17. Deterministic retries and repair

Local repair stays inside the phase that owns the failing feature. There is **no free-form generic search**. The exact sequence is:

```text
1 advance the owning phase's already-defined, stably ordered candidate iterator
2 if that phase explicitly defines a bounded corridor expansion, perform its next expansion
3 if that phase explicitly defines an ordered crossing-method fallback, try its next method
4 when that phase's documented candidate/method limits are exhausted, fail the phase
5 tear down the attempt and start the next whole-world effective seed
```

A phase that does not define one of those operations does not gain it from this section. This section never authorizes new coordinates, a new crossing type, a new tunnel, wider search bounds, weaker constraints or an implementer-selected fallback.

Local candidate/retry limits come directly from:

```text
retry.max_local_retries_per_stage
retry.max_alternate_candidates_per_feature
```

Whole-world retries are exactly Section-8 Phase-19's three effective seeds. Attempt teardown preserves only immutable content metrics and reusable scratch capacity; it clears all semantic matrices, vectors, runtime ids and attempt checksums.

Release quality targets remain diagnostics, not runtime decisions:

```text
>=95% MEDIUM seeds pass attempt 0
>=99% MEDIUM seeds pass by attempt 1
<=1 exhausted seed in the mandatory 256-seed MEDIUM batch
```

If these targets fail, fix planner quality. Do not weaken quotas, silently widen fairness thresholds or increase whole-world attempts.

# 18. Validation and acceptance tests

## 18.1 Mandatory cold seed batches

```text
256 MEDIUM seeds
64 SMALL seeds
64 LARGE seeds
32 dedicated-server MEDIUM seeds
```

For every seed record:

```text
generation wall time
peak transient bytes
attempt index
A* expansions
routed-edge count
crossing-problem/station count
refinement sample count
LOS rays
worker count
all stage checksums
```

A cached completed world is disabled for these tests.

Wall-clock release targets from Section 3 are evaluated on the same declared CI/reference machine for all compared builds. The hard watchdog is always enforced in real map-start execution.

## 18.2 Determinism tests

For a fixed representative seed set:

```text
1 worker vs 4 workers -> identical authoritative stage/final checksums
client vs dedicated server -> identical topology/physics-authoritative checksums
repeated same input -> identical checksums
effective seed N reconstructed from manifest -> identical world
save/load reconstruction -> identical generated-world identity before runtime mutations
```

Render-only dedicated-server objects are excluded only if the existing manifest already marks those exact records non-authoritative.

## 18.3 Automatic gameplay/geometry validation

Every batch map must pass:

```text
heavy base-to-base path
Edmonds-Karp route_family_count independence
Harvester path from each base to home field
required contested-field equal-access check
all road class grade/turn/cross-slope limits
bridge/tunnel/overpass span/width/height/attachment checks
Section-5 quotas
starting-base footprints/production exits/expansion area
spawn sightline and mandatory infantry exposure thresholds
navigation vs planned-topology agreement
single/mixed biome ownership invariants
no decoration intersection with structural exclusion envelopes
```

## 18.4 Live visual/render smoke suite

The coding agent must create/document deterministic seeds covering:

```text
temperate single
snow single
desert single
tiberium-wasteland single
mixed biome
coastal_peninsula
highland_pass
```

The smoke suite verifies rendering/integration defects that numeric tests cannot see; it does **not** authorize design changes. A seed fails visual smoke when any of these concrete defects is visible:

```text
road ribbon visibly detached from its collision/terrain grade
bridge deck detached from either road approach
bridge visibly intersects river bank/canyon wall incorrectly
water surface visibly climbs terrain or leaves its closed lake/coast polygon
cliff/canyon mesh has a visible hole/self-overlap at a spline join
terrain material ignores the priority mask, including a road covered by biome material or a vertical cliff/canyon/ravine face covered by ordinary biome ground material
foliage/prop visibly stands in road, water, bridge approach or base production corridor
mixed biome appears as isolated checkerboard/islands rather than the one divider/transition band
```

Driving smoke covers Humvee, heavy reference vehicle, Harvester and infantry on at least one example of each applicable route/crossing class. A visual failure is logged against the owning phase and fixed there; the agent does not ask the user how the defect should be interpreted.

# 19. Exact implementation/migration sequence

This is the coding order. Do not implement a later item before the gate/test named by the previous item passes. The runtime Phase number is shown so implementation order and runtime dependency cannot drift.

1. **I0 / runtime Phase 0:** inventory canonical owners, seed domains, profiles, self-checks and live procedural entry points; record baseline build/tests.
2. **I1 / Phase 1:** add hybrid records, enums/profile fields, parser migration and one schema/generator version bump; do not change generation output yet.
3. **I2 / shared prerequisite:** implement Section-0.3 deterministic helpers, metric-cache preflight and `ProceduralPlanningSpatialHash`; add unit/self-checks for each.
4. **I3 / Phase 2:** implement immutable `MapIdentity` resolution/checksum only.
5. **I4 / Phase 3:** implement strategic role/route-family graph in memory only; add graph-cycle/role-count checks.
6. **I5 / Phase 4:** implement semantic matrices/reservation compiler; add matrix checksum/conflict checks. No new terrain output yet.
7. **I6 / Phase 5:** implement vector landform planner and counts; adapt no runtime geometry owner yet beyond debug/test records.
8. **I7 / Phase 6:** implement vector hydrology/priority-flood/D8/basin planning; adapt `WaterSystem` interface only enough to consume later accepted vectors.
9. **I8 / Phase 7:** replace native macro geography decision logic with semantic/vector-driven height realization through `WorldTerrainSystem`; keep authored/legacy path intact.
10. **I9 / Phase 8:** adapt base/resource anchor selection to frozen semantic reservations + realized terrain; add exact fairness/exit-sector tests.
11. **I10 / Phase 9:** replace native MST topology with route-family cyclic graph and semantic road classes; no A* yet until topology self-check passes.
12. **I11 / Phase 10:** implement bounded heading-aware A*, corridor expansion and spline constraints; keep crossing intervals unresolved records.
13. **I12 / Phase 11:** implement unified deterministic interaction solver and bridge/overpass/tunnel/cut/pass selection using existing runtime definitions.
14. **I13 / Phase 12:** wire accepted splines/solutions into existing Road/Bridge/Tunnel systems, road cut/fill and swept-envelope validation.
15. **I14 / Phase 13:** adapt Tiberium generator to fixed role counts/footprints/access and contestability check.
16. **I15 / Phase 14:** adapt existing base-layout/placement-rule planner to frozen base centers and reserve all start/production/expansion envelopes.
17. **I16 / Phase 15:** implement tactical exposure analysis and exact local repair while respecting all structural exclusion envelopes.
18. **I17 / Phases 16-17:** implement global/mixed biome masks, material priority and deterministic decoration. Cities remain disabled.
19. **I18 / Phase 18:** consolidate integration so the full navigation build occurs once after all geometry; verify no earlier phase still triggers a global rebuild.
20. **I19 / Phase 19:** implement read-only final validator, exact three-attempt behavior, manifest effective-seed fields, failure-on-exhaustion and activation gate.
21. Run Section-18 batch/determinism/performance/live smoke gates.
22. Only after all gates pass, remove obsolete **native hybrid** planner branches that have no remaining caller. Keep authored/legacy plan-disabled compatibility paths.
23. Reconcile repository documentation under Section 22.

Each implementation item must leave the project buildable. When an old and new record must coexist during migration, add an adapter and delete it only in item 22 after call-site search proves it unused.

# 20. Required stage manifest/checksum order

The final generated-world manifest must represent enabled authoritative stages in stable order:

```text
1 generator/profile/content identity
2 map identity/archetype/biome mode
3 strategic graph
4 coarse semantic matrices
5 vector landforms
6 vector hydrology
7 macro terrain
8 selected bases/resources
9 transportation topology
10 road splines
11 crossing solutions
12 final roads/bridges/tunnels/overpasses
13 Tiberium fields
14 base-layout reservations
15 tactical terrain/cover refinement
16 material/biome state relevant to deterministic world identity
17 authoritative foliage/static state when repository treats it as gameplay/collision relevant
18 navigation/integration revision
19 final world revision
```

Disabled/non-authoritative render-only stages use the repository-standard absent/zero representation consistently.

---

# 21. Networking, save/load and runtime mutation

Do not transmit generated meshes as the normal network architecture.

Server/client reconstruction remains based on:

```text
requested seed
effective seed / attempt index
profile
version
content revision
```

The server is authoritative for the accepted `attempt_index`/`effective_seed`. Clients do not independently try attempt 0/1/2 after the server has accepted one; they reconstruct the exact accepted effective seed and compare authoritative checksums.

Client/server authoritative generation checksums must match before gameplay proceeds according to the existing generated-world synchronization design.

Save/load stores/reconstructs the same generator identity plus runtime mutations through the existing persistence system.

Commander construction, bridge destruction, terrain/runtime world changes and later gameplay mutation operate **after** the initial generated world has committed. They do not rerun the startup generator.

---

# 22. Documentation reconciliation

At completion update the repository's procedural documentation so it describes the implemented hybrid pipeline and no longer claims that:

- roads are derived from a minimum spanning tree;
- major geography is primarily random noise;
- city/settlement generation is active in the native pipeline;
- biome selection normally creates multiple local climate patches;
- performance budgets are unspecified;
- bridges are discovered only after roads have already been committed;
- retry exhaustion may activate the last incomplete world.

Document city/settlement generation as a **future feature requiring a defined civilian/abandoned-building asset set and a later generator-version bump**, not as unfinished work inside this rewrite.

---

# 23. Definition of done

The rewrite is complete only when all of the following are true:

- procedural generation occurs during map startup and meets the performance gates;
- the default native generator uses strategic graph -> semantic matrix -> vector features -> interaction solver -> local refinement -> final geometry;
- no active procedural city/settlement generation exists;
- default maps use one broad biome theme, with mixed mode available through the explicit randomizer/profile control;
- roads form a deliberate cyclic hierarchy rather than a tree;
- highways, two-lane roads, dirt roads and trails have distinct semantics;
- road routes use bounded analysis-grid search and smooth splines;
- river/lake/cliff/canyon/ravine features are coherent continuous structures;
- bridges/overpasses/tunnels are selected before final road terrain deformation;
- bridge approaches and clearances pass real vehicle-envelope validation;
- Tiberium fields are role-based, distributed and Harvester-accessible;
- base start layouts and production exits are reserved before cover/foliage;
- tactical refinement protects FPS play without destroying RTS vehicle maneuver space;
- terrain theme is consistent on single-biome maps and broad/contiguous on mixed-biome maps;
- expensive navigation is built once after final geometry;
- all mandatory seed-batch, performance, determinism and gameplay tests pass;
- legacy authored maps remain unaffected when procedural generation is not selected;
- existing canonical runtime owners are reused instead of replaced without cause.

---

# 24. Final instruction to the coding agent

Implement the Section-19 coding sequence strictly in order and the Section-8 runtime phases strictly in order. Do not skip a gate because a later phase would make an earlier test easier. Do not ask the user to choose an algorithm, constant, fallback, feature count, biome rule, road class, crossing method or retry behavior: this document supplies the choice. When source symbols differ, apply Section 0.1 and continue.

The first meaningful milestone is intentionally ugly:

> A gray/untextured generated battlefield in which bases, Tiberium fields, rivers, lakes, cliffs, canyons, ravines, highways, two-lane roads, dirt roads, trails, bridges, overpasses and optional tunnels appear in coherent places; required vehicles can traverse them; alternate strategic routes exist; and generation finishes within the startup budget.

Biome materials, foliage, lamps and other visual dressing are considered complete only after that milestone passes the seed-batch and traversal gates.

The generator is not finished because every requested feature appeared somewhere. It is finished when those features form a coherent battlefield, interact correctly, validate physically, reproduce deterministically, and are cheap enough to create while the map loads.
