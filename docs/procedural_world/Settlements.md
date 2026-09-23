# Settlements, POIs, and the Section 28A Asset Registry (V2 Phase 13)

Roadmap Section 34 Phase 13 -- "settlements, abandoned buildings, industrial areas, and strategic
POIs." Section 14, Section 28, Section 28A. See `BaseSelection.md`/`TiberiumFields.md` for the
reserved regions this phase must not conflict with.

## Section 28A, real and reused -- not a parallel format

`ProceduralAssetRegistry` (`Code/Combat/proceduralassetregistry.h/.cpp`) is Section 28A's own
"reusable assets require machine-readable metadata... must not independently reverse-engineer the
same W3D geometry through bounding-box guesses," implemented by reusing Section 39's existing
`BuildingModelDescriptor`/`BuildingDescriptorManager` rather than inventing a second descriptor
format. `WorldMetricsClass::Get_Building_Footprint` already turns a descriptor's real, curated
width/depth into the circumscribing placement radius a generator needs; `ProceduralAssetRegistry`
adds only what Section 28A.4 asks for that Section 39 has no reason to carry (category, cover
classification, neighbour spacing, nav-obstacle flag).

## What is real, and how it was verified

Every W3D file this tree currently ships under `data/building_models/` was opened directly --
`Code/ww3d2/w3d_file.h`'s own `W3D_CHUNK_HIERARCHY`/`W3D_CHUNK_PIVOTS` layout, read offline the
same way item 4's Humvee width was -- and every pivot/bone name was dumped. The result: three
real building kinds exist with real geometry (`G_Comcntr.w3d`, `mgpwr_ext.w3d`, `mnpwr_ext.w3d`),
and **none of the three carries a bone named for an entrance, a door, or a pedestrian approach**
-- Section 28A.2's own connector list. `data/building_descriptors/gdi_communications_center.json`'s
own `mct_anchor`/`purchase_terminal_anchor` values (`MCT`, `PT01`) were already flagged in that
file's own comment as unverified; the dump confirms they are also simply absent from
`G_Comcntr.w3d`'s 93 real pivots. `ProceduralAssetDescriptor::Has_Entrance_Anchor` returns false
for all three, honestly, rather than orienting a building by a bone that does not exist.

Every other `data/building_descriptors/*.json` entry (`gdi_advanced_communications_center`,
`nod_advanced_communications_center`, `gdi_tiberium_silo`, `nod_tiberium_silo`) names a model or
part list `data/building_models/` has no files for yet, so none of them is registered.

**What this means for the actual settlements this phase places.** The registry has exactly three
real entries, all GDI/Nod Commander-structure geometry (a Comm Center, two Power Plants) -- there
is no real house, ruin, or industrial-decoration asset in this tree today. A generated settlement
is therefore a structurally real, correctly-placed, collision-checked arrangement of the *only*
real building geometry available, not evidence that GDI power plants belong in a civilian town.
Real `ow_`-prefixed settlement/decoration content is the open request this leaves, the same
"missing art never blocks engine implementation" pattern this project's own instructions state.

## Steps implemented for real

- **Step 1's conflict list**, via `Conflicts_With_Reserved_Ground`: `WorldBoundsService::
  Can_Place_In_Region` (boundary/cliff band, the one authoritative owner), both selected bases'
  own `BaseSelectionPlanner::ProtectedRadius` (Phase 11), and every resolved
  `TiberiumFieldGenerator` field's own radius (Phase 12), and every tunnel's footprint (V2 Phase 10,
  `TunnelSystem::Overlaps_Footprint`). Bridge approaches are not checked -- a bridge approach has no
  queryable region of its own this generator would have to re-derive from `RoadSystem` geometry.
- **Step 2**, layout type: a deterministic hash draw (`TerrainMaskClass::Hash_Value`) over the six
  named `SettlementLayoutType` values, stored for real per settlement.
- **Step 3**, road connectivity: `Nearest_Road_Distance` walks every already-tessellated
  `RoadSplineClass`'s own real points and reports the true nearest distance;
  `RoadConnected` is that distance within `RoadConnectorMaxDistance`. **Not implemented**: a new
  physical connector spline through `RoadSystem::Add_Road`. Building one safely needs the same
  integration `RoadGraphGenerator::Reroute_Edge_Avoiding` already uses to fold a new road into an
  already-built, already-junction-linked network without disturbing it; adding that integration
  for a settlement spur specifically is a real, disclosed simplification given this generator's
  own primary job is layout and placement, not road-network mutation.
- **Step 4**, structure placement: real instances from the registry, ring-arranged inside the
  settlement's own reserved footprint, cycling through every eligible real asset (so
  `AllowDuplicateAdjacent`'s own false default is honoured by construction), each candidate
  position checked against `WorldTerrainSystem::Query_Placement_Surface` (on-terrain, slope),
  `Conflicts_With_Reserved_Ground`, and every already-placed structure's own `MinNeighborSpacing`
  -- a position that fails is a structure not placed, never a fabricated one.
- **Step 9**, POI kind: a second deterministic hash draw over the six named `PoiKindType` values,
  stored for real. `HasRealAsset` is false for every one -- no `ASSET_CATEGORY_POI` entry exists.

## What is NOT resolved this phase

Steps 5-8 (building entrances facing streets, interior access, alleys, vehicle street clearance)
all depend on real per-asset connector/street-facing data this tree's own geometry does not carry
-- see "What is real" above. Step 5 specifically is structurally impossible to satisfy honestly
today, not merely unattempted. Live instantiation (a real `StaticPhysClass`/collision object) and
nav registration (steps 10-11) are not attempted -- the same "generator produces the record, a
later phase instantiates it" split `TiberiumFieldGenerator`'s own class comment already draws for
a Harvester. Debug bounds/entrance markers (step 12) are a rendering concern with no device here.

## What is proven, and what is not

`terrain_settlements`/`fds_terrain_settlements` (`-terrainselfcheck settlements`) chain the same
terrain/hydrology/strategic-plan/road-graph/bridge-plan/base-selection/Tiberium-field setup
`terrain_tiberiumfields` already proves, registering the three real descriptors via
`BuildingDescriptorManager::Load_Descriptor_From_Text` (the same "no file on disk" pattern that
class's own header documents for a self check, since `data/building_descriptors`'s relative path
depends on the test binary's own working directory) with the real, verbatim footprint values from
`data/building_descriptors/*.json`. It asserts: refusal reports a reason and leaves no state; a
real chain resolves at least one settlement with a real reserved region and a recognised layout
type; every placed structure's footprint is legal and no two overlap; every POI names a recognised
kind and honestly claims no real asset; and the same inputs reproduce the same checksum. Not
proven: a scenario engineered so `RoadConnected` is actually false (this terrain's own settlements
land close enough to the generated road network), and everything named under "What is NOT resolved"
above.
