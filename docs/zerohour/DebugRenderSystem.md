# Debug rendering tools

Roadmap Section 33, Zero Hour / SAGE feature 20. Donors: `W3DDebugDisplay`, `W3DDebugIcons`.

`Code/ww3d2/debugrendersystem.{h,cpp}` (the batched line pool and layer toggles) and
`Code/Combat/worlddebugdisplay.{h,cpp}` (the Combat-layer owner that turns an enabled layer into
real geometry), console commands `debug_layer`/`debug_status` in `Code/Commando/consolefunction.cpp`.

## Capability first, coverage second

The acceptance -- *"every new world/Commander subsystem has enough visualization to diagnose
placement, culling, transform, and rendering errors"* -- is a claim about capability existing, not
a claim that all eighteen of the roadmap's named overlays draw something this pass. Several of the
systems those overlays would visualize do not exist yet: building semantic nodes are Phase 39,
purchase-terminal and MCT anchors are Phase 38, the Commander cursor ray is Phase 35/37. What this
phase had to build is the one thing every one of those future producers will need and did not
have: a named layer to gate on, and a shared batched primitive to submit into, so the tenth debug
overlay does not reinvent its own line buffer the way stock Renegade's ad hoc debug boxes did.

## Immediate mode, not handle-addressed

Unlike `StatusOverlayRenderer` (Section 32), `DebugRenderSystem` has no handles. A debug overlay
is redrawn from live state every frame it is on -- there is nothing worth holding a reference to
between frames. `Begin_Frame` clears the pool; `Add_Line`/`Add_Box`/`Add_Sphere`/`Add_Cross`
(decomposing into 12/48/3 line segments respectively) queue into it; `Render` submits whatever is
left, batched through `WorldParticleManager`'s shared streak pool -- the same one tracers (Section
27), status rings (Section 32) and weather (Section 36) already share. The pool is bounded
(`DEBUG_LINE_POOL_MAX` = 4096) and refuses rather than grows.

## The split

`DebugRenderSystem` (`Code/ww3d2`) knows nothing about which subsystem a layer belongs to -- no
terrain, no roads, no `GameObjManager`, no wwphys of any kind, the same layering reason
`StatusOverlayRenderer` cannot reach `WorldShroudManager` directly. `WorldDebugDisplay`
(`Code/Combat`) is where a layer's toggle actually becomes a query against a real system, and it
is also the lazy scene object owner (same shape as `TracerSystemClass`/
`StatusOverlaySceneObjectClass`): nothing is created until at least one layer is turned on, so a
normal match pays nothing for this system.

## What draws this pass

`DEBUG_LAYER_BUILDING_FOOTPRINTS` -- `WorldDebugDisplay::Update` walks `GameObjManager`'s list
(the same `BaseGameObj -> As_ScriptableGameObj -> As_BuildingGameObj` path Section 31's tactical
model established, required because a `BuildingGameObj` is not a `PhysicalGameObj`) and draws a
wireframe box from `BuildingGameObj::Get_Bounds()` for every building found. This is real,
existing data with no new accessor required, chosen specifically because it needed none.

**Update, a later pass**: two more layers draw for real now, both chosen because they needed no
new per-instance iterator on a world system, only a debug consumer for data that already existed:

- `DEBUG_LAYER_RADAR_CONTACTS` -- `Draw_Radar_Contacts` calls
  `TacticalIntelligenceModel::Build_Strategic_Contacts` for the local player's own team (from
  `COMBAT_STAR->Get_Player_Type()`) and draws a coloured cross at every contact's position (green
  = own team, red = a visible enemy, white = unowned -- a resource/objective/marker; buildings
  drawn larger). This is the first renderer `Build_Strategic_Contacts` has had at all, debug-only,
  not the Commander map itself (Phase 36+).
- `DEBUG_LAYER_SHROUD_CELLS` -- `Draw_Shroud_Cells` uses two new `WorldShroudManager` accessors,
  `Get_Grid_Geometry`/`Get_Cell_State_By_Index` (`Code/wwphys/worldshroudmanager.{h,cpp}` --
  Section 30's own doc named this overlay as the reason to add them), and draws a flat quad
  outline per non-`SHROUD_UNKNOWN` cell of the viewer's own team grid, coloured by state
  (green=`VISIBLE`, yellow=`RADAR_DETECTED`, orange=`LAST_KNOWN`, dim blue=`EXPLORED`). Four
  lines per cell, not `Add_Box`'s twelve, since this is a 2D grid, not a volume; a very large
  explored area is bounded the same way every other layer already is -- `DebugRenderSystem`'s own
  line pool drops and counts anything past its budget rather than growing.

Every other layer is a real, toggleable, named entry in `DebugLayerType` that still draws nothing:

| Layer | Awaiting |
|---|---|
| `terrain_chunks`, `terrain_materials` | `WorldTerrainSystem` has no public per-chunk/per-material iterator today |
| `road_splines`, `road_triangles` | `RoadSystem` has no public per-spline/per-triangle iterator today |
| `bridge_sections` | `BridgeSystem` has no public per-section iterator today |
| `water_volumes` | `WaterSystem` has no public per-volume iterator today |
| `foliage_cells` | `FoliageSystem` has no public per-cell iterator today |
| `spatial_index_cells` | `WorldSpatialIndex` is a query wrapper, not its own introspectable grid |
| `dynamic_light_radii` | `WorldLightManager` exposes aggregate bounds/counts, not per-light enumeration |
| `shadow_projectors` | `WorldShadowManager` exposes aggregate counts, not per-caster enumeration |
| `collision_bounds` | no per-object WWPhys bounds walk was added this pass |
| `building_semantic_nodes` | Phase 39 (building semantic JSON) |
| `placement_overlap` | Phase 37 (ghost placement) |
| `purchase_terminal_anchors`, `mct_anchors` | Phase 38 (auto PCT/MCT) |
| `commander_cursor_ray` | Phase 35/37 (Commander camera, ghost placement) |

Adding a public per-instance iterator to the seven already-shipped world systems still listed
above would have been a far larger change than Section 33 itself asks for, for overlays with no
consumer to actually look at them yet. Each row above is a small, well-scoped follow-up once
something needs to look at it.

## Console commands

`debug_layer <name> <0|1>` toggles one layer by its lowercase name (`debug_layer` alone lists
every layer and its state); `debug_status` reports the current frame's queued/dropped/drawn line
counts. "Development-only debug controls are allowed" and are not legacy/new feature selectors" --
neither command touches gameplay state, and nothing in `Code/Combat` gameplay logic ever reads a
`DebugLayerType`'s state.

## Checks

`renegade -shaderselfcheck debugrender`, registered as ctest `shader_debugrender` and
`fds_shader_debugrender`. Device-less, same construction as the tracer/overlay checks. Pins layer
toggles, `Disable_All_Layers`, name lookup (including the `DEBUG_LAYER_COUNT` sentinel), box/cross
decomposition counts, a real `Render` call drawing geometry placed in front of the check's default
camera, `Begin_Frame` clearing between frames, and the line pool refusing past its bound.

**Not covered by any automated check**: the building-footprint wiring itself needs a real
`BuildingGameObj` in a live combat scene, which `-shaderselfcheck` cannot produce. The console
commands are unexercised by anything but a person at a prompt.
