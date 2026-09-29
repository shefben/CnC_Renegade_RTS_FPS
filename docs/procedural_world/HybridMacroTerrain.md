# Hybrid rewrite I8 — runtime Phase 7, macro terrain realization

Owner of the plan's runtime Phase 7 (`Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md`
Section 8 Phase 7, Section 10.1).

| Concern | Owner |
|---|---|
| The height expression, the detail-noise primitive, the protection factor, the river walk, the gate | `Code/wwphys/hybridmacroterrain.{h,cpp}` |
| The heightfield itself and its chunk registry | `WorldTerrainSystem`, unchanged |
| The one write (create, fill, relax, registry, masks, collision, far terrain) | `MacroTerrainGenerator::Generate`, now with a hybrid mode |
| Frozen river water surfaces and bed heights | `HybridMacroTerrainSet::Rivers`, filled by `Hybrid_Resolve_River_Surfaces` |
| Profile adapter, gate invocation and the per-attempt record | `ProceduralWorldManager::Build_Macro_Terrain` / `Get_Hybrid_Macro_Terrain` |
| Gate self-check | `Check_Hybrid_Macro_Terrain` in `Code/Commando/terrainselfcheck.cpp`, ctest `terrain_hybridmacroterrain` + `fds_terrain_hybridmacroterrain` |

## Why the formula and the write are separate owners

The plan requires the native macro geography noise channels to be *replaced*, not joined, and it
requires the heightfield to be written exactly once. Both are true at the same time only if the
single existing writer keeps its job: `MacroTerrainGenerator` still creates the heightfield, writes
it, relaxes it, builds the chunk registry once and builds collision and the far-terrain
representation. What changes is where each vertex's height comes from: a request carrying the Phase
4-6 records (`MacroTerrainParams::UseHybrid`) takes `Hybrid_Macro_Sample_Height` and never touches
`Sample_Elevation`'s four noise channels or the D-003 plan path, which stay exactly as they were for
an authored/legacy (plan-disabled) world.

## The height expression

```text
tier_height  = sea_level + (base_elevation_tier(p) - floor((tier_count-1)/2)) * tier_step
bias         = macro_height_bias(p) + island_macro_height_bias(p)      // blended across 2W
landform     = sum of accepted Phase-5 fields at p                    // Hybrid_Landform_Height
carve        = Hybrid_Macro_Water_Carve(..., base)                    // base = tier_height + bias + landform
n0           = HybridValueNoise2D(p, max(feature_scale, 8S), 0)
n1           = HybridValueNoise2D(p, max(feature_scale/3, 4S), 1)
detail       = detail_weight * tier_step * 0.35 * (0.70*n0 + 0.30*n1) * detail_protection_factor(p)
height       = clamp(base - carve + detail, sea_level+min_elevation, sea_level+max_elevation)
```

`detail_protection_factor` is the plan's list with the minimum taken where rules overlap: zero in a
base core, 0.25 in a Tiberium reserve, zero inside standing water, zero out to half a river's local
width, a smoothstep across each river bank blend `max(river_width, 2W)`, and a smoothstep across `2W`
outward from a lake or sea shoreline.

## Resolutions the plan leaves open

**The detail-noise draw has four keys, the shared helper takes three.** The plan writes
`Hash01(terrain,"detail_noise",octave,ix,iy)`. The octave folds into the named key with the standard
golden-ratio odd constant, leaving `ix` and `iy` as the lattice keys the plan spells out, so the two
octaves are independent draws and the lattice keying is unchanged.

**The mouth is fixed before the upstream walk, not before the downstream one.** Steps 2 to 4 pin the
mouth to its sink level, walk downstream taking the minimum, then walk upstream taking the maximum
"from the fixed mouth". A downstream minimum walk can lower the mouth it was just given, so the pin
is re-applied after step 3; step 4 then raises the reach above it. Without that, a river bound to a
lake could end below its own lake.

**The water carve is a bed target, not a fixed depth.** Phase 6's `Hybrid_Water_Carve_Depth` answers
"how deep is the water here" and stays what it is. Phase 7 needs the ground taken *to* the frozen bed
height from the longitudinal walk, so `Hybrid_Macro_Water_Carve` returns `base_height - bed`, blended
out across the bank. That is what makes a river bed follow its own surface downhill instead of
tracking whatever the land happened to be doing.

**A coast is cut to sea level.** `WaterSystem::Define_Default_Water` gives ocean and coast a zero bed
depth, and the plan names `WaterDefinitionClass::BedDepth` as the only carve source, so the sea floor
is cut exactly to the water surface. The shoreline then lands where the Phase-6 polygon says it does
without inventing a depth the water definition does not carry.

**Protected profiles are excluded from relaxation, not from the write.** The ordinary-slope pass runs
only outside explicit cliff/canyon/ravine walls, river beds and banks, and lake/sea polygons and
their shoreline band (`Hybrid_Macro_Is_Protected_Profile`). Those vertices are still written with
their computed height; they are simply never a term in the relaxation loop's arithmetic, which is how
a vertical-faced cliff survives a pass whose whole job is to flatten steep ground.

**"Still represented" is a sign test against the tier reference.** The gate checks each Phase-5
feature at its own mid-sample: a hill or ridge must stand above `tier_height + macro_bias` there, a
canyon or ravine floor below it. A probe the water carved, or one the elevation clamp pinned, is
skipped — water outranks relief in the plan's own formula, and the water record is checked on its own
terms in the same pass.

**The gate re-checks window grade on real ground.** Phase 6 retained each window from the
*provisional* surface. After the heightfield exists the same `PRIMARY_2` grade rule is measured on the
written ground, so a window that detail noise or relaxation pushed out of reach fails the attempt here
rather than in Phase 11's routing.

## Failure behavior

Every failure is a named attempt failure and leaves the record empty: `bed_above_surface` (the global
elevation floor would lift a bed above its own water, or realized ground stands above a water
surface), `river_not_monotonic`, `elevation_limit`, `feature_lost` (a Phase-5 feature is no longer
represented, or a window's approaches no longer connect within `PRIMARY_2` grade) and
`invalid_input`.
