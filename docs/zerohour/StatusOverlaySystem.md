# Waypoint / status-circle renderers

Roadmap Section 32, Zero Hour / SAGE feature 17. Donors: `W3DStatusCircle`, `W3dWaypointBuffer`.

`Code/ww3d2/statusoverlayrenderer.{h,cpp}` (the pool) and `Code/Combat/statusoverlaysystem.{h,cpp}`
(the Combat-layer owner that adds team ownership and shroud gating), wired into
`CombatManager::Think`/`CombatManager::Shutdown` (`Code/Combat/combat.cpp`) exactly where
`BulletManager::Update`/`DebrisManager::Update` are.

## One kind field, not eight classes

Selection, target, move point, defend area, repair target, patrol point, construction footprint
and building-placement-validity are the same geometry -- a ring at a world point -- with a
different colour and a different owner. `StatusOverlayKindType` is that list as one enum; there is
one pool, one draw path, one bounded ceiling (`OVERLAY_POOL_MAX` = 256), the same decision
`WorldSurfaceMarkManager` (Section 35) already made for scorch/dirt/oil/construction marks.

## Two layers, split where the roadmap's own sentence splits them

*"These visuals must be reusable by Commander Mode but are not permission/gameplay logic
themselves."* `StatusOverlayRenderer` (`Code/ww3d2`) is the "not gameplay logic" half: it has no
team field and no shroud query anywhere in it, because `ww3d2` sits below `wwphys` in this
codebase (nothing under `Code/ww3d2` includes a `World*Manager` header) and cannot reach
`WorldShroudManager` even if it wanted to. It is a handle-addressed pool -- add, update, remove,
or let a positive lifetime expire it -- aged by one clock and drawn as batched line segments
through `WorldParticleManager`'s existing streak pool, `PARTICLE_POOL_STREAK`, the same one
`TracerRenderer` (Section 27) and the weather layer (Section 36) already share. A ring becomes
twelve short segments; N rings in one frame is still one `Submit` call, which is what "large
numbers of markers are batched" means as arithmetic rather than as a claim.

`StatusOverlaySystem` (`Code/Combat`) is the "reusable by Commander Mode" half and the one that
owns *who*: a ring's team, and the policy that decides whether a given viewer may see a ring that
belongs to someone else. `StatusOverlaySystem::Update`, called once a Combat tick, ages the pool
and then calls `StatusOverlayRenderer::Set_Visible` for every live handle against the current
viewer's team (`COMBAT_STAR`) -- the pool obeys the flag, it does not compute it.

## The visibility rule

A ring with no owner (`team < 0` -- an unowned construction footprint or placement-validity
marker, never someone's secret) or a ring owned by the viewer's own team is always visible. A ring
owned by a different team is visible only where `WorldShroudManager::Get_Shroud_State(viewing_team,
position)` is `SHROUD_VISIBLE` or `SHROUD_RADAR_DETECTED` -- the same two states Section 31's
tactical model treats as "there is something real to report here." This is exposed as a pure
function, `StatusOverlaySystem::Is_Overlay_Visible_To(handle, viewing_team)`, so it can be checked
without a scene, a device, or `COMBAT_STAR` existing at all, and it is the same function
`Update()` calls -- there is exactly one place this policy is written.

## Lazy scene object

Same shape as `TracerSystemClass` (`Code/Combat/bullet.cpp`, Section 27): a private
`RenderObjClass` subclass declaring `IRenderComponent` (Section 29), created and added to
`COMBAT_SCENE` the first time `StatusOverlaySystem::Add_Overlay` is called, never before. A
session that never creates a status ring pays nothing for this system beyond the static pool
arrays, which is the same "lazily created, costs nothing until first use" discipline P26's
`CompositeAttachmentClass` established.

## Checks

`renegade -shaderselfcheck overlays`, registered as ctest `shader_overlays` and
`fds_shader_overlays`. Device-less, same construction as the tracer check (a bare `CameraClass`/
`RenderInfoClass` need no device): pins persistent-vs-timed lifetime, `Set_Visible` suppressing a
ring's contribution to the draw, add/remove bookkeeping, twenty rings drawing 240 segments in
exactly one streak submission, and the pool refusing past its bound rather than growing. The
visibility policy is checked as a state change, not asserted: an unowned handle and a handle
`StatusOverlaySystem` was never told about are both always visible; a team-owned handle is hidden
from another team until that team is given a real `WorldShroudManager` sensor source reaching the
ring's position, and only then becomes visible -- `Set_Overlay_Team` is exposed separately from
`Add_Overlay` specifically so this scenario can be built without needing `COMBAT_SCENE`.

**Not covered by any automated check**: nothing calls `StatusOverlaySystem::Add_Overlay` for a
real gameplay reason yet -- unit selection, Commander orders and ghost-placement validity are all
later phases (Commander sidebar, ghost placement). The visual result -- whether a ring actually
looks like a ring on a real screen -- is unverified for the reason every HUD/world-overlay claim
in this project is: it needs a display.
