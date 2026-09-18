# WorldParticleManager — particle batching

Roadmap Section 26, Zero Hour / SAGE feature 10. Donor concept: `W3DParticleSys`.

`Code/ww3d2/worldparticlemanager.{h,cpp}`.

## The cost this removes

Stock Renegade draws one emitter per draw call. `ParticleBufferClass::Render_Particles`
ends in `PointGroupClass::Render`, and that function sets a material, a shader and a
texture, locks a dynamic vertex buffer, fills it, and draws. All of that is per emitter.
A firefight with sixty explosions in it pays sixty render-state blocks, sixty
vertex-buffer locks and sixty draw calls to put up a few thousand quads that nearly all
share one texture.

The allocations are the same shape. A `ParticleBufferClass` constructor allocates on the
order of twenty arrays sized to its particle count — positions, diffuse, size, frame,
orientation, velocity, timestamps, the keyframe tables and the random tables — and frees
them when the effect dies. A firefight is therefore also an allocator workload, and both
halves are what Section 26's acceptance means by *draw calls and allocations growing
linearly with particle count*.

## What moves and what does not

The **simulation does not move.** `ParticleBufferClass` still ages particles, interpolates
its keyframes, applies its randomisers and decides what is alive. That is content
behaviour — it is what a stock `.w3d` emitter means — and a second implementation of it
would be a second way for stock content to look wrong.

What moves is the **submission**. A buffer hands its live particles to the manager instead
of drawing them. They are gathered into one of three fixed pools, and at flush each
distinct combination of texture, shader and point shape is drawn once. Draw calls stop
counting emitters and start counting materials.

## The three pools

Section 26 names sprite particles, point particles and streaks. They map onto the stock
render modes exactly:

| Pool | Render mode | Drawn by |
|---|---|---|
| `PARTICLE_POOL_SPRITE` | `W3D_EMITTER_RENDER_MODE_QUAD_PARTICLES` | `PointGroupClass`, `QUADS` |
| `PARTICLE_POOL_POINT` | `W3D_EMITTER_RENDER_MODE_TRI_PARTICLES` | `PointGroupClass`, `TRIS` |
| `PARTICLE_POOL_STREAK` | `LINEGRP_TETRA`, `LINEGRP_PRISM` | `LineGroupClass` |

They are three pools rather than one with a mode field because a streak needs a second
position and a second colour that a sprite has no use for.

**`W3D_EMITTER_RENDER_MODE_LINE` has no pool and is not batched.** It is a single
continuous polyline threaded through one emitter's particles in order, so two emitters'
particles cannot share one: concatenating them would draw a line joining two unrelated
effects. It stays one draw per emitter, deliberately.

## Per-particle data

Section 26 lists position, size, colour, alpha, angle and lifetime. The batch carries
position, size, colour and alpha (as one `Vector4` diffuse), and angle (the orientation
index). It does **not** carry lifetime, because the batch never simulates: lifetime is
what the buffer used to derive the size, colour and alpha it handed over, and it stays
with the simulation that reads it.

## The batch key

Two submissions are the same draw when all of these agree:

- the texture;
- the shader bits;
- the point shape (`QUADS`/`TRIS`, or `TETRAHEDRON`/`PRISM`);
- the frame row/column count (what a frame index means for that texture);
- **whether the primary gradient is disabled.**

The last one is not obviously render state. `PointGroupClass` and `LineGroupClass` both
decide, at draw time, to turn the primary gradient off for a submission that has no
diffuse array, a white opaque default colour and a texture — an optimisation for untinted
content. It is a shader bit, so merging a submission that gets it with one that does not
would change the image for one of them. It is in the key, and the manager computes it with
the same expression the two renderers use.

## Why the image does not change

The staging arrays always carry size, orientation and frame per particle, even for a
buffer that had none of those arrays. That is bit-identical to the constant-offset loops
in `PointGroupClass::Update_Arrays`: those loops index `_QuadVertexLocationOrientationTable`
and `_TriVertexLocationOrientationTable` with `DefaultPointOrientation` and multiply by
`DefaultPointSize`, and materialising an array of exactly those values makes the indexed
loop compute the same vertices. The same holds for the frame UV generation.

Draw **order** is preserved by where the flush happens. `WW3D::Render_And_Clear_Static_Sort_Lists`
walks the sort levels from back to front and already flushes `TheDX8MeshRenderer` at the
end of each one; the particle flush is placed immediately before that mesh flush, which is
where a particle buffer's own draw happened in the stock engine — inside its `Render`,
while the level's meshes were still queued. Merging is therefore confined to one sort
level, and nothing moves in front of or behind anything it was not already in front of or
behind. Particles that never went on a sort list — the path taken when the sorting
renderer is enabled — are flushed at the top of `WW3D::Flush`, before the mesh renderer,
for the same reason.

`particle_batching off` restores the stock path exactly: `Submit` refuses, and every buffer
draws itself.

## Budget classes

`CRITICAL_GAMEPLAY`, `COMBAT_NEAR`, `COMBAT_FAR`, `ENVIRONMENT`, `AMBIENT`, in that order,
which is also priority order.

A budget class answers one question: when more particles want to exist than a pool can
hold, whose disappear? Nothing in a `.w3d` emitter can say — the format has no field for it
and re-export is forbidden — so a buffer carries `PARTICLE_BUDGET_AUTO` until code says
otherwise, and AUTO is resolved at submission time to `COMBAT_NEAR` or `COMBAT_FAR` by the
distance from the camera to the effect. The same explosion is near when it is in the
player's face and far when it is across the map. `ParticleEmitterClass::Set_Particle_Budget`
is how anything else says something different; `ENVIRONMENT` is reserved for Section 36's
weather and `CRITICAL_GAMEPLAY` for effects a player reads as information.

### How the caps are decided

**Contention is measured, not predicted.** A submission arriving on its own is
indistinguishable from the first of five hundred, so any rule that rations at submission
time has to assume the worst and ends up thinning a pool that is nearly empty. Instead
each flush records what every class asked for, and that decides the caps the next
accumulation runs under:

- **If the last flush's total demand fit in the pool, there are no caps at all.** This is
  the ordinary case, and it is why a quiet scene is untouched.
- Under contention, each class is granted the smaller of what it asked for and its
  reserve, and the remainder is handed out in priority order to whoever asked for more.
- `CRITICAL_GAMEPLAY` is granted its whole reserve whether or not it asked for anything
  last time, because the case it exists for is exactly the one where it was not there a
  moment ago — a beacon is planted, and the plume that says so must not lose to smoke.
- A cap may raise a class above its reserve; **it never takes it below one.** Without that
  floor, a class absent from the previous flush would have a cap of nothing, and the first
  frame of a firefight arriving into a scene that had only been making smoke would show no
  firefight.

Default shares: `CRITICAL_GAMEPLAY` 0.25 (fixed — a guaranteed minimum that can be tuned
to nothing is not a guarantee), `COMBAT_NEAR` 0.35, `COMBAT_FAR` 0.15, `ENVIRONMENT` 0.15,
`AMBIENT` 0.10.

When a submission does not fit, it is **thinned, not truncated**: the kept particles are
spread evenly across the offered ones, so a plume gets sparser rather than shorter.

A submission that is accepted must not also be drawn by the caller, even when it was
thinned to nothing — drawing the remainder would cost the draw call the whole thing exists
to avoid. `Submit` returns false only when the manager is off or down.

## Bounds

Everything is fixed and allocated once.

| Bound | Value | What happens past it |
|---|---|---|
| Sprite pool | 8192 particles | submissions thinned by budget |
| Point pool | 4096 particles | " |
| Streak pool | 4096 particles | " |
| Batches per pool | 64 | further distinct materials refused and counted |
| Runs per pool | 512 | further submissions refused and counted |

The pool capacity **is** the budget; there is no second number that could disagree with it.
`particle_pool <name> <count>` resizes one, which reallocates its staging arrays.

The two renderers (`PointGroupClass`, `LineGroupClass`) are built on first flush rather
than at `Init`, because constructing either asks `VertexMaterialClass::Get_Preset` for a
material and that dereferences a null until a client has run `VertexMaterialClass::Init`.
A dedicated server never does. `Init` itself touches no device, so the pools, the batch
table and every piece of the arithmetic are valid on the FDS and in a device-less check.

## Console

- `particle_status [clear]` — emitters per draw call, per pool; the ratio is the acceptance.
  In the stock engine it is 1.00 by construction.
- `particle_batching [on|off]` — the A/B against the stock path.
- `particle_pool <sprite|point|streak> <particles>` — resize a pool.

## Checks

`renegade -shaderselfcheck particles`, registered as ctest `shader_particles` and
`fds_shader_particles`. Device-less: a flush with no device does every part of its work
except the draw, so the batch a submission lands in, the table its particles are gathered
through and the budget that paid for them are all decided and counted exactly as they
would be on a screen.

What it pins:

- 64 emitters sharing one material open **one** batch and draw as one draw call, with all
  512 particles drawn and none thinned;
- 32 emitters using four shaders open **four** batches — a state difference is a draw;
- offering a 1024 pool 4096 particles draws at most 1024, and drawn plus thinned equals
  offered, so nothing is dropped silently, and the pool does not grow;
- after a flush that saw four times the pool in `AMBIENT` demand, `CRITICAL_GAMEPLAY`
  still gets its full reserve;
- a submission naming no class is charged to `COMBAT_NEAR` or `COMBAT_FAR` by distance;
- a streak is charged to the streak pool and not the sprite pool;
- a disabled manager refuses, so the emitter draws itself;
- 256 distinct materials open 64 batches and refuse 192 submissions rather than growing.

## What is not done here

- Nothing yet sets a budget class other than AUTO. `CRITICAL_GAMEPLAY` and `ENVIRONMENT`
  have no callers until the Commander work (Sections 40–45) and Section 36's weather.
- No particle is lit; Section 25's `WorldLightManager` has nothing to filter for them.
- `W3D_EMITTER_RENDER_MODE_LINE` is unbatched by design, as above.
- Nothing has been drawn on a screen. The checks run without a device.
