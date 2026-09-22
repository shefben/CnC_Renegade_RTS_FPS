# Biomes, Materials, and Natural Decoration (V2 Phase 14)

Roadmap Section 34 Phase 14 -- "biomes, terrain materials, forests, rocks, and natural
decoration." Section 13, Section 10.6, Section 28.

## Real placement through the one existing system

`FoliageSystem::Define_Default_Foliage` (already called by `FoliageSystem::Init`) defines five
real, working foliage types -- `ow_tree_conifer`, `ow_bush_scrub`, `ow_tree_dead`,
`ow_grass_clump`, `ow_rock_boulder` -- each with real blocking/destructible/LOD-distance/
scale-range data, but (per that file's own comment) "none of them names a model... a forest of
them plans exactly the same batches and draws nothing at all" until there is art. `BiomeDecorationGenerator`
(`Code/Combat/biomedecorationgenerator.h/.cpp`) decides *where* those five real types belong and
places real instances through `FoliageSystem::Add_Instance` -- never `FoliageSystem::Scatter`,
whose own uniform-disc placement checks no terrain legality at all (confirmed by reading its own
implementation: it samples height and nothing else). Legality -- on terrain, dry, clear of the
road/Tiberium masks and every reserved region from Phases 11-13 -- is checked per candidate point.

## Masks already real, reused rather than re-derived

`TERRAIN_MASK_ROAD` is stamped by `RoadSystem`; `TERRAIN_MASK_TIBERIUM` by `TiberiumFieldGenerator`
(Phase 12); `TERRAIN_MASK_WATER_DISTANCE` by `WaterSystem::Update_Water_Distance`. Step 1's own
"moisture" biome input is that real water-distance mask, sampled directly and folded with a
deterministic hash (`TerrainMaskClass::Hash_Value`) so an otherwise-uniform ring around a river
does not become a uniform ring of forest. Slope, height and cliff-band exclusion are free: a
chunk survives eligibility only when `TerrainChunkInfoClass::Buildable` is already true, the same
`!CliffBand && !InteriorCliff && slope <= cap` guarantee every other generator in this chain
already reads rather than re-checks.

## Forest vs. rock, one real differentiated signal

A chunk near the top of the buildable slope range (`MaxSlopeRadians` close to the shared
buildable-slope cap) reads as scree/rock terrain rather than forest -- the same chunk-registry
data every other generator in this chain already reads, not a second guess at it. Trees mix three
of the five real types (70% conifer, 20% dead, 10% scrub) by a deterministic hash draw per
planting, not a uniform stand.

## Step 13's real acceptance

`FoliageSystem::Build_Cells`/`Build_Batches` are called for real after placement -- the existing,
already-working LOD/budget system, not a new one. `terrain_biomedecoration` asserts both actually
report built afterward.

## What is NOT resolved this phase

Street lamps (steps 9-10) are not placed -- no `ow_streetlamp`-equivalent asset exists in
`FoliageSystem`'s own registered types or `ProceduralAssetRegistry`, the same "missing art never
blocks engine implementation" gap Phase 13's own entrance-anchor problem already documents.
Terrain-material assignment through the v1 texture layer system (step 2) is not driven here --
`TerrainTextureSystem`'s own layer rules already read these same masks once layers are defined for
a level; this generator's job is populating real mask data, not authoring layer rules. Runtime
lighting/destructibility (step 11) needs no new code -- every placed type already carries real
`Destructible`/`Blocking` data from `Define_Default_Foliage`. Debug overlays (step 14) are a
rendering concern with no device here.

## What is proven, and what is not

`terrain_biomedecoration`/`fds_terrain_biomedecoration` (`-terrainselfcheck biomedecoration`)
proves the generator's own documented optional-dependency design for real -- it runs over nothing
but terrain and hydrology, no strategic plan/road graph/base selection required -- then asserts:
refusal reports a reason and leaves no state; a real chain places at least one real instance, and
the generator's own tree/rock counts add up to exactly what `FoliageSystem` holds; no instance
sits in the road mask or off terrain; `Build_Cells`/`Build_Batches` both report built afterward;
and the same seed/terrain/hydrology reproduce the same checksum. Not proven: placement against a
real strategic plan/base selection/settlement chain together (each reservation check is exercised
individually elsewhere, e.g. `Conflicts_With_Reservations` reusing exactly
`SettlementGenerator`'s own pattern, but not chained through this specific check), and everything
named under "What is NOT resolved" above.
