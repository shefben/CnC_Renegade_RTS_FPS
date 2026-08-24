# Reflective water / river system

Roadmap Section 22. Donors: Zero Hour/shared-Core `W3DWater`, `W3DWaterTracks`.

Acceptance: *water works visually and physically without a competing collision system.*

## What is here

| File | Holds |
|------|-------|
| `Code/WWPhys/watertype.h/.cpp` | `WaterCategoryType`, `WaterDefinitionClass`, `WaterStationClass`, `WaterAreaClass` |
| `Code/WWPhys/watersystem.h/.cpp` | `WaterSystem` |
| `Code/Commando/terrainselfcheck.cpp` | `Check_Water`, run as `terrain_water` and `fds_terrain_water` |
| `Code/Commando/consolefunction.cpp` | `water_test`, `water_clear` |

## The acceptance, read literally

*"Without a competing collision system"* is the load-bearing phrase, and it is a constraint
on architecture rather than on rendering quality. Renegade already has a way to answer "is
this point wet": a soldier's feet report `SURFACE_TYPE_UNDERWATER_DIRT` standing on a lake
bed, and a ray cast up from there looking for `SURFACE_TYPE_WATER_PERMEABLE` is how the
existing wake and splash code finds the surface. See `Code/Combat/soldier.cpp` around its
water-wake block and `Code/Combat/surfaceeffects.cpp`'s `Is_Surface_Permeable` -- neither is
touched by this system. A second, area-based system that answered the same question its own
way, with its own collision volumes, would be exactly the competing system the acceptance
rules out.

So this does not build one. What it builds is authoring and rendering; the physical answer
is Renegade's existing one, reused by construction.

## The mesh is tagged, not special

`WaterSystem::Build_Area_Geometry` builds a surface mesh the same way `RoadSystem` and
`FoliageSystem` build theirs -- through the shared `DynamicMeshBuilderClass` -- and then does
two things to it that make it water rather than an ordinary decoration:

- **`mesh->Set_Collision_Type(COLLISION_TYPE_PROJECTILE)`.** A soldier and a vehicle pass
  through it; a ray cast and a bullet find it. `roadsystem.h` already explains why a road
  surface must not collide -- it would be a second floor a few centimetres above the first --
  and the same reasoning applies here even more directly, because the real floor a swimmer
  or a boat needs is the lake bed underneath, not this plane.
- **Every polygon's `Get_Poly_Surface_Type_Array()` entry is set to `SURFACE_TYPE_WATER_PERMEABLE`.**
  This is the one line that makes the existing reactions fire: nothing about
  `soldier.cpp`'s wake code, `surfaceeffects.cpp`'s splash and damage tables, or
  `wheel.cpp`'s contact surface reporting had to change, because they were already written
  against this surface type. They were waiting for something to tag geometry with it.

Both are two lines in `Build_Area_Geometry`. The alternative -- inventing a
`WaterVolumeClass`, testing points against it every frame, and teaching every system that
currently asks the physics scene about wetness to ask this one instead -- is the competing
system, and it would also mean every one of those existing reactions had to be rewritten
against the new source of truth. Tagging costs less and reuses more.

## Shaping the ground

`WorldTerrainSystem::Apply_River_Cut` already existed, built for exactly this: it takes a
polyline whose points carry their own surface height, levels the terrain to that height
first, then cuts down from it by a depth -- "levelling first" is what stops the river bed
from carrying the terrain's own bumps down with it, per its own header comment.

`WaterSystem::Shape_Terrain` uses it three ways:

- **River and stream** pass their stations straight through: an open, sloping polyline is
  exactly what `Apply_River_Cut` wants.
- **Lake and pond** first cut the boundary ring itself (the stations, closed into a loop, at
  one flat authored height), then sweep the interior in horizontal scanline strips
  (`Carve_Water_Basin`), each strip found by an even-odd crossing test against the boundary
  -- the standard polygon scanline fill, which handles a concave shoreline correctly where a
  single cut from the centroid would not. The row count is capped
  (`WATER_MAX_CARVE_ROWS`) and the spacing doubles to stay under it, the same
  cap-by-doubling idiom `FoliageSystem::Build_Cells` uses for its grid.
- **Ocean and coast do not carve at all.** There is no boundary to a coastline for a channel
  to be cut along, so these two trust the level's own terrain to already sit below the
  declared surface height. `Water_Category_Carves_Terrain` is the one-line gate.

Because `Blend_Height_Region`'s falloff is a smoothstep, not a hard edge, a single scanline
stamp only pulls the ground fully to target at its own centre and partially nearby -- the
self-check's own margin (`carved < -1.0f` against a full carve of roughly -2.5) is sized
around that, not around an assumption that one pass perfectly flattens everything it
touches. A denser interior sweep would converge closer to the flat target; the current
spacing was chosen for a bounded number of terrain edits per pond rather than a perfectly
flat bed, which is the same trade every other capped-cost system in this codebase makes.

## The shoreline

Section 18 built `TERRAIN_MASK_RIVER` and `TERRAIN_MASK_WATER_DISTANCE` and left both with
nothing to write into them -- see `terraintexturesystem.h`'s mask list and
`TerrainTextureSystem::Update_Water_Distance`, which turns the river mask into a distance
field already wired to a shoreline-blend rule. `WaterSystem::Stamp_Mask` is the writer that
was missing, for every category, not only rivers: a closed area stamps its **boundary ring**
rather than filling its interior, because the interior is beneath the water mesh and never
seen -- only the edge, where dry ground shows through, needs a blend at all.
`WaterSystem::Stamp_All_Masks` also calls `Update_Water_Distance` once at the end, so
placing several bodies of water costs one distance-field rebuild, not one per area.

## Authoring: stations, not two separate shapes

Section 22 asks for ocean/coast, lake, pond, river and stream, which read as two different
kinds of shape -- a boundary and a line. `WaterAreaClass` represents both with the same
`WaterStationClass` list (`watertype.h`), distinguished by one `Closed` flag:

- **Closed** (ocean, coast, lake, pond): the stations are a ring, and `Height` is the single
  flat surface a still body actually has -- a lake cannot slope, so unlike an open area's
  per-station height there is exactly one number.
- **Open** (river, stream): the stations are a centre line, and each one's own Z is the
  surface height there, which is what lets a river run downhill. Width is per-station too,
  so a river can narrow and widen along its length.

This mirrors `BridgeClass`'s "two points and a kind" and `RoadSplineClass`'s "a line
somebody drew" -- a water area is the same idea generalised to a boundary or an open line of
any number of points, not a new representation invented for this phase.

## Headless queries

`Contains_Point`, `Get_Surface_Height` and `Get_Flow_Direction` on `WaterAreaClass`, and
`Find_Area_At`, `Get_Water_Height`, `Get_Water_Depth`, `Get_Flow` and `Is_Navigable` on
`WaterSystem`, all answer from the authored station data, never from the physics scene or
the render mesh. This is the same shape as `FoliageSystem::Is_Blocked`: a path-finder, a
naval unit's draft check, or a server with no scene at all can ask "how deep is it here" and
get a real answer with nothing built. `Get_Flow` is the one query that joins two objects
deliberately kept separate -- direction comes from the area's own geometry (which way the
centre line runs at the nearest point), speed comes from the kind's definition
(`WaterDefinitionClass::Get_Scroll_Speed`) -- because a river's direction is a fact about
its shape and its speed is a fact about what kind of water it is, and conflating them would
mean every river needed its own copy of a number that is really a property of "river".

## What was taken from the donor and what was not

**Taken.** Water as a bounded area with a kind, not one giant system: `W3DWater` supports
several ponds and rivers per map, each with its own extent, and so does this.

**Not taken -- a dedicated water collision volume.** This is the acceptance's own point, made
concrete: SAGE's water gameplay layer is commonly a polygon trigger separate from the
rendered water plane. Here the rendered plane **is** the collision answer, tagged the way
any other authored geometry is tagged, per the section above.

**Not taken -- the mega-texture or a shared water buffer.** Unlike `FoliageSystem`, there was
never a donor structure here worth preserving or rejecting for efficiency -- six named kinds
and a handful of placed areas per map is already cheap, so there is no batching problem
Section 21's cell-and-batch machinery was built to solve. A water area builds one mesh; that
is the whole of its cost.

## Shared with the road, bridge and foliage systems

`DynamicMeshBuilderClass::Triangle` (fan geometry for a closed area) and the general
count-then-build pattern are reused exactly as `RoadSystem`, `BridgeSystem` and
`FoliageSystem` use them. `WorldTerrainSystem::Apply_River_Cut` was written during Section
19's road work and used here unmodified -- it already took a per-point surface height and a
depth, which is precisely what a lake's boundary and a river's centre line both need.

## Not done

- **Nothing has been seen.** The checks run device-less, so `Build_Geometry` correctly
  declines with no physics scene. What a pond looks like is `water_test` in a running
  client, pointed at a texture that exists.
- **No water art exists.** Nothing draws until it does; the names and the constraints are in
  [../assets/WaterModels.md](../assets/WaterModels.md).
- **Flow does not animate anything yet.** `WaterAreaClass::Get_Flow_Direction` and
  `WaterDefinitionClass::Get_Scroll_Speed` are real, queryable numbers, and nothing drives a
  per-frame UV offset from them -- the same gap `FoliageSystem` has with per-frame LOD
  switching, and for the same reason: it needs a per-frame caller this phase does not add.
- **No reflection.** Listed in Section 22's rendering requirements. A planar reflection
  needs a render-to-texture pass the renderer does not have; nothing here works around that.
- **Nothing plants anything.** `WaterSystem::Add_Area` exists and only the console and the
  self-check call it. Reading water areas out of a level's own data is a level-format
  question, not this one.
- **Naval and amphibious unit integration is a query away, not wired up.** `Get_Water_Depth`
  and `Is_Navigable` are exactly what a naval factory or an amphibious unit's movement code
  would ask, and nothing asks them yet -- that is Commander/gameplay-layer work, listed
  separately in the roadmap.
- **Nothing damages the surface from gameplay.** Projectile disturbances are free once a
  bullet hits `SURFACE_TYPE_WATER_PERMEABLE` -- `SurfaceEffectsManager` already reacts to
  that surface type -- but nothing yet fires a bullet at a client-only water plane that has
  no gameplay object standing in for it, the same missing piece the bridges and the foliage
  proxies have.
