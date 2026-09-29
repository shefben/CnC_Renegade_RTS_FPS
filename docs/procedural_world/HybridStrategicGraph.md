# Hybrid rewrite I4 — runtime Phase 3, strategic gameplay graph

Owner of the plan's runtime Phase 3 (`Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md`
Section 8 Phase 3, Sections 2.1, 5, 6.1 and 6.10).

| Concern | Owner |
|---|---|
| Role placement, legality repair, route families, obligations, intents, gate, checksum | `Code/wwphys/hybridstrategicgraph.{h,cpp}` |
| Site/edge records (extended for this item) | `Code/wwphys/hybridplanrecords.h` — `HybridStrategicSite::RouteFamily`, `HybridStrategicEdge::RoadClass` |
| Placeable-boundary query | `WorldBoundsService` (unchanged; Phase 3 only consumes it) |
| Profile/identity adapter and per-attempt lifetime | `ProceduralWorldManager::Build_Hybrid_Strategic_Graph` |
| Gate self-check | `Check_Hybrid_Strategic_Graph` in `Code/Commando/terrainselfcheck.cpp`, ctest `terrain_hybridstrategicgraph` + `fds_terrain_hybridstrategicgraph` |

Same split as I2/I3: the phase itself is pure over `(HybridStrategicGraphParams, HybridMapIdentity,
HybridArchetypeIntentSet)` plus the configured world boundary, and Combat fills the params from the
validated profile and the frozen preflight metric cache. Phase 3 runs once per attempt in
`Generate_World_Attempt`, after `Configure_World_Bounds` (its reserves are tested against that
boundary) and after Phase 2 (every guide it reads is frozen there).

## What Phase 3 produces

`HybridStrategicGraph` — `HybridStrategicSite` records in the plan's creation order
(`GDI_BASE`, `NOD_BASE`, `HOME_TIBERIUM_GDI`, `HOME_TIBERIUM_NOD`, `FRONT_CENTER`, contested
fields 1/3/5, remote-rich fields 1/1/2, route-family waypoints), `HybridStrategicEdge` intents
(one complete GDI→Nod chain per heavy family, base→home-field, non-home field→attachment point),
`HybridCrossingObligation` records for every archetype guide tagged bridge/distinct-crossing/pass,
and the Phase 3 gate checksum.

Reserve radii are exact: base `R_BASE_PROTECTED`, resource `resource.field_radius_max`, route and
analysis waypoints `0`. The only keyed draw in this phase is the remote-rich field's `u`
(`Hash01(map_plan,"remote_rich_u",index)`); every other position comes from the frozen Section 6.1
anchors, the fixed Phase-3 guide table or the frozen Section 6.10 intents.

## Decisions this item had to make

**Only *frozen* intents are hard regions.** Phase 3's role rules name water and immutable-ground
intents, but Phase 2 freezes real geometry for exactly two of them: the guided-river centerline
spline and the coast's sea boundary `v` on the drawn side. Section 6.10 says as much for the
river — it is never moved around bases later, so the role moves instead. Every other table entry
(lake, basin, canyon, ridge, ravine, peninsula neck, `OPEN_MANEUVER` disc) is a guide with a
*target* size whose polygon does not exist yet, and the phases that realize them are required to
keep clear of what Phase 3 froze: Phase 5 rejects a landform candidate intersecting
`BASE_PROTECTED` or a Tiberium hard reserve, Phase 6 does the same for water, and Phase 8 fails
the attempt when realized terrain violates a role. Treating the target radii as hard now would
also contradict the archetypes: a Section 6.3 lake radius of `0.18D..0.25D` centred at `v=±0.38`
always covers the strategic centre, while Section 6.3 simultaneously requires a dry central
corridor there — only the shaped Phase-6 polygon can deliver both.

**`PRINCIPAL_RIDGE` is not hard even as geometry.** Phase 3's base rule lists canyon/ravine/cliff,
not ridge, and a ridge is crossed at the mandatory `SURFACE_SADDLE` rather than blocking a role.

**The river-adjacent contested field is tested against the whole centerline.** Section 6.10 fixes
the candidate offset at `0.5*river_width + FieldRadiusMax + 7W` from the station being walked, but
Section 6.2 states the requirement globally — the field's nearest edge lies `4W..10W` from the
principal river bank. On a tight meander the *next* bend can be nearer than the station normal, so
a candidate whose reserve comes within `4W` of any part of the frozen spline is not hard valid and
the station walk continues. Without that test a station on the inside of a bend put the field in
the water it is meant to sit beside (measured: `-8.8` world units of bank clearance before the fix).

**Crossing obligations come from intent tags, never from the archetype name.** A
`ROUTE_FAMILY_CROSSING` intent tagged `BRIDGE_REQUIRED` becomes `RequiredMethod = BRIDGE`; one
tagged `DISTINCT_CROSSING_REQUIRED` becomes `ANY` with `DistinctCrossingRequired`; the single
`SURFACE_SADDLE` becomes family 0's `PASS` obligation on the ridge. The obstruction kind is mapped
once from the resolved archetype (`river_valley`→RIVER, `twin_lakes`→LAKE,
`canyon_crossroads`→CANYON, `highland_pass`→RIDGE, `coastal_peninsula`→COAST,
`plateau_basin`→CLIFF for the basin rim, `ravine_network`→RAVINE, `open_frontier`→none), so Phase 9
matches on the kind. Untagged crossing guides (river_valley, twin_lakes) produce no obligation, as
the table intends.

**`BASIN_ENTRY` counts as an archetype family guide.** Section 6.10 gives `plateau_basin` two basin
entry points carrying route families 0 and 1, and Section 6.7 requires two independent descents, so
they replace those families' default guides. They carry no bridge/distinct/pass tag, so they create
no obligation.

**Waypoint records carry their family; the `SERVICE_DIRT_2` branch carries its class.** Resource
attachment needs a waypoint's family for the `(quantized distance, family_id, waypoint StableId)`
tie order, and Phase 9 step 8 expects the Phase-3 branch intent to already be `SERVICE_DIRT_2`.
Both are one new field on the existing I1 records rather than a parallel table;
`HYBRID_ROAD_CLASS_COUNT` on an edge still means "Phase 9 assigns the class", which is the normal
case.

**A waypoint's placeability exemption belongs to its guide, not to the repaired position.** The
plan lets a route waypoint sit on its own explicit crossing/pass intent — that is what puts a
mandatory bridge guide on the water. Once the repair search moves it off that guide it is an
ordinary point and must be placeable.

## Known attempt-failure modes

`coastal_peninsula` can fail Phase 3 with `NO_LEGAL_RESOURCE_POSITION`. Section 6.6 draws the sea
boundary at `|v| = 0.55..0.70` on a drawn side while the fixed Phase-3 table puts the first
remote-rich field at `v = +0.70*s`; when both draws land on the same side and the sea reaches its
innermost legal position, no candidate inside Section 6.1's `0.10D` repair budget is dry. The plan's
answer is a named attempt failure, not a wider search, and `Generate_World` retries with a new seed
(which redraws the coast side, the flank sign and, on `archetype=auto`, the archetype). The
self-check asserts that this is the only archetype allowed to fail and that a failed attempt leaves
no partial graph.

## Validation

`terrain_hybridstrategicgraph` / `fds_terrain_hybridstrategicgraph` cover: Section 5 role counts at
all three size classes; the gate (role counts, one complete GDI→Nod chain per family, one attachment
plus one `SERVICE_DIRT_2` branch per non-home field, a cycle rather than a tree); the measured
separations (base↔base, resource↔resource `2*FieldRadiusMax`, resource↔`BASE_PROTECTED`); the
`0.10D` repair cap; identical inputs giving an identical checksum; every archetype's obligation set;
the Section 6.2 bank clearance for the river-adjacent field; and, with a configured boundary, that
every accepted role clears it by its full reserve radius plus the profile margin.

`terrain_native_bridges` exercises the whole attempt path with Phase 3 in it (8 seeds × up to 3
attempts) and stays green.
