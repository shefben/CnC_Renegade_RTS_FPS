# DebrisRenderer / DebrisManager — destruction debris

Roadmap Section 28, Zero Hour / SAGE feature 13. Donor concept: `W3DDebrisDraw`.

`Code/ww3d2/debrisrenderer.{h,cpp}` (VISUAL_DEBRIS, the pooled render facility) and
`Code/Combat/debrissystem.{h,cpp}` (GAMEPLAY_DEBRIS and the glue that picks a tier), fired
from `BuildingGameObj::On_Destroyed` (`Code/Combat/building.cpp`) and from
`BridgeSystem`'s pending-debris queue, drained once a simulation frame by
`DebrisManager::Update`.

## What this replaces

Nothing existed. Unlike the tracer half of Section 27, there was no per-shot anti-pattern to
remove here -- buildings and bridges simply drew no debris at all when destroyed. The
acceptance is the same shape as every other Section 26-28 phase even so: *large destruction
events do not require every visible fragment to be a complete networked `GameObj`*, and the
roadmap asks for it before anything produces destruction events at scale, not after.

One real piece of dead plumbing did already exist: `BridgeSystem::Note_Debris` (P15-A) has
recorded what came off a broken span and where since before this phase, in a queue nothing
ever drained. `DebrisManager::Update` is that drain. See `docs/assets/BridgeSections.md`.

## Two tiers, because collision is not free

The roadmap names `GAMEPLAY_DEBRIS` and `VISUAL_DEBRIS` and says gameplay debris "may use
existing WWPhys objects when collision matters." That is permissive, not a requirement that
every piece collide -- and a `RigidBodyClass` is a full physics object with its own
integration, contact box and inertia tensor, not an array entry. Most of what a destroyed
building or bridge scatters is decoration nobody will ever stand on; a handful of pieces
where the shape matters can afford to be real.

### VISUAL_DEBRIS -- `DebrisRenderer`, `Code/ww3d2`

The cheap majority. A fixed pool of chips -- position, velocity, spin, colour, a spawn time
and a lifetime -- aged by one shared clock and submitted every frame to
`WorldParticleManager`'s **sprite** pool, the same one weather's snow draws through. No
collision: each chip is a simple ballistic arc (`Age` integrates a fixed gravity constant)
that fades out over its lifetime rather than testing the ground, because a ground-height
query is a wwphys concern and this layer may not depend on wwphys -- exactly the same
boundary that keeps `GAMEPLAY_DEBRIS` out of this file. A short-lived arc-and-fade chip
reads as debris without ever needing to know where the ground is.

`Age`/`Render` are the same two-call split every pooled renderer this project has built uses
(`TracerRenderer`, `WeatherEnvironmentRenderer`): `Age` moves the clock and the physics
regardless of whether the layer is on screen, `Render` only reads what `Age` last left.

Spin is real, not invented: `PointGroupClass`'s `Orientation` byte is a discretised 2D
rotation about a sprite's own centre (`Code/ww3d2/pointgr.h`), the same field weather leaves
at its default of no rotation. A debris chip's accumulated spin angle is encoded into it each
render, so a chip visibly tumbles as it falls -- at no extra cost, because the pool was
already submitting an orientation byte per particle for every stock quad emitter.

### GAMEPLAY_DEBRIS -- `DebrisManager`, `Code/Combat/debrissystem.h`

The small, real-collision minority. Each fragment is a `RigidBodyClass` -- the same class
every vehicle in the game already is -- built and added to `COMBAT_SCENE` the same way
`WeaponClass::Make_Shell_Eject` (`Code/Combat/weapons.cpp`) already adds a `ProjectileClass`
shell casing: constructed directly, given a model, a mass, a transform and a velocity, and
handed to `Add_Dynamic_Object`. It is not networked and it is not a `GameObj` -- every client
that reaches the same destruction event (a replicated health-to-zero, not a replicated
fragment) builds its own fragments locally, which is the acceptance's own wording taken
literally: no fragment's existence or position is ever put on the wire.

A fragment needs a named model to exist at all -- `RigidBodyClass::Get_Bounding_Box` and
`Get_Transform` both assert a model is set, and there is no sane "invisible but solid"
fallback for a physics object the way there is for a decoration. `DebrisManager::Spawn` tries
`WW3DAssetManager::Create_Render_Obj` for the caller's model name and, finding nothing, gives
that piece to the visual tier instead of refusing it outright -- the missing-art rule applied
to a case where the *engineering* fallback is a different tier, not a placeholder shape.

**A definition-lifetime trap worth writing down.** `PhysClass::Init` stores a raw pointer to
the `PhysDefClass` it is given (`Definition = &def;`), not a copy. Every other caller in this
codebase gets its definition from `DefinitionMgrClass::Find_Definition`, which lives in a
permanent registry; passing a stack-local definition would leave that pointer dangling the
moment `Init` returns. `_TheDebrisFragmentDef` (`Code/Combat/debrissystem.cpp`) is a
module-static `RigidBodyDefClass`, constructed once and never destroyed, precisely so every
fragment's `Init` call has something permanent to point at. Nothing is read from it --
`Set_Model`/`Set_Mass`/`Set_Collision_Group`/`Set_Transform`/`Set_Velocity` are all set on the
instance afterward -- it exists only to be a safe address.

**Ordering matters once.** `Set_Model` is what builds a `RigidBodyClass`'s collision box and
recomputes its inertia (`Update_Cached_Model_Parameters` -> `Compute_Inertia`), so it has to
run before `Set_Mass`, whose own recompute would otherwise measure an empty box.

Fragments are capped small (`GAMEPLAY_DEBRIS_MAX`, 32) and self-expire on a plain countdown
`DebrisManager::Update` ages every frame -- there is no `Lifetime` field on `RigidBodyClass`
the way there is on `ProjectileClass`, so the countdown and the removal
(`COMBAT_SCENE->Remove_Object` + one held reference released, the same single-owner
convention `BulletManager` keeps for its tracer scene object) both live in `DebrisManager`.

## `DebrisManager::Spawn` -- how a caller's count splits

One entry point takes a total count and how many of them should try the real tier:

```
Spawn(center, radius, total_count, gameplay_count, model_name, color, lifetime)
```

Up to `gameplay_count` fragments try `GAMEPLAY_DEBRIS` first, using `model_name` if it is
non-null. The first one that a missing model or a full gameplay pool turns down stops the
attempt -- neither reason gets better by retrying -- and everything left in `total_count`,
including every piece the gameplay tier already turned down, falls back to
`DebrisRenderer::Spawn` on the visual tier. The count a caller asked for is always what
appears; only whether it collides is ever in question.

## Consumers wired this pass

- **Bridges.** `BridgeDebrisClass` already carries a position, a piece count and a model name
  per break event (P15-A); `DebrisManager::Update` drains `BridgeSystem`'s pending queue
  every frame and clears it. All three default bridge kinds now name a rubble model and a
  count for their span's `BROKEN` state (`ow_bridge_concrete_rubble`, `_timber_rubble`,
  `_steel_truss_rubble` -- none of which exist yet; see `docs/assets/BridgeSections.md`), so
  the queue is live rather than merely wired. Every fragment tries the gameplay tier first,
  since a broken span's rubble landing on the roadway or the ground is exactly the case
  collision is for.
- **Buildings.** `BuildingGameObj::On_Destroyed` calls `DebrisManager::Spawn_Building_Debris`,
  scattering 14 visual-tier chips across the building's collected bounding box (`Get_Bounds`
  -- a building is an aggregate of static meshes with no model of its own, so there is no
  single model to measure). Buildings name no debris model of their own, so this is always
  the visual tier; nothing stops a later pass from adding one.

## What is not done here

- **Vehicles.** The roadmap names them as a consumer. `VehicleGameObj::Object_Expired` is
  server-only and drives a networked `ExplosionManager::Server_Explode` event; finding the
  right *client-side* "this vehicle is now a wreck" hook to scatter debris from needs more
  investigation than this pass gives it, rather than a guess dressed up as one. Left
  unstarted, recorded in `WIP_features.md`.
- **Future generated city props.** No such system exists yet to hook into.

## Console

- `debris_status` -- both tiers on one line: the visual pool's cap/alive/drawn/dropped, and
  the gameplay pool's cap/alive/dropped.
- `debris_budget [count]` -- the visual tier's cap. The gameplay tier's cap
  (`GAMEPLAY_DEBRIS_MAX`) is small enough on purpose that tuning it from the console has not
  come up.
- `debris_layer [on|off]` -- scatter debris in either tier, or do not.

## Checks

`renegade -shaderselfcheck debris`, registered as ctest `shader_debris` and
`fds_shader_debris`. Device-less, against `DebrisRenderer` directly -- the same layer the
tracer and weather checks exercise their own renderer at, **not** `DebrisManager`. The
gameplay tier cannot be exercised this way: `DebrisManager::Spawn` calls
`COMBAT_SCENE->Add_Render_Object`/`Add_Dynamic_Object` the moment it has anything to draw,
and `-shaderselfcheck` never calls `CombatManager::Init` -- there is no scene for it to touch.
What the visual-tier check pins:

- a chip inside its lifetime is alive and drawn;
- re-rendering the same clock draws the same count -- `Render` does not itself decide what is
  alive;
- a chip aged past its lifetime is reaped and stops drawing;
- ten chips scattered in one frame make one submission to the sprite pool, not ten;
- those ten are charged to the near-combat budget and never to the environment budget, which
  stays weather's alone even though weather's own snow shares the sprite pool;
- a pool capped at four holds at most four after six arrive, and reports exactly two dropped;
- a disabled layer accepts no scatter and draws nothing.

**Not covered by any automated check**, for the reason above: `DebrisManager::Spawn`'s
tier-routing arithmetic with a real model name, `Try_Spawn_Gameplay_Fragment`,
`Update_Gameplay_Fragments`'s countdown and removal, and `Drain_Bridge_Debris` actually
finding something in the queue. All of it compiles and follows verified WWPhys precedent
(`Make_Shell_Eject`'s own `Add_Dynamic_Object` call, `RigidBodyClass::Set_Model`'s
model-driven collision setup traced through `Code/wwphys/rbody.cpp`), but none of it has run.
