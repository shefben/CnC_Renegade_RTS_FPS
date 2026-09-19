# WeatherEnvironmentRenderer — the weather layer

Roadmap Section 36, Zero Hour / SAGE feature 23. Donor concept: `W3DSnow`.

`Code/ww3d2/weatherenvironment.{h,cpp}`, driven by `WeatherMgrClass` in
`Code/Combat/WeatherMgr.{h,cpp}`.

## What this replaced, and why it is a replacement rather than an addition

Renegade already had weather. `WeatherMgrClass` ramps a density for rain, snow and ash, drives
a wind, sets the fog, saves and loads, replicates to clients, and is reachable from scripts,
the console and LevelEdit. **All of that is untouched.** Its public interface, its chunk IDs,
its network variables and its console commands (`rain`, `snow`, `ash`, `wind`, `fog`) are the
same ones, doing the same thing.

What it drove underneath was a per-particle simulation, and that is what is gone. Each of the
three precipitation types was a `RenderObjClass` owning:

- a linked list of `RayStruct` fall lines, each holding a raycast result, re-cast every time
  it drifted out of an emitter that follows the camera — which is continuously;
- a doubly linked list of pooled `ParticleStruct`s, spawned at a rate derived from the
  density, each carrying a position, a velocity, a collision time, a lifetime, a surface
  normal and a texture page;
- its own index buffer, its own dynamic vertex buffer and its own draw call.

Three of those existed at once, and the only bound on the whole thing was a shared
`_GlobalParticleCount` clamped at `USHRT_MAX / 6`.

Keeping that alongside a new renderer would have left two implementations of weather selected
by a flag, which the architecture rules forbid. So the simulation was deleted and the manager
was pointed at arithmetic instead.

## The idea worth porting

The donor's snow is not simulated. A flake's height is a function of which column it is in and
what time it is, so nothing has to remember it. Three facts about Renegade's own weather make
that description fit it exactly, which is why this is a rewrite of the *machinery* and not of
the image:

- **A weather particle has constant velocity.** There is no gravity term and a live particle
  is never re-steered, so its path from the emitter plane to the point its ray hit is a
  straight line at a constant speed — which is a clock.
- **Particles on one fall line are evenly spaced.** A freshly cast ray was filled in one go
  with `ParticlesPerUnitLength * length` particles. That is the same statement as *the n'th
  particle on this column is n/N of the way through its cycle*.
- **The ray, not the particle, is what touched the world.** Collision was already one raycast
  per fall line. It was never per particle.

So the only state weather needs is per column, and there is a bounded number of columns. What
used to be a particle is now an index.

## The model

A column is identified by a pair of integers on a world-anchored lattice. For each one:

| Quantity | Where it comes from |
|---|---|
| Landing point in XY | the lattice index, plus a hash-derived jitter inside the cell |
| Floor height and surface normal | one **vertical** raycast, cached (below) |
| How many particles it carries | `ParticlesPerUnitLength * fall length`, as stock |
| Phase | a hash of the index |
| How long a landed particle lingers | `Lerp(MinStatic, MaxStatic, hash)`, as stock |
| Which atlas frame each shows | a hash of the index and the particle number |

and for the n'th particle, at `t = wrap(clock + (phase + n/N) * cycle, cycle)`:

- while `t < fallTime` it is falling, at `z = ceiling - speed * t`, displaced **upwind** of its
  landing point by `wind * (fallTime - t)` and sideways by the mode's wander;
- afterwards it has landed, and sits at the floor point for the rest of the cycle, shrinking if
  the mode settles and showing the splash frame if the mode has one.

Nothing is allocated per particle, per frame or per emitter. Nothing is carried between frames:
rendering the same clock twice gives the same weather, which the check pins.

### Why the lattice is indexed by where a particle lands

This is the one real departure from both the donor and the stock system, and it is what makes
the raycast cache possible.

Stock seeded a fall line at a point on the emitter and cast a ray *along the velocity* to find
where it ended. The velocity contains the wind, and the wind changes every frame, so the answer
went stale constantly — which is exactly why the stock system re-cast so much.

Here the lattice is anchored at the **landing** point and the cast is **straight down**. Wind
then slides the *start* of the fall upwind and leaves the landing where it is, so a column's
floor is a fact about the world and not about today's weather. It can be cached until the
level changes. The image is the same one: a particle still arrives at the ground at the place
the ground actually is, having travelled a slanted path to get there.

### The cache

4096 columns, open-addressed, four-way probe, evicting whichever probed entry has gone longest
without being wanted. Walking away from a column and back again costs nothing.

Casting is rationed: `Set_Max_Casts_Per_Frame`, default 256. A column that cannot be cast this
frame is **left out of the frame entirely** rather than guessed at. That direction is chosen
deliberately — a guessed floor is rain falling through a roof, and a missing column is one
fall line's worth of thinner rain for a frame or two. Walking at speed exposes a few columns
per frame along one edge, so the allowance is not normally touched.

With no provider at all — a dedicated server, or the device-less check — nothing is under the
weather: every column falls the full depth of its box and nothing lands.

## The bound

This is the acceptance, and it is decided *before* any particle is evaluated.

Every spacing is `0.25 * 2^k` world units. The renderer starts from the spacing the density
asks for, quantises down to the lattice, and then **coarsens — raises k — until the estimated
count fits the frame's allowance**. The leftover density is made up by keeping only a fraction
of the lattice's columns, chosen by hashing the index so the choice is stable frame to frame.

The consequence is that weather over-asked-for gets *sparser*, never *truncated*: the scene
stays covered to its full extent and the fall lines move apart. The check tells the two apart
by the spacing.

The allowance is **static**, because it belongs to the frame and not to any one kind of
weather. Rain, snow and ash falling at once share one budget; the stock cap was effectively
per system.

`Set_Max_Particles`, default 4096, hard-limited by the staging arrays at 8192.

## Draw

Columns are evaluated into fixed staging arrays and handed to `WorldParticleManager`
(Section 26) charged to `PARTICLE_BUDGET_ENVIRONMENT` — the budget class that was reserved for
this work when the pools were built and that has had no caller until now. Everything past that
point is the particle manager's: which draw call this shares, whether the pool has room, and
what is thinned if it has not.

So all weather on screen is **one draw call per material**, where it used to be one per
precipitation type, and stock precipitation shares one atlas.

| Mode | Pool | Art |
|---|---|---|
| `rain` | streak | `ow_weather_rain.tga` (missing; draws untextured) |
| `snow` | sprite | `WeatherParticles.tga`, frames 8-11 |
| `ash` | sprite | `WeatherParticles.tga`, frames 12-15 |
| `dust` | sprite | `ow_weather_dust.tga` (missing) |
| `tiberium` | sprite, additive | `ow_weather_tiberium.tga` (missing) |
| `falling` | sprite | `ow_weather_falling.tga` (missing) |

Rain is a streak for two reasons: a raindrop is three times longer than it is wide and a sprite
has one size, and the stock atlas's rain band is two frame-grid rows tall and so cannot be
addressed as a frame at all. See `docs/assets/WeatherParticles.md`.

A missing texture is dropped rather than drawn: the asset manager answers a missing name with
an opaque placeholder, and a placeholder drawn several thousand times a frame is worse than no
texture. Untextured weather is a plain coloured particle.

## What is kept, exactly

- **Density** means what it meant — particles per square world unit — and arrives from the same
  ramped parameters. The lattice is derived from the stock formula with the emitter area
  divided out: columns per unit area is `density / (4 * ppul * speed)`.
- **Fall speeds, emitter sizes and heights, particle sizes and the static periods** are the
  arguments the stock `RainSystemClass`, `SnowSystemClass` and `AshSystemClass` passed to
  `WeatherSystemClass`, carried over one for one into the mode table.
- **The alpha fade** over the top 20% of the emitter height, so nothing appears out of nothing
  at the ceiling.
- **Landing behaviour**: rain shows its splash frame, snow and ash settle and shrink over one
  to two seconds.
- **The raycast**: terrain only, as a projectile — the stock decision about what counts as
  ground for a raindrop.
- **Sort level 31**, by keeping a `RenderObjClass` in the scene rather than calling the
  renderer from the game loop. That is where weather belongs in the draw order.
- **Per-particle frustum rejection**, as stock did it.

## What is not kept

- **Priming.** `_Prime` existed so weather turned on mid-level did not have to fall from the
  sky before it looked like weather; a freshly cast ray was filled with back-dated particles.
  Every fall line is full at every instant now, so there is nothing to prime and the flag is
  gone.
- **Camera-relative streak alignment.** Stock aligned a raindrop to its velocity *minus the
  camera's velocity*, so running sideways through rain slanted it. Rain is aligned to its own
  velocity here. This is a real, if small, loss and is recorded in `WIP_features.md`.
- **The `WEATHER_PARTICLE_SORT` switch**, which selected a sorted index buffer and was compiled
  to 0.

## Console

- `weather_status` — the bound, what was drawn against it, how far apart the fall lines ended
  up, and how much of the ground under them has been looked at.
- `weather_budget [particles] [casts]` — the bound itself.
- `weather_layer [on|off]` — draw the layer, or do not.

The stock `rain`, `snow`, `ash`, `wind` and `fog` commands are unchanged and are still the way
to make weather happen.

## Checks

`renegade -shaderselfcheck weather`, registered as ctest `shader_weather` and
`fds_shader_weather`. Device-less, with no collision provider. What it pins:

- weather falls at all, and the count is inside the bound;
- the same clock and density drawn twice give the identical count — there is no state to drift;
- doubling the density draws more particles while there is room;
- twenty times the density against a budget of 256 draws at most 256, **and the fall lines end
  up further apart than in an unbudgeted scene**, which is how coarsening is told from
  truncation;
- rain and snow in one frame come to the bound *between them*, and neither is starved;
- rain submits to the streak pool and snow to the sprite pool, one submission each;
- the particles are charged to `ENVIRONMENT` and not to a combat class;
- wind widens the bounding box, so the scene cannot cull away weather that is on screen;
- a disabled layer draws nothing.

## What is not done here

- **Nothing has been drawn on a screen.** The checks run without a device.
- `TIBERIUM_ATMOSPHERIC_PARTICLES`, `DUST` and `GENERIC_FALLING_PARTICLES` have no caller:
  `WeatherMgrClass` exposes rain, snow and ash because those are what its saved state, its
  network variables and LevelEdit know about. The three new modes are reachable only from code
  until something is given a way to ask for them.
- No weather is lit. Section 25's `WorldLightManager` has nothing to filter for it.
- Weather does not affect gameplay — no traction, no visibility, no sound but the stock rain
  loop.
