# Commander Mode camera / embodied player

Roadmap Section 41, Phase 35.

`Code/Combat/commandermodecontroller.{h,cpp}`, hooked into `Code/Combat/combat.cpp`'s existing
"Camera 1" block and `CombatManager::Init`/`Shutdown`; console commands `commander_mode <0|1>` and
`commander_status`.

## "Without replacing/degrading FPS controls"

`CCameraClass` (`Code/Combat/ccamera.cpp`) already carries first-person, third-person, sniper and
cinematic host-model camera behaviour. None of it is touched. `CommanderModeController` is an
external, narrow hook at the one call site that would have driven it
(`combat.cpp`'s `"Camera 1"` block): while Commander Mode is active,
`CommanderModeController::Update` runs instead of `CCameraClass::Update`, computing and applying
its own transform directly to the scene camera; the instant Commander Mode ends, that call site
goes back to calling `CCameraClass::Update` exactly as it did before this phase existed. *"FPS
behaviour outside Commander Mode is unchanged"* holds because nothing about `CCameraClass` was
edited at all -- the acceptance line is true by construction, not by testing every camera mode
`CCameraClass` already had.

## Embodiment, enforced by construction

`Enter_Commander_Mode(SoldierGameObj * star)` refuses a null star -- *"keeps the same physical
`SoldierGameObj`"* is impossible without one -- and refuses re-entering while already active.
Nothing in this file ever constructs a `GameObj` of any kind, so *"does not spawn a second player
object"* is also true by construction rather than by a runtime check that could be wrong.

`CommanderModeController` never *dereferences* the star it is handed --
`SoldierGameObj` is only forward-declared in the header, and the `.cpp` never includes `soldier.h`
either. The one fact this class needs about the star's condition, whether it is still alive, is a
parameter `Update` takes (`star_is_alive`) rather than something this class looks up itself --
the same "pass in the fact, get back a decision" split `GuardStateControllerClass` (Section 37)
already uses, and for the same reason: it makes *"death immediately exits Commander Mode"*
checkable with no device, no scene, and no live `SoldierGameObj` at all. The real caller
(`combat.cpp`) computes it from `COMBAT_STAR->Get_Defense_Object()->Get_Health() > 0.0f` -- the
same dead-soldier check `RadarManager`/`TacticalIntelligenceModel` (Section 31) already use.

## Camera

`Set_Focus`/`Pan` clamp to `Set_World_Bounds`; `Set_Zoom`/`Zoom` clamp to `Set_Zoom_Range`;
`Set_Tilt` clamps to `Set_Tilt_Range` (*"configurable tilt"*); `Set_Rotation` wraps rather than
clamps, since a full turn is always allowed (*"rotation if desired"*). `Compute_Camera_Transform`
is a pure function of the current focus/zoom/tilt/rotation -- an orbit camera at exactly `Zoom`
world units from `Focus`, built from `forward`/`right`/`up` basis vectors via
`Matrix3D::Set(x, y, z, pos)`. Checked directly: the resulting transform's translation is the
right distance from the focus, and its three axes are unit length and mutually orthogonal -- a
real, valid transform, not merely "some matrix".

`Pick_At_Cursor` reaches Section 16's existing `WorldSpatialIndex::Query_Ray` --
*"terrain + arbitrary W3D geometry picking"* through the one query surface that already exists,
not a second raytest implementation.

## Intelligence firewall

*"Camera movement itself reveals nothing"* holds the same way it holds for every other phase this
pass built on Section 30: `commandermodecontroller.cpp` never includes `worldshroudmanager.h` and
never calls `RegisterStrategicSensorSource`. There is no code path from this file into the shroud
at all, so there is nothing to accidentally get wrong here as later phases (the sidebar, Phase 36;
ghost placement, Phase 37) get built on top of it.

## Authority

*"Camera presentation is client-side. Strategic gameplay actions remain server-authoritative."*
Nothing in this file touches the network -- there is no packet, no replication, and no
authorization check anywhere in it, which is consistent with "camera presentation" being exactly
the client-local half of Commander Mode this phase covers. Any actual strategic action (a
purchase, an order, a placement) reads `CommanderModeController::Is_Active()`/`Get_Embodied_Star()`
and is that later phase's own responsibility to gate server-side.

## What this phase does not build

- **Input rerouting.** *"Redirects camera/input to strategic controls"* is the camera half of
  that sentence; keybinding/input-routing changes for Commander Mode were not attempted this
  pass -- `Handle_Input()` (`combat.cpp`) is untouched, and no production call site calls
  `Enter_Commander_Mode` yet (only the `commander_mode` console command does).
- **Commander overlays/sidebar.** `Is_Active()` is the one flag a future UI layer (Phase 36) would
  read to decide whether to show one; no UI exists yet to read it.
- **`commander_station_anchor` support.** Named in the roadmap as optional; no building-anchor
  consumer calls `Enter_Commander_Mode` from a placed station yet, since nothing places buildings
  yet either (Phase 37).

## Checks

`renegade -shaderselfcheck commandermode`, registered as ctest `shader_commandermode` and
`fds_shader_commandermode`. Device-less: `Enter_Commander_Mode` is exercised with a sentinel
non-null `SoldierGameObj*` that is never dereferenced, matching the real class's own guarantee.
Pins: null-star refusal; double-entry refusal; `Get_Embodied_Star` returning the same body;
pan/zoom/tilt clamping and rotation wrapping; `Compute_Camera_Transform`'s distance and
orthonormality; death (`star_is_alive == false`) exiting Commander Mode and clearing the embodied
star; and `Update` being a safe no-op while inactive.

**Not covered by any automated check**: `Pick_At_Cursor`, deliberately -- it reaches
`WorldSpatialIndex::Query_Ray`, which this pass did not verify is safe to call against a
`PhysicsSceneClass` that does not exist, and risking a self-check crash to find out was judged
worse than leaving it named here as untested. The visual result of any of this -- what the
strategic camera actually looks like -- needs a display, the same gap every rendering claim in
this project has.
