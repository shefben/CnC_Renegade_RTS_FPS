# Generator Determinism (V2 Phase 1 initial content)

Roadmap Section 3B's determinism contract, made concrete for this codebase. Required output of
Section 34 Phase 1. Extended by later phases as new authoritative numeric/ordering rules are
needed (Section 3B.6); this document is not re-created, only appended to.

## RNG contract (Section 3B.1)

| Item | Value |
| --- | --- |
| Algorithm | `Random4Class` (`Code/wwlib/random.h/.cpp`) -- a from-scratch Mersenne Twister (MT19937) implementation, not `std::mt19937`/`rand()`/any compiler- or platform-supplied RNG. Fully specified by this tree's own source, so client/server/platform parity does not depend on a library's implementation-defined behavior (Section 3B.1's own requirement). |
| Seed width | 32 bits, `unsigned int`, used unchanged end to end: `ProceduralWorldProfile::Seed` -> `WorldSeedService::Normalize_Seed`/`Derive_Subsystem_Seed` -> `Random4Class`'s constructor parameter. |
| Seed hashing algorithm | `Derive_Subsystem_Seed(world_seed, domain) = CRC::String(domain, Normalize_Seed(world_seed))` (`Code/wwlib/crc.h`'s table-driven CRC, already linked and already used elsewhere in this tree -- `cnetwork.cpp`, `ini.cpp`, `modpackage.cpp`). No floating point, no implementation-defined behavior, table-driven and identical on every platform. |
| Seed-domain strings | Exactly the twelve `WorldSeedService::SEED_DOMAIN_*` constants: `boundary`, `terrain`, `hydrology`, `roads`, `bridges`, `tunnels`, `settlements`, `tiberium`, `bases`, `biomes`, `decoration`, `combat_space_repair`. Section 4 names the first eleven; Phase 1 step 6 adds the twelfth. A later phase introducing a new seed-consuming subsystem adds a new named constant to `worldseedservice.h` rather than passing an ad hoc string. |
| Integer-to-float conversion | `Random4Class::Get_Float()` (`draw * 2.3283064370807973754314699618685e-10f`, i.e. `draw / 2^32`, already implemented) is the one canonical unsigned-32-bit-draw-to-`[0,1)` conversion. A generator needing a float in `[0,1)` calls this, never a hand-rolled division. |

**Zero-seed normalization.** A raw world seed of exactly `0` is remapped to `1` by
`Normalize_Seed` before anything derives from it, because this codebase already treats a bare
`0` as "unset" in many other places (network/object ids, `RuntimeDoorService::BuildingId`, and
so on) and a procedural world identified by seed 0 should not be ambiguous with "no seed was
ever set." Every other seed value passes through unchanged, and normalizing an already-normalized
seed is a no-op (both proven by the `worldprofile` self check).

**Subsystem seed independence** (Section 3B.3). `Derive_Subsystem_Seed` is a pure function of
`(world_seed, domain)` with no shared mutable state -- deriving the `roads` domain's seed and
then drawing any number of values from a `Random4Class` seeded with it has zero effect on what
`Derive_Subsystem_Seed(world_seed, "terrain")` returns, because nothing about that call touches
global state. Worker/thread scheduling order therefore cannot change any subsystem's own seed;
whatever order later phases choose to run generator stages in, each stage's derived seed is the
same regardless of what ran before it. Proven directly by the `worldprofile` self check ("changing
RoadSeed consumer does not change TerrainSeed").

## Stable ordering and tie-breaking (Section 3B.2)

No candidate-scoring/selection code exists yet in this pass (Phase 1 has no candidates to
select among). Documented here as a standing requirement for Phase 3 onward: every authoritative
candidate collection must be sorted by a stable generated ID before scoring, tie-break keys must
be documented per scorer, and no subsystem may rely on unordered-container iteration order for
an authoritative result. `WorldGenerationManifestService::Compute_Content_Manifest_Hash` already
follows this rule for its own input (an insertion-sorted copy of the caller's identifier list,
proven order-independent by the `worldprofile` self check) as the first concrete instance of it
in this codebase.

## Stable generated object identities (Section 3B.5)

No generator produces an authoritative object yet. `WorldGenerationManifest`'s own
consequential-decision lists (`SelectedBaseCandidateIds`, `SelectedResourceFieldIds`, ...) are
typed as string-ID lists, not indices or pointers, specifically so that whichever later phase
populates them can adopt Section 3B.5's hierarchical ID convention (`boundary-cliff:segment:87`,
`road:17`, `base:gdi`, ...) directly, with no manifest-shape change required.

## Content-manifest inputs (Section 29A.1)

`WorldGenerationManifestService::Compute_Content_Manifest_Hash` takes a caller-supplied list of
stable content identifiers and hashes them (sorted, so caller order never matters). Today that
list is empty in every real caller (`Create_Empty_World_Manifest`'s own default use), because
Phase 0's own baseline search confirmed no generator/asset registry exists anywhere in this tree
yet -- an empty content manifest is the honest current state, not a placeholder. As later phases
introduce real content sources (W3D/collision content, procedural asset descriptors, bridge/
tunnel module definitions, road cross-section definitions, traversal profiles, terrain/generator
tables, Tiberium/economy tables), each one is responsible for contributing its own identifiers
to that list; the hashing function itself never needs to change.

## Old-generator-version compatibility (Section 3B.6)

`ProceduralWorldProfile::SchemaVersion` and `::GeneratorVersion` are independent by design (see
`WorldProfileSchema.md`). Today both are `1` (`CURRENT_SCHEMA_VERSION`/`CURRENT_GENERATOR_VERSION`),
and no generator exists yet to produce version-2 output, so there is nothing to migrate. The
standing rule this section records for when that changes: a world created by generator version
`N` must remain loadable and reproducible as version `N` -- a schema/generator version bump must
never silently reinterpret an old seed under new logic. `WorldGenerationManifest::GeneratorVersion`
is carried on every persisted manifest specifically so a later loader can tell which algorithm
produced it before attempting to reconstruct anything.

## What is proven now vs. deferred

Proven by the `worldprofile` self check (`Code/Commando/shaderselfcheck.cpp`,
`Run_World_Seed_And_Profile`), all pure arithmetic, no device/scene/clock:

- a default-constructed profile validates;
- each of Phase 1 step 3's named invalid cases is rejected with a specific reason;
- a profile round-trips through JSON text exactly, including a field the JSON left unset
  keeping the struct's own documented default;
- malformed JSON is rejected with a reason, never silently defaulted;
- the same `(seed, domain)` always derives the same subsystem seed;
- deriving one domain's seed never changes another domain's derived seed;
- different domains from the same world seed never collide with each other;
- a different world seed changes every derived subsystem seed;
- `Normalize_Seed` maps `0` to a fixed nonzero value, passes every other value through
  unchanged, and is idempotent;
- the profile hash is deterministic (two identical profiles hash equal) and field-sensitive
  (changing one field changes the hash);
- the content-manifest hash is order-independent and sensitive to real content;
- a manifest survives a real `Serialize`/`Deserialize` JSON round trip with every field intact,
  including its string-ID lists;
- `Format_Reproduction_String` matches Phase 1 step 12's exact required format.

Not yet provable by anything in this codebase, same reason every other Commander/RTS self check
carries this caveat (`awaiting_answers.md` Q-012): the self check itself has not been executed
in this environment this session (standing rule: no `ctest -R`, no launching `renegade`/
`renegadeserver`). It is registered as `shader_worldprofile`/`fds_shader_worldprofile` in
`Code/Commando/CMakeLists.txt` and covered by the same `ctest -C Debug -R
"shader_|fds_shader_|terrain_shroud" --output-on-failure` sweep Q-012 already names for every
other family.


## Phase 3 additions

### A new seed domain

`boundary_cliffs` joins the twelve domains above. It is deliberately **not** shared with
`boundary`: the cliff rim noise would otherwise be drawn from the stream that already shaped the
boundary polygon, correlating the two so that a bump in the outline always arrived with a
matching bump in the cliff height. Keeping generators from interfering with each other is the
entire point of per-subsystem derivation, and two features of the same subsystem are no
exception.

Nothing about the derivation itself changed: `Derive_Subsystem_Seed(seed, "boundary_cliffs")` is
the same `CRC::String(domain, normalized_seed)` every other domain uses.

### Ordering and smoothing

Cliff rim noise is drawn from `Random4Class` in **station order** — a fixed traversal of an array,
never a hash-table walk — and then smoothed three times circularly. The smoothing is part of the
contract, not a filter applied afterwards: it is applied identically on every run, and its
circular wrap is what lets the ring close without a seam in the noise.

### The cliff ring checksum

`WorldGenerationManifest::CliffRingChecksum` (Phase 3 step 17) is a CRC over the canonical station
array — each station's position and both bench heights, quantized to a thousandth of a world unit
per Section 3B.4's never-hash-a-raw-float rule — taken in station order.

It is what turns "the same seed produced the same ring" from an assumption into something two
processes can compare. `Plan_Ring` computes it, so it exists in a device-less process with no
physics scene.

### The profile hash covers the fitted profile

A world built with `fit_to_level` hashes the profile **as used**, including the width, height,
centre and sea level taken from the host level. Hashing the profile as authored would produce a
manifest that describes a different world than the one it came from.

### One correction to Phase 1

`Format_Canonical_Profile_String` fed `MinBaseExpansionArea` to the `fa.resdiff` slot instead of
`MaxInitialResourceTravelTimeDifferenceSeconds`, so that threshold never affected the profile
hash and `MinBaseExpansionArea` was hashed twice. Fixed in Phase 3. Any profile hash recorded
before this point is not comparable with one recorded after; the generator version bump to 2
covers that.

## Phase 4 additions

### No new seed domain

Macro terrain uses the existing `terrain` domain (`SEED_DOMAIN_TERRAIN`), reserved by Phase 1
for exactly this purpose and unused until now. No new domain was needed.

### Noise channel offsets are not separate streams

`MacroTerrainGenerator`'s four noise channels (macro, ridge, hill, valley) do not each derive a
separate subsystem seed. `Derive_Subsystem_Seed` is called once, for the whole terrain domain;
each channel then offsets that one integer seed by a small, fixed, documented constant
(`seed + 1013`, `+ 2027`, `+ 3041`, ...) before sampling `TerrainMaskClass::Hash_Value`. This is
deliberately narrower than Section 3B.1's per-subsystem derivation contract, which governs
separate *subsystems* (terrain, hydrology, roads, ...) rather than internal channels of one
generator's own noise. Two subsystems drawing correlated noise is the fault that contract
prevents (see `SEED_DOMAIN_BOUNDARY_CLIFFS`'s own reasoning); four channels of the same
subsystem sampling the same deterministic hash at different fixed offsets is not a new source
of correlation risk, since all four are already coupled by construction (they describe one
landform).

### The chunk registry checksum

`WorldTerrainSystem::Get_World_Chunk_Checksum` (Phase 4 step 17) is a CRC over every chunk's
own checksum, in row-major order; each chunk's checksum is itself a CRC over its height/slope
extent and its `Buildable`/`CliffBand` flags, quantized to a thousandth of a world unit or
radian per Section 3B.4. It is computed by `Build_Chunk_Registry`, which needs no physics
scene, so — like `BoundaryCliffGenerator`'s ring checksum — it exists in a device-less process
and is what `terrain_macro`'s determinism check actually compares.

### Bounded, not proven, slope relaxation

The generation-time slope cap (`terrain.max_buildable_slope_degrees`) is enforced by a fixed
number of relaxation passes (`SLOPE_RELAXATION_PASSES = 4`), not iterated until the limit is
satisfied everywhere. This is consistent with Section 3C.1's bounded-retry philosophy, but it
means the enforcement is *reduces toward the limit*, not *guarantees the limit*, for a profile
aggressive enough to defeat four passes. `MacroTerrainGenerator::Check_Ordinary_Slope_Limit`
reports the worst violation it finds rather than asserting there are none, which keeps the
check honest about what a bounded pass count actually proves.

### `fit_to_level` skips the generator entirely

Unlike the cliff ring, which still builds around a fitted world, macro terrain generation does
not run at all when `world.fit_to_level` is true — `ProceduralWorldManager::Build_Macro_Terrain`
returns success having done nothing. `WorldGenerationManifest::TerrainChecksum` is `0` for
every world produced by the one caller that exists today, which is the correct, non-degenerate
value for "the generator did not run", not a sign of a broken checksum. See
`docs/procedural_world/MacroTerrainGeneration.md`.

## Phase 5 additions

### A new seed domain, deliberately

`InteriorCliffGenerator` uses its own domain, `SEED_DOMAIN_INTERIOR_CLIFFS`, rather than
reusing `terrain`. Interior cliff placement reads the terrain's own chunk registry as input, so
drawing its candidate-density gate, its type pick and its traversability pick from the same
stream that shaped the terrain in the first place would correlate the two — the exact fault
`SEED_DOMAIN_BOUNDARY_CLIFFS`'s own reasoning already names for the boundary ring versus the
boundary polygon.

### Two hash draws per feature, on their own offsets

`InteriorCliffType` and `InteriorCliffTraversabilityType` are each one `TerrainMaskClass::
Hash_Value` draw off the feature's own starting chunk coordinates, offset by `+5001` and
`+6007` respectively — far enough from `MacroTerrainGenerator`'s own small per-octave offsets
(`+1013`, `+2027`, ...) and from each other that neither draw, nor the candidate-density gate
above, correlates with either of the others.

### The interior cliff checksum

`InteriorCliffGenerator::Get_Checksum` is a CRC over every built feature's own type,
traversability, length, tallest station and bypass point, quantized per Section 3B.4, computed
with no physics scene the same way the ring and chunk-registry checksums are. `WorldGeneration
Manifest::InteriorCliffChecksum` carries it into the manifest, `0` under the same three
conditions `TerrainChecksum` can be `0` for: no chunk registry to read (`fit_to_level`), macro
terrain itself failed, or interior cliff generation itself failed.

### The walk is deterministic; the noise realism claim is not

Given the same candidate set, the greedy chunk-to-chunk walk `InteriorCliffGenerator` uses has
no random input of its own — same candidates in, same walk out, on any platform. What is *not*
claimed is that the walk finds the best or most natural-looking wall through a given terrain;
see `docs/procedural_world/InteriorCliffs.md` for why ridge-line extraction was deliberately not
attempted. The self check proves determinism and structural correctness (no cliff-band overlap,
no single-chunk spikes, no inverted faces) against a synthetic uniform-slope terrain built
specifically to remove all doubt about whether a candidate exists at all — it does not, and
cannot without a display, prove that a real generated seed's interior cliffs look right.

### `fit_to_level`, and terrain failure, both skip this generator too

`ProceduralWorldManager::Build_Interior_Cliffs` only runs when `Build_Macro_Terrain` itself
reported `terrain_built`, which already folds in both the `fit_to_level` skip and any macro
terrain failure — there is exactly one place that decides whether a chunk registry exists this
run, and interior cliffs reads that decision rather than re-deriving it. See
`docs/procedural_world/InteriorCliffs.md`.

## Phase 6 additions

### No new seed domain

`HydrologyGenerator` uses `SEED_DOMAIN_HYDROLOGY`, reserved by Phase 1 for exactly this purpose
and unused until now — the same situation Phase 4 found `SEED_DOMAIN_TERRAIN` in.

### The drainage walk has no random input of its own

Flow direction and accumulation are pure functions of the chunk registry's own height data —
no hash draw anywhere in that computation. Given the same terrain, the same candidate/headwater
set and the same chunk-to-chunk walk happen on any platform. What determinism actually has to
answer for is narrower than in the other two generators: there is no per-feature hashed
type/traversability pick here at all (unlike `InteriorCliffGenerator`'s two), because shore
classification and crossing kind are both computed from real, measured slope and width rather
than drawn from the seed. The seed's only role in this generator's own output is indirect, via
whichever seed shaped the terrain being drained — `HydrologyGenerator` reads that terrain, it
does not draw its own noise from `params.Seed`.

### The hydrology checksum

`HydrologyGenerator::Get_Checksum` is a CRC over every built feature's own type, shore
classification, enclosed flag, length and crossing candidates, quantized per Section 3B.4,
computed with no physics scene. `WorldGenerationManifest::HydrologyChecksum` carries it into the
manifest, `0` under the same three conditions `InteriorCliffChecksum` can be `0` for: no chunk
registry to read (`fit_to_level`), macro terrain itself failed, or hydrology generation itself
failed.

### `fit_to_level`, and terrain failure, both skip this generator too

`ProceduralWorldManager::Build_Hydrology` only runs when `Build_Macro_Terrain` reported
`terrain_built`, the same single gate `Build_Interior_Cliffs` reads. It runs whether or not
interior cliffs themselves succeeded — an interior cliff feature failing to build says nothing
about whether the chunk registry's own height data (all hydrology actually reads) is wrong. See
`docs/procedural_world/Hydrology.md`.

## Phase 7 additions

### No new seed domain, narrowly consumed

`StrategicPlanner` uses `SEED_DOMAIN_BASES`, reserved since Phase 1. Unlike every other
generator so far, almost nothing here is drawn from the seed at all: every ranking is a pure
function of the terrain and of what earlier phases already placed. The seed's one job is a
small per-chunk score jitter (`TERRAIN_SCORE_JITTER_WEIGHT = 0.05`) that resolves ties between
otherwise-equal candidates differently per seed, so a large regular field does not rank
identically regardless of which world it is. See `docs/procedural_world/StrategicPlanning.md`.

### The strategic plan checksum

`StrategicPlanner::Get_Checksum` is a CRC over every node's own id, type, faction, position and
suitability, quantized per Section 3B.4, computed with no physics scene. `WorldGeneration
Manifest::StrategicPlanChecksum` carries it into the manifest, `0` under the same conditions
`HydrologyChecksum` can be 0 for. This is deliberately not what `SelectedBaseCandidateIds` and
its four siblings mean — those stay empty; a checksum over the full candidate pool is not a
claim that anything has been selected from it.

### `fit_to_level`, and terrain failure, both skip this generator too

`ProceduralWorldManager::Build_Strategic_Plan` only runs when `Build_Macro_Terrain` reported
`terrain_built`, the same gate every generator downstream of macro terrain shares. It runs
whether or not interior cliffs or hydrology themselves succeeded, reading whatever candidates
either left behind (zero, if a stage did not build) rather than failing over a sibling's own
failure.

## Phase 8 additions

### `SEED_DOMAIN_ROADS`, narrowly consumed, same as Phase 7

The routing cost field and the minimum spanning tree are both pure functions of the terrain and
the strategic plan; the seed's one job is `RoadGraphGenerator`'s own small per-edge cost jitter
during Dijkstra, resolving ties between otherwise-equal paths differently per seed.

### The road graph checksum, and what "selected" means here

`RoadGraphGenerator::Get_Checksum` is a CRC over every built edge's own class, endpoints, length
and unresolved-crossing count, computed with no physics scene (`Build_Geometry` is never
called). `WorldGenerationManifest::RoadGraphChecksum` — reserved since Phase 1, `0` until this
phase — now carries it. Unlike Phase 7's candidate ids, `SelectedRoadGraphEdgeIds` genuinely
means "selected" from this phase on: a road graph edge is a road `RoadSystem::Add_Road` actually
built, not a proposal, so populating that list here is correct where it would have been
dishonest for Phase 7's own candidate pool.

### Gated behind the strategic plan, not merely the terrain

`ProceduralWorldManager::Build_Road_Graph` runs only when `Build_Strategic_Plan` itself
succeeded — a stricter gate than `terrain_built` alone, because a road graph connects strategic
nodes and there is nothing to connect without them.
