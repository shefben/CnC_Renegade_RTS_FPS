# Hybrid rewrite I2: shared prerequisites

I2's gate is closed. This document records where each Section 0.3 / 0.5 / 0.6 / 0.8 /
2.3 / 7.1 / 16 requirement lives and what proves it, so I3 can start from evidence
rather than from a re-reading of the plan.

## Where the work lives

`Code/wwphys/hybridplanning.{h,cpp}` owns the content-independent half: the keyed
deterministic helpers (`Hash01`, `PickIndex`, `PickSign`, `Range`, `RoundHalfUp`) over
`WorldSeedService::Hash01`, the shared quantizers and the quantized hash stream, the
Section 0.5 constants table, the Section 0.6 size limits and size class, the Section 7.1
road cross-section and corridor/grade helpers, `ProceduralPlanningSpatialHash`, the
Section 16.1 work counters and budgets, and the Section 16.3 scratch arena.

`Code/Combat/hybridpreflight.{h,cpp}` owns the content half: `HybridPreflightInputs`
gathers the runtime registries, `Hybrid_Preflight_Request_Check` refuses a request at the
menu on Section 5 features and Section 0.6 size, `Hybrid_Run_Preflight` resolves the
Section 0.8 metric cache in the plan's order, `Hybrid_Structural_Content_Revision`
digests the collision-relevant content, and `Hybrid_Planning_Input_Hash` is the Section
0.3 `HybridPlanningInputHash`.

## Runtime metric snapshot

Collision extents are full WORLDBOX extents in vehicle-root axes, widths **and** heights,
taken through `WorldMetricsClass::Get_Collision_Box` / `Get_Class_Collision_Box`, which
answer only when `HasExtents` is true. A render bounding box or an engine default is never
accepted: an unresolved metric leaves the input unmeasured and preflight rejects the
request by name. Provenance -- the definition id each measurement came from -- is kept in
the cache and hashed.

- Humvee: the lowest-id physics definition whose model is `V_GDI_HUMVEE`.
- Heavy ground class: `WorldMetricsClass`'s per-axis largest measured `LOCOMOTION_TRACKED`
  member; definitions with no WORLDBOX are counted and left out rather than guessed.
- Harvesters: each team's starting refinery's own Harvester definition, resolved through
  the starting-building descriptor's game-object preset. Both are required; the cache
  keeps the wider/taller of the two and both definition ids.

The offline measurements in [VehicleCollisionMetrics.md](VehicleCollisionMetrics.md) are
the expected answers for shipped content, and the self-checks drive preflight with them:
`H = 2.555953` produces the Section 0.5 `W = 3.322739`, which the plan rounds to `2.556`
and `3.3228`. Nothing in the engine hard-codes a dimension.

Bridges are frozen per registered definition (clear width, deck thickness, required
clearance, support spacing and clearance, and the '|'-joined list of every colliding or
traversable model), together with `BridgePlannerParams`'s span and water-clearance limits.
`BridgePlannerParams` states no approach length, so the cache falls back to `8W` and says
so. Tunnel modules are frozen per module with their full geometry (model, length, clear
bore, wall thickness, turn and grade limits, portal compatibility); a zero clear bore is
the procedural tube, which is sized per tunnel. Starting-building footprints come from the
Section 39 descriptors; the descriptor owns the footprint, and the render-derived height is
deliberately excluded from both digests. Terrain layers are frozen by name, kind and
surface type, and Section 4.2's material slots resolve to the lowest layer index on a tie.

## Identity

`Hybrid_Structural_Content_Revision` digests exactly the collision-relevant records above.
Cosmetic facts -- decoration and lamp availability, textures, bridge dressing -- are left
out, so a cosmetic content change does not invalidate a plan.

`Hybrid_Planning_Input_Hash` covers the Section 0.3 field list: schema and generator
version, the structural content revision, world dimensions/chunk size/sea level/centre,
boundary settings, the terrain and water settings this rewrite consumes, every active
`plan` field, resource/Tiberium settings, the active world-feature toggles, road grading
limits, every fairness field, both retry limits, the starting-building footprints, and the
resolved structural asset identities (vehicle metrics and provenance, fairness speeds,
tunnel portal and bore, ford availability, every registered bridge plus the per-class
choice, and each biome's resolved material slots). Wall clock, watchdog, worker count, the
inert legacy settlement/lane fields and render-only availability are excluded.

Both values are carried by the canonical owners without touching authored worlds:
`WorldGenerationManifest` stores them next to the legacy profile hash and round-trips them
through its JSON and its reproduction string, `ProceduralWorldTools` reports them, the
attempt reproduction string names them, and `WorldSyncHandshakeClass` (handshake version 2)
carries the planning hash and refuses a join with `WORLD_SYNC_PLANNING_MISMATCH`. A legacy
or authored world leaves both at zero and behaves exactly as before.

## Request integration

`ProceduralWorldManager::Request_World` runs `Hybrid_Preflight_Request_Check` while the
player is still at the menu, so a profile with a required feature disabled or an
out-of-envelope size is refused before a level load. `Generate_World` runs the full
preflight before any seeded stage. The cache is built by the first attempt that needs it
and reused by the bounded retries -- Section 17's teardown preserves immutable content
metrics, and nothing preflight reads changes with the effective seed -- and it is
invalidated at the start of each request.

## Validation

Self-checks `terrain_hybridplanning` / `fds_terrain_hybridplanning` cover the helpers,
constants, size classes, road cross-sections, spatial hash, work counters and scratch
arena. `terrain_hybridpreflight` / `fds_terrain_hybridpreflight` cover the preflight: every
named failure, the optional missing assets that only omit their own type, size-class
derivation and the developer oversize path, biome slot resolution and mixed-pair legality,
hash sensitivity and exclusions, the manifest carry, and one pass over the live registries
that proves `Gather_From_Runtime` is repeatable, reads every registered bridge, tunnel
module and terrain layer, and -- with no content mounted -- leaves every vehicle metric
unmeasured so preflight names the missing metric instead of inventing one.
