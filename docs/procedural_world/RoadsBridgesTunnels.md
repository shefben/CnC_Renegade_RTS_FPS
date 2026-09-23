# Roads, Bridges and Tunnels

Section 33 required document. Roads and bridges are covered in full by their own phase docs, and
this file points at them rather than copying them:

- **Roads:** `RoadGraph.md` (V2 Phase 8, `RoadGraphGenerator`, realized through `RoadSystem`).
- **Bridges:** `BridgePlanning.md` (V2 Phase 9, `BridgePlanner`, realized through `BridgeSystem`).

The rest of this file is **tunnels, caves and underpasses** (V2 Phase 10, Sections 12 and 12A),
laid out to the Section 33 per-feature template.

## Ownership

| Responsibility | Owner |
| --- | --- |
| Where tunnels go (candidates, sizing, legality, retries, road handoff) | `TunnelPlanner` (`Code/Combat/tunnelplanner.*`) |
| Tunnel data, module chain, geometry, headwalls, lights, portal cuts, interior bounds | `TunnelSystem` (`Code/wwphys/tunnelsystem.*`) |
| Heightfield holes | `WorldTerrainSystem::Set_Cell_Hole` (stock `QF_HIDDEN`) |
| Cliff-face cuts | `EscarpmentSystem::Add_Portal_Cut` |
| Nav entrance/interior/exit | `NavMeshGenerator` (sector graph), `WorldTraversabilityGraph` (`TRAVERSE_EDGE_TUNNEL`) |
| Placement/mutation exclusion | `GeneratedWorldPlacementRules`, via `TunnelSystem::Overlaps_Footprint` |

This is the same split the bridge already has: one geometry owner in wwphys and one planner in
Combat. Only `TunnelPlanner` adds tunnels. No second tunnel path exists.

## Inputs

- The chunk registry and heightfield, via `WorldTerrainSystem::Sample_Height`/`Sample_Slope`.
- **Candidate source 1:** `BridgePlanner`'s diagnostics. Every crossing the bridge chain finally
  left unresolved becomes a road tunnel candidate. This is the bridge chain's own fallback
  stage 3, "alternate crossing type".
- **Candidate source 2:** `StrategicPlanner`'s `STRATEGIC_NODE_TUNNEL_CANDIDATE` nodes that no road
  edge passes through. Each one is an infantry flank or cave shortcut through the interior cliff
  that produced it, and its axis runs across that escarpment's face.
- **Class envelopes:** `WorldMetricsClass::Get_Class_Metrics` for infantry, wheeled and tracked,
  plus the profile's `min_vehicle_lane_clearance` and `max_buildable_slope_degrees`
  (`ProceduralWorldManager::Build_Tunnel_Plan`).
- `WorldBoundsService`, `WaterSystem`, and the strategic base candidates.
- The profile's `world_features.tunnels` switch. `false` produces no tunnels and is not an error.

## Classes (Section 12A.1)

| Class | Produced from | Envelope |
| --- | --- | --- |
| `VEHICLE` | unresolved highway/primary/secondary road crossings | widest and tallest loaded ground vehicle, never narrower than the road; lane clearance each side and overhead; gentlest climb limit |
| `MIXED` | unresolved street/service/track crossings | the vehicle envelope plus one infantry corridor |
| `INFANTRY_ONLY` | cliff bypasses through escarpment, ravine and quarry faces | two infantry corridors (passing and fighting room), a body radius each side, a step overhead |
| `CAVE_SHORTCUT` | cliff bypasses through mountain faces and canyon walls | infantry envelope, `cave_rock` module family |
| `UNDERPASS` | nothing yet (see gaps) | inherits the vehicle envelope and the carried road |

If a class's metrics did not resolve (no definitions loaded), its clearance is left invalid and
every candidate of that class is refused as `no_clearance_data`. The planner never builds a
bore to a guessed size.

## Seed derivation

`WorldSeedService::Derive_Subsystem_Seed(profile.Seed, SEED_DOMAIN_TUNNELS)`. The seed is used
for one thing: which side a flank tries first when repositioning. Everything else is a pure
function of the terrain, the candidates and the params.

## Hard constraints (Section 34 Phase 10 step 2, Section 12.4)

| Constraint | How it is checked |
| --- | --- |
| Open ground at both portals | Walk out from the barrier along the axis. The first run of passable slope, `MinPortalStraight` long and a full roof clearance below the highest ground crossed, is a portal. A flat ridge crest never qualifies. A high bench falls back to its first passable run. |
| Grade | `|Δz| / plan ≤ tan(MaxGrade)`. The high side is lengthened until it fits, bounded by the bore length. |
| Real overburden | Somewhere along the bore the ground is at least `MinCover` above the roof. Otherwise the refusal is `no_overburden` (a ramp or bridge case, not a tunnel). |
| Continuity | The ground never drops more than half a bore height below the floor between the portals. |
| Boundary | `TunnelSystem::Breaches_Boundary` checks the centre line and both outer edges, wing walls included. `Segment_Crosses_Hard_Boundary` must be false and no sample may reach `WORLD_ZONE_CLIFF_BAND`. |
| Protected base zones | Distance to every base candidate must be at least the base-selection footprint radius (`sqrt(min_base_expansion_area / π)`, the same formula) plus the outer half-width. |
| Major water | Water at the barrier point, or anywhere the bore is open to the sky (approaches and mouths), is refused. |
| Other bores | The plan-view centre lines must be at least both outer half-widths plus a wall apart. |
| Length | `MaxBoreLength` defaults to the longest bridge span (`BridgePlannerParams::MaxSpanLength`). |

## Soft constraints and bounded fallback (Section 3C)

- A **road tunnel** is tried once. It has to stay on its road, and a sideways move would take it
  off.
- A **flank** is retried at deterministic sideways offsets along the cliff: 0, ±1, ±2 ×
  `RepositionStep` (8 units). The first sign comes from the seed. Retries are bounded by the
  profile's `max_alternate_candidates_per_feature`.
- After retries run out the candidate is refused. The reason is recorded as a
  `TunnelResolutionRecord`, logged, and shown by `tunnel_status`. For a road candidate the road
  simply stays as it was, climbing over the barrier on the surface. Whether the world is still
  acceptable is `TransportationNetworkValidator`'s decision, as it already is for an unbridged
  crossing.

## Outputs

- `TunnelSystem` tunnels:
  - straight bore, `Start`/`End` floor points;
  - module chain joined socket to socket (`Layout_Tunnel`);
  - portal stations (where cover begins);
  - headwall tops;
  - up to 4 interior light anchors.
- **Geometry**, one `StaticPhysClass` per module plus one per headwall. The authored model is used
  where its declared clearance fits; otherwise the procedural tube (see
  `docs/assets/TunnelModules.md`).
- **Terrain holes.** A cell is opened when it lies inside the outer footprint and its surface is
  within the tube or less than `MinCover` above the roof. Cover and the ground beside the bore
  stay closed. `TunnelSystem` remembers exactly which cells it opened and only closes those.
- **Cliff cuts.** A disc at every point where the bore's centre line crosses an escarpment
  polyline. The face strip is not built inside the disc, and the headwall stands there instead.
- **Road handoff (step 14).** `RoadSystem::Trim_Road_Range` cuts the road at both portals. Both new
  ends get `ROAD_ENDPOINT_TUNNEL` with `TargetID` set to the tunnel id, so
  `TransportationNetworkValidator` counts them as bound.
- **Nav (step 12, Section 12.5):**
  - `NavMeshGenerator` registers an interior sector (the bore's interior bounds) linked by an
    entrance portal and an exit portal to the nearest walkable chunk outside each mouth, and
    records the chunk slots on the `ResolvedTunnel`.
  - `WorldTraversabilityGraph` adds a `TRAVERSE_EDGE_TUNNEL` between the two mouth cells. It does
    this only for a body the class admits (ground vehicles only in vehicle-carrying classes) and
    whose width and height `Crossing_Carries` accepts.
- **Interior bounds (step 11):** `TunnelSystem::Find_Tunnel_At` and `Conform_Point` (floor height,
  only for a caller actually down in the bore).
- **Minimap:** one `MINIMAP_MARKER_TUNNEL_PORTAL` per mouth.
- **Manifest:** `SelectedTunnelCandidateIds` lists the candidate id of every tunnel actually built.
- **Debug (step 15):** `debug_layer tunnels 1` draws each bore's outline and roof line, its mouths,
  portal-station risers and light anchors. Vehicle-rated and infantry bores use different colours.
- **Console:** `tunnel_status`.

## Runtime invalidation dependencies

- `TunnelPlanner::Clear` (called from `ProceduralWorldManager::Destroy_World`, in reverse build
  order) destroys the geometry and lights, closes exactly the cells and cliff cuts it opened, and
  refreshes terrain collision and cliff faces where a scene exists.
- Terrain holes dirty their patch. `WorldTerrainSystem::Update_Collision` refills it and
  re-dresses its materials.
- The tunnel footprint (mouths, approaches and covered bore) is a
  `GENERATED_WORLD_EXCLUSION_TUNNEL` zone. Commander placement is refused there
  (`PLACEMENT_ON_TUNNEL`), and so is every world operation except `RESTORE_REGION`. A crater or a
  foundation grade over the bore could otherwise open its roof.
- Tunnels have no damage state. Nothing at runtime changes a tunnel, so the nav and
  traversability built at generation stay valid.

## Save / network representation

No separate tunnel state is saved or sent. A tunnel is a deterministic function of
`(seed, profile, generator version)` and the terrain the earlier phases produced from them, like
every other generated feature here. A loaded or late-joining world regenerates it identically.

- `TunnelSystem::Get_Checksum` covers class, quantized endpoints, bore and module count, plus the
  opened-cell count.
- `TunnelPlanner::Get_Checksum` folds that together with every candidate's outcome.
- `SelectedTunnelCandidateIds` in the manifest names which candidates became tunnels.

The `terrain_tunnels` self check proves the same candidate on the same ground realizes the same
checksum.

## Validation

`-terrainselfcheck tunnels` (ctest `terrain_tunnels` / `fds_terrain_tunnels`) checks:

- **Portal placement.** On a ridge, both portals are on the valley floors, never the crest, and
  the bore is the class envelope plus both margins.
- **Step ramp.** On a 20-unit step, the ramp is lengthened on the high side to the class grade.
- **Refusal reasons.** `no_clearance_data`, `no_overburden`, `boundary`, and a too-short bore
  limit.
- **Module chain.** The module count is correct, every module starts at the previous one's exit
  socket, and the chain ends exactly at the far portal. A module with no declared clearance never
  claims a model.
- **Layout.** Portal stations are in order, light anchors number 1 to 4, and the headwall rises
  above the roof.
- **Holes.** Approach ground is opened while the crest and the ground beside the bore stay closed.
  An opened cell reaches the patch as `QF_HIDDEN`. A ray through it misses while a ray beside it
  hits. Removal closes every hole and cliff cut.
- **Cliff cuts.** Exactly one cut, where the bore crosses a face.
- **Placement footprint.** The mouth and the ground over the bore are refused, and ground 36 units
  beside it is not.
- **Determinism.** The checksum is identical on a re-plan.
- **Boundary.** `Check_No_Boundary_Breach` is 0.
- **Road handoff.** A road over the ridge is split, both new ends hand over to the tunnel, the
  bore is no narrower than the road, and `Check_Road_Tunnels_Bound` is 0.

The heightfield collision paths that ignored `QF_HIDDEN` (`Cast_OBBox`, `Intersect_OBBox`,
`Collide_Quad`) now honour it, matching the stock paths that already did.

## Disclosed gaps

- **Settlement underpasses and strategic alternate routes produce no candidates.**
  - The road graph is a tree with no grade-separated crossings, so no underpass is needed.
  - Nothing downstream of this phase feeds a route request back before it runs. Base selection's
    `add_tunnel_candidate` and combat-space's `tunnel_candidate` repairs are recommendations
    (names) today, not re-plans.
  - Both sources are named in `TunnelCandidateSourceType` and counted as zero.
- **Straight bores only.** Module metadata carries turn and grade limits (12A.2), but the planner
  lays the straight line between the two portals. Bend radius is recorded, and a straight line
  never violates it.
- **No authored module art yet.** Every module is the procedural tube; see the asset doc.
- **One sector graph.** An infantry-only bore is linked into the `PathfindClass` sector graph only
  when that graph is not also sized for ground vehicles (Phase 16's single-graph limit).
  `WorldTraversabilityGraph`, which is built per locomotion class, has no such limit.
- **Tunnels are indestructible.** No damage states, so no runtime nav invalidation is needed. Not
  a roadmap requirement.
- **Not verified without a live client:**
  - an infantry actor and a vehicle actually driving through;
  - portal collision snags;
  - AI entering and exiting;
  - how the procedural tube and headwall look.

  Needs `renegade --gamedir "C:\Westwood\Renegade_full"`, then **Procedural Skirmish**, then
  `tunnel_status` and `debug_layer tunnels 1`, then walking and driving each listed bore.
