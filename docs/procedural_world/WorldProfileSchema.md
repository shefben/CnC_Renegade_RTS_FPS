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
| `terrain.hill_density` | normalized weight, 0..1 | see terrain settings note below | 0.45 | >= 0, finite | yes |
| `terrain.mountain_density` | normalized weight, 0..1 | see note below | 0.18 | >= 0, finite | yes |
| `terrain.interior_cliff_density` | normalized weight, 0..1 | see note below | 0.16 | >= 0, finite | yes |
| `terrain.valley_density` | normalized weight, 0..1 | see note below | 0.30 | >= 0, finite | yes |
| `water.rivers`/`lakes`/`ponds`/`coastline` | bool | feature enabled | all `true` | -- | yes |
| `world_features.forests`/`settlements`/`tunnels`/`highways`/`tiberium_fields` | bool | feature enabled | all `true` | -- | yes |
| `retry.max_local_retries_per_stage` | count | Section 3C.1 step-1 ceiling every generator stage's local-adjustment loop is bounded by | 8 | > 0 | no (soft/behavioral, does not change chosen output given the same seed unless a stage actually exhausts it) |
| `retry.max_alternate_candidates_per_feature` | count | Section 3C.1 step-2 ceiling | 6 | > 0 | no, same reasoning |
| `fairness.*` | see Section 17A.1 | quantitative fairness thresholds, consumed starting Phase 15/17A | see example above | each field's own sign (nonnegative unless noted) | no (soft preference thresholds a repair pass may fail against, not generation inputs themselves) |

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

### Version semantics

`CURRENT_SCHEMA_VERSION` and `CURRENT_GENERATOR_VERSION` both moved to 2, independently and for
different reasons: the schema gained fields, and the algorithm changed — the same seed and the
same schema-1 profile now produce a world with cliffs in it where before it produced only an
invisible boundary.

The shipped profile is `data/procedural_world/default.json`, loaded by
`ProceduralWorldManager::DEFAULT_PROFILE_PATH`.
