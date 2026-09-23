# Procedural World Toolchain

V2 roadmap Phase 27 (Section 27). `Code/Combat/proceduralworldtools.{h,cpp}`, plus a small
reproduction-string logging addition in `Code/Combat/proceduralworldmanager.cpp`.

## Why one tool, not several

Section 27.1 (seed preview), 27.2 (step viewer) and 27.4 (runtime debug overlays) are one CPU-side
top-down raster tool with a `layer_mask`, not three separate systems. No runtime debug-draw/overlay
hook existed anywhere in this codebase before this phase (confirmed by search), so a live in-engine
HUD toggle was not the honest thing to build against nothing; a deterministic image file a developer
can open is. Toggling `layer_mask` bits both selects what a "preview" shows and satisfies "viewing
generation after macro terrain / hydrology / roads / ... " (Section 27.2), since every generator
stage's own data survives to the end of a successful generation pass -- there is nothing to freeze
mid-pipeline, only layers to include or omit from the same finished world.

## Validation report (27.3)

`ProceduralWorldToolsService::Build_Validation_Report` does not re-derive anything: `CombatSpaceValidator`
and `TransportationNetworkValidator` (Phase "gap report" items already built) keep their own last
`Validate()` call's metrics/failure counts as queryable static state. This function only reads it and
the active `WorldGenerationManifest`. `Serialize_Validation_Report` (JSON) and
`Format_Validation_Report_Text` (human-readable) are two views of the same struct;
`Write_Validation_Report_To_File` is the real-walk wrapper that builds, serializes and writes.

## Preview raster (27.1/27.2/27.4)

`Write_Top_Down_Preview(file_path, layer_mask, resolution, error_out)` walks the live generated
world's own singletons and writes a raw binary PPM (P6) -- no image-library dependency, three bytes
a pixel:

| Layer | Source |
|---|---|
| `PREVIEW_LAYER_HEIGHT` | `WorldTerrainSystem::Sample_Height`, shaded by range within the sampled world |
| `PREVIEW_LAYER_CLIFFS` | `WorldTerrainSystem::Get_Chunk_Info` (`CliffBand`/`InteriorCliff`) |
| `PREVIEW_LAYER_WATER` | `WaterSystem::Is_Point_In_Water` per pixel |
| `PREVIEW_LAYER_ROADS` | `RoadGraphGenerator::Get_Edge` + `StrategicPlanner::Get_Node` for endpoints |
| `PREVIEW_LAYER_BRIDGES` | `BridgePlanner::Get_Bridge` + `StrategicPlanner::Find_Node_By_Id` |
| `PREVIEW_LAYER_BASES` | `BaseSelectionPlanner::Get_Selected_Site` (NOD/GDI) |
| `PREVIEW_LAYER_TIBERIUM` | `TiberiumFieldGenerator::Get_Field` |
| `PREVIEW_LAYER_SETTLEMENTS` | `SettlementGenerator::Get_Settlement` |
| `PREVIEW_LAYER_BOUNDARY` | `WorldTerrainSystem::Get_Bounds` rectangle (an AABB proxy for the real cliff-ring polygon -- `WorldBoundsService` exposes no public vertex-iteration accessor) |

`World_To_Pixel`/`Fill_Buffer`/`Plot_Point`/`Plot_Filled_Square`/`Plot_Line` are pure pixel-math
primitives, self-check-covered directly. `Write_Top_Down_Preview` itself is the real walk and is
**not** self-check-covered -- it needs `WorldTerrainSystem::Has_Chunk_Registry()` true, which only a
live generated world (a physics scene, per `ProceduralWorldManager::Request_World`'s own doc comment)
produces; the self-check harness has none, the same disclosed limit every real walk against live
generator singletons has carried since Phase 24.

**Known simplification:** the water layer is a per-point `WaterSystem::Is_Point_In_Water` query, not
a polygon read from `WaterAreaClass` -- accurate at pixel resolution for what actually renders, but
does not reproduce a water body's exact analytic shape between samples.

## Reproduction command (27.5)

`WorldGenerationManifestService::Format_Reproduction_String` already produced exactly Section 27.5's
`generator_version=<n> seed=<n> profile=<hash> content_manifest=<hash>` format before this phase
(Phase 1). What this phase adds:

- `ProceduralWorldToolsService::Get_Reproduction_String()` -- a console-visible wrapper around it.
- `world_tools repro` console command.
- **Auto-logging on failure.** Before this phase, `ProceduralWorldManager::Generate_World` logged the
  reproduction string only on a *successful* generation (its own existing line). The three places it
  already logged a *failure* (an attempt failing outright, an attempt leaving required connectivity
  broken, and the bounded retry loop exhausting all attempts) did not include it. All three now do,
  via a small file-static `Format_Attempt_Reproduction_String` helper in `proceduralworldmanager.cpp`
  built from the failed attempt's own profile (`Manifest` itself may never have been committed for a
  failed attempt) -- this is Section 27.5's literal "every ... validation failure should log a
  concise reproduction string," not a new logging subsystem.

## Seed batch runner

`Run_Seed_Batch(start_seed, count, profile_path, results_out, error_out)`: for each seed, calls
`ProceduralWorldManager::Destroy_World()` (confirmed to leave the level/physics scene untouched --
it only tears down what the procedural generator itself built), `Request_World(seed, ...)`, then
`Generate_World(...)`, recording `CombatSpaceValidator::Passed()`/`TransportationNetworkValidator::
Passed()` per seed. Stops outright (returns fewer than `count` rows) on the first hard
`Request_World`/`Generate_World` failure rather than skipping it and continuing -- roadmap step 9's
literal requirement. Bounded by `MAX_SEED_BATCH_RESULTS` (64).

**Not exercised anywhere, including this phase's own self check:** the loop needs a live physics
scene the same way `Write_Top_Down_Preview` does; no existing code in this repository has ever run
this Destroy/Request/Generate sequence in a loop before this phase wrote it.

## Console

`world_tools <repro|validate [json_path]|preview <path> [layer_mask] [resolution]|
seed_batch <start_seed> <count> [profile_path]>` (`Code/Commando/consolefunction.cpp`,
`WorldToolsConsoleFunctionClass`).

## Self check

`shaderselfcheck worldtools` (`Run_Procedural_World_Tools`, `Code/Commando/shaderselfcheck.cpp`)
covers: `World_To_Pixel`'s in-bounds/out-of-bounds/degenerate-bounds/north-above-south cases;
`Fill_Buffer`/`Plot_Point`/`Plot_Line` on a small in-memory canvas, including that an out-of-bounds
plot never corrupts an in-bounds pixel; `Build_Validation_Report`/`Serialize_Validation_Report`/
`Format_Validation_Report_Text` against both validators' real `Validate()` output on an (honestly)
empty world; and `Get_Reproduction_String`'s self-consistency with the manifest layer it wraps.

**Not exercised by the self check** (both require a live physics scene, confirmed unavailable in
the harness): `Write_Top_Down_Preview`, `Write_PPM_Buffer_To_File`'s real disk I/O, and
`Run_Seed_Batch`.

**Next exact action** for the user, on a generated map: `world_tools repro` and confirm it matches
the log line `ProceduralWorldManager: world generated -- ...`; `world_tools validate report.json`
and open the file to confirm it is valid JSON matching the printed text; `world_tools preview
preview.ppm` and open the resulting file in any PPM-capable viewer (or convert with a tool such as
ImageMagick) to confirm it actually resembles the generated world's terrain/roads/bases; then, with
a small seed range, `world_tools seed_batch 1 5` and confirm it either reports 5 rows or stops early
with a named hard failure.
