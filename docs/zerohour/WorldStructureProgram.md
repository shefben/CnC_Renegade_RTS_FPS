# The Section 52B procedural-world prerequisites: Phases 63-65

Roadmap Section 52B, complete. Three capabilities that did not exist in the tree and that no
amount of generator code could have supplied for itself.

**This section still does not implement the procedural world generator.** Section 17's
prohibition stands and nothing here approaches it. These are the queries and representations a
generator would need, and that hand placement, LevelEdit validation, Commander building
placement and AI route checking already want.

All three build and link clean across `wwphys`, `combat`, `renegade` and `renegadeserver`.
Three device-less `-shaderselfcheck` families cover the arithmetic surface — `worldmetrics`,
`escarpment`, `traversability` — each with an `fds_` twin.

---

## P63: world metrics service

**Owner.** `Code/Combat/worldmetrics.cpp` (new), with read-only accessors added to
`wwphys/phys3.h`.

**Why it lives in `Combat` and not `wwphys`.** The roadmap's owner list names physics files, but
the service has to answer about building footprints too, and those come from the Section 39
descriptor, which lives in `Combat`. `wwphys` cannot depend on `Combat`. `WorldSpatialIndex` —
the Section 16 service, listed under `wwphys` in the roadmap for the same reason — already sits
in `Combat` on exactly this argument, so the precedent is the project's own.

**The problem, confirmed in the source.** `STEP_HEIGHT` is a file-scope constant at
`phys3.cpp:99` *and* a per-definition `Phys3DefClass::StepHeight`. The soldier's eye height
(`1.62f`) and its crouch drop (`0.56f`) were literals inside `SoldierGameObj::Get_Weapon_Height`
and nowhere else, so anything else wanting a soldier's posture height had to guess at them.
Maximum climbable slope is implicit in `Phys3DefClass::SlideAngle` and written down nowhere.

**Implement.** `WorldMetricsClass` answers per definition and per locomotion class. Every value
is read from a definition or computed from one; none is restated as a new constant.

- **Step height** and **slide angle** come straight from `Phys3DefClass`, through new read-only
  getters. Getters rather than a friend declaration, because the values are worth reading from
  anywhere and worth writing from nowhere but the definition.
- **Maximum climbable slope** is the slide angle for anything that walks. Vehicles have no slide
  angle — they are rigid bodies, and what stops them on a slope is tyre friction, not a slide
  test — so theirs is computed once from `VehiclePhysDefClass::TractionMultiplier`, which is
  exactly the term that scales the friction they can produce. This is the roadmap's "computed
  once from the locomotor's own parameters and cached, not typed in".
- **Turning radius** is the bicycle model, `wheelbase / tan(max_steering_angle)`, from
  `WheeledVehicleDefClass::MaxSteeringAngle`. Tracked vehicles counter-rotate and report zero
  (turns on the spot); a locomotor that cannot steer reports a negative marker rather than a huge
  number, so a caller comparing radii never silently treats "cannot turn" as "turns very wide".
- **Envelope extents** come from the model's `WORLDBOX` sub-object, which is where `Phys3Class`
  itself gets its collision box, so asking the same question of the definition gives the same
  answer without needing an instance.
- **Building footprint radius** is the circumscribing radius of the descriptor's width/depth
  rectangle — half the diagonal, because a building may be placed at any heading and a test that
  only allowed for the width would fail on the diagonal.

`SoldierGameObj::Get_Weapon_Height` now reads its two numbers from the service. The values are
unchanged; they simply have one home.

**Two honest limits, recorded rather than papered over.**

- Extents need the model resolvable through the asset manager. On a dedicated server with no art,
  or for a definition naming a model nobody has made, the physics-derived half of the answer is
  still filled in, the extents fall back to `Phys3Class`'s own defaults, and `HasExtents` says so.
  A missing model degrades the answer; it never blocks it and never throws.
- **Prone is not a posture this engine has.** `HumanStateClass` has `UPRIGHT` and a
  `CROUCHED_FLAG` and no third stance. `Get_Prone_Height` returns the crouch height and
  `Is_Posture_Supported(POSTURE_PRONE)` returns false. Inventing a prone height would be
  inventing a number, which is the exact failure this service exists to stop.

---

## P64: vertical world structure

**Owner.** `wwphys/escarpment.cpp` (new), `wwphys/bridgesystem.cpp`,
`wwphys/bridgesection.cpp`.

### Escarpments

`HeightfieldClass` is a function of x and y, so it cannot represent a vertical face. A cliff
authored into it is a run of very steep cells: the slope query correctly calls them unwalkable,
and they read as stretched ground texture, because nothing there knows it is rock.

An escarpment sits **beside** the heightfield rather than inside it. It is a polyline with an
upper and a lower height at each point, and it owns exactly one thing: the face between those two
heights. The heightfield stays the single authority for ground height on both benches —
`EscarpmentSystem::Conform_To_Terrain` *reads* the benches out of it rather than asserting them,
sampling one inset either side of the polyline, and nothing in the class ever answers "how high
is the ground at x,y".

Which side is down is stated once and kept to: the face drops to the right of the direction of
travel. That is what lets the batter lean the top back into the hill instead of out over the drop.

Geometry is one quad per polyline segment, built in world space and given an identity transform —
the same thing the road and bridge builders do, and for the same reason: a polyline is not at any
one place, so there is no sensible local origin. Collision uses the same bits a bridge pier does;
there is no case where you want to walk through the middle of a cliff.

Dressing comes from the existing terrain layer table: the escarpment's own layer index if it names
one, otherwise the first layer of kind `TERRAIN_LAYER_CLIFF`, which Section 18's default table
already defines. So an escarpment that says nothing still comes out as rock.

Missing art degrades the way every world system in Sections 17-23 does: with no cliff layer and no
texture in the mix files, the face is still built, still collides, and is simply untextured. The
count is reported through `Get_Untextured_Face_Count`.

`Is_Segment_Blocked(a, b, max_climbable_height)` is the query Phase 65 consumes. It takes the
climbable height as a parameter rather than deciding it, because infantry and a tank get different
answers about the same cliff, and that difference is the entire point of the graph.

### Bridge structural vocabulary

`BridgeClass` had two endpoints and a width. Its height above the ground was wherever the author
happened to put the endpoints, and clearance under it was an accident of how deep
`Apply_River_Cut` had cut — which meant a crossing could not state what it was for, and nothing
could check whether the terrain agreed.

- **`BridgeClass::DeckHeight`** — the deck's height above the higher abutment. Zero keeps the old
  behaviour exactly (the deck follows the endpoints), so every bridge predating this is unchanged.
  One branch, and the old path is the default.
- **`BridgeClass::StartAbutmentHeight` / `EndAbutmentHeight`** — an *output*, filled in by
  `Conform_Abutments` from the terrain at each bank. The ground is still the heightfield's to
  answer. A bank the terrain cannot answer for leaves that bridge unconformed rather than zeroed.
- **`BridgeDefinitionClass::RequiredClearance`** — headroom under the span for the traffic the
  crossing is meant to carry. Deliberately *not* `SupportClearance`, which already existed and is
  a rule about when a pier is worth building; this is a promise the crossing makes about itself.

And the thing that makes stating it worthwhile: **`BridgeSystem::Check_Clearance`** walks the
span, samples the terrain under it, and reports the tightest headroom found and where. Three
outcomes, kept distinct on purpose. A crossing whose definition states no clearance is not judged
(every definition predating this phase is in that case and none should start failing). A crossing
over ground the terrain cannot answer for is not judged either, rather than being passed on the
strength of an absent ravine. Only a crossing that stated a number and got real ground back is
graded.

It returns a report rather than a bool because "this ravine is 0.4 metres too shallow, forty
metres along" is actionable and "false" is not.

---

## P65: traversability and connectivity graph

**Owner.** `Code/Combat/worldtraversability.cpp` (new), in `Combat` for the same reason P63 is.

**This is an analysis structure, not a pathfinder.** `PathfindClass` stays the authority for how a
unit moves and nothing here is ever consulted to move one. The graph exists to answer questions
about the *shape* of the world, which a pathfinder is the wrong tool for: a pathfinder tells you
one route and says nothing about whether it was the only one.

**Nodes.** One per grid cell, cell size defaulting to the locomotion class's own corridor width
from P63 — a graph whose cells are narrower than the body they describe reports routes that body
cannot take. Four tests per cell, in the order that rejects most cheaply: on the field at all;
flat enough for a body of this radius (`Query_Placement_Surface` samples a disc rather than a
point, which is the corridor width applied); not under more water than this class can wade; and
not standing on a cliff face.

There is a hard cap of 262,144 cells, and exceeding it refuses loudly. A caller asking for a cell
size that fine has asked for a second pathfinding grid, which is exactly what this is not.

**Edges.** East and north only, so each pair is considered once and the graph is undirected
without duplicates. Diagonals are deliberately *not* edges: a coarse graph that lets a body slip
diagonally between two blocked cells reports routes through walls.

`Can_Step` allows a transition when either the rise is inside the step height or the slope it
makes across the gap is climbable. Both, because they are different facts — a kerb is a small rise
at an infinite slope, and a hillside is a large rise at a gentle one. P64's escarpments are then
consulted: a cliff between two cells this class cannot scale is the *absence* of an edge, not an
expensive edge.

**Crossings, which is where the acceptance line lives.** A bridge joins the cell at one abutment
to the cell at the other, once, regardless of length — the graph is about connectivity and
everything between is deck. Whether the edge exists at all is decided by `Crossing_Carries`,
comparing the width and clearance the crossing states against the corridor width and overhead
clearance the class needs. So a narrow footbridge is an edge in the infantry graph and not in the
tank's, and a low underpass is absent from the tank's for the other reason. A crossing that states
nothing carries everything, so nothing predating P64 starts refusing traffic it always carried.

**Queries.**

- **Reachability** is a component comparison, not a search — the components are built once by a
  breadth-first flood, iteratively, because a recursive flood over a hundred thousand cells is a
  stack overflow waiting for a large map.
- **Distinct routes** are edge-disjoint: find a route, strike its edges out, find another. That is
  what "alternate route" has to mean for the answer to be worth anything — otherwise a corridor
  with a pillar in it reports two routes.
- **Chokepoints** are the graph's bridges, found by the standard iterative lowlink walk: exactly
  the edges no alternate route covers. (Term collision worth naming: a graph bridge and a river
  bridge are different things, and this finds the first kind. A river bridge is very often also
  one, which is the point.)
- **Unreachable count** from a node is a generator's own sanity check on its output: a world with a
  large marooned region has a hole in it.

**Bridge state is read at build time.** A bridge that breaks mid-match invalidates the graph,
which the caller rebuilds if it cares. Making the graph track live state would make it a second
world database, which Section 16's own rule forbids.

---

## What this section did not attempt

- **The generator.** Section 17's prohibition stands, and nothing here approaches it.
- **Runtime verification.** No level has been loaded, no graph built against real terrain, no
  cliff seen on screen. The standing "do not run the game" instruction covers this section the
  same way it covers Section 52A; **Q-012** tracks it. Everything stated above is a claim about
  what the code does given what its inputs were read to contain.
- **Escarpment authoring.** Nothing in the tree creates an `EscarpmentClass` yet — there is no
  LevelEdit surface for one and no level that has any. The representation, its collision, its
  dressing and its queries all exist and are reachable; what does not exist is a level that uses
  them. That is authoring, not code, and it is the same position Sections 17-23 left their own
  systems in.
- **A `BaseWalletCap`-style default for `RequiredClearance`.** No bridge definition states one, so
  `Check_Clearance` reports "not stated" for all of them today. Picking a number would be picking
  a gameplay value out of the air; the check is ready for whoever sets one.
