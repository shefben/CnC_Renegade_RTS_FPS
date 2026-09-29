# Map plan (Phase 2B, D-003)

Owner: `MapPlanGenerator` (`Code/wwphys/mapplangenerator.h/.cpp`). Runs in
`ProceduralWorldManager::Build_Map_Plan` after the cliff ring and before macro terrain.
Profile block: `plan` (`WorldProfileSchema.md`). Seed domain: `map_plan`.

## Why it exists

Before D-003 the heightfield came first and every gameplay phase reacted to it: cliffs were
whatever chunks stayed steep, bases were chosen at Phase 11 after roads existed, and fairness
was search plus repair plus whole-world retries. C&C battlefields are designed the other way
round. RA2's random map generator and Renegade's multiplayer maps put start positions, the
Tiberium field and the lanes down first, then raise plateaus, then dress tier edges with
cliffs and ramps. The plan is that first step: a 2D layer stack on a chunk-resolution grid.

## Layers

| Layer | Content | Query |
| --- | --- | --- |
| L0 layout graph | `MapPlanSite` list: one `BASE_GDI`, one `BASE_NOD`, one `FIELD_CENTER`, 0..2 `FIELD_FLANK`, two `CHOKE` | `Get_Site`, `Get_Site_Count_By_Kind`, `Get_Base_Site_Weight` |
| L1 height tiers | integer tier per cell, `0 .. tier_count-1` | `Get_Tier_At`, `Sample_Tier_Height` |
| L2 water plan | lowest-tier open cells marked `WATER` | `Get_Kind_At` |
| L3 passability | `OPEN`, `BASE`, `LANE`, `FIELD`, `CLIFF_SEAM`, `RAMP`, `WATER` per cell | `Get_Cell`, `Get_Kind_At` |

One cell is one chunk (`world.chunk_size`). Callers address the plan by world position;
the plan's own cell grid is `ceil(width / chunk)` by `ceil(height / chunk)` centred on the
world centre, which is not guaranteed to match `WorldTerrainSystem`'s chunk registry by
index.

## Construction order

1. **Sites.** GDI base at a seeded angle, 0.72 of the usable half-extent from the centre,
   clamped inside `boundary_margin_cells + base_radius_cells`. Nod base by the plan symmetry
   (`rotational` = 180 degrees about the centre, `mirror_x` = across the vertical axis,
   `none` = opposite side with a hashed offset). Centre field at the axis midpoint. Flank
   fields at 90 degrees off the axis, one per `flank_lanes`. Chokes at each base-to-centre
   midpoint. Base discs paint `BASE`, field discs paint `FIELD` without overwriting.
2. **Lanes.** Thick corridors GDI to centre to Nod, and GDI to each flank field to Nod, with a
   smooth seeded perpendicular wobble that is zero at both ends. Paint `LANE` on open cells.
3. **Tiers.** Seeded Voronoi regions, seeds mirrored under the symmetry, each with a hashed
   tier. Base discs plus one cell are forced to the middle tier, the centre field to tier 0,
   flank fields flattened to their region tier. A bounded fixed-point pass pulls walkable
   cells (`LANE`, `BASE`, `FIELD`) down so no walkable 4-neighbour pair differs by more than
   one tier.
4. **Seams and ramps.** A cell whose tier differs from any 4-neighbour becomes `RAMP` if it
   was walkable, `CLIFF_SEAM` if it was open.
5. **Water.** Tier-0 `OPEN` cells hashed against `water_fraction`, kept only with two or more
   wet 4-neighbours so water pools rather than speckles.
6. **Checksum.** CRC over tiers, kinds, sites and dimensions. Logged and available to the
   manifest through `Get_Checksum`.

Every step is a fixed-order grid walk over `TerrainMaskClass::Hash_Value`; no RNG state.
Same seed and profile give the same plan on server and client.

## Realization

- **Macro terrain** (`MacroTerrainParams::UsePlan`, `PlanDetailWeight`): the vertex height is
  `Sample_Tier_Height` plus the noise relief scaled by `detail_weight`. `RAMP` cells drop the
  detail entirely, `BASE` and `FIELD` cells keep a quarter of it, so a lane crossing a tier
  stays under the buildable slope cap and a base disc is flat. `Sample_Tier_Height` blends
  tier heights bilinearly except inside a `CLIFF_SEAM` cell, which keeps its own tier so the
  step stays sharp within one chunk. The existing slope relaxation, cliff band lock and
  approach flattening run unchanged after that.
- **Interior cliffs** need no change: a seam cell realized as a sharp step is a chunk still
  steep after relaxation, which is exactly the candidate `InteriorCliffGenerator` already
  looks for.
- **Strategic candidates**: `Compute_Chunk_Scores` records `PlanBase` and `PlanField`
  (`Get_Base_Site_Weight`, `Get_Field_Site_Weight`) beside the terrain score. The Tiberium
  candidate ranking (step 5) skips planned base discs and adds `PLAN_FIELD_SITE_BONUS`
  (2.0) for planned field discs; the base ranking adds `PLAN_BASE_SITE_BONUS` (2.0); the
  settlement ranking uses neither. Eligibility, fairness gates and base selection are
  unchanged; the plan proposes, the planners dispose. `TiberiumFieldGenerator` needs no
  change: it resolves fields from the resource candidates, which now sit on the plan's
  fields.
- **Hydrology**: before the standalone-sink pass, every chunk whose centre lies in a plan
  `WATER` cell is offered to `Build_Basin_Feature` in index order, within the same
  `max_river_count` budget. Cliff band, sea and already-claimed chunks still refuse.
  Adjacent planned cells are allowed so a planned pool comes out as one body, not a
  speckle. Rivers and coastline are unchanged.
- **Roads**: `Compute_Route_Chunks` multiplies a `LANE` or `RAMP` chunk's cost by
  `PLAN_LANE_COST_SCALE` (0.5). Blocks, crossings and the boundary approach lift are
  untouched, so a corridor the terrain made impassable is still not routed through.
- **Bridges, tunnels, settlements, decoration**: unchanged owners reading terrain that now
  has planned basins, seams, flat sites and lane-following roads.

## Skips and failures

- `plan.enabled:false` or absent: `Build_Map_Plan` returns true with nothing built; the
  pipeline is the schema-10 noise-first pipeline.
- `world.fit_to_level:true`: skipped with a log line.
- Invalid params: `Generate` refuses with a reason and leaves no plan; the manager fails the
  attempt, which the retry loop reseeds like any other phase failure.
- `UsePlan` on `MacroTerrainGenerator` without a plan present is refused.

## Validation

`renegade -terrainselfcheck mapplan` (`terrain_mapplan`, `fds_terrain_mapplan`): refusals,
site counts, rotational symmetry, base tier and kind, centre field tier, lane and water
continuity, presence of ramps and seams, tier height sampling, determinism across
regeneration, distinct checksums across seeds, mirror and no-symmetry builds, and terrain
realization placing the planned base within 8 units of its tier height, site weights, and the
consumers on that realized terrain: a base candidate inside a planned base disc, a resource
candidate inside a planned field disc, none inside a base disc, and at least one water feature
when the plan holds any `WATER` cell.

## Not yet done

- No visual run of the native profile with the plan enabled has been made (D-002 gate still
  open). No self-check drives the whole `ProceduralWorldManager` pipeline; the plan-to-world
  integration is proven per owner (`terrain_mapplan` runs macro terrain, strategic planning
  and hydrology on one planned world) and not end to end.
