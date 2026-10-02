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

## Required navigation capabilities

Section 12.1 has every road record store its *required navigation capabilities*. Phase 9 sets them on
`HybridTopologyEdge::Capabilities` when it creates the edge, because that is the only place the edge's
role is known, and Phase 10 copies the mask unchanged into `HybridRoute::Capabilities`. Both checksums
include it. Nothing downstream re-derives it, and nothing reads the road class to decide it: class sets the
cross section, grade, cross slope, turn radius and render tier, while which vehicles an edge must carry is
set by what the edge is for.

The mapping is Section 8 Phase 12's own list of swept-envelope checks -- light reference on every road,
heavy reference on every heavy-capable strategic edge, Harvester envelope on every home/resource economy
edge -- applied to the roles Phase 9 creates (`Hybrid_Topology_Edge_Capabilities`):

| edge role (plan step) | kind | road class | capabilities |
| --- | --- | --- | --- |
| heavy route-family trunk (5) | `FAMILY_TRUNK` | `HIGHWAY_4` or `PRIMARY_2` | light + heavy |
| mandatory synthesized river-bridge cross-link (3) | `CROSS_LINK` | `SECONDARY_2` | light |
| mandatory synthesized canyon/ravine-bridge cross-link (4) | `CROSS_LINK` | `SECONDARY_2` | light |
| home-field road (7) | `HOME_ROAD` | `PRIMARY_2` or `SECONDARY_2` | light + Harvester |
| contested-field branch (8) | `FIELD_BRANCH` | `SERVICE_DIRT_2` | light + Harvester |
| remote-rich-field branch (8) | `FIELD_BRANCH` | `SERVICE_DIRT_2` | light + Harvester |
| quota-filling service link (9) | `SERVICE_FILL` | `SERVICE_DIRT_2` | light |
| `TRAIL_1` shortcut (10) | `TRAIL` | `TRAIL_1` | light |
| optional heavy cross-link (11) | `CROSS_LINK` | `SECONDARY_2` | light + heavy |
| synthesized overpass cross-link (13) | `CROSS_LINK` | `SECONDARY_2` | light |
| base internal | `BASE_INTERNAL` | none | none (never realized) |

The bridge-quota and overpass-quota cross-links exist to meet a count; the plan gives them no heavier role,
so they carry light only rather than an invented heavy or Harvester requirement. Two consequences are
deliberate and tested: two edges of one class can carry different masks (a `PRIMARY_2` trunk `0x3` and a
`PRIMARY_2` home road `0x5` occur in real worlds, as do `SECONDARY_2` cross-links with and without heavy),
and a trail never becomes heavy-capable.

Phase 11 uses the widest envelope in the mask for every width test: the ground roadbed
(`required_swept_width <= RoadbedWidth(class)`), a pass floor's continuous width
(`swept + 2 * min_vehicle_lane_clearance`), Section 12.4's bridge clear width and Section 12.5's tunnel
bore. Preflight's per-class `BridgeRequiredWidth` table is unchanged: it runs before any edge exists and
answers only whether shipped content can carry a class at all.

## Structural-pose realizability (Q-023)

A Phase-9 structural binding must be geometrically realizable as an oriented road pose before the topology is
frozen; Phase 10 then routes it on the terrain and Phase 11 compiles it. Every candidate crossing window --
obligation, quota fill, synthesized cross-link -- is tested in this order:

1. Phase 6's per-class capability: the window's `ApproachClasses` bit for the binding class;
2. the corridor;
3. `>= 12W` along the obstruction from an already selected window;
4. **wrong side**: with `A` the window's axis oriented from the previous structural point to the next,
   `dot(incoming, A) >= 0` and `dot(outgoing, A) >= 0`;
5. **pose**: the window as a pose (`HybridRoadPose`: point, axis, `8W` straight either side) must connect to
   the neighbouring poses on the edge in the empty plane with the class's own radius construction
   (`Hybrid_Road_Poses_Connect`, Phase 10's arcs-and-tangents corner rule -- no terrain A*, no second
   geometry solver, no distance formula);
6. ranking, then `StableId`.

An impossible candidate is skipped and the next ranked window considered (`RecoveredByNextRank`); only when
none remains does the phase fail. The binding class is `HIGHWAY_4` for a heavy-family binding made before
highway assignment -- the family may still become a highway, so a window a highway cannot line up with is not
bound to it -- `SECONDARY_2` for a synthesized cross-link, and the edge's actual class otherwise. A window is
never discarded globally because a wider class cannot use it.

Overpass obligations get the same pose rule on both edges, with `max(10W, half deck)` of straight either side
(`Hybrid_Overpass_Pose`, `Hybrid_Overpass_Half_Deck` -- the same measure Phase 10 keeps straight), on top of
the unchanged `60..120` degree, `10W`, exclusion, spacing and slope tests. The gate re-checks every frozen
chain. `HybridTopologyResult` carries the census of the attempt (`RejectedApproachClass`,
`RejectedWrongSide`, `RejectedPose`, `RecoveredByNextRank`) whether or not it succeeded.

## The overpass target (Q-024)

Section 5's overpass number (`Hybrid_Overpass_Target`: 0 / 1 / 2) is a deterministic target, not a minimum.
Steps 12 and 13 enumerate every candidate and apply every hard-valid rule before ranking. Those rules are the
`60..120` degree angle (measured between the lines, on the intent sub-segment the crossing lies on), the `10W`
arms on that sub-segment, the base / Tiberium / water / wall / window exclusions, the `12W` spacing, the slope
disc and Q-023 pose realizability on both roads. Candidates are taken in ranked order until the target is met or
none remains. The record freezes `RequestedOverpassTarget` and `ReservedOverpassCount`;
`Overpass_Target_Shortage()` is the diagnostic, logged as `OVERPASS_TARGET_SHORTAGE`, and is never an attempt
failure. The angle rule had never refused anything before this: it compared the 0..90 line angle against both
ends of the directed band.

Bridge quota selection binds a preferred window into the family whose heavy corridor made it preferred, not
whichever family's node happens to be nearest. A non-preferred window gets the plan's bank cross-link, and
otherwise the fallback into any family whose chain passes within its corridor. Crossing obligations are no longer
pairable road endpoints: a road ending there would end inside the structure.

`HybridTopologyResult` carries the quota census (`RiverQuota`, `CanyonQuota`: rejections by binding path and
reason; `OverpassQuota`: step-12 and step-13 refusals by rule) whether or not the attempt succeeded.

## Entrance corridors and Tiberium

Step 6 reserves each base entrance corridor against unrelated Tiberium, so a candidate corridor that already
overlaps a frozen Tiberium hard reserve is treated like one outside the viable sector and the next ranked
sector is tried (`Corridor_Fits_Sector`). Measured before the fix: 148 of 512 frozen corridors over 420 worlds
overlapped a reserve, and their trunks could not leave their own base. After it: none.

## Failure behavior

Every failure is a named attempt failure and leaves the record empty: `no_entrance_sector`,
`obligation_unbound`, `bridge_quota`, `no_field_access`, `service_quota`, `trail_quota`,
`overpass_quota`, `edge_budget` (more routed edges than `R` allows), `no_cycle` and `invalid_input`.
An optional cross-link shortage is explicitly **not** a failure.
