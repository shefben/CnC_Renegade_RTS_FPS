# Terrain layer textures

Roadmap Section 18 (terrain texture system). Six textures.

`TerrainTextureSystem::Define_Default_Layers` in `Code/WWPhys/terraintexturesystem.cpp`
builds six layers, each with a rule that decides where on the ground it shows. Every rule
works today and is checked. None of the six names a texture, so the ground draws with the
rules doing their job and nothing to show for it.

The full signature is
`Define_Layer(name, kind, rule, texture_name = nullptr, meters_per_tile = 10, surface_type = 0)`,
and a null texture name means "a rule with no content yet". So giving these six their art is
six calls at `terraintexturesystem.cpp:270-322` gaining three arguments each -- the name, the
tiling suggested below, and the surface type that decides what a soldier's footsteps sound
like on it -- and nothing else anywhere.

## Constraints that apply to all six

**Tiling, both axes.** Terrain UVs are `world_x / metres_per_tile` and
`world_y / metres_per_tile` (`RenegadeTerrainPatchClass::Update_UVs`), so every texture
repeats across the map in both directions and any seam shows as a grid over the whole
world. Square, power of two, wrapping cleanly.

**Projected from above.** Both UVs come from world X and Y — the height is not in them.
A texture on flat ground reads at its true scale; on a slope it is stretched by one over
the cosine of the slope, and on something near vertical it smears into streaks. This
matters most for the two rock layers, which by definition only ever appear on steep ground.
Paint them so that vertical streaking reads as strata rather than as a mistake, and avoid
detail whose scale is the point.

**Mirroring is available.** The material carries an `Are_UVs_Mirrored` flag, which flips
alternate tiles and hides a seam at the cost of visible symmetry. Worth using for the
ground, not for anything with direction in it.

**Layers stack.** The first layer with any influence on a quad draws opaque and the rest
draw over it with their share of the total, which is stock Renegade's own composite rule.
So the five above the ground are blended over what is under them, and their edges are
feathered by the rules rather than by their alpha.

## The six

### `ow_ter_ground.tga`

- **For:** the base ground, everywhere. Every square metre of a generated world that no
  other layer covers is this texture, so it sets the world's colour more than any other
  asset in this folder.
- **Rule:** none — that is what being the base means.
- **Suggested tiling:** 8 m.
- **Without it:** untextured, vertex-lit ground.

### `ow_ter_macro.tga`

- **For:** large-scale variation over the base, so a plain is not one flat colour. Broad
  soft patches of lighter and darker ground, low contrast, no readable features.
- **Rule:** hash noise at frequency 0.03 — features around 30 m across — at half strength.
- **Suggested tiling:** 64 m or more. It is the one layer that should be obviously bigger
  than everything else.
- **Without it:** the ground is uniform at large scale.
- **Note:** the stock terrain pass carries one UV rate per material, which is enough for
  this because the rate is per layer. What it cannot yet do is a *second* rate on the same
  layer, which is what a close-up detail layer would need. Detail is a separate, unlisted
  layer for that reason.

### `ow_ter_rock.tga`

- **For:** broken stone and scree on anything steep. The general "this is too steep to be
  soil" cover, and the more common of the two rock layers by a long way.
- **Rule:** slope over 20 degrees, feathered in over the 10 degrees below that.
- **Suggested tiling:** 4 m.
- **Without it:** hillsides look like flat ground standing up.
- **Constraint:** see the projection note above. This is worn, rubbly, non-directional
  material, which tolerates stretching better than anything else would.

### `ow_ter_cliff.tga`

- **For:** exposed cliff face, as distinct from the scree below it. Strata, hard edges,
  visibly stone rather than earth.
- **Rule:** slope over 45 degrees **and** convex curvature — which is what tells the lip of
  a cliff from the pile at the bottom of one, since slope alone cannot.
- **Suggested tiling:** 6 m.
- **Without it:** cliffs are just steeper hillside.
- **Constraint:** appears almost exclusively on near-vertical ground, so it is the texture
  that the top-down projection stretches most. Horizontal banding survives that; anything
  with a vertical rhythm does not.

### `ow_ter_shore.tga`

- **For:** wet ground at a water's edge — dark, darker where it meets the water, sand or
  silt.
- **Rule:** within 4 m of water by the water-distance mask, feathered over the next 4 m.
- **Suggested tiling:** 4 m.
- **Without it:** land meets water at a hard edge with no transition.
- **Depends on:** the water-distance mask, which has a writer only once Section 22's water
  exists. Until then this layer is correct and never triggers.

### `ow_ter_road.tga`

- **For:** the worn ground a road sits on and the verge it fades into. **Not the road
  surface itself** — that is `ow_road_street.tga` and friends in
  [RoadSurfaces.md](RoadSurfaces.md), a separate mesh 8 cm above this.
- **Rule:** the road mask above 0.5, feathered over 0.3, and exclusive — a road that let
  the rock layer through would be a road with a hill drawn on it.
- **Suggested tiling:** 4 m.
- **Without it:** the road ribbon meets the ground at a hard edge instead of through a band
  of worn dirt.
- **Note:** this is the layer the road system's shoulders are made of. `RoadSystem` stamps
  the mask at carriageway width and feathers it across `shoulderWidth`, so the width of
  this layer on the ground is decided by the road, not by the texture. It should read as
  dirt worn by traffic, and it must look right *beside* the road surface texture, because
  that is the only place it is ever seen.

## Stock art as a stand-in, deliberately not as an answer

Renegade ships ground, rock and sand textures in `always.dat`, and `Define_Layer` takes any
name, so pointing the six layers at stock names is a one-line-each change that makes the
whole system visible today without anybody painting anything. That is worth doing, and it is
the next action recorded against P13.

It is a stand-in and not the entry above it, for two reasons. Stock terrain art was drawn for
hand-painted level geometry at the scales those levels use, not for rules that place it by
slope and curvature across a generated world. And a shipped build that depends on stock
content having particular names has made asset compatibility into an asset *requirement*,
which is the opposite of the rule.

So: stock names to look at it, `ow_` names to ship it. When the six textures above exist,
they replace the stand-ins and nothing else changes.

## Not listed here, and why

**Detail, biome and scorch** have layer kinds in the system and no entries in the default
table, so nothing asks for their art yet. Detail wants a second UV rate on one layer that
the stock pass does not carry; biome wants a generator to write the biome mask; scorch
wants damage events to stamp it, which is Section 35's decal work. They get entries in this
folder when the code that names them exists.
