# WorldTerrainSystem

Roadmap Section 17 (Zero Hour / SAGE Feature 4, hybrid heightmap / terrain framework).

`Code/WWPhys/heightfield.{h,cpp}`, `Code/WWPhys/worldterrainsystem.{h,cpp}`,
`Code/Commando/terrainselfcheck.{h,cpp}`, and the `terrain_test` / `terrain_clear` console
commands in `Code/Commando/consolefunction.cpp`.

## Shape

Two classes, and the split is the point.

`HeightfieldClass` is the data: a regular grid of heights with an origin and a cell
size, divided into patches, and a triangulation. It knows nothing about the world.

`WorldTerrainSystem` is the service Section 17 asks for. It owns exactly one
heightfield, and nothing else in the program owns one. Section 17's requirement that
collision, placement, roads, bridges, water, shroud, foliage and generated terrain all
refer to the same canonical world coordinates and the same source data is kept by there
being only one copy to refer to.

The donor's split — `BaseHeightMap` for ownership and lifetime, `HeightMap` and
`FlatHeightMap` as rendering strategies, `TerrainVisual` / `TerrainLogic` as drawing
versus asking — maps onto this as: the service is the ownership, the query API is the
logic half, and the render LOD selection is where the strategies will hang. Section 17
says not to recreate several independent terrain engines, and there is one here.

## Terrain is additive

`Has_Terrain()` is false for every Renegade level that exists, and on those every query
returns false and writes nothing. Callers fall through to the W3D geometry they already
used. Section 17's world model is heightfield terrain **and** arbitrary static W3D
meshes, interiors, tunnels and overhangs — the heightfield is one thing in the world,
not the world. A query that answered `0.0` for a level with no terrain would put an
invisible floor under the whole game, which is why "off the field" and "at height zero"
are different answers everywhere in this code.

## The triangulation is a contract

A cell is two triangles split along the diagonal from its low corner to its high
corner, and `Get_Cell_Triangles` is where anything that turns height data into geometry
reads it — a renderer, a collision mesh, a ray.

Sampling is planar within a triangle, not bilinear. A bilinear surface is not made of
triangles: sample it and collide against a triangle mesh built from the same grid and
the two disagree by centimetres, which is a soldier standing off the ground he is drawn
on. Section 17 requires that rendered terrain and terrain collision derive from the same
source data, and the way that is made true is that both ask this class and get the same
arithmetic.

`terrain_rays` is the check that holds it: for a grid of points, cast straight down and
compare the hit against `Sample_Height` at the same x and y. If those ever part company
the phase's central promise is broken, and it is broken silently.

## Heights come from memory

There is no file format and no loader. Section 17 requires that the service accept
height data from a generated source, and the simplest way to be certain it can is to
have no other kind. `Create_Terrain` allocates, `Set_Heights` fills from a row-major
array, and `Modify_Height_Region` / `Blend_Height_Region` are the primitives everything
else is built from.

`Apply_Road_Grade` and `Apply_River_Cut` are polylines walked with those primitives at
half-width steps, so the result is a continuous strip rather than a row of dishes. They
live on the terrain service rather than in the road and water systems because the ground
has one owner and they ask it to change. Grading blends rather than stamps — a road that
sets its height leaves a wall where it crosses a slope.

## Patches

A patch is `PATCH_CELLS` (16) cells square and is the unit of rebuild: renderers and
collision meshes are not rebuilt a vertex at a time. Each carries its min and max height,
which is what lets a ray skip it and what a far representation will sample.

A vertex on a patch seam belongs to the patches on both sides, and moving it moves the
edge of each. `Invalidate_Patches_Touching` dirties both, because dirtying only the one
the index divides into is how a crack appears down the middle of a level.

## Queries

```text
Sample_Height   Sample_Normal   Sample_Slope   Ray_Intersect_Terrain
Get_Material    Get_Bounds      Get_Patch      Invalidate_Patch
Modify_Height_Region   Apply_Road_Grade   Apply_River_Cut
Query_Placement_Surface
Select_Render_LOD      Get_Visible_Terrain_Patches
```

`Query_Placement_Surface` takes nine samples — centre, axes, diagonals — and reports the
height range, the worst slope and whether all of it was on terrain. A footprint measured
only at its middle is how a building ends up with one corner in the air; a full sweep of
every vertex under it costs more than a placement preview following a cursor can spend.

`Select_Render_LOD` measures distance in patch widths rather than metres, so a field of
half-metre cells and a field of five-metre cells do not need different constants to look
the same.

## Collision: the engine's own terrain patch

`Build_Collision` turns the heightfield into geometry the physics scene can see. One
`RenegadeTerrainPatchClass` per heightfield patch, each wrapped in a `StaticPhysClass` and
handed to `PhysicsSceneClass::Add_Static_Object`.

That render object is not new and not mine. It is stock Westwood — Patrick Smith, 2002 — and
it is what the Renegade level editor has always built terrain out of. Using it rather than
writing a terrain mesh type is the whole safety of this slice, because of one line in its
`Collide_Quad`:

```cpp
tri1.V[0] = &Grid[v0_index];   // (x,   y  )
tri1.V[1] = &Grid[v2_index];   // (x+1, y+1)
tri1.V[2] = &Grid[v3_index];   // (x,   y+1)
tri2.V[0] = &Grid[v2_index];
tri2.V[1] = &Grid[v0_index];
tri2.V[2] = &Grid[v1_index];   // (x+1, y  )
```

It splits a cell along the diagonal from the low corner to the high corner — the same diagonal
`HeightfieldClass::Get_Cell_Triangles` uses, chosen independently and before this class was
found. So Section 17's requirement that rendered terrain and terrain collision derive from the
same source data is not kept by two systems being careful with each other; it is kept by there
being one grid, one triangulation, and one object that is both the collision geometry and the
render object.

It is also not a second implementation in the sense directive 0.4 forbids. There is no
alternative terrain mesh type to select between: the service owns the only heightfield, and the
patches are cut from it. And because the class itself is untouched, a level that already
contains saved terrain patches loads exactly as it did before.

### Lifecycle

```text
Build_Collision     create what is missing, refill what is dirty
Update_Collision    refill what is dirty, create nothing
Destroy_Collision   remove from the scene and release
```

`Update_Collision` refuses to create on purpose. Something shaping the ground every frame wants
the patches it moved rebuilt; it does not want a world built underneath it because it called
the wrong function too early. `Destroy_Terrain` destroys collision first, while the field the
patches were cut from is still there to describe them.

A patch's height range is taken from the field's own patch extents rather than left to grow
from the vertices, because `Set_Vertex_Pos` only ever raises the ceiling — a patch refilled
after the ground was lowered would keep the old one forever.

The dirty bit has exactly one consumer, which is why clearing it here is safe: the collision
object and the render object are the same object. If that ever stops being true, the bit has to
become one bit per consumer, and it will not announce itself.

### What it costs, and what has not been tried

- A static object added at runtime lands in whichever `AABTreeCullClass` node contains it, and
  when none does, in the root — where it is tested against every query. Correct, and not
  optimal. The tree is built for a level's shipped geometry and knows nothing about ground that
  did not exist when it was built.
- Patches added at runtime have no vis sector id, so nothing here has been tried against a
  vis-solved level. Collision does not go through vis; drawing does.
- Nothing has been measured.

### Standing on it

`terrain_test <cells> <cell_size> <amplitude>` lays a rolling heightfield down centred on the
player, in whatever level is loaded, and builds its collision. `terrain_clear` takes it away.
Rolling rather than flat on purpose: a plane collides correctly even when the triangulation is
wrong, so it would prove nothing.

This is what Section 17's acceptance line — *FPS and vehicle gameplay works on runtime-created
heightfield terrain while arbitrary W3D geometry remains supported* — has to be judged against,
and judging it needs someone to walk on it.

## Not built yet, and saying so

- **Drawing it.** The patch now takes material passes from Section 18
  (`TerrainTextureSystem.md`) whenever layers are defined, so `Build_Collision` dresses what it
  builds. What is still missing is art: the default layers carry rules and no texture names, so
  a pass has nothing to put on the ground. Nothing here has been seen on a screen.
- **`Build_Far_Terrain_Representation`** still returns false and logs once. It is declared
  because Section 17 names it and callers should be written against the final shape, and it
  refuses rather than pretends because a service that silently returned success would be worse
  than one that is honestly incomplete. It is Section 34's own phase (Feature 21,
  `W3DTerrainBackground`).
- **`Get_Material`** now asks Section 18's layer table, which landed: see
  `TerrainTextureSystem.md`. The placeholder enum is gone and the answer is a layer index, with
  `Get_Surface_Type` beside it for the stock surface type footsteps and decals already read.
- **`Build_Collision` with no physics scene** returns false and says so once. That is not a
  refusal to implement, it is the honest answer: `PhysClass::Set_Model` asks the scene singleton
  whether it already holds the object, so there is nothing to build into. A check that runs
  before the game is initialised gets that answer, and so would a tool that never made a scene.

## Checks

`renegade -terrainselfcheck <sampling|rays|shaping|service|collision>`, registered as
`terrain_sampling`, `terrain_rays`, `terrain_shaping`, `terrain_service`,
`terrain_collision` and their `fds_` twins. Ten tests, all green.

`terrain_collision` is the one that holds the diagonal contract across the boundary between the
two classes: it fills a patch from a field with a different height at every corner of every
cell, drops rays on the patch's own collision, and requires each hit to agree with
`Sample_Height` at the same point. A flat field cannot catch a wrong diagonal — both diagonals
of a planar quad give the same answer everywhere — which is why the ramp is not used there. It
also casts down the seam between two patches from either side and requires the shared grid line
to be in the same place in both, since a disagreement there is a strip of level you fall
through.

They run before anything is initialised and need no device, no level and no physics
scene — it is a grid of numbers, a triangulation and some arithmetic. The ramp they
build (height equals x index, cell size one, a forty-five degree slope) has an answer
for every height, normal and slope that can be written down rather than measured.
