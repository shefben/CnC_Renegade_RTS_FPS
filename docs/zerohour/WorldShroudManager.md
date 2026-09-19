# Shroud / fog of war + sensor-source API

Roadmap Section 30, Zero Hour / SAGE feature 15. Donor concept: `W3DShroud`.

`Code/wwphys/worldshroudmanager.{h,cpp}`, driven from `PhysicsSceneClass` (`Code/wwphys/pscene.cpp`)
and brought up/down by `WWPhys::Init`/`Shutdown` (`Code/wwphys/wwphys.cpp`), exactly where
`WorldLightManager` (Section 25) is wired, because the two services have the same shape: one
world, a per-purpose grid over it, no second world database.

## The rule this phase is shaped by

Section 30 states it directly: *do not black out the physical first-person world because the
Commander intelligence layer has not explored it.* That is a claim about what this class must
never touch, and it is provable by construction rather than by a runtime check: `worldshroudmanager.h`
includes nothing from rendering, physics collision, targeting or the FPS HUD, and every public
method takes only a team id, a world point, and the sensor-source fields the roadmap names. There
is no method whose name or signature could be mistaken for "hide this render object" or "block
this hit test", because no such method exists here. An FPS caller that wants to know if it can
shoot something never has a reason to call into this file, and nothing in this file calls into
FPS code either.

## Five states, per cell, per team

```text
UNKNOWN          never covered by this team's sensors
EXPLORED         covered at some point; permanent terrain memory
LAST_KNOWN       was VISIBLE or RADAR_DETECTED recently; decaying
RADAR_DETECTED   inside a source's radar radius this tick
VISIBLE          inside a source's vision radius this tick
```

The record is the **cell**, not a handle to whatever stood there. `Get_Shroud_State` derives its
answer from a stored `Explored` bit plus a transient state and a countdown; nothing about a
LAST_KNOWN cell references a `GameObj`, a `RenderObjClass`, or any other live thing. That is what
makes the roadmap's *"LAST_KNOWN must not track an unseen object's live transform"* true by
construction: there is nothing in a `ShroudCellClass` capable of tracking anything. When a cell's
coverage drops, `Timestep` freezes the cell where it was and starts a retention countdown; nothing
updates that snapshot again until either the countdown reaches zero (falls back to `EXPLORED`) or
a source covers the cell again (jumps straight back to `VISIBLE`/`RADAR_DETECTED`).

## The sensor-source API, verbatim

```cpp
RegisterStrategicSensorSource(team, world_transform, vision_radius, radar_radius,
                               last_known_retention_seconds, online) -> handle
UpdateStrategicSensorSource(handle, world_transform, vision_radius, radar_radius, online) -> bool
UnregisterStrategicSensorSource(handle) -> bool
```

This is the **only** path by which any cell becomes anything other than `UNKNOWN`/`EXPLORED`.
There is no `Reveal_Point`, no `Set_Team_Sees_Everything`, nothing a camera could call to shortcut
the grid. A Commander camera that wanted to reveal something would have to register itself as a
source through this exact API — the same one a building or a unit uses — which is precisely what
the roadmap forbids it from doing, and nothing wires a camera to it. When Phase 35's Commander
camera lands, **it must not call any of these three functions**; that is the concrete form the
rule takes for that later phase, and it is written here so the constraint travels with the code
that would violate it.

Communications Center/Advanced Communications Center coverage (Phase 34) is meant to call
`RegisterStrategicSensorSource` once when powered on and `UnregisterStrategicSensorSource` (or
`UpdateStrategicSensorSource(..., online=false)`) when destroyed or powered down, exactly as the
roadmap specifies: *"Communications buildings later use this API instead of creating another
shroud implementation."* No building calls it yet — Phase 34 is later in this same pass and wires
it when the power-grid state that gates "online" exists.

## Team independence

Coverage is stored per team in an independent grid (`TeamGridClass`, capped at
`WORLD_SHROUD_MAX_TEAMS` = 8 — refuses rather than grows, the same discipline as every bounded
table in this project). Team A's sources never write into team B's grid; `shroud` mode's check
proves this directly by placing two teams' sources far enough apart that only geometry, not a
missing feature, would make them independent by accident.

## Radar vs. vision

A source stamps two circles: `vision_radius` produces `VISIBLE`, `radar_radius` (normally the
larger of the two) produces `RADAR_DETECTED` for the ring between them. A cell inside both reads
`VISIBLE` — vision always wins the merge, since it is the stronger claim. This gives Section 31's
radar model (Phase 28) a cell-level answer to "do we have a line of sight here or only a radar
blip" without Phase 28 having to duplicate any geometry.

## Grid, bounds, and cost

The grid is 2D over the world's X/Y plane — the same ground-plane convention
`WorldTerrainSystem::Sample_Height(x, y, ...)` uses — at a fixed 500-unit cell size, coarsened
automatically (never past `WORLD_SHROUD_MAX_GRID_CELLS` = 65536 cells) if a map is large enough
that 500 units would exceed the bound. `Timestep` costs the sources and the cells their circles
touch, not the size of the map or the number of `GameObj`s in it — the grid is rebuilt into a
scratch buffer each team each tick from the current source list only.

`Re_Partition` is called from the same place `WorldLightManager::Re_Partition` is
(`PhysicsSceneClass::Re_Partition_Dynamic_Culling_System`, `pscene.cpp:1770`), and drops every
team's cell memory on a size change, the same as a light grid resize; sensor-source
*registrations* are not dropped, so a still-standing building does not have to re-register itself
across whatever triggered the resize. `Release_Resources` (world unload) drops cell memory the
same way `WorldLightManager::Release_Resources` drops dynamic lights — shroud memory belongs to
the world it was earned in.

## TT visibility integration

The roadmap asks this phase to *"avoid needlessly replicating tactically forbidden information
where existing network visibility controls can suppress it."* This class answers that structurally
rather than by touching TT's network code: a shroud query returns a coarse per-cell state, never a
full object transform, so a networked reveal built on top of it (Phase 28's radar contacts) only
ever has to replicate "team X can see cell Y", which is already the minimum information — nothing
here forces a wider broadcast than that. No TT per-player visibility code was changed this pass;
there was nothing to merge, because nothing in TT already implements a shroud grid.

## What this phase does not build

No consumer yet reads shroud state — Phase 28 (radar/tactical intelligence model) is the first
reader, and the Commander map/HUD presentation is later still. No building calls
`RegisterStrategicSensorSource`; that arrives with Phase 34's communications buildings. That is
the same discipline the project already applies to missing art and unimplemented component
contracts: the service is real and checked now, and its callers arrive with the phases that need
them.

## Checks

`renegade -terrainselfcheck shroud`, registered as ctest `terrain_shroud` and `fds_terrain_shroud`.
Device-less — no scene, no device. What it pins: an unpartitioned manager answers `UNKNOWN`
everywhere rather than asserting; partition/cell-count bounds; two teams' independent VISIBLE
coverage from two sources; the vision/radar ring distinction; that taking a source offline removes
its coverage; that lost coverage falls to `LAST_KNOWN` and only reaches `EXPLORED` once retention
is spent, without anything re-touching the frozen point in between (proving the no-live-tracking
claim); dynamic register/unregister, including double-unregister refusing; and the source table's
256-entry bound refusing rather than growing.

**Not covered by any automated check**: everything that needs a real building, unit or Commander
camera to exist — none of Phase 28 through 35's consumers are built yet. The camera-independence
claim is proved by construction (no camera header is reachable from this file) rather than by a
runtime assertion, the same category of proof P26 used for `HTree` bone-riding.
