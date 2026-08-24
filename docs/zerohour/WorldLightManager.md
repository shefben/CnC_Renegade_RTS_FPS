# Dynamic-light filtering

Roadmap Section 25 (Zero Hour / SAGE feature 11).

Goal: *stop passing all dynamic lights to all renderable content.*

Acceptance: *lighting cost scales mainly with nearby/relevant lights instead of total world
light count.*

## What is here

| File | Holds |
|------|-------|
| `Code/WWPhys/worldlightmanager.h/.cpp` | `WorldLightManager` — the dynamic lights, the grid they live in, and the composition |
| `Code/WWPhys/pscene.cpp` | `Add_Dynamic_Light` / `Remove_Dynamic_Light` as forwarders; `Render_Object` composes; the frame and the tick |
| `Code/WWPhys/pscene_collision.cpp` | `Collect_Lights` finally means its `dynamic_lights` argument |
| `Code/Combat/worldspatialindex.h` | `Query_Nearby_Lights` is now both kinds of light |
| `Code/Commando/terrainselfcheck.cpp` | `Check_Lights`, run as `terrain_lights` and `fds_terrain_lights` |
| `Code/Commando/consolefunction.cpp` | `light_status`, `light_test`, `light_clear` |

## The dynamic half of the lighting system was a comment

It is worth being exact about what was wrong, because it was not slowness.

- `PhysicsSceneClass::Add_Dynamic_Light` was **declared and never defined**. Any caller would
  have been a link error, so there were no callers.
- `Collect_Lights` took a `bool dynamic_lights` and ignored it, under `// TODO: Dynamic
  lights!!`, twice.
- `Add_Collected_Lights_To_List` named its dynamic parameter out and finished with `// link the
  dynamic lights` / `// TODO!!`.

So Section 25's goal — stop passing all dynamic lights to all content — could not be met by
making an existing scan cheaper, because the scan had never been written. It is met instead by
building the thing that would have grown that scan, and building it filtered from the first
line: the only way to ask this system for lights is to ask it about a place.

**Static lights were never the problem, and this does not claim them.** They have lived in an
AABTree since Renegade shipped, an object collects only the ones whose volume reaches it, and
the result is cached on the object until something dirties it. That half already scaled with
what is nearby. It is left exactly as it was.

## Three gates, and the first one is free

| Gate | Cost | What it answers |
|---|---|---|
| The union bound of every dynamic light | three comparisons | can *any* light reach this place |
| The grid, expanded by the largest light | the cells within one light-radius | which lights are near enough to be worth testing |
| `Is_Within_Attenuation_Radius` | one distance test per candidate | which of those actually reach it |

The first gate is the one the acceptance rests on. `Render_Object` runs once per visible object
per frame, so a per-object light query is only affordable if the answer *no lights* costs
almost nothing — and with no dynamic lights in the world at all, which is most of the time in
most levels, it costs one box test and the object is handed back **the very same cached
lighting environment pointer it came in with.** Nothing is copied, nothing is allocated, and
the rendering path is byte-for-byte what it was before this existed.

The third gate is there because a grid query is a superset and never the answer — the same rule
`WorldSpatialIndex.md` states for every other query in the tree. Counting a light as *applied*
when it does not reach the object would make the numbers below lie.

## Where a composed lighting environment lives, and why that is the hard part

The obvious implementation — build the object's lights into a scratch environment and hand over
a pointer to it — is wrong here, and it is worth writing down why, because it is not visible
from the call site.

`MeshClass::Render` does this:

```cpp
WW3D::Add_To_Static_Sort_List(this, sort_level);
Set_Lighting_Environment(rinfo.light_environment);
```

It **keeps the pointer** and reads it later, when the static sort list is flushed at the end of
the frame. Today that is safe because the pointer is the object's own `StaticLightingCache`,
which is stable. A single shared scratch would mean every sorted mesh in the frame reading the
last object's lights.

So a composed environment has to stay alive for the whole frame. The choices were a second
cache on every `PhysClass` — which grows with the content, for a benefit only objects near a
light ever get — or a fixed pool recycled once a frame. It is the pool:
`WorldLightManager::Begin_Frame`, called from `Pre_Render_Processing`, resets the index; each
lit object takes the next one. Past the pool an object is drawn with its cached static
lighting, which is the picture it had before any of this existed rather than somebody else's,
and the overflow is counted.

## A light that expires

A muzzle flash lasts a twentieth of a second. `Create_Point_Light` takes a lifetime, and
`Timestep` — next to the shadow tick in `PhysicsSceneClass::Update` — fades and removes.

The fade is applied to **intensity, not to the attenuation range**, deliberately. The
attenuation range is the light's cull box, so fading the range would re-bucket a dying light in
the grid every frame for no visual difference. Fading intensity leaves the box alone for the
light's whole life.

## Bounds

| Bound | Value | What it stops |
|---|---|---|
| Dynamic lights | 64 | a light per effect per unit |
| Light radius | 60 m, clamped and counted | see below |
| Lights applied to one object | 8 | summing ambient from a crowd of lights |
| Objects given a composed environment per frame | 256, at most 1024 | a frame's worth of live pointers |
| Grid cells | 4096 | a large level costing megabytes of empty cells |

The radius bound is not a matter of taste, it is the grid's. A grid query expands its volume by
the largest object the grid holds, so a light allowed to be a kilometre across would make every
query sweep a kilometre and the filtering would filter nothing. Cells are one maximum radius
across, which caps a point query at three cells on each axis whatever size the world is.

## The acceptance, as a number

The claim is about scaling, so the check makes it a number rather than an argument.
`Check_Lights` plants eight lights two hundred metres apart, asks one question in the middle of
the field, and counts how many lights had to be examined to answer it. Then it fills the table
to all sixty-four and asks **exactly the same question again**.

```text
found with 8 lights   == 1
found with 64 lights  == 1
examined with 8       <= 4
examined with 64      <= examined with 8
```

If the second examination count were larger, lighting would be costing what exists rather than
what is near, and that is the failure Section 25 names. The check also asserts the free gate:
a point outside every light runs **zero** grid queries and is counted as a trivial reject.

`light_status` prints the same two numbers at runtime — lights in the world against lights
examined per query — so the property can be read in a real level rather than only in a test.

## The dedicated server needs no special case

Nothing here touches a graphics device. A dynamic light is a position, a colour, a radius and a
clock; it is created, it fades, it expires, and on a server nothing ever asks it to light
anything, so the composition never runs and the pool is never touched. The self check runs in
exactly that state, which is why it is the state it was written for.

## What was taken and what was not

**Taken — the shape of a SAGE dynamic light.** `W3DDynamicLight` is a light with a decay and a
lifetime that removes itself; that is the model adopted, with the fade moved onto intensity for
the re-bucketing reason above.

**Not taken — `PartitionManager`.** Section 16's rule stands: do not import SAGE's world
database as a second unrelated one. OpenW3D has culling systems, and the dynamic lights use
`TypedGridCullSystemClass`, the same primitive `DynamicProjectorCullingSystem` already uses.
The lights get their **own** culling system rather than joining the dynamic objects, because a
light query that had to walk every dynamic object and ask each one whether it happened to be a
light would scale with the content — which is the thing being fixed, wearing a different hat.

## What Section 25's list covers, and what it does not

Section 25 names the consumers to produce small light sets for. Seven of them arrive together,
because they are all `PhysClass` and they all render through `PhysicsSceneClass::Render_Object`:
**terrain chunks, foliage cells, bridges, roads, vehicles, infantry, buildings.** One
composition site serves all seven; there are not seven implementations and there is no per-system
lighting code.

Two are not covered, and both for the same honest reason — there is nothing there to filter:

- **Particles / effects.** `ParticleBufferClass::Render` never reads `rinfo.light_environment`;
  particles in this engine are unlit and additive. Filtering a light set for a consumer that
  does not consume one would be pretend work. If particles are ever lit, they light at their
  buffer's centre and `Apply_Dynamic_Lights` is the call.
- **Commander ghost building.** It does not exist yet. When it does it will be a `PhysClass`
  like everything else in the first list, and it will be lit by the same line of code.

## Not done

- **Nothing in gameplay creates a dynamic light yet.** The system exists, is bounded, is
  counted and is checked; the callers are the weapon, explosion and Commander-feedback work that
  arrives later. `light_test` is how one is made today.
- **A terrain chunk is lit at its centre**, like every other object, because that is what
  `LightEnvironmentClass` is: an approximation of the lighting *at a point*. A patch large
  enough for that to look wrong wants to be lit per-vertex or split, and both are decisions for
  the terrain-rendering work rather than for this.
- **Static lights are still the only ones with a PVS.** A dynamic light is not vis-culled,
  because it has no precomputed sector; it is culled by distance alone. That is correct rather
  than a gap — a light that moves cannot have a precomputed answer — but it means a dynamic
  light will light through a wall that a static light in the same place would not.
- **Nothing has been measured in a real level.** The scaling property is proved on a synthetic
  field; what a frame costs with a firefight's worth of lights in a real map wants a run, and
  `light_status` plus the `WWPROFILE("setup lights")` block already around the composition is
  where that is read.
- **No art gap.** A dynamic light is a colour and a radius; there is nothing for `docs/assets/`
  to list.
