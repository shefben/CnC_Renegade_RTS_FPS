# Particle batching

Roadmap Section 26, Zero Hour / SAGE feature 10. Donor: `W3DParticleSys`.

Owner: `WorldParticleBatchManager` (`Code/WWPhys/worldparticlebatchmanager.{h,cpp}`), with the
kinds, the budget classes and the shape of one particle in
`Code/WWPhys/particlebatchtype.{h,cpp}`.

## What is here

- one pool of 4096 particles for the whole world, allocated once;
- three shapes a particle can be drawn as -- sprite, point, streak -- sharing that pool;
- five budget classes, each with a reservation it can always have and a cap it may never
  exceed, and a deterministic policy for who loses a particle when the pool is full;
- one set of vertex buffers per texture group, allocated once at the group's own ceiling and
  refilled in place every frame;
- one render object per group, so the scene culls and draws a group rather than an effect;
- the `MATERIAL_PROGRAM_PARTICLE` pipeline of Section 15, registered and consumed;
- `particle_status`, `particle_test`, `particle_texture`, `particle_budget`, `particle_clear`
  at the console, and `terrain_particles` / `fds_terrain_particles` as checks.

## What stock Renegade pays, and per what

A Renegade particle effect is a `ParticleEmitterClass` render object that makes a
`ParticleBufferClass` render object. The buffer allocates, per emitter: a new-particle queue,
up to two position arrays, a diffuse array, colour, alpha, size, orientation and frame arrays,
and a keyframe and randomiser table for each of six animated properties. The scene then draws
it on its own.

That is a sensible design for a dozen authored effects in a level. The cost is per emitter,
and it is paid whether the emitter has four particles or four hundred: two hundred impacts in
a courtyard is two hundred render objects, two hundred buffers, two hundred allocation
storms and two hundred draw calls. Section 26 exists because an RTS-scale firefight produces
effects by the hundred.

## What this is not

**It is not a second implementation of the authored emitters.** A W3D emitter is content: its
keyframed colour, opacity, size, rotation, frames, line properties and randomisers are in the
asset, `ParticleBufferClass` is the one thing that knows how to play them, and Renegade
content has to keep loading unmodified. Nothing in this service touches that path, and
`particle_toggle` still disables exactly what it always did.

What this service is for is the particles the **engine** throws, where the caller has a
position, a velocity and an opinion about how much the effect is worth, and wants no asset
pipeline in between: weather (Section 36), tracers and beams (Section 27), debris (Section
28), construction dust and Commander feedback (Sections 44 and 42).

So there are deliberately two particle implementations in the tree, divided by what owns the
behaviour -- an asset, or a caller -- and not by which is newer. That is the one case the
no-duplicate-paths rule does not cover: these are not two implementations of the same thing
selected by a flag, they are two different things.

## The pool

One array of `BatchParticleClass`, 4096 entries, allocated in `Init` and never again. A
particle is a slot in it: no reference count, no virtual function, no per-particle object for
anything in the engine to hold a pointer to. Per particle the pool stores position, velocity,
colour, alpha, size, angle, age and lifetime -- which is exactly roadmap Section 26's list --
plus a per-spawn tint and size scale, the group it draws with, and the budget class it was
spawned into.

Free slots are a stack threaded through the pool itself (`ListNext` on a free slot is the next
free slot), so there is no second array and recycling a particle is two assignments.

Live slots are on a doubly linked list per budget class, appended at the tail. Every live
particle is aged by the same `dt`, so spawn order and age order are the same thing, and the
head of a class's list is its oldest particle. That is what makes "shed the oldest particle of
the least important class" a constant-time answer rather than a scan of four thousand entries
at exactly the moment the pool is busiest.

## The budget classes, and the headroom

| Class | Reserved | Cap |
|-------|---------:|----:|
| `CRITICAL_GAMEPLAY` | 512 | 4096 |
| `COMBAT_NEAR` | 1024 | 3072 |
| `COMBAT_FAR` | 768 | 2048 |
| `ENVIRONMENT` | 512 | 1536 |
| `AMBIENT` | 256 | 768 |

The reservations add up to 3072 of 4096 -- three quarters -- and the missing quarter is the
point. It belongs to nobody, anything may use it, and a class inside its reservation can never
be evicted to make room for anything. Reservations that added up to the whole pool would be a
policy with no give in it: every class permanently at its promise and the first unexpected
explosion refused.

`Get_Headroom()` is what is left of the pool. `Get_Class_Headroom(c)` is what that class could
still have: whichever runs out first, its own cap or the pool. `Set_Budget` retunes a class and
**refuses** a reservation that does not fit next to the others, rather than clamping it,
because a caller asking for the impossible wants to be told.

## The policy, in one place

`Allocate_Slot` asks three questions in order, and the order is the policy.

1. **Is the group at its own ceiling?** Then it recycles its own oldest particle. An emitter at
   its cap should go on looking alive without getting bigger, and taking the slot from
   somewhere else would let one effect's ceiling be paid for by another effect.
2. **Is the class at its cap?** Then it recycles the oldest particle of that class. Checked
   after the group's ceiling, so a definition with a small cap recycles within itself rather
   than being told its class is full.
3. **Is the pool full?** Then the least important class **strictly below** this one that is
   holding more than its reservation loses its oldest particle. If no class can give one up,
   the particle does not happen and is counted against its class -- a service that quietly
   drops work is worse than one that admits it.

An ambient ember can therefore never take a slot from a beacon's plume, a class on its
reservation is never touched, and the bottom of the list has nowhere to take from at all.

Lowering a cap below what a class already holds evicts nothing. The particles age out within
seconds, and tearing live particles out of the world because somebody typed a number is a
worse surprise than a budget that takes a moment to take effect.

## The groups, and what a submission costs

A group is a definition: one texture, one shape, one budget class, one set of curves. Up to 24
of them, up to 1024 particles each. Content names its own particle textures, so a group is
usually interned on demand by `Find_Or_Define_Texture_Group` from a texture name, a shape and
a budget class -- which is what makes two hundred unrelated effects that happen to share a
texture into one group.

The buffers for a group are made once, at the group's own ceiling, and refilled in place from
then on: positions, diffuse, size, and -- only for the shapes that have them -- orientation,
sprite-sheet frame, and the tail position and tail diffuse a streak needs. Building them to fit
would allocate every frame, which is the unbounded allocation the acceptance rules out;
building them at the pool size would allocate twenty-four times what any one texture can need.

`Build_Geometry` fills every group in **one** pass over the pool -- not a pass per group, which
would be the pool times the groups -- and then hands each group's arrays to its batcher and
counts what drawing them will cost:

- a sprite group is `PointGroupClass` in `QUADS` mode: four vertices a particle, and the
  dynamic vertex buffer takes 1200 vertices at a time, so **300 particles a submission**;
- a point group is `TRIS` mode: three vertices a particle, **400 a submission**;
- a streak group is `LineGroupClass`, which fills one index buffer and one vertex buffer for
  however many lines it holds and submits them once: **one submission**, whatever is in it.

So the cost of a frame is the particles the camera can see divided by three hundred, and not
the number of effects that produced them.

## The render path

Each group gets one `ParticleBatchBufferClass` -- a render object that carries a group index, a
bounding box and nothing else -- set as the model of a `DecorationPhysClass` added to the
physics scene as a dynamic object. The scene therefore culls and sorts a group the way it culls
and sorts anything else, and the group's bounds are the bounds of the particles the last fill
wrote, grown by each particle's own size because a sprite is drawn around its position rather
than at it.

`Render` makes the same decision `ParticleBufferClass` makes -- a translucent thing drawn in
the order the scene happened to reach it is a thing drawn through whatever was already in front
of it -- so it goes on the static sort list and draws when the list is flushed.

The draw itself sets `MATERIAL_PROGRAM_PARTICLE` and resets it afterwards. That program, in
`shadermgr.cpp`, owns the one piece of device state the batchers do not set: `PointGroupClass`
and `LineGroupClass` each set their own shader and their own texture stage and neither sets a
material, so before this a particle was lit by whatever material the last thing drawn happened
to leave behind -- the same explosion looking different depending on what was in front of it.
The program is emissive white with opacity from the vertex alpha, which is what a particle is.

It registers in `ShaderManagerClass::Init` rather than in the pool's own `Init`, because that
function rebuilds the registry from nothing every time the device comes up and a program
registered once elsewhere would be lost at the first device reset.

## No texture, no object

The two halves are separate on purpose. Filling a group's buffers needs no device, no asset
manager and no scene -- they are plain arrays handed to the engine's own batchers. Only
attaching the render object needs a world and a texture.

So a group whose definition names no texture still holds particles, still ages them, still
fills its buffers and is still counted; it simply never reaches a screen, which is what a
missing texture should cost. All six default kinds are in that state today
(`docs/assets/ParticleSprites.md`), and `particle_texture` points them at one that resolves.

It is also why the whole thing needs no dedicated-server special case: a server emits nothing,
so it allocates nothing, and every number the checks read is a number it would read too.

## The acceptance, as numbers

> Large firefights do not explode draw calls or allocations linearly with particle count.

`terrain_particles` (and `fds_terrain_particles`, the same check in the dedicated server) fills
two sprite groups with nine hundred particles each, one emission at a time the way a firefight
produces them, and reads back:

- 1800 particles live;
- **6** draw submissions -- 900 / 300, twice -- where one render object per effect would be
  1800;
- **2** buffer allocations, and the count does not move as the second nine hundred arrive;
- 3600 triangles for 1800 quads, and 1600 more for 400 streaks that add **one** submission.

Then it fills the pool to exactly 4096 out of four classes and reads the policy back out: a
critical particle into a full pool sheds the oldest ambient one and the pool stays exactly
full; shedding stops when the ambient class reaches its reservation and moves to the
environment class instead; an ambient particle into a full pool is refused and counted; a
reservation of the whole pool is refused; six particles into a group of four recycle twice
inside that group and take nothing from anyone else.

None of it needs a graphics device, a level or a physics scene.

## Console

| Command | What it does |
|---------|--------------|
| `particle_status` | Live, pool, headroom and peak; drawn, submissions, objects, culled; the per-class table of held / reserved / cap / headroom / refused; sheds, recycles, refusals. |
| `particle_test <count> [speed]` | A burst of each default kind thrown up around the camera. |
| `particle_texture <texture>` | Points every default kind at a texture that exists, so the system can be seen. No argument clears it again. |
| `particle_budget <class> <reserve> <cap>` | Retunes one budget class, by index: 0 critical, 4 ambient. No argument resets them all. |
| `particle_clear` | Removes every particle. The buffers are kept. |

`particle_toggle` is a different and older thing: it disables the authored W3D emitters.

## What Section 26's list covers, and what it does not

Covered: shared buffers and pools for sprite, point and streak particles; the per-particle
data (position, size, colour, alpha, angle, lifetime); all five budget classes.

Gravity, drag, spin and the sprite-sheet walk are not in Section 26's list and are here because
a particle that cannot fall or slow down is not a particle; they are two-ended curves on the
definition rather than the keyframes the authored emitters have, deliberately, because the
callers of this service are code rather than content.

## Not done

- **No gameplay caller yet.** Nothing in `Combat` emits into this pool. The callers it was
  built for are later phases: weather (Section 36), tracers and beams (Section 27), debris
  (Section 28), construction dust and Commander feedback (Sections 44 and 42). The authored
  impact and explosion emitters keep their own path, by the rule above.
- **Nothing has been drawn.** The checks run device-less and no default kind names a texture,
  so no batched particle has been on a screen. `particle_texture` plus `particle_test` is the
  manual check for that.
- **Particles are unlit**, which is what they were before this existed -- `PointGroupClass`
  reads no lighting environment -- and why Section 25's dynamic-light filtering deliberately
  does not cover them.
- **No per-particle collision.** A particle does not bounce off the terrain or stop at a wall.
  Doing it would cost a ray per particle per frame, which is the thing a budgeted pool exists
  to avoid; a caller that needs an effect to sit on the ground should place a surface mark.
- **The distance cut is flat.** A particle past `Get_Draw_Distance()` (220 m) is left out of
  the buffer entirely. There is no decimation in between of the kind `ParticleBufferClass` does
  with its LOD levels.
- **Nothing is networked or saved**, by design: a particle is a function of an event every
  machine already saw, and a loaded game has not been shot at yet.
