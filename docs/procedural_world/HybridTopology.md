# Hybrid rewrite I10 — runtime Phase 9, transportation topology

Owner of the plan's runtime Phase 9 (`Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md`
Section 8 Phase 9, Section 5, Section 12.2).

| Concern | Owner |
|---|---|
| The fourteen construction steps, the quotas, the gate, the checksum | `Code/wwphys/hybridtopology.{h,cpp}` |
| Section-12.2 unit-capacity edge-disjoint max-flow with `BASE_INTERNAL` contracted | `Hybrid_Topology_Max_Flow` (used by the cross-link score and, later, by Phase 19) |
| The exit-sector convention entrances are assigned from | `Hybrid_Exit_Sector_Direction` (Phase 8's, unchanged) |
| Entrance-corridor reservation and its `NO_DECORATION` stamp | written into the Phase-4 matrices, reusing Phase 4's reservation record |
| Profile adapter and the per-attempt record | `ProceduralWorldManager::Build_Hybrid_Topology` / `Get_Hybrid_Topology` |
| Gate self-check | `Check_Hybrid_Topology` in `Code/Commando/terrainselfcheck.cpp`, ctest `terrain_hybridtopology` + `fds_terrain_hybridtopology` |

## What the phase produces

`HybridTopology` holds nodes, edges and base entrances:

- **nodes** — the logical base node, one `BASE_ENTRANCE_POINT` per heavy family per base, the Phase-3
  family waypoints, one `CROSSING_OBLIGATION` per bound Phase-6 window, one shared
  `OVERPASS_OBLIGATION` per grade-separated road-over-road crossing, and one `FIELD_ACCESS_POINT` per
  frozen Tiberium anchor;
- **edges** — `BASE_INTERNAL` (zero cost, never realized, contracted in the max-flow), the family
  trunks, the home roads, the field service branches, the quota-filling service edges, the trails and
  the optional or synthesized cross-links. Each edge carries its road class and its ordered frozen
  waypoints, which is what Phase 10 routes its sub-legs through;
- **entrances** — the plan's `(base, family, sector ordinal, entrance point, entrance corridor)` tuple,
  with the corridor reserved against unrelated Tiberium and decoration.

It replaces the native MST road topology: `RoadGraphGenerator`'s minimum spanning tree is not
consulted on the hybrid path, and the authored/legacy path is untouched.

## Resolutions the plan leaves open

**"Non-base waypoint" is every node a road may end at.** A two-family map has exactly two Phase-3
family waypoints, so exactly one pair between them — which cannot fill a Section-5 trail quota of two
in any order. Crossing waypoints and field access points are waypoints in the same sense and join the
candidate pool for steps 9, 10 and 11; the "different heavy families" rule still refuses two nodes of
one family, which is the part of the rule that carries the intent.

**A bridge window with no far-bank waypoint binds to the nearest family chain.** Step 3/4's fallback
assumes a heavy node stands on the opposite bank. Often none does — a river or ravine near the map
edge leaves every heavy node on one side. Such a window is then bound to the nearest family chain, the
procedure a *preferred* window already gets, and only when it lies inside that family's own
`MaxCorridorHalfWidth(PRIMARY_2)`, so the binding cannot drag a route across the map. A window that
fails even that is set aside — one physical window is offered once — and the search moves on; the
quota fails only when every bridge-capable window has been tried. **Q-019** records the pass-rate
question this leaves open.

**Banks come from the window's own approach axis.** `ApproachA` and `ApproachB` are the two ends of the
crossing Phase 6 froze, so the sign of a node's offset along that axis says which bank it is on. The
obstruction's local tangent says nothing about a point far from the station.

**A base entrance is one of its family's nodes.** Both for the desired outgoing direction and for the
bank search, an entrance is the family's endpoint on that side of the map.

**Waypoint order inside an edge is along-edge order.** A crossing or overpass waypoint is inserted into
the edge whose straight segment passes nearest it, at the position that keeps the waypoint list ordered
from `NodeA` to `NodeB` — which is the order Phase 10 routes the sub-legs in.

## Two precision fixes this item required in earlier phases

Phase 8 measures a base and a field reserve geometrically, while Phases 5 and 6 were testing the
rasterized coarse flag. A cell is `S` wide and is flagged from its own centre, so a wall or a river
could reach half a cell into a reserve without ever standing in a flagged cell — and Phase 8 then
refused the whole attempt over ground the earlier phase could simply have avoided. Measured on five
MEDIUM seeds, that mismatch refused three of five worlds at Phase 8 with `resource_illegal`.

- Phase 5 now also rejects an immutable wall whose own samples fall inside a Tiberium or base reserve
  **disc**, not only inside a flagged cell.
- Phase 6 now keeps rivers, lakes and coast out of a Tiberium reserve disc. That is what makes Phase
  8's "the full `field_radius_max` reserve is dry" requirement satisfiable at all, and it follows
  Phase 4's own reservation ranks, where a Tiberium reserve is exclusive and water is not one of its
  compatible layers.

## Failure behavior

Every failure is a named attempt failure and leaves the record empty: `no_entrance_sector`,
`obligation_unbound`, `bridge_quota`, `no_field_access`, `service_quota`, `trail_quota`,
`overpass_quota`, `edge_budget` (more routed edges than `R` allows), `no_cycle` and `invalid_input`.
An optional cross-link shortage is explicitly **not** a failure.
