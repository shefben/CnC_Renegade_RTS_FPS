# Procedural World Architecture

Section 33 required document (V2 Phase 30). This file maps the V2 runtime: who owns what, how a
procedural world attaches to a real level, and which authorities exist exactly once. The
per-feature detail lives in the phase docs linked from `docs/RequiredDocumentationIndex.md`.

## One world, laid over a real level

A procedural world never replaces a level. It is generated over one.

1. One of these calls `ProceduralWorldManager::Request_World`:
   - the main menu's **Procedural Skirmish** button (`dlgmainmenu.cpp`);
   - the campaign's `launch` verb (`PersistentCampaignService::Launch_Territory_Battle`).

   `world_tools seed_batch` generates directly, for tooling.
2. `Request_World` validates the profile *before* anything loads. A bad profile is refused while
   the player can still see why.
3. The level loads as normal: lighting, spawners, presets, and the physics scene itself.
4. `Generate_World` runs from `Post_Load_Level` against the loaded level.

A legacy authored map with no request never enters any of this: no request means no generation,
`WorldBoundsService` stays unconfigured (every query takes its unbounded answer), and no
generator state exists. Procedural mode is always an explicit choice, never a default.

## Owners (Section 30 step 3: exactly one each)

| Concern | Canonical owner | Layer |
| --- | --- | --- |
| World request/lifecycle, pipeline order, manifest | `ProceduralWorldManager` | Combat |
| Profile schema, load, validation | `ProceduralWorldProfile` / `ProceduralWorldProfileManager` | Combat |
| Seeds | `WorldSeedService` (named per-domain derivation) | Combat |
| Manifest, reproduction string, profile/content hashes | `WorldGenerationManifestService` | Combat |
| Bounds and zones | `WorldBoundsService` | wwphys |
| Terrain (heightfield, chunks, collision, holes, far layer) | `WorldTerrainSystem` | wwphys |
| Cliff faces (boundary ring and interior) | `EscarpmentSystem`, fed by `BoundaryCliffGenerator` and `InteriorCliffGenerator` | wwphys |
| Water | `WaterSystem`, fed by `HydrologyGenerator` | wwphys |
| Roads | `RoadSystem`, fed by `RoadGraphGenerator` | wwphys / Combat |
| Bridges | `BridgeSystem`, fed by `BridgePlanner` | wwphys / Combat |
| Tunnels | `TunnelSystem`, fed by `TunnelPlanner` | wwphys / Combat |
| Strategic candidates | `StrategicPlanner` (pure data) | Combat |
| Bases, Tiberium, settlements, decoration | `BaseSelectionPlanner`, `TiberiumFieldGenerator`, `SettlementGenerator`, `BiomeDecorationGenerator` (foliage through `FoliageSystem`) | Combat |
| Validation | `TransportationNetworkValidator`, `CombatSpaceValidator` | Combat |
| Navigation truth | `PathfindClass` sectors built by `NavMeshGenerator`; per-locomotion reachability by `WorldTraversabilityGraph` | Combat |
| Runtime terrain mutation | `WorldOperationJournal` (the only path that edits generated ground after generation) | Combat |
| Snapshots, checksums, late join, repair | `WorldSnapshotService`, `WorldSyncService` | Combat |
| Buildings, power | `BuildingDescriptorManager`/`BuildingGameObj`, `TeamPowerGridService` (V1, unchanged) | Combat |
| Commander actions | The V1 human Commander API. The AI Commander (`CommanderAIService`) calls the same API and has no second action path. | Combat |
| Campaign | `PersistentCampaignService` (territory/faction state above one match) | Combat |
| Tools | `ProceduralWorldToolsService` (report, preview, seed batch) | Combat |

Two navigation structures exist on purpose, and they answer different questions.

- `PathfindClass`'s sector graph is what the engine's AI paths over.
- `WorldTraversabilityGraph` is a per-locomotion-class reachability and chokepoint analysis.

Neither is a copy of the other: the sector graph is built from the chunk registry, and the
traversability graph is rebuilt on demand from live terrain, bridges and tunnels.

The pattern that repeats everywhere a generator touches a shared collection is **owned ids**:

- `RoadGraphGenerator`, `BridgePlanner`, `InteriorCliffGenerator` and `HydrologyGenerator` each
  record the ids they added to a shared system.
- Their `Clear` removes only those, so a hand-authored road, bridge or cliff is never touched.

`TunnelPlanner` owns `TunnelSystem`'s whole collection, because nothing else adds tunnels. It
records exactly the ground cells and cliff cuts it opened.

## Authority

The server generates, and every client generates the same world from the same
`(seed, profile, generator version, content manifest)`. No generated geometry travels over the
wire.

- `WorldSyncService`'s handshake refuses a client whose generator version, profile hash or
  content hash differs.
- The chunk checksums and snapshots of Phase 19 catch drift and repair a bad region.
- Runtime changes to the ground go only through `WorldOperationJournal`, which the server orders
  and replicates.

See `NetworkingAndPersistence.md`.

The dedicated server runs the same generation code. It never builds render-only state: geometry
builders decline without a scene, and client-only UI and rendering stay behind the existing
`FREEDEDICATEDSERVER` guard. See `docs/zerohour/DedicatedServerRules.md`, and
`docs/integration/Phase28IntegrationGate.md` for the dependency confirmation.

## Determinism

`GeneratorDeterminism.md` holds the full contract. In short:

- named, derived seeds per domain;
- no wall-clock or address-derived input;
- stable ordering;
- quantized hashing;
- bounded retries with a deterministic fallback;
- a failed generation logs its reproduction string.
