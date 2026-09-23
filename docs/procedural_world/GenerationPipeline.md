# Generation Pipeline

Section 33 required document (V2 Phase 30). The exact order in which
`ProceduralWorldManager::Generate_World` builds a world, what gates each stage, and what happens
when a stage fails. Each stage's own detail is in the doc named beside it.

## Entry

```text
Request_World(seed, profile_path)          validate profile now; refuse before any level loads
  -> level loads normally
  -> CombatManager::Post_Load_Level
  -> Generate_World
```

## Outer loop: bounded whole-world retry (Section 3C)

`Generate_World` runs `Generate_World_Attempt` up to `MAX_WORLD_GENERATION_ATTEMPTS` (3) times.

- **Seed per attempt.** Attempt *n* uses `Normalize_Seed(base_seed + n)`, a deterministic bump,
  never a random reseed.
- **Validation.** Each attempt re-validates its profile.
- **Retry triggers.** An attempt that fails outright is retried. So is one whose required base
  routes came back disconnected (`TransportationNetworkValidator`) or whose combat space failed
  hard (`CombatSpaceValidator`).
- **Reproduction.** Every failure is logged with its reproduction string
  (`generator_version=<n> seed=<n> profile=<hash> content_manifest=<hash>`), so the exact
  failing attempt can be regenerated.
- **Exhaustion.** After the last attempt, the last completed world stands and the log says
  plainly that it is not fully connected. Nothing is ever presented as connected when it is not.

## One attempt, in order

| # | Stage | Owner / doc | Runs when | On failure |
| --- | --- | --- | --- | --- |
| 1 | World bounds | `ProceduralBoundsBuilder` → `WorldBoundsService` (`WorldBoundsAndBoundaryCliffs.md`) | always | attempt fails |
| 2 | Boundary cliff ring | `BoundaryCliffGenerator` (same doc) | bounds built | logged; interior still generated |
| 3 | Macro terrain, or fitted terrain sampled from the host level | `MacroTerrainGenerator` / `Build_Fitted_Terrain` (`MacroTerrainGeneration.md`) | always | attempt fails |
| 4 | Interior cliffs | `InteriorCliffGenerator` (`InteriorCliffs.md`) | chunk registry | logged; later stages skip what they need from it |
| 5 | Hydrology | `HydrologyGenerator` (`Hydrology.md`) | chunk registry | logged |
| 6 | Strategic candidates | `StrategicPlanner` (`StrategicPlanning.md`) | chunk registry | logged; roads onward skipped |
| 7 | Road graph | `RoadGraphGenerator` (`RoadGraph.md`) | strategic plan | logged; bridges onward skipped |
| 8 | Bridges | `BridgePlanner` (`BridgePlanning.md`) | road graph | logged |
| 9 | **Tunnels** | `TunnelPlanner` → `TunnelSystem` (`RoadsBridgesTunnels.md`) | chunk registry and `world_features.tunnels` | logged |
| 10 | Base selection and fairness repair | `BaseSelectionPlanner` (`BaseSelection.md`) | bridge plan | logged |
| 11 | Tiberium fields | `TiberiumFieldGenerator` (`TiberiumFields.md`) | base selection | logged |
| 12 | Settlements and POIs | `SettlementGenerator` (`Settlements.md`) | base selection or Tiberium | logged |
| 13 | Biome decoration | `BiomeDecorationGenerator` (`BiomeDecoration.md`) | chunk registry | logged |
| 14 | Transportation validation | `TransportationNetworkValidator` | always | feeds the retry trigger |
| 15 | Combat-space validation | `CombatSpaceValidator` (`CombatSpaceValidation.md`) | always | feeds the retry trigger |
| 16 | World integration (collision, structures) | `GeneratedWorldIntegrator` (`FinalWorldIntegration.md`) | settlements | logged, never a retry |
| 17 | Navigation | `NavMeshGenerator` (same doc; tunnel links in `RoadsBridgesTunnels.md`) | chunk registry | logged, never a retry |
| 18 | Minimap base layer | `MinimapBaseLayer` | chunk registry | logged |
| 19 | Shroud coverage | `WorldShroudManager` partition | chunk registry | logged |
| 20 | Commander expansion rules | `GeneratedWorldPlacementRules`, `FoundationGrading`, `ExpansionRegionRegistry` (`CommanderExpansion.md`) | always | logged |
| 21 | Logistics graph | `LogisticsGraphService` (`LogisticsLayer.md`) | always | logged |
| 22 | Manifest commit | `WorldGenerationManifestService` | always | — |

Tunnels sit between bridges and bases, as the roadmap's own phase order has them. A road crossing
no bridge could resolve becomes a tunnel candidate. Base selection then chooses among candidates
that tunnels have already respected, using the same protected radius.

## Why these gates

- A stage runs only when its real inputs exist, and it says so in the log when it is skipped.
  That is the same shape for every stage. For example, the tunnel stage needs only the chunk
  registry, because its bridge-diagnostic source is simply empty when no bridge plan ran.
- A validator failure is a reason to retry with the next seed. It can be a layout problem, which
  a different seed can fix. An integration or navigation failure cannot be fixed by another seed,
  so those are never retried.
- **Explicit opt-outs.** `world_features.tunnels = false`, `world_features.settlements = false`
  and similar flags succeed with nothing built. They are not failures.

## Teardown

`Destroy_World` runs in reverse build order. Each generator removes only what it added (the
owned-id idiom; see `Architecture.md`). `TunnelPlanner::Clear` also closes exactly the ground
cells and cliff cuts its tunnels opened.

## Where the output goes

- **Manifest:** generator version, seed, profile hash, content-manifest hash, the per-stage
  checksums, and the "selected" id lists. Bases, Tiberium, settlements, bridges and tunnels are
  listed only when a real object was built.
- **Tools:** `world_tools repro`, `validate`, `preview` and `seed_batch` report and reproduce any
  of it (`docs/tools/ProceduralWorldTools.md`). The summary log lines at the end of
  `Generate_World_Attempt` name every stage's count and checksum.
