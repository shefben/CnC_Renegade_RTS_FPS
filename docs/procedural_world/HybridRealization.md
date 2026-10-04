# Hybrid Phase 12 -- roads on the ground

Runtime Phase 12 builds what Phases 9 to 11 froze. It does not reroute a road, move a crossing, change a
method or reopen an earlier phase; a failure fails the attempt (Plan Section 12).

Owners:
- `Code/wwphys/hybridrealization.{h,cpp}`: the arithmetic (the plan, the deformation, the solver).
- `Code/Combat/hybridroadrealizer.{h,cpp}`: the runtime half on the canonical owners (`WorldTerrainSystem`, `RoadSystem`,
  `BridgeSystem`).
- `ProceduralWorldManager::Realize_Hybrid_Roads`: runs both once per attempt on the native hybrid path.

Gated by `renegade -terrainselfcheck hybridrealization`. On that path Phase 12 is the sole runtime realization owner of
roads, bridge and overpass decks, junction surfaces and road cut/fill. The legacy strategic plan, road graph, bridge
and tunnel plans and base selection do not run there.

**Tunnels are deferred** (user, 2026-10-03). `Hybrid_Native_Tunnels_Available` forces `tunnel_available = false` in
Phases 6, 9 and 11, so Phase 11 skips the optional tunnel fallback and a road takes the plan's own
fallback/reroute/fail sequence. No tunnel is realized or adapted. Authored/legacy `TunnelSystem` is unchanged.

## Section 12.1 -- the vertical profile

`Hybrid_Solve_Vertical_Profile` is the only road centerline vertical-profile solver on the hybrid path. It
propagates forward, then backward, walks the clamp, quantizes and revalidates.

After both propagations the clamp walk's interval is never empty in exact arithmetic. A walk standing exactly on
a propagated bound can still overshoot it by a float rounding unit. An excess within `1e-5` of the height (far
below the `0.001` position quantum) takes the bound and is not a refusal. Step 4 still quantizes and revalidates
every grade.

`Hybrid_Solve_Road_Stretch` grades one stretch with every structure on it holding its own surface.

## The plan -- `Hybrid_Plan_Realization`

The plan holds every physical road in routing order (`Hybrid_Route_Priority_Rank`: the family trunks, then group
1b, then the rest; `StableId` within a group). A road is one of:
- a heavy family's whole trunk, once (Q-025);
- an interchange ramp or crossing edge;
- any other edge on its own.

Each road is realized by `Hybrid_Realized_Physical_Road` and graded once by Section 12.1 over its whole length:
- Its compiled structures (bridges, tunnels, overpass decks) hold their own surfaces.
- Every junction surface it shares holds it, as a level `2Q` patch:
  - an interchange local junction holds every road through `L_s` at its compiled `z_L`;
  - a ramp's gore holds the ramp at the highway's compiled surface there;
  - an overpass holds its lower road at the compiled `LowerSurfaceZ`;
  - any other junction at a topology node holds every road through that node at the surface the first road realized
    through it already has. An incidental junction holds the later road at the earlier road's surface.

Each solved sample carries its actual paved edges, shoulder included, left and right of the road's travel:
- **Symmetric classes:** the half roadbed.
- **Ramps:** `Hybrid_Ramp_Effective_Edges`, the D-022 gore transition and D-024's `E`. The inner edge is left of
  the ramp's traffic, which an `OFF_RAMP`'s stored `L_s -> gore` order runs against.
- **`HIGHWAY_4`:** its ramp side widened to the compiled merge/diverge `E(x)` over each parallel interval (12.6.11).

Samples on a deck or inside a bore are marked `Structure`.

The road also records the width and symmetric shoulder handed to `RoadSystem`: `0.25W` for a ramp (12.6.13 item
1). The asymmetric/effective edges are what the canonical owner's optional per-sample edge profile receives.

## Bridges -- `BridgeAssembly`

Every compiled bridge and overpass deck is built as **CompiledBridge -> `HybridBridgeAssembly` -> ordered
`HybridBridgePiece` records**:
- Phase 11 chose the logical family (`BridgeKind`) and the structural requirements.
- Phase 12 owns the physical assembly.

This version builds every assembly from exactly one `WHOLE` piece, the existing single bridge/deck realization.
The kinds `START_ABUTMENT`, `END_ABUTMENT`, `CENTER`, `SHORT_CENTER`, `SUPPORT`, `DAMAGED_TRAVERSABLE` and `BROKEN`
are reserved for a later modular family. Navigation, route independence and gameplay see one assembly as one
logical crossing. Piece ids and transforms are ordered and part of the stage checksum, and every piece seam is a
swept-validation sample.

## The road owner's edge profile

`RoadSplineClass` has an optional per-sample asymmetric edge profile (`RoadEdgeProfileSample`). It holds extra half
width for the carriageway and for the shoulder on each side, by distance along the road as authored.
- **When it is read.** Tessellation reads it once onto every `RoadPointClass` (`ExtraLeft`, `ExtraRight`,
  `ShoulderExtraLeft`, `ShoulderExtraRight`). A trim interpolates it at the cut.
- **Who uses it.** It is consumed by:
  - the ribbon (each edge at its own half width);
  - the junction polygon corners;
  - the road mask (`TerrainMaskClass::Stamp_Polyline_Asymmetric`);
  - grade stamping (the widest extent, `Get_Widest_Total_Width`);
  - the on-road query (per side).
- **Default.** An empty profile is the default and changes nothing, so every existing road is unchanged.

Phase 12 hands the canonical owner its nominal width and symmetric shoulder, and expresses the asymmetric and
effective edges as this profile.

## Section 12.2 -- the deformation, `Hybrid_Deform_Terrain`

The deformation runs over a height grid, road by road in plan order:
1. The roadbed and shoulder are set to the solved height, out to the sample's actual edges on each side (asymmetric
   where the road is).
2. A smoothstep blends from the paved edge to the terrain over exactly `outer_blend = max(W, A/2)`.
3. Nothing is changed under a deck, inside a bore, or in water.
4. An earlier road's roadbed and shoulder are never changed. The earlier road owns that surface, including a
   junction's shared patch, so a later road's blend (a ramp's at its gore) never modifies it.

Steps 6-9 of Section 12.2 (the ditch helper if one exists, commit, realization, cleanup list) belong to the runtime
owners.

## Junction surfaces (12.2 step 5)

A road-road junction's shared patch is processed once, under the earlier road:
- Wherever a later road's centerline runs over an earlier road's roadbed, it is graded to that road's realized surface,
  sample by sample (`Add_Shared_Surfaces`). The stretch runs 1.5 terrain cells past the roadbed edge, so the grid
  vertices it is interpolated from hold one surface.
- The deformation keeps the earlier road's roadbed vertices.
- An interchange local junction holds every road through `L_s` at its compiled `z_L`.
- A ramp's gore holds the ramp at the realized highway's surface there (12.6.10 item 4).
- An overpass holds its lower road at the compiled `LowerSurfaceZ`.

Two refinements to the deformation:
- The roadbed test uses the true distance to the road (a round cap at road ends, never a band along an end segment's
  line).
- A deck's two abutment seats (one and a half grid cells inside each end) are graded with the road, so the road's last
  point stands on the deck's own end height. The rest of the span stays untouched.

## Runtime realization -- `HybridRoadRealizer`

The runtime half runs in this order:

1. **Terrain.** `Realize` runs `Hybrid_Deform_Terrain` over the `WorldTerrainSystem` field and commits it through
   `WorldTerrainSystem::Set_Heights`.
2. **Bridges.** One `BridgeSystem` bridge per `BridgeAssembly`, from its single `WHOLE` piece: the compiled ends and
   width, the Phase-11 kind's definition, and `DeckHeight` 0 so the deck follows its ends exactly.
3. **Roads.** One `RoadSystem` road per stretch of a planned road between its decks:
   - nominal width and symmetric shoulder (a ramp's `W` / `0.25W`);
   - the asymmetric and effective edges as the road's edge profile;
   - no RoadSystem grading, since the ground already carries the profile;
   - ends handed to their decks (`ROAD_ENDPOINT_BRIDGE`, bound both ways);
   - a ramp's gore end attached to its highway (`ROAD_ENDPOINT_ATTACHED`, which RoadSystem never discovers as a tee).
4. **Network.** `RoadSystem::Build_Network` and the bridge layout and geometry.

Every created id is on the cleanup list. `Clear` removes them in reverse order, keeping anyone else's objects.

**`Verify`** (12.3's record half) checks:
- realized exactly once, with every stretch and assembly once and nothing added;
- every road point on its frozen centerline at its solved height, within `tan(grade) * cell + 2Q`;
- every bridge at its assembly's ends, width and kind, bound at both ends;
- no RoadSystem-discovered junction inside an active gore reserve.

The manager runs it after realization, and again (the live gate) just before road geometry is built.

## Migration adapters

`Publish_Adapters` translates Phase 12's records into the legacy planners the downstream systems still read. It
creates, moves and owns nothing:
- **`StrategicPlanner::Adopt_Plan`:**
  - the frozen bases (`BASE_CANDIDATE`) and fields (`RESOURCE_CANDIDATE`);
  - each bridge's crossing node (`CROSSING_CANDIDATE`);
  - every road node as `STRATEGIC_NODE_ROUTE_POINT`, a new adapter-only type with no gameplay role.
- **`RoadGraphGenerator::Adopt_Graph`:**
  - each realized logical edge, split at its river/canyon bridges into bank-to-bank deck edges;
  - each base's internal link and each field's apron link (no road);
  - every road edge names its `RoadSystem` stretch.
- **`BridgePlanner::Adopt_Bridges`:** each river/canyon bridge, pristine, counting as connectivity. Overpass decks are
  Phase-12 objects but are not "bridges" to these consumers.
- **`BaseSelectionPlanner::Adopt_Selection`:** Phase 8's frozen base pair.

Base selection, Tiberium, combat-space and transportation validation, navigation, the minimap, logistics, the Commander
AI and placement rules read these. They stay until each consumer is migrated by its own item (checklist 22).

## Checksum

`Hybrid_Realization_Checksum` is the stage-12 checksum. It covers:
- every road's edges, class, tier, capabilities, group, widths, and each sample's position, station, height, edges and
  structure flag;
- every assembly and its ordered pieces.
