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
