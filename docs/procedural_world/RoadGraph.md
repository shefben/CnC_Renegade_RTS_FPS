# Road Graph (V2 Phase 8)

Roadmap Section 34 Phase 8 -- "road and highway graph plus runtime road geometry." Section 10.
See `StrategicPlanning.md` for the node graph this phase connects and `RoadSystem.md`
(`docs/zerohour/`) for the v1 system that turns a route into a real road.

## Connect, then realize through the one road system

`RoadGraphGenerator` (`Code/Combat/roadgraphgenerator.h/.cpp`) decides which strategic anchors
need a road between them and finds a real, terrain-costed path for each. The road itself --
centre line, ribbon, junctions, grading, mask -- is `RoadSystem`'s own, unmodified: this
generator calls `RoadSystem::Add_Road`/`Build_Network`, the same entry points the console
command and self check already use, and never builds geometry of its own.

## Six conceptual classes, three physical ones

Section 10.1 names six road classes; v1's `RoadClassType` (`Code/wwphys/roadspline.h`) has
three, already complete and already tested. Extending it purely to grow a name list would touch
`BridgeSystem`'s own default bridge-to-class mapping for no reason this phase needs.
`RoadGraphClassType` is Section 10.1's own six names, used only for this file's own cost-field
weighting and default width -- a question of how important a route is. `Road_Graph_Class_To_
Road_Class` maps it down to `RoadClassType`'s three for the question `RoadSystem` actually asks:
which physical rendering/grading tier a road belongs to.

Default widths are Section 10A.1's own real derivation now, not fixed placeholders. Rather than
name a live GameObj definition and guess at a preset identifier, `GDI_HUMVEE_WIDTH` (2.556 world
units) is read directly out of the shipped `Data/always.dat`: the MIX1 archive format
(`Code/wwlib/mixfile.cpp`) locates `v_gdi_humvee.w3d` by CRC without guessing an offset, and the
W3D box chunk format (`Code/ww3d2/w3d_file.h`) reads the `V_GDI_HUMVEE.WORLDBOX` sub-object's own
Y extent -- the same canonical physics collision envelope `WorldMetricsClass::Resolve_Extents`
would load at runtime, just read offline instead of from a live session. The axis (Y is lateral
width, not X) is confirmed against two other vehicles' own WORLDBOX chunks
(`v_nod_buggy.w3d`, `v_gdi_apc_m.w3d`), both showing the same length > width > height ordering.
`ROAD_GRAPH_NORMAL_LANE_WIDTH` applies 10A.1's own formula (`C = max(0.35m, H*0.15)`,
`NormalLaneWidth = max(H+2C, H*1.30)`) using `WorldMetricsClass::Get_World_Units_Per_Meter`
(resolved 1:1 -- see that method's own comment) for the metre term, and each class's width is
that times Section 10A.3's own default lane count (4/2/2/2/2/1). See
`Code/Combat/roadgraphgenerator.cpp`'s own header comment above `GDI_HUMVEE_WIDTH` for the full
derivation, byte layout included.

Not resolved by this: Section 10A.2's full `RoadCrossSection` descriptor (shoulder/median/
sidewalk fields beyond the existing separate `ROAD_GRAPH_CLASS_SHOULDER` array), lane-level
geometry (`RoadSplineClass` has no lane-count field, unchanged from Phase 8), and per-class
turning-radius/traversal-envelope validation (10A.1's own "required routes must also pass
traversal-envelope validation" is not implemented).

## The routing cost field

One entry per chunk, reusing the chunk registry and the same owners' own exclusion tests Phase 7
already established:

- **Blocked, always:** a `CliffBand` chunk.
- **Blocked unless a crossing candidate is there:** an `InteriorCliff` chunk, or a chunk
  `WaterSystem::Is_Point_In_Water` reports wet. Every `StrategicNode` of type
  `CROSSING_CANDIDATE`/`TUNNEL_CANDIDATE` within a fixed radius lifts exactly the chunk(s) it
  sits in, at a real (large but finite) added cost -- Section 10.3's "bridge/tunnel candidate
  cost", made concrete rather than a phrase.
- **Cost, for every routable chunk:** 1.0 base, plus a slope term (fraction of the shared
  buildable-slope cap) and a boundary-approach term (stronger as `Get_Inward_Distance` shrinks),
  each independently weighted by a profile-facing parameter.

A small deterministic hash jitter breaks routing ties between otherwise-equal paths differently
per seed -- the same narrow seed usage `StrategicPlanner` already established for the same
reason.

### Planned lanes (D-003)

When a map plan exists, a chunk whose centre lies in a `LANE` or `RAMP` cell has its cost
multiplied by `PLAN_LANE_COST_SCALE` (0.5) after the slope and approach terms. Blocks and
crossing lifts are unchanged. See `MapPlan.md`.

## Which edges, and how they are found

Step 3's "select required graph edges" is a minimum spanning tree (Prim's) over every
`BASE_CANDIDATE`/`RESOURCE_CANDIDATE` node, using `StrategicPlanner::Get_Travel_Cost`'s own
straight-line distances -- bounded at N-1 edges for N anchors rather than a complete graph.
Settlements, POIs and crossing/tunnel candidates are not independently wired into this skeleton;
they exist for a later phase to branch off of, not for this one to connect exhaustively.

Each MST edge is then realized as a real route: Dijkstra over the chunk cost grid (a plain
O(chunk_count²) linear-scan, no heap -- the same bounded-simplicity every other chunk-grid pass
in this tree already accepts) from the chunk nearest one endpoint to the chunk nearest the
other. A route that cannot reach its destination under this cost field (every path blocked) is
simply not built -- Phase 8's own do-not-proceed forbids required nodes staying disconnected
*without* an explicit pending edge, and an MST edge that fails to route is exactly that: absent,
not silently skipped and forgotten, which `Check_Required_Nodes_Connected` verifies by counting.

An edge's own class is decided by what it connects: base-to-base is a highway; touching a
resource candidate is a primary road (the harvester route); touching a settlement is a city
street; everything else defaults to a dirt track.

## Unresolved crossings, per edge

Every chunk a route passes through that lifted its block via a crossing candidate is recorded on
that edge's own `UnresolvedCrossingIds` -- Phase 8 step 15's own list, kept per edge rather than
one global bag, so a later phase (9 for bridges, 10 for tunnels) knows exactly which road needs
which crossing resolved, and can tell a `BRIDGE_CROSSING`-flavoured crossing (Phase 5's interior
cliffs) from a `FORD`/`BRIDGE`-flavoured one (Phase 6's hydrology) by reading the
`StrategicNode` the id names.

## What is deliberately not attempted this phase

**Intersections** need no extra code: `RoadSystem::Build_Network` already finds and builds
junction polygons for any roads that meet, automatically, the same mechanism the console command
and self check already exercise.

**Parallel infantry-route/cover corridors** (step 12) are not generated. **Lane-level modelling**
(discrete lane counts, medians, sidewalks -- Section 10A.2's full `RoadCrossSection`) is not
attempted; `RoadSplineClass` itself has no lane-count field to model it with, only a continuous
width, and extending it is a `RoadSystem`-owned change out of this phase's scope. **Off-road
traversal classification** (Section 10A.8) is not built. **Route independence** in Section
17A.2's real sense (two paths that do not immediately merge into the same choke) is not measured
-- a spanning tree by construction gives each anchor exactly one path to any other, never an
alternate one; a second, genuinely independent route is a later phase's own addition, not
something an MST produces.

## What is proven, and what is not

`terrain_roadgraph`/`fds_terrain_roadgraph` (`Code/Commando/terrainselfcheck.cpp`,
`-terrainselfcheck roadgraph`) prove, device-less, against the same large synthetic
uniform-slope ramp `Check_Strategic_Plan` uses (real base/resource candidates from a real,
passing strategic-plan run, so this check exercises the real hand-off between the two
generators rather than a hand-built node list): parameter refusal; the anchor nodes are spanned
(edge count meets the N-1 floor a spanning tree requires); no edge crosses the cliff band with
none configured; no edge passes through water without naming a crossing candidate; `RoadSystem`
itself reports its network built; and the same seed, terrain and plan reproduce the same
checksum.

What is not proven, because nothing here runs with a display or against a real generated seed's
own uneven terrain: whether a routed road looks like a road rather than a chunk-to-chunk zigzag;
whether the width/class choices read right once art exists. The reachability gap every phase
since 4 carried -- this generator only running when the strategic plan itself ran, which only
ever happened when macro terrain ran, which the one real caller's `fit_to_level` profile made
never -- is closed by V2 Phase 10's fitted-terrain sampling; see `completed_features.md` and
`GeneratorDeterminism.md`. The road-to-bridge handoff this file's own edges feed into
`BridgePlanner` for is likewise closed -- see `BridgePlanning.md`.
