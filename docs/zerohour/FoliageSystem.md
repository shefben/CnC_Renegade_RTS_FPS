# Tree / foliage buffer

Roadmap Section 21. Donor: Zero Hour's `W3DTreeBuffer`.

Acceptance: *large forests render with substantially fewer submissions than individual
`RenderObj` instances.*

## What is here

| File | Holds |
|------|-------|
| `Code/WWPhys/foliagetype.h/.cpp` | `FoliageCategoryType`, `FoliageBlockingBitsType`, `FoliageLodType`, `FoliageTypeClass`, `FoliageInstanceClass` |
| `Code/WWPhys/foliagesystem.h/.cpp` | `FoliageCellClass`, `FoliageBatchClass`, `FoliageProxyClass`, `FoliageSystem` |
| `Code/WWPhys/dynamicmeshbuilder.h` | `Triangle` and `Box`, shared with the road and bridge systems |
| `Code/Commando/terrainselfcheck.cpp` | `Check_Foliage`, run as `terrain_foliage` and `fds_terrain_foliage` |
| `Code/Commando/consolefunction.cpp` | `foliage_test`, `foliage_cut`, `foliage_clear` |

## The acceptance, and where it is answered

A batch is one mesh and one thing to draw. Within a cell, everything sharing a material and a
level of detail is merged into one, so a wood of two thousand trees over four hundred metres
is drawn as about a hundred and twenty things rather than two thousand — a factor of sixteen,
and better on a denser wood because the ratio is trees per cell.

The important property is that **this is answerable without a graphics device**. Batching is a
question about how instances group by cell and by material, not about what they look like, so
`Build_Cells` and `Build_Batches` run headless, `Get_Batch_Count()` against
`Get_Instance_Count()` is the acceptance ratio, and `terrain_foliage` checks it in the same
binary the game ships. Geometry is a separate step that needs a scene.

`Get_Visible_Batch_Count()` is the sharper number — how many are submitted from a given point
of view after culling and LOD — and it is smaller again.

## The shape

Section 21 asks for cells, instances, visibility, distance LOD, material batches and optional
collision proxies. Each is a thing here rather than a note in a comment.

### Cells

A grid over the instances, not over the map. It is built from the extents of what was planted,
so a copse in the corner of a large map costs a handful of cells; if the extents and the cell
size would ask for an unreasonable number of squares the cell size doubles until they do not.
Only squares that hold something become cells.

A cell is the unit of three different things, which is why its size matters and why 32 m is
the default:

- **culling** — one distance test covers everything standing in it;
- **batching** — a batch never spans cells, so a batch is never larger than a cell's worth;
- **rebuilding** — felling one plant rebuilds its cell's meshes and nothing else.

### Instances

A type, a position, a yaw and a scale. Nothing per instance is allocated, nothing per instance
is submitted, and nothing per instance exists in the scene. `Scatter` generates a wood from a
seed, deterministically, so a client can reconstruct the server's wood rather than be sent it.

### Material batches

The batch key is `(cell, material, level of detail)`. Two types naming the same texture merge
into one batch, which is the cheapest optimisation a level author has available.

A batch is capped at 256 instances at planning time, because a mesh is drawn through a
sixteen bit index buffer and a dense cell would otherwise produce one too large to build. A
capped cell produces several batches of the same material, which is still a small number, and
the cap is applied when the batches are *planned* so that the number the acceptance is
measured by is the number that will actually exist.

### Visibility

`Update_Visibility` takes an eye, a forward direction, an optional cone half-angle cosine and
an optional maximum distance. Cells are culled by distance and by the cone — widened by the
cell's own radius, so a cell whose centre is off to the side but whose near corner is straight
ahead survives — and a culled cell takes every batch in it with it.

### Distance LOD

Two levels are built and the third is neither:

- **near** — the models' own triangles, merged;
- **far** — crossed quads carrying the type's impostor sheet, merged;
- **culled** — nothing.

Both meshes are built **once** and hidden or shown, rather than a buffer being refilled as the
camera moves. A batch stores the earliest near distance and the latest cull distance of the
types that contributed, so it keeps its detail until the first thing in it would drop detail
and survives until the last thing in it would go. Merging means a batch is drawn or not; where
that forces a choice, erring towards drawing is the error nobody sees.

A type that names no impostor sheet has no far level at all and simply stops being drawn past
its near distance. That is right for grass and wrong for a tree on a skyline, which is why the
sheet is an asset request rather than an optional extra.

### Collision proxies

Genuinely optional: a cell with nothing solid in it gets none. Where they exist they are
merged the same way, one invisible mesh per cell per set of blocking bits, so a wood that
stops infantry and vehicles is one collision object per cell rather than one per tree.

A proxy is a **post** — the type's declared radius and height — and never the model. A tree's
leaves are not a thing to walk into, and the drawn batch never collides at all.

`Is_Blocked` answers the same question from the instance data rather than from the scene, so a
path-finder can ask before any geometry exists and a server with no scene at all still refuses
to walk a soldier through a tree.

## Categories and behaviours are two axes, not one

Section 21 lists five categories — tree, bush, dead tree, grass clump, decorative rock — and
separately lists visual-only, infantry blocker, vehicle blocker and destructible. Those are
two different questions about the same thing and they are two fields: a category is a look and
a distance scale, and blocking is a flag set with visual-only as the absence of both flags.
Destructible is a bool for the same reason — a bush can be either, and so can a tree.

The five default types are one per category, chosen between them to cover all four behaviours.
None names any art; see [../assets/FoliageModels.md](../assets/FoliageModels.md).

## What was taken from the donor and what was not

**Taken.** The central idea: read a model's triangles once, stamp them many times into shared
geometry, and draw the result as one thing. Also the notion of a type table separate from the
instances, and an area partition for spatial queries.

**Not taken — the single global buffer.** `W3DTreeBuffer` packs every tree in the world into
one vertex and index buffer with hard caps of 4000 trees and 30000 vertices, and refills it
when visibility changes. Section 21 explicitly says not to preserve inefficient donor
structure where something better fits, and this is where. Cells give the same batching with no
global cap on how many plants a map may have, no per-frame refill, and culling that is a test
per cell instead of a test per tree.

**Not taken — the mega-texture.** The donor packs every tree type's texture into one atlas so
that everything can share a material. Batching per material per cell gets the same result
without an atlas to build or UVs to rewrite; the cost is a constraint on the art, which is
stated in the asset list rather than worked around.

**Not taken — sway, topple and push-aside.** The donor animates trees in the wind, topples
them when something drives through, and pushes them aside as a unit passes. All three move one
plant, and a merged mesh cannot move one plant. They would need those plants back out of the
batch as individual objects, which is the thing this phase exists to stop doing. If they are
wanted later the honest way is a small pool of un-batched objects for plants currently being
disturbed, promoted out of and demoted back into their batch — which is a phase of its own and
not a line in this one.

**Not taken — the partial bubble sort.** The donor sorts trees by distance a few iterations
per frame, and its own source says the sort is no longer used and messes up the order. Alpha
test rather than alpha blend removes the need for it.

## Shared with the road and bridge systems

`DynamicMeshBuilderClass` gained `Triangle` — one triangle as a fan of three, which is what
merging somebody else's mesh needs — and `Box`, which was `Emit_Box`, private to
`bridgesystem.cpp`. The bridge system's slab and the foliage system's collision post are the
same box, so it is written once and the bridge's copy was deleted.

## Not done

- **Nothing has been seen.** The checks run device-less, so `Build_Geometry` correctly
  declines. What a batched wood looks like is `foliage_test` in a running client, pointed at a
  model that exists.
- **No foliage art exists.** Nothing draws until it does; the names and the constraints they
  have to be built to are in [../assets/FoliageModels.md](../assets/FoliageModels.md).
- **Nothing plants anything.** `Scatter` and `Add_Instance` exist and only the console calls
  them. Reading foliage out of a level's own data is a level-format question, not this one.
- **Nothing damages a plant from gameplay.** `Apply_Damage` exists and only `foliage_cut`
  calls it; wiring it to weapon damage needs a gameplay object over the proxy, which is the
  same missing piece the bridges have.
- **Destruction is not replicated.** Which plants have been felled is one bit each and would
  travel the same way the bridges' section states will; there is no transport for either yet.
- **Nothing calls `Update_Visibility` per frame.** The batches are static objects in the
  physics scene, so the scene's own culling already draws only what it should; the LOD switch
  is the part that needs a per-frame caller and does not have one.
- **Pathing does not ask.** `Is_Blocked` answers the question a path-finder would put; no
  path-finder puts it yet.
