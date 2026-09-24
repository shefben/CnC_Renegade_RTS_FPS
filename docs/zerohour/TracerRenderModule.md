# TracerRenderer — the ballistic tracer / hitscan visual

Roadmap Section 27, Zero Hour / SAGE feature 12. Donor concept: `W3DTracerDraw`.

`Code/ww3d2/tracerrenderer.{h,cpp}`, fired from `WeaponClass::Fire_Bullet` in
`Code/Combat/weapons.cpp` through `BulletManager::Create_Tracer` in
`Code/Combat/bullet.cpp`.

## What this replaced

Every hitscan weapon whose ammo definition sets `DisplayLaser` used to draw a visible
line from its muzzle to wherever its raycast landed. The raycast is untouched -- same
collision group, same `COLLISION_TYPE_PROJECTILE`, same owner-ignore guard -- but what
happened with the result is what moved.

Stock allocated a fresh `TimedDecorationPhysClass` and a fresh `Line3DClass` **on every
shot**, added each pair to the scene as its own dynamic object, and let it sit there for a
flat 0.6 seconds before the scene's own lifetime mechanism tore it down. A firefight with
ten rifles going is ten allocations and ten scene insertions a second before anything else
in the frame is counted -- exactly the pattern the roadmap's acceptance names: *pooled
render modules without script-spawned effect objects for every tracer*.

## The idea

A tracer carries no simulation of its own. Its start, its end, its width and its colour
are all fixed at the instant `WeaponClass::Fire_Bullet` calls `Create_Tracer` -- the
raycast that decided where it ends has already run -- so a live tracer needs nothing but
those four things and when it dies. That is a fixed-size array, not an object.

`TracerRenderer` is that array: up to `TRACER_POOL_MAX` (512) records, aged by one shared
clock, submitted every frame to `WorldParticleManager`'s streak pool -- the same pool
Section 36's rain draws through. A rifle stops paying for a `RenderObjClass`; it pays for
one array write.

### Ageing and drawing are two different calls, on purpose

`Age(seconds)` advances the clock, reaps whatever just expired, and recomputes the
bounding volume from what survives. It is meant to run once a simulation frame regardless
of whether the tracer layer is currently on screen -- exactly the reason
`WeatherEnvironmentRenderer` separates `Update` from `Render`: a bounding volume the scene
culls by has to stay true while the object is off screen, or the next time it comes into
view its own culling test is stale.

`Render(rinfo, camera_position)` only reads what `Age` last left: it stages the tracers
still alive, frustum-rejects each at its midpoint, and submits. Calling it twice against
the same clock submits the same tracers -- nothing about drawing decides what is alive,
matching the same property the weather check pins for its own renderer.

### Direction of failure

A shot arriving when the pool is already at its cap (`Set_Max_Tracers`, default 256) is
**dropped, and counted** (`Get_Dropped_Count`) rather than displacing a tracer already on
screen. The alternative -- evicting the oldest live tracer to make room -- would mean a
tracer already being looked at popping out early to make way for a new one nobody has
seen yet, which is a worse-looking failure than one shot quietly not getting a line. This
is the same choice the weather layer makes for a column it cannot cast in time, applied to
a discrete pool instead of a lattice.

In practice the cap is never close: tracer lifetimes are a fraction of a second, so the
pool would have to be fed several thousand shots a second to fill it.

### Fade instead of pop

Stock cut a tracer at a hard 0.6-second boundary with no fade at all. `Render` scales
alpha linearly by the fraction of its lifetime remaining, so a tracer fades out instead of
vanishing -- a small, deliberate improvement licensed by "it can look better, it just
can't look worse."

## Draw

Every live tracer submits to `PARTICLE_POOL_STREAK` as a `LineGroupClass::PRISM`, exactly
the primitive weather's rain uses, with no texture -- a plain coloured prism, the same
class of image `Line3DClass`'s solid box drew, just camera-facing instead of world-fixed.
`width` keeps `Line3DClass`'s own meaning (full width in world units; the submission halves
it for the pool's radius convention), so a caller carrying the old constants over draws
the same size line.

The submission's `Budget` is `PARTICLE_BUDGET_AUTO`, resolved by `WorldParticleManager`
from the centroid of whatever was staged that frame -- the same near/far combat
classification any unlabelled emitter gets. Weather is the only user of
`PARTICLE_BUDGET_ENVIRONMENT`; tracers never touch it, and the check pins that the two
stay separate even though both draw through the one streak pool.

## The scene object

`TracerSystemClass` (`Code/Combat/bullet.cpp`) is a single `RenderObjClass`, created the
first time a shot asks for a tracer and never removed until `BulletManager::Shutdown`
-- lazily, for the same reason the beam manager's own pool of beams is lazy: the scene a
tracer would be added to is not guaranteed to exist yet at `BulletManager::Init` time.
Its `Render` just forwards into `TracerRenderer::Render`; it does not put itself on a
static sort list, because ordinary particle content does not either when full sorting is
enabled -- `ParticleBufferClass::Render` only reaches for one when `WW3D::Is_Sorting_Enabled()`
is false. A tracer is drawn immediately, through the pool's own per-primitive translucent
sort, the same path stock particle content already takes.

`BulletManager::Update` calls `TracerRenderer::Age` every simulation frame, next to the
beam manager's own lifecycle calls.

## What was already done, and left alone

**The laser/beam donor (`W3DLaserDraw`) is not this file.** `BeamEffectManagerClass`
(`Code/Combat/bullet.cpp`) already recycles its `SegmentedLineClass` /
`TimedDecorationPhysClass` pairs through a free list instead of allocating one per beam --
fixed in P03-Q, before this phase -- so it already satisfies "pooled... render module"
for the sense that matters: no allocation per shot. It was not folded into
`WorldParticleManager`'s streak pool, because a beam's noise, subdivision and UV-scroll are
per-instance geometry a shared batch submission has no field for; recycling the object was
the available economy, and it already has it.

## What is not done here

- **Projectile trail / stream** (`W3DProjectileStreamDraw`). `AmmoDefinitionClass::TrailEmitter`
  is a saved, loaded field with no reader anywhere in the engine -- a name a level or a
  weapon can already set that nothing acts on. A trailing stream behind a slow, visible
  projectile is a different shape of problem from a static tracer line: it has to sample a
  moving object's own path across several frames, not one instant's start and end. Left
  for its own pass rather than folded into this one; recorded in `WIP_features.md`.
- **Rope/cable strip.** The roadmap calls this in "where needed by a donor visual," and no
  donor visual inspected so far needs one. Not started.
- **Nothing has been fired on a screen.** The check runs without a device or a weapon.

## Console

- `tracer_status` -- layer on/off, the cap, how many are alive, how many drew last frame,
  how many shots the cap has turned away.
- `tracer_budget [count]` -- the cap itself.
- `tracer_layer [on|off]` -- draw tracers, or do not.

## Checks

`renegade -shaderselfcheck tracers`, registered as ctest `shader_tracers` and
`fds_shader_tracers`. Device-less, against `TracerRenderer` directly -- the same layer the
weather check exercises `WeatherEnvironmentRenderer` at, not the `BulletManager` call site.
What it pins:

- a tracer inside its lifetime is alive and drawn;
- re-rendering the same clock draws the same count -- `Render` does not itself decide what
  is alive;
- a tracer aged past its lifetime is reaped and stops drawing;
- ten tracers fired in one frame make one submission to the streak pool, not ten;
- those ten are charged to the near-combat budget and never to the environment budget,
  which stays weather's alone;
- a pool capped at four holds at most four after six arrive, and reports exactly two
  dropped;
- a disabled layer accepts no shots and draws nothing.

## Projectile trails (D-001 P24)

`AmmoDefinitionClass::TrailEmitter` is read now. Nine retail ammo definitions name one (rocket,
grenade and smoke trails, pulse rings -- enumerated from `objects.ddb`). A client starts the
emitter with the bullet, moves it with the projectile every think, and stops it at impact with
remove-on-complete, so the trail already in the air fades on its own. A dedicated server creates
none. Rope and cable strips stay deferred: nothing in the donor or the content asks for them.
