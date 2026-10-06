# World Profile Schema (V2 Phase 1)

Roadmap Section 34 Phase 1's required output. Data owner: `ProceduralWorldProfile`
(`Code/Combat/proceduralworldprofile.h`). Loading/validation:
`ProceduralWorldProfileManager` (same file pair). Mirrors `BuildingModelDescriptor`/
`BuildingDescriptorManager`'s own split (v1, `buildingdescriptor.h`) exactly: pure data class,
separate loader/validator, nlohmann JSON, same `FAIL`-macro validation style.

`SchemaVersion` is this struct's own field-set version, bumped only when a field is added,
removed or renamed. `GeneratorVersion` identifies the algorithm a seed+profile will be run
through (Phase 2 onward); the two are independent (Section 3B.6).

## Example

```json
{
  "schema_version": 1,
  "generator_version": 1,
  "seed": 123456789,
  "name": "example",

  "world": { "width": 4096.0, "height": 4096.0, "chunk_size": 128.0, "sea_level": 0.0 },

  "boundary": {
    "shape": "irregular_ring",
    "cliff_band_width": 180.0,
    "cliff_min_height": 90.0,
    "cliff_max_height": 180.0,
    "hard_barrier_offset": 20.0,
    "air_ceiling": 260.0
  },

  "terrain": {
    "macro_relief": 0.6, "hill_density": 0.45, "mountain_density": 0.18,
    "interior_cliff_density": 0.16, "valley_density": 0.30
  },

  "water": { "rivers": true, "lakes": true, "ponds": true, "coastline": true },

  "world_features": {
    "forests": true, "settlements": true, "tunnels": true, "highways": true, "tiberium_fields": true
  },

  "retry": { "max_local_retries_per_stage": 8, "max_alternate_candidates_per_feature": 6 },

  "fairness": {
    "max_initial_resource_travel_time_difference_seconds": 15.0,
    "max_projected_initial_income_difference_percent": 10.0,
    "max_base_to_front_travel_time_difference_seconds": 20.0,
    "max_base_to_base_travel_time_difference_seconds": 20.0,
    "min_vehicle_approach_count": 2,
    "min_infantry_approach_count": 2,
    "min_base_expansion_area": 4096.0,
    "allow_single_bridge_dependency": false,
    "max_mandatory_exposed_infantry_run_distance": 60.0,
    "max_mandatory_exposed_infantry_run_time_seconds": 12.0,
    "preferred_cover_interval": 25.0,
    "min_vehicle_lane_clearance": 1.0
  }
}
```

Every block is optional; an absent block keeps every field's documented default (shown above --
they are also `ProceduralWorldDimensions`/etc.'s own C++ default-member-initializer values, so
the JSON example and the struct defaults can never silently drift apart).

## Per-field table (Section 4A's required semantics/defaults/limits contract)

| Field | Units | Meaning | Default | Hard constraint | Checksum-affecting |
| --- | --- | --- | --- | --- | --- |
| `world.width` | world units | playable-plus-boundary extent, X axis | 4096.0 | > 0, finite | yes |
| `world.height` | world units | playable-plus-boundary extent, Y axis | 4096.0 | > 0, finite | yes |
| `world.chunk_size` | world units | `WorldChunkManager` grid cell size | 128.0 | > 0, <= width and height | yes |
| `world.sea_level` | world units | height every hydrology/terrain height is relative to | 0.0 | finite | yes |
| `boundary.shape` | tag | generator style (legal set defined by Phase 2) | `irregular_ring` | non-empty | yes |
| `boundary.cliff_band_width` | world units | ring width reserved for the boundary cliff | 180.0 | > 0; two bands must leave a positive interior on the shorter world axis | yes |
| `boundary.cliff_min_height` | world units | shortest legal cliff segment height | 90.0 | > 0 | yes |
| `boundary.cliff_max_height` | world units | tallest legal cliff segment height | 180.0 | >= cliff_min_height | yes |
| `boundary.hard_barrier_offset` | world units | distance past the cliff band where actors/geometry are hard-rejected | 20.0 | >= 0, finite | yes |
| `boundary.air_ceiling` | world units above sea level | aircraft flight-volume top (Section 1.5) | 260.0 | > cliff_max_height | yes |
| `terrain.macro_relief` | normalized weight, 0..1 | overall relief intensity (Section 18B MACRO scale) | 0.6 | >= 0, finite | yes |
| `terrain.land_elevation_bias` | world units | raises ordinary land above sea level before height clamping; does not move water or the boundary cliff foot | 0.0 | finite | yes |
| `terrain.hill_density` | normalized weight, 0..1 | see terrain settings note below | 0.45 | >= 0, finite | yes |
| `terrain.mountain_density` | normalized weight, 0..1 | see note below | 0.18 | >= 0, finite | yes |
| `terrain.interior_cliff_density` | normalized weight, 0..1 | see note below | 0.16 | >= 0, finite | yes |
| `terrain.valley_density` | normalized weight, 0..1 | see note below | 0.30 | >= 0, finite | yes |
| `water.rivers`/`lakes`/`ponds`/`coastline` | bool | feature enabled | all `true` | -- | yes |
| `world_features.forests`/`settlements`/`tunnels`/`highways`/`tiberium_fields` | bool | feature enabled (`tunnels` drives only the legacy fit-to-level generator; the native hybrid generator builds no tunnels and ignores it, D-032) | all `true` | -- | yes |
| `retry.max_local_retries_per_stage` | count | Section 3C.1 step-1 ceiling every generator stage's local-adjustment loop is bounded by | 8 | > 0 | no (soft/behavioral, does not change chosen output given the same seed unless a stage actually exhausts it) |
| `retry.max_alternate_candidates_per_feature` | count | Section 3C.1 step-2 ceiling | 6 | > 0 | no, same reasoning |
| `fairness.*` | see Section 17A.1 | quantitative fairness thresholds, consumed starting Phase 15/17A | see example above | each field's own sign (nonnegative unless noted) | no (soft preference thresholds a repair pass may fail against, not generation inputs themselves) |
| `fairness.min_base_to_base_travel_time_seconds` | seconds at the base selector's reference vehicle speed | minimum route time between opposing starting bases on large FPS worlds | 0.0 | >= 0 and <= the configured maximum base-to-base route time | yes |

**Density-field semantics** (Section 4A's explicit requirement -- these are not left
implementation-defined): each of `terrain.hill_density`/`mountain_density`/
`interior_cliff_density`/`valley_density` is a **normalized weight**, not an area fraction, a
feature count, or a probability. `0.0` means the terrain generator (Phase 3/4) may omit that
feature category entirely; `1.0` means the generator's own maximum density for that category.
The mapping from weight to actual feature count/area/placement density is the terrain
generator's concern, established when Phase 3/4 is implemented -- this schema only carries the
weight.

**Hard constraint vs. soft preference**: every field in the first table block (world/boundary/
terrain/water/features) is a hard constraint -- `Validate_Profile` rejects the whole profile if
violated. The `fairness.*` block is soft preference data: Phase 1's own validation only checks
each field is finite and correctly signed (a malformed number is still rejected), but exceeding
a fairness threshold during generation is a Phase 15 combat-space-validation failure, not a
Phase-1 schema-validation failure -- the distinction Section 17A itself draws between "the
number is well-formed" and "the generated world satisfies it."

## Loading and validation

```text
ProceduralWorldProfileManager::Load_Profile_From_Text(json_text, &profile, &error)
ProceduralWorldProfileManager::Load_Profile_From_File(file_path, &profile, &error)
ProceduralWorldProfileManager::Validate_Profile(profile, &error)
```

`Load_Profile_From_Text`/`Load_Profile_From_File` parse only -- a syntactically valid JSON with
a semantically invalid value (e.g. `world.width: -1`) still parses successfully and must be
separately validated with `Validate_Profile` before use, exactly mirroring
`BuildingDescriptorManager`'s own parse/validate split. Every rejection carries a specific,
actionable reason string (`"world.width must be positive"`, never a bare `false`).

## Hashing and the generation manifest

See `docs/procedural_world/GeneratorDeterminism.md` for `ComputeProfileHash`,
`ComputeGenerationContentHash`, `WorldGenerationManifest`, and the RNG/seed-derivation contract
every later phase consumes.


## Schema version 2 (Phase 3)

Six fields were added when Phase 3 built the cliff geometry that consumes them. A schema-1
profile still loads; each new field takes the default below.

### `boundary` — the rest of Section 5.5's named controls

| Field | Type | Default | Rule | Meaning |
| --- | --- | --- | --- | --- |
| `rim_roughness` | float | `0.35` | 0..1 | How far the cliff rim wanders inside the min/max height band. 0 puts every station on the **midpoint** of that band, not on the minimum, so turning roughness off gives a level rim of sensible height. 1 is the generator's own maximum. |
| `face_roughness` | float | `0.45` | 0..1 | How much the face inflects between toe and rim. 0 builds the face as a single row of quads, which is exactly the geometry an escarpment produced before Phase 3. |
| `toe_width` | float | `14.0` | `>= 0`, `< cliff_band_width` | World units of scree apron at the foot of the face. Wider than the band would put scree in the playable interior, which Section 5.9 forbids as firmly as it forbids building there. |
| `collision_simplification` | int | `2` | `>= 1` | Render stations per collision station. **Parsed, validated and hashed; not consumed** — see `WorldBoundsAndBoundaryCliffs.md` for why an escarpment's collider being the mesh it draws makes this unimplementable without introducing a second answer to "is there a wall here". |

Values above 1 for either roughness are **rejected**, not clamped: a value above 1 is not "more
variation", it is a profile author misunderstanding the unit, and clamping would hide that.

### `world` — placing a world over a hand-authored level

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `fit_to_level` | bool | `false` | Take `width`, `height`, `sea_level` and the centre from the host level's own measured extents (`PhysicsSceneClass::Get_Level_Extents`) instead of from this file. The fields above then act only as the fallback for a level reporting no usable extent. |
| `center_x`, `center_y` | float | `0.0` | Where the world sits. The origin is right for a world generated from nothing; a stock Renegade map is not built around the origin, so a ring centred there would enclose the wrong ground. |

The right size for a ring enclosing a hand-authored map is a **measurable property of that map**,
not a number a profile author should have to know — which is why `fit_to_level` exists rather
than a documented "typical Renegade map size". The values actually used are recorded in the
generation manifest, so a fitted world stays reproducible.

## Schema version 3 (Phase 4)

Seven fields were added to the `terrain` block when Phase 4 built the macro terrain generator
Section 18B's density weights were parsed-but-unconsumed placeholders for. A schema-1 or
schema-2 profile still loads; each new field takes the default below. See
`docs/procedural_world/MacroTerrainGeneration.md` for what the generator actually does with
them.

| Field | Type | Default | Rule | Meaning |
| --- | --- | --- | --- | --- |
| `min_elevation` | float | `-30.0` | finite, `<= max_elevation` | World units relative to `world.sea_level` — the relief floor a basin can reach. |
| `max_elevation` | float | `160.0` | finite, `< boundary.air_ceiling` | World units relative to `world.sea_level` — the relief ceiling a hill/ridge can reach. Section 1.5's aircraft rule extends naturally: nothing generated may reach where aircraft fly. |
| `max_buildable_slope_degrees` | float | `28.0` | `(0, 90)` exclusive | Both the generation-time ordinary-slope cap (step 9) and the chunk registry's buildability threshold. One number serves both deliberately — see `MacroTerrainGeneration.md`. |
| `feature_scale` | float | `900.0` | `> 0` | World units — the approximate wavelength of a macro landform. |
| `ridge_sharpness` | float | `0.55` | `0..1` | How sharply mountain-density noise peaks along its own ridge line. |
| `plateau_chance` | float | `0.15` | `0..1` | How much of the elevated area gets terraced into a flat-topped rise. `0` means the generator never does. A heightfield approximation of a mesa, not Phase 5's vertical-faced one. |
| `boundary_approach_flattening` | float | `0.6` | `0..1` | How strongly relief is pulled toward the flat cliff band as the boundary-approach zone nears it. |

Values above 1 for the three normalized weights are **rejected**, not clamped, the same rule
`rim_roughness`/`face_roughness` already established: a value above 1 is a profile author
misunderstanding the unit, not "more variation" to silently allow.

## Schema version 4 (Phase 6)

Four fields were added to the `water` block when Phase 6 built the hydrology generator
Rivers/Lakes/Ponds/Coastline were flags for since Phase 1 with nothing to size them. A
schema-1/2/3 profile still loads; each new field takes the default below. See
`docs/procedural_world/Hydrology.md` for what the generator actually does with them.

| Field | Type | Default | Rule | Meaning |
| --- | --- | --- | --- | --- |
| `river_width` | float | `6.0` | `> 0` | World units — the width a channel reaches once its own accumulated drainage meets `river_accumulation_chunks`. |
| `stream_width` | float | `2.5` | `> 0`, `<= river_width` | World units — the width at a channel's own source, before accumulation has grown it. |
| `river_accumulation_chunks` | float | `6.0` | `> 0` | Chunks of upstream drainage a channel needs before it counts as a river rather than a stream/tributary (Section 9.2). |
| `max_river_count` | int | `4` | `> 0` | Bounded count of distinct channel systems generated, same bounded-output discipline every other generator in this schema already follows. |

Depth is deliberately not a profile field: `WaterDefinitionClass::BedDepth`, already set per
water category by `WaterSystem::Define_Default_Water`, is carve depth's one source of truth. A
second depth number here could only ever disagree with it.

## Schema version 5 (Phase 12)

The `resource` block gained the Tiberium field generator's own controls — field radius range,
initial yield range, crystal density, harvest anchor count, Harvester load amount and the two
growth fields. See `docs/procedural_world/TiberiumFields.md`; the fields are listed in
`ProceduralWorldResourceSettings` (`Code/Combat/proceduralworldprofile.h`) with their defaults.

## Schema version 6 (Phase 17)

### `grading` — what the Commander may do to ground a generator already made

Section 34 Phase 17 step 5's four limits, named exactly as that step lists them. Absent in a
schema-1..5 profile, which still loads; each field takes the default below.

Every one of these is a ceiling on changes to terrain that a generator produced and that Phase 15's
combat-space validation already accepted, which is why they belong to the profile rather than to a
building descriptor: a descriptor can say how steep a foundation *tolerates*, but only the world can
say how much of itself may be *moved*.

| Field | Type | Default | Rule | Meaning |
| --- | --- | --- | --- | --- |
| `max_cut_depth` | float | `6.0` | `> 0` | World units — how far below the original ground a single foundation may dig. |
| `max_fill_height` | float | `6.0` | `> 0` | World units — how far above it a single foundation may build up. |
| `max_slope_change_radians` | float | `0.5` | `> 0`, `< π/2` | The steepest face the transition apron may be left at. Defaults to the same buildable-slope cap every terrain-reading generator in this chain already shares, so a grading can never leave ground steeper than ground a generator would have been willing to make. |
| `max_affected_area` | float | `4096.0` | `> 0` | World units², in plan — the levelled pad plus its apron. The one limit that bounds how much of the map a single building can reshape. |

All four are validated strictly positive. A zero limit does not mean "no grading" — it would refuse
every grading including a no-op one — so it is a broken profile rather than a strict one, and
`FoundationGradingService::Set_Limits` clamps it back to the documented default. A profile that
wants no grading turns the Commander off, not this.

See `docs/procedural_world/CommanderExpansion.md` for what the service actually does with them,
including why sizing the apron is a bounded fixed-point solve against a measurement rather than a
formula.

## Schema version 7 (Phase 21)

### `logistics` — Section 22.1's own profile toggles

Absent in a schema-1..6 profile, which still loads; each field takes the default below, and the
defaults are chosen so an absent block leaves the logistics graph observable-but-inert per
Section 22.1's own "default gameplay must remain viable without mandatory micromanagement".

| Field | Type | Default | Rule | Meaning |
| --- | --- | --- | --- | --- |
| `enabled` | bool | `false` | — | Whether `ProceduralWorldManager::Generate_World` builds the logistics graph (`LogisticsGraphService`) at all. |
| `convoy_enabled` | bool | `false` | — | A further, separate opt-in: whether `LogisticsGraphService::Start_Convoy` accepts any order at all. |
| `construction_support_radius` | float | `0.0` | `>= 0` | World units. `0` means no bonus (Section 22.2 step 5); `LogisticsGraphService::Is_Under_Construction_Support` reads this back. |

See `docs/procedural_world/LogisticsLayer.md` for what the graph actually is and which of Section
22's seven named node kinds have a real source in this tree today.

## Schema version 8 (Phase 22)

### `vehicle_subsystems` — Section 23's own optional vehicle damage toggles

Absent in a schema-1..7 profile, which still loads. Section 23's opening rule — "Do not alter
default Renegade vehicle handling unless enabled by game profile" — is the whole design of this
block: `enabled` defaults to `false`, and a legacy or stock map never loads a profile at all, so
`VehicleGameObj::Apply_Control`'s live control path is provably byte-for-byte what it has always
been unless a profile author asks otherwise.

| Field | Type | Default | Rule | Meaning |
| --- | --- | --- | --- | --- |
| `enabled` | bool | `false` | — | Master toggle. While false every accessor returns its canonical answer at every health, so the feature is behaviorally absent rather than merely quiet. |
| `allow_immobilize` | bool | `false` | — | Section 23 step 5's own separate opt-in. While false, health alone never removes basic drivability — the engine floor below is the worst any driver sees. Ignored entirely when `enabled` is false. |
| `engine_damage_start_fraction` | float | `0.66` | `0..1` | Health fraction *below which* acceleration and sustained propulsion begin to degrade. `0` switches engine degradation off without touching the others. |
| `mobility_damage_start_fraction` | float | `0.5` | `0..1` | Health fraction *below which* turning begins to degrade. `0` switches mobility degradation off without touching the others. |
| `weapon_damage_start_fraction` | float | `0.66` | `0..1` | Health fraction *below which* rate of fire begins to degrade. `0` switches weapon degradation off without touching the others. |
| `min_engine_factor` | float | `0.25` | `0..1` | The engine factor at zero health. A floor rather than zero, so engine damage alone never fully immobilizes. |
| `min_mobility_factor` | float | `0.35` | `0..1` | The mobility factor at zero health. |
| `min_weapon_factor` | float | `0.3` | `0..1` | The rate-of-fire factor at zero health. At `0.3` a wrecked turret takes ~3.3× as long between shots. |
| `immobilize_fraction` | float | `0.05` | `0..1` | Consulted only when `allow_immobilize` is true: health fraction at or below which all movement input is zeroed. |
| `weapon_disable_fraction` | float | `0.0` | `0..1` | Optional hard cut-off: health at or below which the weapon stops entirely. `0` (the default) turns this rule off, so a damaged weapon slows but never falls silent. |
| `retreat_to_repair_fraction` | float | `0.3` | `0..1` | Health fraction at or below which a vehicle with **no human driver** walks itself to the nearest same-team repair bay. A human driver's own choices are never overridden. |

All three subsystems share one response shape: untouched above the start threshold, then linear
down to the configured floor at zero health. `terrain_vehiclesubsystems` proves each curve is
bounded, monotonic (taking damage can never make a vehicle faster or its weapon quicker), lands
exactly on the configured floor, that switching one subsystem's threshold to `0` leaves the other
two alone, and that every answer is canonical at every health while `enabled` is false.

**Engine damage costs real top speed, not just a smaller input number.** Scaling the throttle is
enough for a tracked vehicle — its track resistance is linear in speed, so terminal velocity is
genuinely proportional to throttle — but not for a wheeled one, whose top speed is set by where
its engine torque curve falls off at a throttle-independent redline, with the quadratic drag term
that would otherwise cap it defaulting to zero. A damaged wheeled vehicle would therefore
accelerate worse and still reach nearly its normal top speed. `VehicleGameObj::
Govern_Subsystem_Speed` closes that at the control layer, withholding propulsion above
`engine_factor` × the fastest ground speed that vehicle has been observed to reach while
undamaged. The physics integrator is not modified, per Section 1.9.

**Rate of fire degrades per weapon instance.** `WeaponClass::Set_State` divides the freshly
computed `StateTimer` by the owning vehicle's weapon efficiency — the same per-instance,
owner-conditional multiply the adjacent `WEAPON_RATE_CHEAT` block already performs. The shared
`AmmoDefinitionClass::RateOfFire` is never written, so one damaged tank does not slow that
weapon's rate for every other object using the same preset, and the modifier is owner-gated so
infantry weapons are untouched.

**One honest gap remains.** Section 23.1 lists `sensor damage -> reduced AI targeting`, but no
sensor, perception or detection-radius concept exists anywhere in `Code/Combat` to hook into —
implementing it would mean inventing a targeting system, not wiring an existing one, so no
`sensor_*` field is offered rather than shipping one nothing reads. Accuracy is likewise not
degraded: the only per-shot spread value (`SprayAngle`) is definition-owned, and while it could be
scaled at point of use the same way the fire rate is, nothing asked for it and it was left rather
than added speculatively.

### Version semantics

Generator 9 adds the mandatory native river bridge: road planning reserves a real crossing, bridge planning must connect both banks, and a world without a usable crossing is rejected for bounded seed retry. The profile schema remains 10; no new JSON fields are needed.

Schema 10 adds `terrain.land_elevation_bias` and `fairness.min_base_to_base_travel_time_seconds`, both defaulting to zero in older profiles. Generator 8 identifies the changed native terrain shaping, shoreline, and transportation output for a given seed.

`CURRENT_SCHEMA_VERSION` and `CURRENT_GENERATOR_VERSION` moved to 2 with Phase 3, to 3 with
Phase 4, and to 4 with Phase 6, independently each time and for the same two reasons: the schema
gained fields, and the algorithm changed. (Phase 5's interior cliffs moved neither number — it
added no profile field, reusing `terrain.interior_cliff_density`, reserved since Phase 1.)
Schema 2 / generator 2: the same seed and the same schema-1 profile now produce a world with
cliffs in it where before it produced only an invisible boundary. Schema 3 / generator 3: a
world with `world.fit_to_level` false now stands on generated macro terrain instead of the flat
plane a heightfield-less world always was. Schema 4 / generator 4: that same world may now carve
rivers, streams, lakes and ponds into its own generated terrain where before it stood dry. A
`fit_to_level` world's own ground is unaffected by either the generator-3 or generator-4 change
— it never calls either generator; see `MacroTerrainGeneration.md`'s "Coexist, not replace"
section, which `Hydrology.md` follows for the same reason (hydrology reads the chunk registry
only macro terrain builds).

Schema 5 / generator 5 (Phase 12): a world with `features.tiberium_fields` true and a strategic
plan's own resource candidates may now carry real, harvestable fields where before a resource
candidate was only a scored point. Schema 6 / generator 6 (Phase 17): the Commander may now level
terrain within the `grading` limits, so two worlds from the same seed are no longer the same world
once a match has been played in them — which is exactly what a generator version is for. Phases
7-11 and Phase 16 moved neither number: each reused fields Phase 1 had already reserved, or (Phase
16) read everything it needed from the chunk registry and added no profile field at all. Schema 7
(Phase 21) moved only the schema number: `LogisticsGraphService` reads terrain/road/building state
generator 6 already produces and writes none of it back, so the same seed and the same
schema-1..6 profile still produce a byte-identical world — a schema field added with no generator
output change, the same independence this file has documented since schema 2. Schema 8 (Phase 22)
moved only the schema number for the same reason: `VehicleGameObj` reads its own already-generated
`DefenseObject` health at control time and writes no world state at all, so terrain from a given
seed is untouched by it.

The shipped profile is `data/procedural_world/default.json`, loaded by
`ProceduralWorldManager::DEFAULT_PROFILE_PATH`. It still declares `schema_version` 4 and carries
none of the schema-5 `resource` fields, the schema-6 `grading` block, the schema-7 `logistics`
block, or the schema-8 `vehicle_subsystems` block; all four take their documented defaults, which
is the whole point of versioning them separately, and is why an older shipped profile is not a bug
to fix here.

## Schema version 9 (D-001 priority 5)

Four additions, each defaulting to exactly what a schema-8 profile did. Generator stays 7: none of
them changes the terrain a seed produces. None is part of the profile hash, like the other
gameplay blocks (logistics, vehicle subsystems).

| Field | Default | Limits | Meaning |
| --- | --- | --- | --- |
| `base.gdi_initial_buildings`, `base.nod_initial_buildings` | empty | descriptor names | Section 16A.2: the starting base, in placement order; the first is the anchor on the selected site. Empty places nothing. |
| `base.spawn_candidate_count` | 12 | 0..64 | Section 16A.6's spawn pool size per faction. |
| `base.spawn_zone_radius` | 30 | >= 0 | Section 16A.5's fallback team spawn zone. |
| `logistics.enemy_threat_radius` | 0 | >= 0 | A known enemy this close to a route closes it to that team's convoys; 0 is off. |
| `vehicle_subsystems.sensor_damage_start_fraction` | 0.66 | >= 0 | Where an AI vehicle's sight range starts to shrink with health. |
| `vehicle_subsystems.min_sensor_factor` | 0.3 | 0..1 | Its floor at zero health. |
| `weather.dust_density` | 0 | >= 0 | Dust, scaled by dryness. |
| `weather.tiberium_density` | 0 | >= 0 | Tiberium haze near generated fields. |

The three shipped profiles move to schema 9 and name a full starting base for each faction
(Construction Yard, power plant, refinery, infantry and vehicle production, base defence). They
leave dust, haze and enemy obstruction off. A schema-1..8 profile still loads.

## `plan` (schema 11, D-003)

Optional. Absent or `enabled:false` skips Phase 2B and the pipeline behaves as it did under
schema 10. A `fit_to_level` world always skips it. Owner: `MapPlanGenerator` (`MapPlan.md`).

| Field | Type | Default | Meaning |
| --- | --- | --- | --- |
| `enabled` | bool | `false` | build the 2D map plan before terrain |
| `symmetry` | `"none"` / `"rotational"` / `"mirror_x"` | `"rotational"` | how the Nod half relates to the GDI half |
| `tier_count` | int >= 2 | `3` | integer height tiers |
| `tier_step` | float > 0 | `18.0` | world units of elevation per tier |
| `detail_weight` | 0..1 | `0.35` | share of the noise relief kept inside a tier; ramps get 0, base/field discs a quarter |
| `lane_width_cells` | int >= 1 | `1` | lane corridor half-width in plan cells (one cell = one chunk) |
| `base_radius_cells` | int >= 1 | `3` | flat base disc radius in plan cells |
| `flank_lanes` | 0..2 | `1` | flank lanes besides the centre lane, each through its own flank field |
| `water_fraction` | 0..1 | `0.08` | share of lowest-tier open cells planned as water |
| `boundary_margin_cells` | float >= 0 | `2.0` | cells kept between a site disc and the world edge |

The manifest canonical string carries every field as `pl.*`, so a plan change is a new world.
