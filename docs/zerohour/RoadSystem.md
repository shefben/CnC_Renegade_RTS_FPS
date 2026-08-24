# RoadSystem

Roadmap Section 19 (Zero Hour / SAGE Feature 3, runtime road system).

`Code/WWPhys/roadspline.{h,cpp}`, `Code/WWPhys/roadsystem.{h,cpp}`, the `roads` mode of
`Code/Commando/terrainselfcheck.cpp`, and the `road_test` / `road_clear` console commands.

## What is authored is a line

Section 19 gives the representation outright: control points, a width, a shoulder width, a
material, a road class, a grade policy, and connections to bridges and tunnels. That is the whole
of what anybody writes down. The centre line between the control points, the ribbon either side
of it, the height it sits at, the texture coordinates along it, the polygon where it meets
another road and the entry in the road mask are all derived, which is what the acceptance line —
*road geometry can be generated at runtime from centre lines without a unique modelled mesh for
every road segment* — actually asks for.

The curve is Catmull-Rom because it passes **through** its control points. A road is drawn by
saying where it goes, and a curve that merely approaches the places it was told about would put
the road somewhere nobody asked for. The case that makes this non-negotiable is the bridge: the
road has to arrive at the abutment, not near it.

## Two reasons to split a segment

Tessellation is adaptive on chord deviation — how far the straight line between two output points
strays from the curve. That is what spends points on a bend and nothing on a straight, and it is
why a long straight road is cheap.

But a straight still gets points, forced by a maximum segment length. A dead straight road over a
hill is exactly the case where the centre line is right and the ground under it is not, and there
is nothing to follow the ground with between two points fifty metres apart. Deviation shapes the
road in plan; segment length is what lets it sit on anything.

## Two kinds of ground

`Conform_Point` asks the terrain service first and drops a ray through the physics scene when
there is no heightfield. Both matter. Generated terrain is Section 17's world; every stock
Renegade level is the other one, and a road laid across a level's own W3D floor has to work or
the feature only exists on maps that do not exist yet.

Neither answering is also an answer: a road built by a tool, or by a check with no world around
it, keeps the height it was authored with rather than being dropped to zero.

## Grading, and why the road is smoother than the ground

A road with `ROAD_GRADE_NONE` takes the ground's height exactly and changes nothing. That is the
correct default and the only safe policy on authored geometry.

A road that grades takes the ground's height **smoothed along its own length** — averaged over a
fixed window in metres, so the smoothing is the same on a straight, where points are far apart,
as on a bend, where they are close together — and the terrain is then brought up to that. Doing
it this way rather than from authored heights means a road drawn as a flat line on a map still
follows the country, and a road drawn across a dip still fills the dip. The width graded is the
carriageway plus both shoulders, because flattening only the carriageway leaves a step at its
edge.

Grading moves the ground, so anything that was sitting on the ground is now wrong: roads that do
not grade are conformed again afterwards, and terrain collision is refreshed. The check asserts
the outcome directly — after grading, the road rises and falls less than half as much as the
country it crosses, and the ground is within a fraction of a metre of it.

## A junction is a polygon, not a part

The donor draws junctions from an atlas of authored corner pieces — tee, Y, four-way, two
flavours of H, an alpha join — chosen by shape and rotated to fit. That is a modelled mesh per
junction kind, which is the thing this section's acceptance rules out, and it is authored against
one particular road-texture layout that we do not have.

Here a junction owns a disc of ground. Every road that reaches it stops its own surface at the
edge of the disc, and the polygon that fills the middle is built from the ends of the roads that
arrive: each road contributes the two corners of its ribbon where it crosses the boundary, the
corners are sorted by the angle they sit at around the centre, and a fan through the middle fills
both the arms and the gaps between them.

Three consequences worth stating:

- a road that passes straight through is **not** split into two roads. Its surface simply has a
  hole in it. So a crossroads costs what a tee costs and nothing has to decide which road is the
  one that ends;
- any number of arms at any angle works, because nothing in it is a case;
- the ribbon and the polygon meet exactly, because the boundary point is computed once and used
  by both. The crossing is found by bisecting against the same radius test that decides whether a
  point is inside — a separately derived root can land on the other side of it.

## Shoulders are terrain, not more ribbon

`shoulderWidth` could have been a second strip of geometry. It is better spent on the ground.

The road mask is stamped at the carriageway width and feathered across the shoulder, so the
terrain texture system's `road` layer covers the carriageway and fades out across the verge. The
road surface then meets the ground through a band of worn dirt instead of an edge. Section 18
built that mask and left it with nothing to write into it; this is the writer it was waiting for.

It also settles what a soldier is standing on. `WorldTerrainSystem::Get_Surface_Type` reads the
dominant terrain layer, so once the mask is stamped, footsteps on a road sound like a road, and
no separate notion of road-ness has to be consulted.

## Roads do not collide

The donor's roads are decoration over a heightfield that owns the collision. That is right here
too, and for a stronger reason: Section 17 requires terrain collision and terrain rendering to
derive from the same source data, and a road with its own collision hull would be a second
surface a soldier could stand on, sitting eight centimetres above the first. A road is a thing
you see on the ground, not a thing you stand on instead of the ground. The mesh sets its
collision type to nothing; the physics scene still culls and draws it.

The eight centimetres are not zero on purpose. Coplanar surfaces fight for depth and the winner
changes with the camera.

## Material batching

One mesh per material across all the roads that use it, not one per road and not one per segment.
A junction goes into the batch of its first arm: two roads of different materials crossing have
to make one polygon in one of them, and the alternative — two coplanar polygons, one per material
— is the depth fight the junction exists to prevent.

A road that names no texture is still built. That is the state a generator is in before anybody
chooses its art, and the state a check is in always; the geometry is what is being generated and
it exists either way.

## What the server runs

`Build_Network` is everything that does not need a graphics device: tessellate, conform, find the
junctions, honour the bridge and tunnel connections, grade, stamp the mask. `Build_Geometry` is
the meshes. A dedicated server runs the first and stops, and nothing below that line changes what
happens in a match — which is exactly why roads not colliding is a simplification rather than a
compromise.

## Bridges and tunnels

Section 20's bridges do not exist yet. What can be settled now is the seam: a road whose end
connection is a bridge or a tunnel records where its surface stopped, which way it was pointing
and what it expects to meet, and the structure that arrives later reads that instead of guessing.
A connection whose `TargetID` is still -1 is a road that has been told a bridge belongs here and
has not been told which one.

## Navigation metadata

Asked as a question rather than stored as a graph, because the graph belongs to the pathing phase
and the roads are what it will be built from. `Find_Nearest_Road` gives the road, the point on its
centre line, how far off it you are in plan, how far along, which way it runs and what class it
is. `Is_On_Road` is the same question with the answer narrowed to the carriageway.

Lateral distance is measured in plan rather than in space: what a caller means by "how far off the
road am I" is how far to one side, and a soldier standing beside a road that climbs is not further
from it for being lower down.

## Checks

`renegade -terrainselfcheck roads`, registered as `terrain_roads` and `fds_terrain_roads`.
Fourteen terrain tests in total, all green.

The ones worth naming: the curve passes through its own control point; a straight spends two
points on being straight and a bend spends many, with the same settings; trimming cuts at the
distance asked for rather than at the nearest existing point; a crossroads makes one junction with
two arms and four stretches of surface, a tee makes one junction and three, and three roads
meeting at a place make one junction with three arms; **no run point lies inside a junction**,
which is the whole no-overlap claim; two roads sixty metres apart make no junction; a conformed
road sits exactly on the sampled ground at every point; the road mask reads one on the
carriageway, something in between on the shoulder and nothing eighteen metres away; the dominant
terrain layer under a road is the road layer; and the graded pair described above.

## Not done

- **Nothing has been seen.** The checks run device-less. `road_test` in a running client is the
  manual run that would, and it works on a stock level as well as on generated terrain —
  `road_test` accepts a texture name so a stock Renegade road texture can be tried without a
  rebuild.
- **No road art exists.** The three surfaces the road classes want are specified, down to the
  UV layout they have to be drawn for, in `docs/assets/RoadSurfaces.md`. Nothing maps a road
  class to a material yet either; the caller names it.
- **No road authoring.** Roads are built by calling `Add_Road`. Nothing reads them from a level,
  and nothing places them procedurally; the world generation phases are where that arrives.
- **The junction polygon is flat in texture space.** Its texture coordinates come from the frame
  of one arm, so a junction between two roads at a sharp angle has the texture running with one of
  them rather than compromising. Good enough to look at; not the same thing as an authored
  intersection tile, and deliberately so.
- **Tunnel portals have a connection type and no geometry.** Like the bridge, the seam exists and
  the structure does not.
- **Junction finding is pairwise.** Every road is tested against every other along their whole
  lengths. That is fine for the tens of roads a map has and would not be for thousands; when it
  matters, P10's spatial index is what it should ask.
