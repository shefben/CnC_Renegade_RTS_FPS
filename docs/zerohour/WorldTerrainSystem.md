# WorldTerrainSystem

Roadmap Section 17 (Zero Hour / SAGE Feature 4, hybrid heightmap / terrain framework).

`Code/WWPhys/heightfield.{h,cpp}`, `Code/WWPhys/worldterrainsystem.{h,cpp}`,
`Code/Commando/terrainselfcheck.{h,cpp}`.

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

## Not built yet, and saying so

`Build_Collision` and `Build_Far_Terrain_Representation` return false and log once.
They are declared because Section 17 names them and callers should be written against
the final shape, and they refuse rather than pretend because a terrain service that
silently returned success would be worse than one that is honestly incomplete.

- **Collision** needs runtime mesh building against WWPhys — a `PhysClass` holding
  terrain geometry, registered with the physics scene, rebuilt per dirty patch. Until
  it exists the heightfield answers queries but the physics scene cannot see the ground,
  so Section 17's acceptance line (*FPS and vehicle gameplay works on runtime-created
  heightfield terrain*) is **not** met.
- **Rendering** is the terrain pipelines enumerated in `ShaderManager.md`, waiting on
  the same phase.
- **The far/background layer** is Section 34's own phase (Feature 21,
  `W3DTerrainBackground`).
- **`Get_Material`** returns one material. Section 18 is the terrain texture system that
  will decide it from height, slope, curvature, biome, moisture, water distance and the
  masks; the question is asked in its final form now so callers written before then do
  not need rewriting after.

## Checks

`renegade -terrainselfcheck <sampling|rays|shaping|service>`, registered as
`terrain_sampling`, `terrain_rays`, `terrain_shaping`, `terrain_service` and their
`fds_` twins. Eight tests, all green.

They run before anything is initialised and need no device, no level and no physics
scene — it is a grid of numbers, a triangulation and some arithmetic. The ramp they
build (height equals x index, cell size one, a forty-five degree slope) has an answer
for every height, normal and slope that can be written down rather than measured.
