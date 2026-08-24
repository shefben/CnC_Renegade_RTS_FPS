# Foliage models and impostor sheets

Roadmap Section 21 (tree / foliage buffer). Five model names, five texture names and five
impostor sheets, and none of them exists.

`FoliageSystem::Build_Batch_Geometry` in `Code/WWPhys/foliagesystem.cpp` never creates a
render object per plant. It asks the asset manager for one copy of each type's model, reads
the triangles out of it, and stamps them into one mesh per cell per material — so what is
being asked for here is a model that is read once and drawn ten thousand times, which
constrains it in ways an ordinary prop is not constrained.

The names live in `FoliageSystem::Define_Default_Foliage`, and today every one of them is
empty.

## What happens without them

Nothing is drawn, and everything else works.

That is the right degradation here and it is not the one the bridges got. A bridge with no
model is a hole a soldier falls through, so a bridge section stands in as a slab; a tree with
no model is a bare hillside, which is exactly what a bare hillside looks like. The parts of
the system that are not art keep working without any:

- the cells are built, the batches are planned, and `Get_Batch_Count()` against
  `Get_Instance_Count()` is the acceptance ratio whether or not anything draws;
- the collision proxies are boxes built from the type's declared radius and height, not from
  the model, so a wood **still blocks infantry and vehicles with no art at all**;
- `Is_Blocked` answers for a path-finder from the same numbers.

`FoliageSystem::Get_Missing_Model_Count` reports how many types named nothing that loaded, and
`foliage_test` at the console prints it.

## The one hard constraint: one material per model

A batch is one mesh drawn with one texture. The triangles of every plant in it come from
whatever models contributed, and they are all drawn with the texture the **type** names — not
with whatever textures the model itself carries.

So a foliage model must use **one material, on one pass**. This is the same constraint the
donor has; the donor works around it by packing every tree in the game into a single
mega-texture and rewriting the UVs, and this does not, because asking for one material is
cheaper than maintaining an atlas and it is a thing an artist can simply do.

A model that breaks the rule is not rejected and does not crash. Its first pass is read, its
first texture is ignored in favour of the type's material, and
`FoliageSystem::Get_Multi_Material_Model_Count` counts it so the mismatch is visible rather
than silent.

Related consequences of being merged:

- **No per-plant animation.** The merged mesh is built once and does not move. The donor's
  sway and topple are not here; see `docs/zerohour/FoliageSystem.md`.
- **No per-plant lighting.** The batch is prelit diffuse, like the road and bridge meshes.
- **Vertex budget.** A batch is capped at 256 plants and about 65000 vertices, because it is
  drawn through a sixteen bit index buffer. At 256 plants a batch that would fit needs a model
  under roughly 250 vertices. Heavier models still work — the cell simply produces more
  batches — but the acceptance ratio is better with a light model, which is the usual reason
  foliage is light anyway.

## The near models

| Name | Category | Blocks | Breaks | Proxy | Drops to impostor at |
|------|----------|--------|--------|-------|---------------------|
| `ow_tree_conifer` | `TREE` | infantry, vehicles | no | 0.7 m post, 9 m tall | 90 m |
| `ow_bush_scrub` | `BUSH` | nothing | yes, 25 health | none | 40 m |
| `ow_tree_dead` | `DEAD_TREE` | infantry | yes, 150 health | 0.5 m post, 6 m tall | 80 m |
| `ow_grass_clump` | `GRASS_CLUMP` | nothing | no | none | 25 m |
| `ow_rock_boulder` | `DECORATIVE_ROCK` | infantry, vehicles | no | 1.2 m post, 1.6 m tall | 80 m |

These five are one per category, chosen between them to be all four of Section 21's separated
behaviours: visual-only, infantry blocker, vehicle blocker and destructible.

### Constraints on the geometry

**The origin is where it meets the ground, and Z is up.** An instance is placed at a point on
the terrain and rotated about Z by its own yaw; nothing else is done to the transform. A model
whose origin is at its centre will be planted half underground.

**Model space is world scale.** The instance's scale multiplies it — the conifer is planted
between 0.85 and 1.25 — so the model should be a plausible tree at scale 1.

**Yaw is the only rotation.** Instances are rotated about Z only, so a model must read from
any horizontal angle. Nothing tilts to the slope it stands on, which is a limitation worth
knowing when placing on steep ground.

**Alpha-tested cut-out, drawn two-sided.** The batch shader enables alpha test and disables
back-face culling, which is what makes single-sided leaf cards work. Leaf geometry can
therefore be flat cards with no back faces.

**One material** — see above.

## The materials

One texture per type, named separately from the model because it is the batch key: two types
naming the same texture merge into the same batch, which is the cheapest thing a level can do
to make its wood faster.

```
ow_tree_conifer.tga
ow_bush_scrub.tga
ow_tree_dead.tga
ow_grass_clump.tga
ow_rock_boulder.tga
```

A leaf sheet with a hard alpha edge. Read by `FoliageSystem::Build_Batch_Geometry` through
`WW3DAssetManager::Get_Texture`. Without them the batch geometry is drawn untextured, which is
white — so a type that names a model but no material looks worse than one that names neither.
Name both or name neither.

## The impostor sheets

Beyond a type's near distance the batch is replaced by one made of **crossed quads** — two
quads at right angles through each plant's position, the whole cell in one mesh. That is the
distance LOD, and it exists only for types that name an impostor sheet. A type with no sheet
simply stops being drawn past its near distance, which is correct for grass and wrong for a
tree on a skyline.

```
ow_tree_conifer_impostor.tga
ow_bush_scrub_impostor.tga
ow_tree_dead_impostor.tga
ow_grass_clump_impostor.tga
ow_rock_boulder_impostor.tga
```

**Constraints.**

- One image of the whole plant, seen from the side, alpha cut out. Not a sprite sheet, not an
  atlas of angles: the same image is used for both quads and every direction.
- The quad is `ImpostorWidth` by `ImpostorHeight` metres at scale 1 — for the conifer, 6 by
  10 — so the image aspect should match, or the plant will be visibly the wrong shape at the
  distance the switch happens.
- UV runs 0..1 across the full image with **v increasing downwards**: the top of the plant is
  v = 0 and its base v = 1.
- The base of the plant must be at the bottom edge of the image. The quads stand on the
  ground, so blank space at the bottom of the sheet lifts the whole plant off it.
- It should read at the distance it appears at, not close up. The switch happens at the
  type's near distance and the plant is by then a few dozen pixels tall.
- **Nothing turns to face the camera.** A merged mesh cannot rotate its plants individually,
  which is why it is a cross and not a billboard. Two crossed cards read acceptably from any
  direction and noticeably badly from directly above.

## Trying it before the art exists

```
foliage_test 2000 200 <a model name from this level>
foliage_cut
foliage_clear
```

`foliage_test` plants a wood around the player and prints how many plants it holds and how
many things it is drawn as. The third argument replaces the conifer's model name, so pointing
it at a tree that already exists in a stock Renegade mix file is how the batching can be
watched working on real art before any of ours exists — a stock tree is one material, which
is the constraint above, so most of them will simply work.

## Not listed here, and why

**Sway textures and topple animations.** The donor animates trees in the wind and topples them
when a vehicle drives through. A merged mesh cannot animate one plant, so neither is here, and
neither is an asset request until something can move a plant. See
`docs/zerohour/FoliageSystem.md`.

**Stumps and debris.** Felling a plant removes it. There is nothing left behind, so there is
nothing to model, until the effect system of Section 35 exists to be handed one.
