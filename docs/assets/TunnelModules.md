# Tunnel module models

V2 roadmap Section 12 / 12A.2, Phase 10. Two model names, and neither exists yet.

`TunnelSystem::Build_Tunnel_Geometry` in `Code/wwphys/tunnelsystem.cpp` builds every module of every
tunnel. It asks `WW3DAssetManager::Get_Instance()->Create_Render_Obj(name)` for the model its
module definition names, but only when the definition declares an authored clearance that
covers the tunnel's required bore and a grade limit that covers the tunnel's grade. The names
live in `TunnelSystem::Define_Default_Modules`.

## What happens without them

Section 12.2 prefers authored sections and allows "simple runtime tunnel tube/portal geometry
only where modular assets cannot satisfy the route". Neither default module declares a clearance
yet (`ClearanceWidth`/`ClearanceHeight` are 0). No authored section exists to prove one, so every
module instance is the **procedural tube**:

- a floor slab, a roof slab and two wall slabs;
- built by `DynamicMeshBuilderClass` to exactly the bore the tunnel's class demands;
- collidable with the same bits a bridge deck uses;
- drawn with `MATERIAL_PROGRAM_BRIDGE` (lined) or `MATERIAL_PROGRAM_TERRAIN` (cave).

Each mouth also gets a procedural headwall. Nothing is missing in gameplay terms. What is missing
is art.

`tunnel_status` prints `missing model(s)`. That count is only non-zero when a definition claims
a model that is not in the mix files. Today no definition claims one, so it reads 0.

## Required assets

| Name passed to the asset manager | Module | Consumer | Absent behaviour |
| --- | --- | --- | --- |
| `ow_tunnel_lined_straight` | `tunnel_lined` (infantry, vehicle, mixed, underpass) | `TunnelSystem::Build_Tunnel_Geometry` | procedural tube, bridge material |
| `ow_cave_rock_straight` | `cave_rock` (cave shortcut) | `TunnelSystem::Build_Tunnel_Geometry` | procedural tube, terrain material |

## Geometry contract (Section 12A.2)

- **Sockets, not bounding boxes.**
  - The entry socket is the model origin. The bore runs along +X, Y runs across, Z is up, and
    the floor surface is at Z = 0.
  - The exit socket is at exactly `(10, 0, 0)`, the module `Length` in
    `Define_Default_Modules`.
  - The next module's origin is placed on this module's exit socket.
- **Straight only.** Both defaults declare `MaxTurnRadians = 0`. The planner only lays straight
  bores today (see `docs/procedural_world/RoadsBridgesTunnels.md`).
- **Clearance.** The authored bore must be at least as wide and as tall as the widest tunnel
  class it will serve. Those numbers are runtime-derived from `WorldMetricsClass` and the
  profile's `min_vehicle_lane_clearance`. `tunnel_status` prints the bore of every tunnel it
  built, so read the real target from a live world. Then set `ClearanceWidth`/`ClearanceHeight`
  on the definition. Until those are set, the model is never used.
- **Grade.** The model must tolerate pitching up to the definition's `MaxGradeRadians` (30°)
  about its entry socket.
- **Collision.** Floor, walls and roof must collide. Keep the walls closed so nothing escapes
  the bore sideways.
- **Lighting anchors.** Interior lights come from the tunnel, not the model: at most 4 per
  tunnel, spaced every `LightAnchorSpacing` = 20 units for lined bores, none for caves. A model
  needs no light nodes.
- **Remainder.** A bore that is not a whole number of modules ends with one shortened
  procedural piece, because an authored section cannot be cut.
