# TerrainTextureSystem

Roadmap Section 18 (Zero Hour / SAGE Feature 5, terrain texture system).

`Code/WWPhys/terrainmask.{h,cpp}`, `Code/WWPhys/terraintexturesystem.{h,cpp}`, the `textures`
mode of `Code/Commando/terrainselfcheck.cpp`, and the `terrain_dress` console command.

## What it replaces is the brush

The drawing half of terrain texturing already existed and is stock Westwood. A
`RenegadeTerrainPatchClass` carries a list of material passes; each has a texture, a tiling
rate, a surface type, and a weight per vertex; the patch composites them in order, the first
opaque and the rest blended over it. That machinery works and nothing here touches it.

What did not exist is a way to decide those weights without a person and a brush. The level
editor filled them by painting. Section 18's acceptance — *a generated heightfield can obtain
coherent terrain materials entirely from runtime data and masks* — is exactly the statement
that nobody paints. So this is the deciding half only, and it hands its answer to the drawing
half in precisely the form the brush used to.

## Everything that is not shape is a mask

Section 18 lists the inputs: height, slope, curvature, biome, moisture, water distance, road
mask, river mask, city mask, Tiberium mask, cliff mask, manual override mask.

The first three are the shape of the ground and come from `HeightfieldClass` — height from the
grid, slope from the vertex normal (the same one the renderer lights with, so what a rule calls
steep and what the ground looks like agree), and curvature from `Compute_Curvature`.

Every one of the others is the same thing wearing a different name: a number per grid point
that something wrote there. So there is one class, `TerrainMaskClass`, and nine instances of
it. That is not tidiness for its own sake — if the only way to say anything about the ground is
to write it into one of these, then no hidden file can be involved, and the acceptance
condition is kept by construction rather than by inspection.

Masks are the same shape as the heightfield, one value per vertex, so a rule can ask about
height and about the road at the same point without converting between two ideas of where a
point is. They are destroyed with the field, because a mask is a statement about *that* grid.

### Water distance is derived, not written

### A stamp is measured, not accumulated

`Stamp_Polyline` measures the distance from each grid point to the whole line and applies the
falloff once. It used to walk a row of overlapping discs, which is the obvious way to do it and is
quietly wrong: each disc moves the value part of the way towards its own, so a point out in the
feathered rim is moved part of the way a hundred times as the line goes past it and arrives at
full strength. The soft edge survived a short line and vanished on a long one — a defect that
could not show up until a road was a road rather than a test, and did not, until Section 19's
shoulder asked for a value between nought and one and got 0.9999.

`Update_Water_Distance` builds a distance field from the river mask: a two-pass chamfer sweep,
zero where the river fires, saturating at a maximum. Nobody draws a beach; they draw a river,
and the beach follows. The chamfer is a few per cent long on diagonals and nothing downstream
of a shoreline blend can see that.

An unwritten water-distance mask starts saturated rather than at zero. A map with no river in
it is a map where nowhere is near water — starting at zero would flood the world before anyone
drew anything.

## Curvature earns its place

Slope cannot tell the top of a cliff from the bottom of one: the ground beside each is equally
steep. Curvature can, because one is convex and the other concave. That is the whole reason
Section 18 lists curvature as an input, and it is what the default table uses to put a cliff
face on the lip and scree at the foot.

The check builds a field with a wall in it and asserts exactly that pair: at the two grid lines
either side of the wall the slope is the same and the curvature is opposite, and only the upper
one is a cliff face.

`Compute_Curvature` is the discrete Laplacian, negated so convex reads positive, in height per
square metre. `Get_Height` clamps at the border, which makes the outermost ring of the field
read as flat — the edge of the field is not a feature of the terrain.

## Rules are windows with soft edges

Every test a layer can make is a window: full strength inside a range, fading to nothing across
a feather outside it. A hard threshold would draw a visible line across the map wherever it was
crossed, which is the thing that makes generated terrain look generated.

A window whose minimum is above its maximum is off, and that is the default, so a rule that
names nothing fires everywhere at full strength. That is the right default because it is what a
base layer is.

```text
height range        slope range        curvature range
one mask + range    noise (amount, frequency, seed)
strength            exclusive
```

The base layer's weight is always one. Something has to be underneath, and the alternative —
a base that weakens where other layers are strong — is a hole in the world wherever two layers
happen to be weak at the same point.

An **exclusive** layer pushes the others aside in proportion to how strongly it applies. A road
at full strength is a road; a road fading out at its edge lets what is beside it come back. The
base is exempt, for the same reason it is always one.

## Deterministic, and what that costs

There is no random number generator anywhere in this system. Variation comes from
`TerrainMaskClass::Hash_Value`, an integer hash of the grid position and a seed.

A generator carries state, and state means the answer depends on what order the map was walked
in and which machine walked it. A hash depends on nothing but its arguments, so a server and
every client compute the same ground without anyone sending it, and reloading a level gives the
map it gave last time. The cost is that the pattern is value noise on a lattice rather than
anything cleverer; the benefit is that "the terrain is different on the client" is not a class
of bug that can happen here.

## What the ground is made of

`Get_Dominant_Layer` answers "what am I standing on", and three layer kinds are deliberately
not candidates for it:

- **base** is the fallback rather than a contender — it always scores one and would always win;
- **detail** and **macro** are how the base is *drawn*, not what the ground *is*. They apply
  almost everywhere by design, and answering "macro variation" to a footstep sound is worse
  than useless.

`WorldTerrainSystem::Get_Material` asks this rather than answering itself, and
`Get_Surface_Type` reads the stock Renegade surface type off the winning layer's material —
the one footsteps and decals already understand. The shape of the ground is the terrain
service's; what it is made of is Section 18's; and asking rather than answering is what keeps
there being one place that decides each.

The placeholder `TerrainMaterialType` enum written in Section 17 is gone, superseded by the
layer list. Its header comment promised callers would not need rewriting when Section 18
landed; that promise was half kept — the question is the same, the answer type changed from an
enum with one value to an index into a table that exists.

## The default table

`Define_Default_Layers` is Section 18's acceptance stated as a function: a heightfield with
nothing painted on it gets coherent materials.

```text
ground   base       no rule -- everywhere, because that is what ground means
macro    macro      hash only, half strength, so a plain is not one flat colour
rock     cliff      slope over 20 degrees, feathered over the 10 below
cliff    cliff      slope over 45 AND convex -- the lip, not the scree
shore    shoreline  within 4m of water, feathered over 4m more
road     road       the road mask, exclusive
```

None of them names a texture. `Define_Layer` treats a null texture name as "a rule with no
content yet", which is what a check with no asset manager and a generator that has not chosen
its art both want. A pass with a null material now returns early in `Render_By_Texture` instead
of reading through the pointer — stock code could always reach that state, since
`Get_Material_Pass` grows the list with `Add_Material(nullptr)`, and runtime-built terrain
reaches it for real.

## Handing it to the renderer

`Build_Patch_Materials` computes every weight for every vertex of a patch up front — a patch's
quads share three quarters of their vertices with a neighbour, and evaluating the rule table
per quad would run it four times per vertex — then walks the quads filling passes.

The composite rule is stock Renegade's, copied deliberately: the first layer with any influence
on a quad draws it opaque, and each later one draws over it with its share of the running
total. Matching what the editor's brush produced means nothing in the renderer has to learn a
second way to read a terrain patch.

`WorldTerrainSystem::Build_Collision` calls `Build_All_Patch_Materials` when any layers are
defined. Geometry that has just been rebuilt is wearing the materials the old shape called for,
so dressing follows building rather than being asked for separately.

## Checks

`renegade -terrainselfcheck textures`, registered as `terrain_textures` and
`fds_terrain_textures`. Twelve terrain tests in total, all green.

The ones worth naming: a stamped road is continuous along its whole length (discs dropped too
far apart make a dashed line); water distance is zero at the river, three cells out reads three
metres, and far away saturates; the base is one everywhere; the cliff/scree pair described
above; a road drawn over a slope lets none of the rock through and does not take the ground out
from under itself; the same weights twice running; and a dressed patch has quads in its opaque
pass, because a patch with none has a hole in it.

## Not done

- **Nothing has been seen.** The checks run device-less and the default layers carry no
  textures, so no screen has shown any of this. `terrain_test` then `terrain_dress` in a running
  client is the manual run that would.
- **Detail, biome and scorch layers** have kinds and no entries in the default table. Detail and
  macro want a second UV rate the stock pass does not carry; biome wants a generator to write
  the biome mask; scorch wants damage events to stamp it, which is Section 35's decal work.
- **The six default layers name no texture.** What each one has to be, and the constraints the
  top-down UV projection puts on it, are written down in `docs/assets/TerrainLayers.md`. Giving
  them their art is six string literals in `Define_Default_Layers` and nothing else.
- **`Get_Meters_Per_Tile`** is per material and per layer, and `Update_UVs` reads it, so tiling
  works — but nothing chooses good values, because nothing has textures to choose them for.
  `docs/assets/TerrainLayers.md` suggests one per layer.
- **Most masks still have no writers.** The road mask now has one — Section 19's road system
  stamps it at the carriageway width and feathers it across the shoulder, so the `road` layer is
  fed by real roads (see `RoadSystem.md`). The river mask waits on Section 22's water, the
  Tiberium mask on the resource work, and the city mask on whatever draws cities. Each arrives
  with its section; the mask is here first so they have somewhere to write.
