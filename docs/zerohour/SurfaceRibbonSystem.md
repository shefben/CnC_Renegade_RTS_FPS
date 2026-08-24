# Terrain tracks / surface ribbons

Roadmap Section 23. Donor: Zero Hour/shared-Core `W3DTerrainTracks`.

Acceptance: *vehicles can leave stable marks without unbounded allocations or per-mark
heavyweight objects.*

## What is here

| File | Holds |
|------|-------|
| `Code/WWPhys/ribbontype.h/.cpp` | `RibbonCategoryType`, `Ribbon_Surface_Takes_Marks`, `SurfaceRibbonDefinitionClass`, `RibbonEdgeClass` |
| `Code/WWPhys/surfaceribbonsystem.h/.cpp` | `SurfaceRibbonClass`, `SurfaceRibbonSystem` |
| `Code/WWPhys/vehiclephys.h/.cpp` | `VehiclePhysClass::Update_Surface_Ribbons` — the emitter |
| `Code/WWPhys/trackedvehicle.h/.cpp` | `Get_Default_Ribbon_Category` — treads, not tires |
| `Code/WWPhys/pscene.cpp` | the per-frame tick, next to the camera shakers |
| `Code/WWPhys/wwphys.cpp` | `Init` — the one allocation |
| `Code/Commando/terrainselfcheck.cpp` | `Check_Ribbons`, run as `terrain_ribbons` and `fds_terrain_ribbons` |
| `Code/Commando/consolefunction.cpp` | `ribbon_texture`, `ribbon_status`, `ribbon_clear` |

## The acceptance is a claim about cost

Both halves of it — "without unbounded allocations" and "without per-mark heavyweight objects"
— are structural, so neither is answered by being careful. They are answered by there being
nowhere for the cost to go.

**The pool.** `SurfaceRibbonSystem::Init` allocates forty-eight `SurfaceRibbonClass` once and
never again. That is the only allocation the system makes in its whole life. `Bind` hands out a
slot; when every slot is taken it takes over the most faded *unbound* ribbon — one whose owner
is already gone and whose marks were about to vanish anyway — and only when every slot is still
bound to a living emitter does it refuse, return `-1`, and count the refusal. A map with two
hundred vehicles on it costs exactly what a map with forty-eight does, and `Get_Pool_Size()`
proves it by never moving.

**The edge.** A mark is a `RibbonEdgeClass`: two corners, a texture coordinate, an age and a
flag. It lives in a plain array inside its ribbon, forty-eight entries long, fixed at compile
time (`SURFACE_RIBBON_MAX_EDGES`). It is not a `RenderObjClass`, not a `PhysClass`, not
reference counted, not in the scene, not networked and not saved. Laying a mark writes into
memory that already exists.

**The draw.** One `DynamicMeshClass` per *kind* of mark, not per emitter and certainly not per
mark. Every tank track on the map is one thing to draw however many tanks are driving, because
they are all strips inside one mesh. `Get_Edge_Count()` against `Get_Object_Count()` is the
acceptance in two numbers, and `ribbon_status` prints them.

## The one place this differs from every other builder here

The road, the bridge, the wood and the water are built when they are placed and then left
alone, so each can afford to allocate a mesh of exactly the size it turned out to need — which
is what `DynamicMeshBuilderClass` is for, counting first and building second.

A track changes every frame. Building it that way would allocate a mesh per frame, which is
precisely the unbounded allocation the acceptance rules out. So the mesh for a kind is made
**once, at its largest** — every slot in the pool, full, laying this kind of mark — and refilled
in place from then on.

Refilling deliberately does not call `DynamicMeshClass::Reset`. That function throws away the
mesh's `MaterialInfoClass` and makes a new one, and the texture and vertex material would then
have to be looked up and re-added every frame with it — a per-frame allocation hiding inside a
function called "reset". Only `Reset_Mesh_Counters` and `Set_Dirty` are called; the shader, the
texture and the material are set once at creation and never touched again.

## Conform is not this system's problem, and that is the point

Section 23 asks marks to conform to terrain, road and bridge. The obvious reading is a ground
query per mark. The cheaper and more correct one is that **the caller already did it**: a
wheel's suspension has just cast a ray and found exactly where it touched, what the normal is
there, and what the surface is made of (`SuspensionElementClass::Get_Contact_Point`,
`Get_Contact_Normal`, `Get_Contact_Surface`). That point is on terrain, on a road, or on a
bridge deck without anybody having to ask which.

So `Add_Point` takes the contact and uses it. Laying a track costs one pass over the wheel
array and no new physics at all — no extra ray, no height sample, no guess about where the
vehicle probably is. A mark on a bridge is on the deck for the same reason the vehicle is.

`Conform_Point` exists for callers with no contact of their own — a scorch trail burned along
the floor, a body being dragged. It composes the two answers that already exist rather than
working either out again: `BridgeSystem::Conform_Point` first, because a mark on a bridge
belongs on the deck and not on the riverbed under it, then `RoadSystem::Conform_Point`, which
is heightfield terrain where a level has it and a ray through the physics scene where the
ground is authored W3D geometry instead.

## Where the ground says a track belongs

Renegade already tags every polygon with a `SURFACE_TYPE`, and stock levels have been tagged
that way since 2002. `Ribbon_Surface_Takes_Marks` reads it: sand, dirt, mud, grass, snow,
tiberium field, foliage and the two underwater dirt types keep a mark; metal, concrete, rock,
glass and everything else do not. No second authored map of where tracks are allowed, and every
existing level works without being touched.

Driving onto concrete does not end the ribbon — it ends the *run*. The marks behind stay and
fade; the next contact on soft ground starts a fresh strip. Otherwise crossing a road would
either erase the track behind it or stretch one triangle across the tarmac.

## Runs, and why one ribbon holds several

`RibbonEdgeClass::StartsRun` is how a single ribbon holds several unconnected strips. Three
different things produce a break, and all three are the same thing to the geometry:

- **Airborne.** No wheel on that side is touching, so `VehiclePhysClass::Update_Surface_Ribbons`
  calls `Break`. The tank lands and starts a new strip.
- **Hard ground.** `Add_Point` breaks the run itself, as above.
- **A teleport.** `Add_Point` measures the step. Further than `BreakDistance` segment lengths in
  one frame is not driving, so it breaks and restarts. **This needs nothing from the thing that
  moved** — no `Notify_Teleport` call to be forgotten at a new call site, no cooperation from
  respawn code that has never heard of ribbons. Section 23 asks for "teleport reset"; this is it
  as a distance test rather than as a protocol.

The alternative — a second ribbon per run — would spend the pool on a vehicle that bounced.

## Two ribbons per vehicle

Not one per wheel. A vehicle accumulates its contacting wheels into a left set and a right set,
averages each, and lays one ribbon from each. A six-wheeled truck therefore costs the same two
pool slots a jeep does, and a tank's two treads read as two treads.

Which side a wheel is on comes from the tank's own `LEFT_TRACK` / `RIGHT_TRACK` suspension flags
where they exist, and otherwise from which side of the hull the contact is on — `+Y` is left,
the same convention `TrackedVehicleClass::Render` already uses to drive its track mappers.

## Nothing is networked, deliberately

Section 23 asks for "network derivation from motion where possible". It is possible here, and
completely: every machine runs the same vehicle physics over the same replicated state, so
every machine's wheels report the same contacts and lay the same marks. Sending them would be
sending a function of something already sent.

The same reasoning makes the drawn objects `DONT_SAVE`. A saved game restores the vehicles, and
the vehicles lay their own tracks again.

## Fade

Two fades, and the smaller wins. Age against the kind's `Lifetime` is the real one. Position
along the ribbon is the donor's behaviour, and is what makes a moving vehicle look like it is
laying track rather than dragging a fixed-length strip: the newest `OpaqueEdges` are solid and
the rest of the tail ramps to nothing behind them. Both arrive as vertex alpha on an
alpha-blended, depth-write-disabled, unculled shader.

## What was taken from the donor and what was not

**Taken — the ring of edges with an anchor.** `TerrainTracksRenderObjClass` lays a cross edge
only when the owner has moved a segment length from the last one, holds them in a fixed ring,
fades the tail, and caps the run when the owner goes airborne. All four are here for the same
reasons.

**Taken — pooling with bind/unbind.** The donor keeps a used list and a free list of track
objects and reuses expired ones. Same idea, flatter: an array and an `Is_Free` test, because
forty-eight slots do not need two intrusive linked lists threaded through them.

**Not taken — one render object per emitter.** The donor makes a `RenderObjClass` per track and
flushes them all through a shared vertex buffer at draw time. That is one scene object per
vehicle, and a custom `Render` that reaches into DX8 buffers directly. Here there is one object
per *kind*, so the count is bounded by the number of kinds rather than by the number of
vehicles, and it is an ordinary `DynamicMeshClass` in an ordinary `DecorationPhysClass` — no
new render object class, no device resource to release and reacquire, and it culls, lights and
sorts like everything else in the scene.

**Not taken — the frame-count clock.** The donor measures lifetime in frames at an assumed
thirty per second (`FADE_TIME_FRAMES`). Lifetime here is seconds and ages by `dt`, so a mark
lasts as long on a fast machine as on a slow one.

**Not taken — `Drawable`.** The donor binds a track to a SAGE `Drawable`. Section 23's donor
notes and directive 0.4 both say not to import the SAGE object model; the owner here is an
opaque integer, which is all `Find_Bound` ever needed.

## Shared with the road, bridge, foliage and water systems

The definition-and-instance split (`SurfaceRibbonDefinitionClass` for the kind,
`SurfaceRibbonClass` for the placed thing) is the same shape as `WaterDefinitionClass` /
`WaterAreaClass` and `BridgeDefinitionClass` / `BridgeClass`. `Ribbon_Shader` is the water
shader with the same reasoning: a translucent surface with no edge to cut around, so blended
rather than alpha-tested. `mesh->Set_Collision_Type(0)` is the road's own line — a mark is
paint, and paint is not a floor.

## Not done

- **Nothing has been seen.** The checks run device-less and no definition names a texture, so
  `Build_Definition_Geometry` correctly declines before allocating anything. What a track looks
  like is `ribbon_texture` in a running client and then driving something.
- **No mark art exists.** Names and constraints are in
  [../assets/RibbonSurfaces.md](../assets/RibbonSurfaces.md).
- **Only vehicles lay marks.** Scorch trails and drag marks are defined, sized and queryable,
  and nothing calls `Bind` for them. A beam weapon or a dragged body would call the same three
  functions a vehicle does; that belongs with whatever adds those, not here.
- **The harvester lays a tire track.** `Set_Ribbon_Definition` exists so a harvester can name
  `ow_ribbon_harvester_track` instead, and nothing calls it — there is no harvester yet.
- **No LOD or distance cull on the marks themselves.** The whole mesh for a kind culls as one
  object. Marks half a map away are still in the buffer, which is bounded but not free; if that
  ever matters, the fix is a distance test in the fill loop, not a structural change.
- **Nothing reads the marks back.** A footprint the AI could follow, or a mark a weapon could
  react to, would need a query this does not have.
