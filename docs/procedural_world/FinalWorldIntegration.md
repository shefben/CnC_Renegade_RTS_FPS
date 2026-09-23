# Final Generated-World Integration (V2 Phase 16)

Roadmap Section 34 Phase 16 -- "collision, navigation, visibility, lighting, shroud/radar,
minimap." Sections 18, 29.

## Steps 1-2: collision/render registration (`GeneratedWorldIntegrator`)

`GeneratedWorldIntegrator` (`Code/Combat/generatedworldintegrator.h/.cpp`) resolves every real
`SettlementGenerator` `PlacedStructureInstance` through the same chain `BuildingGameObj` already
uses: `ProceduralAssetRegistry::Get_Asset_By_Id` -> `BuildingDescriptorManager::Get_Descriptor`
-> `WW3DAssetManager::Create_Render_Obj` -> `StaticPhysClass` via
`PhysicsSceneClass::Add_Static_Object`, mirroring `building.cpp`'s own `Add_Runtime_Part`
pattern exactly (`new StaticPhysClass; Set_Model; Set_Transform; model->Release_Ref();
scene->Add_Static_Object(phys);`). No settlements is a real, correct outcome (an empty world, or
the feature disabled) and validates trivially with zero records, the same convention
`SettlementGenerator::Generate` itself already documents. Any failure along the resolve chain
(unknown asset id, unloaded descriptor, no model name, missing model file on a dedicated server)
leaves that one structure's `HasCollision` false -- the same honest skip
`BuildingGameObj::Build_Runtime_Components` already uses, never a fabricated box.

## Steps 3-8: navigation (`NavMeshGenerator`)

`NavMeshGenerator` (`Code/Combat/navmeshgenerator.h/.cpp`) populates `PathfindClass`'s own real,
live, AI-consulted database (`Add_Sector`/`Add_Portal` -- what `action.cpp`'s
`Does_Pathfind_Data_Exist`/`Find_Waypath` actually query), correcting an earlier recon finding
that no such builder API exists. `PathfindSectorBuilderClass`
(`wwphys/PathfindSectorBuilder.cpp`) looks like a ready-made one but is confirmed dead: absent
from `wwphys/CMakeLists.txt`, calls `PathfindClass` methods that do not exist on the class as
checked in, and is wired to LevelEdit's own interactive UI -- not a reusable reference.

**One sector graph, not three.** `PathfindSectorClass` carries no locomotion-class tag and
`PathfindClass` is one global singleton, so there is no way to register "walkable for infantry,
not for a tank" as two different answers about the same point without a parallel sector format.
`WorldMetricsClass::Get_Class_Metrics`'s own documented answer to this shape of question --the
most restrictive member across every loaded locomotion class-- is what this generator builds: one
sector graph sized by the tightest `MaxClimbableSlope` across infantry, wheeled and tracked
(whichever have a loaded definition), walkable by all of them at once.

**Grid, not a physics floodfill.** One sector per walkable chunk, at
`WorldTerrainSystem::Get_Chunk_Info`'s own resolution (excluding `CliffBand`/`InteriorCliff`/
over-slope/water/structure-footprint chunks) -- reusing the terrain generators' own already-
resolved answer to the same question rather than re-deriving it through a physics simulation, a
disclosed coarser granularity than a per-cell floodfill would give.

**Flight is a bound, not a graph.** This tree's AI never consults `PathfindClass` for flight, so
step 5's "air navigation/flight volume" is built as exactly that: a real bounded volume
(`WorldBoundsService::Get_Air_Ceiling` as the ceiling, tallest real chunk height plus
`FlightFloorClearanceMargin` as the floor), not a fabricated sector graph nothing would query.

**Naval is skipped outright** -- no canonical naval locomotor exists anywhere in this tree
(grepped), which step 6's own "only where canonical support exists" allows.

**Bridges get off-mesh portals** (step 7): each real `BridgePlanner` bridge's own road-edge
endpoint `StrategicNode` positions are resolved via `RoadGraphGenerator`, and the nearest walkable
sector within `BridgeLinkSearchRadius` on each side is linked directly -- bridge-deck geometry is
never modeled as its own sector set. **Tunnels (added by V2 Phase 10):** each accepted bore gets
an interior sector linked by an entrance portal and an exit portal (`Get_Tunnel_Portal_Count`); see
`RoadsBridgesTunnels.md`. Building-interior transitions are still not produced -- no building in
this asset pool has an interior to transition into (Phase 13's own disclosed connector-metadata gap).

**Step 8** (`Get_Revision`) is 0 until the first successful `Build`, 1 after -- real and
meaningful today as "this world's nav was built once"; Phase 18/19's own monotonic world revision
generalises it further once runtime mutation exists.

**A real, tolerated null.** `PathfindClass` is constructed inside `PhysicsSceneClass`'s own
constructor (`pscene.cpp:247`), so `PathfindClass::Get_Instance()` can legitimately be null
whenever `PhysicsSceneClass::Get_Instance()` is -- the same asymmetry `FoliageSystem::Add_Instance`
/`Add_Proxy` already null-guard for the identical reason. `NavMeshGenerator::Build` checks this
once and skips sector/portal creation (returning `true`, 0 sectors) rather than crashing.

## Step 9: no incomplete-nav window

There is no incremental/streaming nav rebuild in this tree, so "AI cannot enter a chunk whose
pending collision/nav rebuild is incomplete" reduces to the coarser, already-real guarantee step
15 provides: `Build_Navigation` runs to completion, synchronously, before
`ProceduralWorldManager::WorldActive` is ever set true. There is no live match for an AI to act in
until navigation is either fully built or has honestly refused (0 sectors); no partial state is
ever observable.

## Steps 10-11: culling and lighting, free from reuse

Every object `GeneratedWorldIntegrator` adds goes through
`PhysicsSceneClass::Add_Static_Object`/the underlying `RenderObjClass` scene registration -- the
same v1 spatial-partition/frustum-culling and runtime static-lighting path every hand-placed level
object already goes through. No new culling or lighting code was needed or written; this is
existing-system reuse, not an unimplemented step.

## Step 12: minimap/tactical base layer (`MinimapBaseLayer`)

`MinimapBaseLayer` (`Code/Combat/minimapbaselayer.h/.cpp`) reuses `WorldTerrainSystem`'s own
chunk registry as its cell grid (not a second, finer terrain summary), classifying each cell from
real, already-established data: `TERRAIN_MASK_ROAD` (threshold 0.5), `WaterSystem::Is_Point_In_
Water`, `TerrainChunkInfoClass::CliffBand`. Markers name real, already-resolved objects by their
owners' own stable ids: `BaseSelectionPlanner` (bases), `SettlementGenerator` (settlements and
POIs), `TiberiumFieldGenerator` (fields), `BridgePlanner`+`RoadGraphGenerator`
(`Find_Node_By_Id` for bridge position), and `TunnelPlanner` (one `MINIMAP_MARKER_TUNNEL_PORTAL`
per mouth, added by V2 Phase 10).

**Real, currently unconsumed.** Grepping this tree for a minimap/tactical-map renderer turns up
nothing; `WorldShroudManager` (live per-team coverage) and `TacticalIntelligenceModel` (live
per-object contacts) both exist but neither owns a *static* map background. This is real output
with no renderer to hand it to yet, the same honesty `CombatSpaceValidator`'s own unproduced
repair-operation names already carry.

## Steps 13-14: shroud/radar and the Commander intelligence firewall

`WorldShroudManager` and `TacticalIntelligenceModel` are both already fully world-coordinate
based (`Get_Shroud_State`, `Is_Radar_Detected` and every sensor source take a `Vector3`), so no
coordinate translation is needed or wanted. But verification found a real gap rather than a clean
pass: `WorldShroudManager`'s per-team grid does not exist at all until `Re_Partition` has been
told where the world is, it does not auto-partition on first use the way `WorldLightManager` does,
and its one existing caller --`PhysicsSceneClass::Re_Partition_Dynamic_Culling_System`
(`pscene.cpp:1787`)-- is documented editor-only and additionally resets vis. Nothing at game
runtime was ever going to size the shroud grid to a generated world.

`GeneratedWorldIntegrator::Partition_Shroud_Over_World` closes that: it derives the generated
world's real AABB from the chunk registry's own patch bounds --the same bounds `NavMeshGenerator`
and `MinimapBaseLayer` already read-- and calls the same `WorldShroudManager::Re_Partition` the
editor path calls, without the vis reset. `ProceduralWorldManager::Build_Shroud_Coverage`
delegates to it on `terrain_built`, sequenced with the other Phase 16 builds before `WorldActive`.
A shroud cell and a world position now line up for a generated world exactly as they do for a
hand-authored one.

**Step 14** needs no new enforcement: Phase 16 adds no fog-of-war bypass and no intelligence
source. `GeneratedWorldIntegrator` adds static collision/render objects only, never a unit or a
contact, and `MinimapBaseLayer` is a static map background that no live per-team query reads. The
firewall's existing enforcement is therefore unaffected by this phase -- verified, not
re-implemented.

## Step 15: world-generation completion barrier

`ProceduralWorldManager::Is_World_Active()`, a pre-existing accessor, is the real barrier: with
Phase 16's three build calls (`Build_World_Integration`, `Build_Navigation`,
`Build_Minimap_Layer`) sequenced inside `Generate_World_Attempt` before `WorldActive` is set true,
no caller can observe an active world whose collision/nav/minimap build has not already run to
completion (successfully or via an honest, disclosed refusal). No new synchronization primitive
was needed.

## Step 16: manifest checksums/revisions

`WorldGenerationManifest::NavigationRevision` -- 0 until Phase 16, pre-planned as exactly that
comment -- is finally populated from `NavMeshGenerator::Get_Revision()`. World integration and
minimap layer are per-session-rebuildable state, not manifest-recorded content (neither changes
what seed/profile combination reproduces the world), the same treatment biome decoration's own
instance placement already gets.

## What is proven, and what is not

Six new self-checks: `terrain_worldintegration`/`fds_terrain_worldintegration`,
`terrain_navigation`/`fds_terrain_navigation`, `terrain_minimap`/`fds_terrain_minimap`
(`-terrainselfcheck worldintegration|navigation|minimap`). Each asserts the honest-refusal case
(no settlements / no chunk registry / no infantry definition) trivially passes, then runs the
same real terrain/hydrology/strategic-plan/road-graph/bridge-plan/base-selection/Tiberium-field/
settlement chain `terrain_combatspace` already proves and asserts real structural properties:
`terrain_worldintegration` -- record count matches structure count, collision/missing-model sums
correctly, every record has a persistence id, at least one real collision when structures exist;
then step 13: a fresh `WorldShroudManager` reports unpartitioned, `Partition_Shroud_Over_World`
partitions it with real cells and real grid geometry, and **every** real patch centre in the world
falls inside the resulting grid -- re-derived from the patch bounds rather than trusting the call's
own return value, the same defence-in-depth `CombatSpaceValidator`'s hard failures already use.
`terrain_navigation` -- branches on whether an infantry definition is loaded (client self-check
context has none; asserts the honest refusal); when one is loaded, asserts sector count > 0,
`PathfindClass::Does_Pathfind_Data_Exist()`, an ordered flight volume, rebuild-determinism, and a
residue-free `Clear()`. `terrain_minimap` -- cell grid dimensions match the chunk registry, marker
count > 0, every marker has a recognised type and a non-empty id, rebuild-determinism.

Not proven: a live client actually walking a generated sector graph or reading the minimap layer
through a real renderer (none exists yet to test against, per steps 12/9's own disclosed gaps
above); object/foliage-aware navigation exclusion beyond the structure-footprint margin already
applied; and naval/tunnel/interior navigation, each a disclosed non-goal rather than an
unattempted one.

`renegade`, `renegadeserver` and `leveledit` all build clean, 0 warnings. `ctest -N` now lists 167
(was 161).

**Unattended `ctest` runs (fixed during this phase).** A full-suite run used to stop dead on a
modal assertion dialog. Two real, pre-existing defects caused it, both now fixed:

- `shaderselfcheck.cpp` declared `CameraClass camera;` with automatic storage in six checks.
  `CameraClass` is a `RenderObjClass`, which is a `RefCountClass`: its constructor starts the
  object at one reference and its destructor asserts that reference is gone, so a stack-allocated
  one tripped `refcount.h:185 (NumRefs == 0)` at every scope exit. The engine itself never
  stack-allocates one. They are now `NEW_REF`'d and released, per refcount.h's own stated rules.
- The `-*selfcheck` entry points installed no assert handler, so `WWDebug_Assert_Fail` fell back
  to a modal Abort/Retry/Ignore box -- one no test runner can answer, re-opening on every Ignore.
  The game installs a handler in `init.cpp` and every other harness in this tree
  (`Code/Tests/PhysTest`, `MeshTest`, `movietest`) installs its own; the self-checks were the only
  ones left without. They now install one that prints `ENGINE ASSERT: <file> (<line>) Assert:
  <expr>` to stderr and exits non-zero, so an engine assert fails the test loudly and visibly
  instead of waiting on a dialog.

`ctest` now runs the whole suite unattended, on both binaries, with every failure reported as
text.
