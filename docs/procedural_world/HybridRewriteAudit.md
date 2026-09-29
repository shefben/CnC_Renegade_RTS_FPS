# Hybrid Generator Rewrite — Phase 0 Repository Audit (I0)

Runtime Phase 0 of `Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md` (§8 P0). Audit only: no generator behavior changed. Baseline taken on branch `feat/procedural-texture-set`, HEAD `c03a19ea` plus the uncommitted working tree.

## Baseline

- Build: `build_tmp.bat` (Release, `build/win32`, ninja) — up to date, exit 0.
- Procedural self-checks: `ctest -C Release -R "terrain_" -j 6` — **92/92 passed** (14.5 s). 207 ctest tests in total; procedural ones are `terrain_*` and `fds_terrain_*`.

## Canonical owners (§1)

| Owner | Location |
| --- | --- |
| `WorldTerrainSystem` | `wwphys/worldterrainsystem.h` |
| `WaterSystem` | `wwphys/watersystem.h` |
| `RoadSystem` | `wwphys/roadsystem.h` |
| `BridgeSystem` (definition registry `Define_Bridge` / `Peek_Definition` / `Find_Definition`, `Layout_Bridge`) | `wwphys/bridgesystem.h:206` |
| `TunnelSystem` | `wwphys/tunnelsystem.h:205` |
| `EscarpmentSystem` | `wwphys/escarpment.h` |
| `FoliageSystem` | `wwphys/foliagesystem.h` |
| `PathfindClass` (nav) | `wwphys/Pathfind.h` |
| Minimap | `MinimapBaseLayer::Build` via `ProceduralWorldManager::Build_Minimap_Layer` (`Combat/proceduralworldmanager.cpp:848`) |
| Shroud | `GeneratedWorldIntegrator::Partition_Shroud_Over_World` via `Build_Shroud_Coverage` (`:858`) |
| Generated-world manifest | `WorldGenerationManifest` (`Combat/worldgenerationmanifest.h`) |
| Network sync | `WorldSyncTransport` (`Combat/worldsynctransport.h:93`) |
| Save/load | `ProceduralWorldPersistenceClass` (`Combat/proceduralworldpersistence.h:32`) |
| Commander chokepoints | `CommanderAI` reads `BridgePlanner::ResolvedBridge::StrategicImportance` (`Combat/commanderai.h:290`) |
| Logistics | `LogisticsGraphService` (`Combat/logisticsgraphservice.h:146`) |
| Seeds | `WorldSeedService` (`Combat/worldseedservice.h`) |
| Descriptor registry | `ProceduralAssetRegistry` (`Combat/proceduralassetregistry.h:137`) |
| Metric cache | `WorldMetricsClass` (`Combat/worldmetrics.h`) |
| World bounds | `WorldBoundsService` (`wwphys/worldboundsservice.h`) |
| Base layout | `ProceduralBaseLayoutService` (`Combat/proceduralbaselayout.h`) |
| Placement validation | `CommanderPlacementValidator` (`Combat/commanderplacement.h`) |
| Building factory | `RuntimeBuildingFactory` (`Combat/runtimebuildingfactory.h`) |
| Integration | `GeneratedWorldIntegrator` (`Combat/generatedworldintegrator.h`) |
| Combat space | `CombatSpaceValidator` (`Combat/combatspacevalidator.h`) |
| Road class mapping | static `Road_Graph_Class_To_Road_Class` (`Combat/roadgraphgenerator.cpp:124`) |

## Planning layers to rewrite and their consumers

Files outside each planner's own source that call its public API (self-checks excluded). Each record these planners expose must stay readable, through an adapter, until item 22.

- `MapPlanGenerator` (`wwphys/mapplangenerator.h`): proceduralworldmanager, proceduralworldprofile, roadgraphgenerator, strategicplanner, hydrologygenerator, macroterraingenerator.
- `HydrologyGenerator` (`wwphys/hydrologygenerator.h`): bridgeplanner, proceduralworldmanager, roadgraphgenerator, strategicplanner, worlddebugdisplay, worldgenerationmanifest.h.
- `StrategicPlanner` (`Combat/strategicplanner.h`): baseselectionplanner, bridgeplanner, combatspacevalidator, commanderai, generatedworldplacementrules, logisticsgraphservice, minimapbaselayer, navmeshgenerator, proceduralworldmanager, proceduralworldtools, roadgraphgenerator, settlementgenerator, tiberiumfieldgenerator, transportationnetworkvalidator, tunnelplanner, worldgenerationmanifest.h.
- `RoadGraphGenerator` (`Combat/roadgraphgenerator.h`): baseselectionplanner, bridgeplanner, combatspacevalidator, generatedworldplacementrules, logisticsgraphservice, navmeshgenerator, proceduralworldmanager, proceduralworldtools, settlementgenerator, tiberiumfieldgenerator, tunnelplanner.
- `BridgePlanner` (`Combat/bridgeplanner.h`): baseselectionplanner, commanderai, generatedworldplacementrules, minimapbaselayer, navmeshgenerator, proceduralworldmanager, proceduralworldtools, tunnelplanner, worldgenerationmanifest.h.
- `TunnelPlanner` (`Combat/tunnelplanner.cpp`): consolefunction, minimapbaselayer, navmeshgenerator, proceduralworldmanager.
- `BaseSelectionPlanner` (`Combat/baseselectionplanner.h`): biomedecorationgenerator, combatspacevalidator, commanderai, commandermodecontroller, expansionregionregistry, logisticsgraphservice, minimapbaselayer, proceduralbaselayout, proceduralworldmanager, proceduralworldtools, settlementgenerator, tiberiumfieldgenerator.
- `TiberiumFieldGenerator` (`Combat/tiberiumfieldgenerator.h`): WeatherMgr, combatspacevalidator, commanderai, expansionregionregistry, logisticsgraphservice, minimapbaselayer, proceduralworldmanager, proceduralworldtools, refinerygameobj, settlementgenerator.
- `BiomeDecorationGenerator` (`Combat/biomedecorationgenerator.h`): proceduralworldmanager.
- `SettlementGenerator` (`Combat/settlementgenerator.h`, bypassed on the hybrid path): biomedecorationgenerator, combatspacevalidator, generatedworldintegrator, logisticsgraphservice, minimapbaselayer, navmeshgenerator, proceduralworldmanager, proceduralworldtools.
- Also rewritten: `MacroTerrainGenerator` (`wwphys/macroterraingenerator.h`), `InteriorCliffGenerator` (`wwphys/interiorcliffgenerator.h`).

## Current pipeline

`ProceduralWorldManager::Generate_World_Attempt` (`Combat/proceduralworldmanager.cpp:1026`) runs:

Configure_World_Bounds → Build_Cliff_Ring → Build_Map_Plan → Build_Macro_Terrain → Build_Interior_Cliffs → Build_Hydrology → Build_Strategic_Plan → Build_Road_Graph → Build_Bridge_Plan → Build_Tunnel_Plan → Build_Base_Selection → Build_Tiberium_Fields → Build_Settlements → Build_Biome_Decoration → collision/far terrain → `RoadSystem::Build_Geometry` → Validate_Transportation_Network → Validate_Combat_Space → Build_World_Integration → Build_Navigation → Build_Minimap_Layer → Build_Shroud_Coverage → Arm_Expansion_Rules → logistics rebuild.

`Build_Chunk_Registry` runs four times: after terrain, road graph, bridge plan and tunnel plan.

Where the current code conflicts with the plan (each is replaced in the named item):

- Bases are chosen after roads; the plan freezes bases first (P8, I9).
- A generator failure is logged and the pass still completes; the plan fails the attempt (§17, I19).
- `MAX_WORLD_GENERATION_ATTEMPTS = 3`, but the last incomplete world is accepted; the plan forbids activating an incomplete world (§17, I19).
- Network and combat validators are "never a reason to refuse"; the plan makes them named 19A failures (I19).
- The chunk registry is rebuilt several times; the plan allows one rebuild (P7) and a single navigation build (P18).

## Seeds and versions

- `WorldSeedService::Normalize_Seed` (maps 0 to 1). `Derive_Subsystem_Seed(seed, domain) = CRC::String(domain, Normalize_Seed(seed))`. Floats come from `Random4Class::Get_Float`, which is a stateful draw.
- Domains (`Combat/worldseedservice.cpp:32-46`): `boundary`, `boundary_cliffs`, `terrain`, `map_plan`, `interior_cliffs`, `hydrology`, `roads`, `bridges`, `tunnels`, `settlements`, `tiberium`, `bases`, `biomes`, `decoration`, `combat_space_repair`.
- `Hash01` / `PickIndex` / `PickSign` / `Range` / `RoundHalfUp` do not exist yet; I2 adds them.
- `ProceduralWorldProfile::CURRENT_SCHEMA_VERSION = 11`, `CURRENT_GENERATOR_VERSION = 10` (`Combat/proceduralworldprofile.h:554`). They are checked at `proceduralworldprofile.cpp:336-338` and persisted at `proceduralworldpersistence.cpp:76,150`. I1 bumps each once, to 12 and 11.

## Profiles and entry points

- Profiles: `data/procedural_world/{default,small,large_supported,native}.json`. `DEFAULT_PROFILE_PATH` is `default.json` and `NATIVE_PROFILE_PATH` is `native.json` (`proceduralworldmanager.cpp:76-77`).
- API: `ProceduralWorldManager::Request_World`, `Request_World_Exact`, `Generate_World`, `Generate_World_Attempt` (`proceduralworldmanager.h:100,108,132,275`).
- Main menu: `MainMenuDialogClass::Start_Procedural_Skirmish` (`Commando/dlgmainmenu.cpp:373`; the native profile with seed 0 is used at `:458`).
- Listen server / dedicated host level load: `Combat/combat.cpp:921` calls `Generate_World`.
- Console: `procedural_world request <seed> [path]` (`Commando/consolefunction.cpp:2863`).
- Campaign: `Combat/persistentcampaign.cpp:274` (launcher at `Commando/init.cpp:1027`).
- Save/load: `Combat/proceduralworldpersistence.cpp:161` calls `Request_World_Exact`.
- Client sync: `Combat/worldsynctransport.cpp:582-583`.
- Offline seed batch: `Combat/proceduralworldtools.cpp:537-543`.

## Metric sources (for the I2 preflight)

- **Metric cache:** `WorldMetricsClass` provides `Get_Metrics(def_id | PhysDefClass*)`, `Get_Class_Metrics(LOCOMOTION_*)`, `Get_Widest` / `Get_Tallest`, `Get_Building_Footprint` and `Get_World_Units_Per_Meter`. It resolves extents from the model `WORLDBOX` (`worldmetrics.cpp:295-358`).
- **Humvee `H`:** today this is a hard-coded literal, `GDI_HUMVEE_WIDTH = 2.556f` (`Combat/roadgraphgenerator.cpp:78`), measured offline from the `V_GDI_HUMVEE` `WORLDBOX`: length 4.723, width 2.556, height 1.548. `C` and `W` are derived at `:87-92` using the plan's §0.5 formula. I2 must resolve `H` through `WorldMetricsClass` and fail with `MISSING_HUMVEE_COLLISION_METRIC`. The literal remains only as an expected-value cross-check.
- **Heavy reference:** existing code uses `Get_Class_Metrics(LOCOMOTION_TRACKED)` (`navmeshgenerator.cpp:266`, `proceduralworldmanager.cpp:662`).
- **Harvester:** no dedicated metric lookup exists. I2 resolves the Harvester definition through `WorldMetricsClass::Get_Metrics`.
- **Starting-building proxies:** `ProceduralAssetRegistry` (`proceduralassetregistry.cpp:54`) calls `WorldMetricsClass::Get_Building_Footprint`.
- **`V_REF`:** `BaseSelectionPlannerParams.ReferenceVehicleSpeed`, default `DEFAULT_REFERENCE_VEHICLE_SPEED = 8.33f` (`baseselectionplanner.cpp:79`).
- **`I_REF`:** `CombatSpaceValidatorParams.ReferenceInfantrySpeed = 4.3f` (`combatspacevalidator.cpp:77`), duplicated as `DEFAULT_REFERENCE_INFANTRY_SPEED = 4.3f` (`baseselectionplanner.cpp:80`). I2 extracts it unchanged into one shared constant.
- **`HarvesterLoadAmount`:** `profile.Resource.HarvesterLoadAmount` (`proceduralworldprofile.h:246`, default 600, JSON `resource.harvester_load_amount`) is passed to `TiberiumFieldParams.HarvesterLoadAmount` (`tiberiumfieldgenerator.h:156`).
- **Quantization:** there is no shared helper. Ad-hoc copies exist: 4-decimal manifest formatting (`worldgenerationmanifest.h:121`), `QuantizedHeight = h*1000` (`worldsnapshot.h:90`), and a quantized CRC in the strategic planner (`strategicplanner.h:161`). I2 adds the shared helper.

## Bridges, tunnels, fords

- `BridgeDefinitionClass` (`wwphys/bridgesection.h:100`): `Width` (clear width), `DeckThickness` (structural depth), `Section_Length(kind)`, `SupportSpacing`, `SupportClearance`, `RequiredClearance`, plus per-kind/per-state models and collide/traversable rules. `BridgeDamageStateType` is at `:58`. The definition has no max-span or approach-length field.
- `BridgePlannerParams` (`Combat/bridgeplanner.h:155`): `Seed` (used only for the random initial damage state, which the hybrid path disables and replaces with `PRISTINE`), `WaterClearanceMargin`, `MaxSpanLength`, `MaxApproachGradeRadians`, `MaxRerouteAttemptsPerEdge`. There is no approach length, so the §12 reserve uses `8W`.
- Tunnels (`wwphys/tunnelsystem.h`):
  - `TunnelClassType`: INFANTRY_ONLY, VEHICLE, MIXED, UNDERPASS, CAVE_SHORTCUT.
  - `TunnelClearanceClass`: bore width/height, safety margins, maximum grade, minimum bend radius, minimum portal straight.
  - `TunnelModuleDefinitionClass`: length, clearance, maximum turn, maximum grade.
  - The planner is in `Combat/tunnelplanner.cpp`.
- **Fordability:** there is no canonical vehicle/water traversal query. The only related code is the hydrology tag `HYDRO_CROSSING_FORD` (`wwphys/hydrologygenerator.h:107`). Under §0.8, `allow_fords=true` must fail with `FORD_CAPABILITY_UNAVAILABLE`, and native profiles keep `allow_fords=false`.

## Name mismatches against the plan (§0.1)

- The plan's "canonical collision/model metric cache" is `WorldMetricsClass`.
- The plan's minimap owner is `MinimapBaseLayer`; the shroud owner is `GeneratedWorldIntegrator::Partition_Shroud_Over_World`.
- The plan's navigation owner is `PathfindClass`, reached through `ProceduralWorldManager::Build_Navigation` / `navmeshgenerator.cpp`.
