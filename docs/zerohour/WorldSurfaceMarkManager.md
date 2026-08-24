# Surface smudge / decal manager

Roadmap Section 35. Donor: Zero Hour `W3DSmudge` / `SmudgeManager`.

Acceptance: *hundreds or thousands of marks can exist without one draw call or one network
object per mark, and mark lifetime/eviction cannot grow memory without bound.*

## What is here

| File | Holds |
|------|-------|
| `Code/WWPhys/surfacemarktype.h/.cpp` | `SurfaceMarkType`, `SurfaceMarkDefinitionClass`, `SurfaceMarkClass`, the two bounds |
| `Code/WWPhys/worldsurfacemarkmanager.h/.cpp` | `WorldSurfaceMarkManager` — the service |
| `Code/WWPhys/pscene.h`, `pscene_decal.cpp` | `Set_Decal_Pool_Size` / `Get_Decal_Pool_Size`, and the note that `Create_Decal` is now a backend |
| `Code/WWPhys/pscene.cpp` | the per-frame tick, next to the ribbons |
| `Code/WWPhys/wwphys.cpp` | `Init` — the one allocation |
| `Code/Combat/explosion.cpp` | blast marks, rerouted |
| `Code/Combat/surfaceeffects.cpp` | impact marks and glass, rerouted |
| `Code/Commando/terrainselfcheck.cpp` | `Check_Marks`, run as `terrain_marks` and `fds_terrain_marks` |
| `Code/Commando/consolefunction.cpp` | `mark_texture`, `mark_test`, `mark_status`, `mark_clear` |

## What was already here, and why it was not enough

Renegade has had a decal system since 2002, and it is a good one: `PhysDecalSysClass` finds
every mesh overlapping a thin slab at the impact, clips the decal's polygons out of each mesh's
real geometry, and hangs the result off the mesh as a `DecalMeshClass`. That is the only way to
put a mark on a doorframe, a crate, a staircase or a vehicle and have it *follow the surface*
instead of hovering over it. Nothing in this section replaces that, and nothing should.

What it could not do is scale. Its recycling ring is **fifty** marks, and fifty is not
"hundreds or thousands". Raising the number does not help, because the cost per mark is a
geometry clip against every overlapping mesh and a vertex array to hold what came out —
precisely the per-mark heavyweight object the acceptance rules out.

So the answer is not to replace it but to stop it being the only option, and to put one owner
in front of both.

## One service, two backends, one database

`WorldSurfaceMarkManager::Create_Mark` is the single entry point. Everything in the game that
leaves a mark calls it, and it decides how the mark gets drawn:

- **The batch**, for a mark that lies on ground the engine can conform to. Four vertices in a
  mesh shared with every other mark of its kind. This is almost every mark the game makes, and
  it is what scales.
- **The projector**, for a mark that must clip into geometry: one that named a specific object,
  one on glass, one on a surface that is not facing upward, and one whose corners cannot agree
  on where the ground is. Bullet holes on walls, marks across a kerb, marks on a staircase
  nose.

Both are entries in the same fixed pool. That is the part that matters. Section 35's complaint
is not about rendering, it is about there being *"separate scorch, smudge, construction-mark
and terrain-track decal databases"* — several places each holding some marks, each with its own
idea of how many is too many. Here there is one array, one count, one eviction policy, and
`mark_status` prints the whole truth in three lines.

## The acceptance is two bounds and one number

**Bound one: the pool.** `Init` allocates 1024 `SurfaceMarkClass` once and never again. That is
the only allocation proportional to how many marks the world can hold. A mark is two vectors,
four corners, a clock and two integers — no reference count, no scene membership, no
`RenderObjClass`, nothing to destroy. Placing one writes into memory that already exists.

**Bound two: the group.** No definition may hold more than 256 marks, which is what lets that
definition's mesh be built once at its largest and refilled in place forever after. And no more
than 32 definitions may exist at all, which is what stops a level naming a thousand decal
textures from interning a thousand meshes. A refusal at either cap is counted, not silent.

**The number: draw calls.** One `DynamicMeshClass` per definition, not per mark. Five hundred
scorches sharing a texture are one object in the scene. `Get_Mark_Count()` against
`Get_Object_Count()` is the acceptance stated in two integers, and `mark_status` prints them
next to each other on purpose.

## The eviction policy

Deterministic, and stated in one place (`Allocate_Slot`):

1. If this mark's group is full, the group's own most-faded mark goes.
2. Otherwise, if there is a free slot, it is used.
3. Otherwise, the pool's most-faded mark goes.

"Most faded" is age over lifetime — the mark closest to vanishing anyway, and therefore the one
whose disappearance is least visible. A mark with no lifetime scores by age on a scale small
enough that *any* fading mark is taken before it, which is what makes a construction mark
outlive the scorches thrown on top of it without needing a priority field.

Clipped marks are ordered by age alone, because the projector draws them at full strength until
they are taken away and there is no fade to measure. They have a budget of their own — 96 — and
the manager evicts its own oldest before asking for another, so the expensive path cannot grow
into the cheap one's pool.

## Why the projector's ring is resized, and to what

The projector recycles its decals on a ring. If the manager holds a record of a decal the ring
has since reused, the manager would be pointing at somebody else's mark. So on every world
change the manager sizes that ring at **twice** its own clipped-mark budget. The manager is the
only thing that ever asks the projector for a decal, and it evicts its own oldest once it is at
budget, so the ring's index cannot lap a live record before the manager has released it. That
is an argument from the one caller, not a hope.

Stock Renegade left the ring at fifty. It is now 192, and the marks that would have overflowed
it are in the batch instead.

## Terrain projection, and where it stops

Section 35 asks for terrain projection with normal and slope handling. The four corners are
conformed **individually** (`Drape_Mark`), not the centre conformed and a flat quad laid on it,
so a mark on a hillside follows the hillside rather than burying one edge and floating the
other. The lift is straight up rather than along the normal, because each corner is already on
the ground it landed on and a per-corner normal would tilt neighbours away from each other.

Where a quad stops being the right answer, the drape says so. If any corner finds no ground, or
if the corners disagree by more than one and a half times the mark's radius, `Drape_Mark`
returns false and the mark goes to the projector to be clipped properly. That is the whole of
the "optional projection onto eligible static W3D surfaces where practical" requirement: the
practical test is performed rather than guessed at, per mark, at the moment it is placed.

`Conform_Point` is not reimplemented here. Bridge first, then road-or-terrain-or-ray, exactly
as the ribbons compose it — there is one implementation of "where is the ground" in the engine
and this is its second caller.

## Culling

The group is one object in the scene and culls through the spatial index like anything else.
Inside that, the refill loop drops marks further than `DrawDistance` (250 m by default) from
`PhysicsSceneClass::Get_Last_Camera_Position`, so a level with a thousand marks scattered over
it fills the buffer with the ones near the camera rather than all of them.

Region-level partitioning — a mesh per group *per cell* — was considered and rejected: it
trades the acceptance's own number back, multiplying draw calls by the number of occupied
cells to save vertex writes that are already bounded at 256 quads per group.

## Nothing is networked, and nothing is saved

Section 35 asks for client-only rendering unless a mark has explicit gameplay semantics. None
of them does. A mark is a function of an event — an explosion, a bullet, a building going up —
that every machine already received, so every machine places the same mark from the same state.
Sending it would be sending something already sent. The drawn objects are `DONT_SAVE` for the
same reason: a reloaded game has not been shot at yet.

## What was taken from the donor and what was not

**Taken — the pool with a free list and a used list.** `SmudgeManager` keeps `SmudgeSet`s of
recycled `Smudge` records rather than allocating per mark. Same idea, flatter: one array and an
`InUse` flag, because a thousand slots do not need two deques threaded through them.

**Taken — batched submission through one shared buffer.** The donor's whole point is that
smudges are drawn together. Here that is a `DynamicMeshClass` per definition rather than a
hand-rolled DX8 vertex buffer, so it culls, sorts and lights like everything else in the scene
and there is no device resource to release and reacquire.

**Not taken — the screen-space smudge effect.** `W3DSmudge` is a heat-haze: it samples the
frame buffer and re-inserts it displaced (`m_offset`, "difference in position between texture
extraction and re-insertion"), and `SmudgeManager` carries a `HardwareSmudgeSupport` flag
because the effect does not exist without the right hardware path. That is a distortion filter,
not a mark on the ground; roadmap Section 35's canonical design asks for *"batched persistent
or semi-persistent surface marks"*, which is the part of `W3DSmudge` worth having. Heat haze,
if it is ever wanted, belongs with the particle and weather layers.

**Not taken — `Smudge::Identifier` as a void pointer.** Handles here are a slot and a serial in
one `uint32`, so a handle to an evicted mark stops matching instead of addressing whatever took
its place. The donor's identifier is the address of the thing that made the smudge, which
answers a different question.

## Shared with the ribbons

The definition-and-instance split, `Mark_Shader` (the same blended, depth-write-disabled,
unculled shader), the build-once-refill-in-place mesh discipline, the deliberate avoidance of
`DynamicMeshClass::Reset` because it reallocates the material info every call, the texture
resolved before anything is built so a dedicated server allocates nothing, and the
`BuiltScene` check that drops geometry and marks when the world changes — all of it is the
ribbon system's, for the reasons written up in
[SurfaceRibbonSystem.md](SurfaceRibbonSystem.md). Section 35 explicitly permits Feature 9 and
Feature 22 to share allocation, material and batching infrastructure; this is that sharing,
arrived at by the second system using the first one's idioms rather than by a common base class
neither of them needs.

The two stay separate where they differ: a ribbon is a strip behind something that moved and
its geometry is a chain of edges; a mark is one event that stayed and its geometry is a quad.

## Not done

- **Nothing has been seen by anyone.** The checks run device-less. What a batched scorch looks
  like on real ground is `mark_texture` and `mark_test` in a running client.
- **The seven named kinds have no art.** Names and constraints are in
  [../assets/SurfaceMarks.md](../assets/SurfaceMarks.md). Content-named marks — which is most
  of what the game actually fires — draw with stock textures today.
- **Nothing places a `CONSTRUCTION` mark.** The type, the definition and the persistence rule
  exist; the deterministic construction sequence that would leave one is a later phase.
- **Nothing places an `OIL_OR_STAIN` or `DIRT` mark.** Same shape of gap: a damaged vehicle
  leaking, or a shell throwing earth, would call the same one function an explosion does.
- **No gameplay reads a mark back.** `Peek_Mark` exists and nothing asks it anything. An AI
  that could follow a scorch trail, or a weapon that reacted to burnt ground, would need a
  spatial query this does not have.
- **Marks do not stack or merge.** Two hundred scorches in the same crater are two hundred
  quads. Merging overlapping marks of the same group would save the buffer, and is only worth
  doing once something is measured saying it matters.
