# Commander Expansion on Generated Terrain (V2 Phase 17)

Phase 16 made a generated world playable. Phase 17 makes it *buildable*: the v1 Commander can
place forward structures on ground a generator produced, and can level that ground within limits
the profile states, without corrupting the world's topology.

Three new owners, one extended one, and one new terrain primitive:

| Owner | File | Steps |
|---|---|---|
| `GeneratedWorldPlacementRules` | `Code/Combat/generatedworldplacementrules.*` | 1-2 |
| `FoundationGradingService` | `Code/Combat/foundationgrading.*` | 3-9 |
| `ExpansionRegionRegistry` | `Code/Combat/expansionregionregistry.*` | 10-13 |
| `CommanderPlacementValidator` (extended) | `Code/Combat/commanderplacement.*` | 1-2 |
| `HeightfieldClass::Grade_Foundation` / `WorldTerrainSystem::Apply_Foundation_Grade` | `Code/wwphys/` | 7 |

Self-checks: `terrain_placementrules`, `terrain_grading`, `terrain_expansion` and their `fds_`
variants. `ctest -N` is now 173, was 167.

---

## Steps 1-2: generated-world placement exclusions

Step 1 asks for generated terrain and world bounds to be fed into the existing v1 placement
validator. Most of that was already true: Phase 2 wired `WorldBoundsService::Can_Place_In_Region`
and Phase 4 wired `WorldTerrainSystem::Query_Placement_Surface` into
`CommanderPlacementValidator::Validate` when each of those owners first existed. What did not
exist then is everything Phases 5-16 went on to generate, and a validator that only asks the
ground how steep it is cannot see any of it.

`GeneratedWorldPlacementRules::Classify(position, radius)` is step 2: one question, answered by
the real owner of each feature, returning the reason rather than a bool. Evaluated in this order,
so a rejection names the most structural true thing about the position:

| Exclusion | Answered by | New `CommanderPlacementResultType` |
|---|---|---|
| boundary/cliff band | `WorldBoundsService::Can_Place_In_Region` + chunk registry `CliffBand` | `PLACEMENT_OUT_OF_BOUNDS` (existing) |
| water | `WaterSystem::Is_Point_In_Water`, centre plus four rim samples | `PLACEMENT_ON_WATER` |
| immutable major cliff | chunk registry `InteriorCliff` + `EscarpmentSystem::Sample_Face` | `PLACEMENT_ON_IMMUTABLE_CLIFF` |
| tunnel portal | `StrategicPlanner` `TUNNEL_CANDIDATE` node positions | `PLACEMENT_ON_TUNNEL` |
| bridge structure/approach | `BridgeSystem::Find_Bridge_At` + each planned bridge's own endpoints and width | `PLACEMENT_ON_BRIDGE` |
| protected resource route | road-graph edge touching a `RESOURCE_CANDIDATE`, within the spline's own carriageway | `PLACEMENT_ON_RESOURCE_ROUTE` |
| invalid nav chokepoint | connectivity of `NavMeshGenerator`'s navigable-chunk graph | `PLACEMENT_BLOCKS_NAV_CHOKEPOINT` |

**No distance in this file is a named constant.** The bridge approach is one bridge-width from an
abutment because that is the only length a bridge itself states. The resource corridor is the road
spline's own half-width. The water rim samples are at the caller's footprint radius. A placement
rule carrying a number no generator agreed to is a rule that disagrees with the world the first
time a profile changes.

Every one of these is silent on a hand-authored map: none of those owners is configured there, so
`Classify` answers `GENERATED_WORLD_EXCLUSION_NONE` everywhere (directive 1.10).

### The chokepoint rule

"Invalid nav chokepoint" is the odd one out — it is not "a feature is here", it is "the walkable
world would be cut in half". Two things exist for it:

- **The exact test**, `Would_Disconnect_Navigation`: remove every navigable chunk the footprint
  covers, flood fill from a survivor, and check whether every other survivor was reached. This is
  what `Classify` uses, so the ghost's colour and the server's verdict are the same answer to the
  same question rather than a cheap approximation and an exact one that disagree at the boundary.
- **The cached articulation set**, `Build_Chokepoints`: the articulation points of the
  navigable-chunk graph (iterative Tarjan), exposed as real map metadata for the Commander and the
  AI. It is *not* used to accelerate `Classify`, because it cannot be: covering an articulation
  point *and* everything it was separating leaves a connected remainder, so the cache would produce
  false rejections.

`NavMeshGenerator` gained `Is_Chunk_Navigable`/`Get_Chunk_Count_X`/`_Y` for this. The walkability
judgement is recorded separately from sector registration, because with no live `PathfindClass`
there is no sector to register and a chunk's walkability is a fact about the terrain either way.
Re-deriving that predicate in the placement rules would have been two answers to one question.

---

## Steps 3-9: bounded foundation grading

Two phases, never one. `Evaluate` measures a request against the real heightfield and the real
protected features and reports cut volume, fill volume, depth, rise, slope, area, apron width and
the first limit it breaks. `Execute` re-evaluates (the world may have moved, and a server must
never trust a client's report), then touches the terrain. A caller cannot reach the second without
the first having said yes.

### Step 5's limits come from the profile

Schema/generator version **6**. New `grading` block:

```json
"grading": {
  "max_cut_depth": 6.0,
  "max_fill_height": 6.0,
  "max_slope_change_radians": 0.5,
  "max_affected_area": 4096.0
}
```

`max_slope_change_radians` defaults to the same 0.5 rad buildable-slope cap every terrain-reading
generator in this chain already shares, so a grading can never leave ground steeper than ground a
generator would have been willing to make. A schema-1..5 profile still loads and takes the
defaults. All four must be strictly positive: a zero limit is a broken profile, not a strict one,
and `Set_Limits` clamps it back to the documented default rather than silently disabling the
feature.

### The shape: a plateau with a ramp

Neither `Modify_Height_Region` nor `Blend_Height_Region` can make it — both fall off smoothly from
the centre, so a building graded with either stands on a dome. `HeightfieldClass::Grade_Foundation`
is the new primitive, wrapped by `WorldTerrainSystem::Apply_Foundation_Grade` alongside
`Apply_Road_Grade` and `Apply_River_Cut`, with the same `Terrain_Edit_Allowed` check and far-layer
invalidation. Nothing in `Code/Combat/` writes a height sample.

**The pad is wider than the footprint, by one heightfield cell diagonal.** Heights live at grid
vertices and the ground between them is interpolated, so a point at the edge of the footprint sits
in a cell whose far corners are outside it. Levelling only the vertices inside the footprint disc
leaves those corners at their old height, which tilts the last cell of the pad — the building's own
rim ends up on a slope, which is exactly what the operation was asked to remove. This was found by
the self-check, not reasoned about in advance: the first implementation left the footprint 0.14
world units off level at its worst point.

### Sizing the apron is a fixed point, not a formula

The naive answer — the height difference under the footprint divided by the steepest gradient
allowed — is wrong on any real slope, and wrong in the dangerous direction. The ramp does not pay
off the difference at the pad's edge; it pays off the difference at its own outer rim, and that rim
moves further uphill as the ramp gets longer. On a 10° ramp the naive sizing produced a graded
surface measured at **0.684 rad against a 0.5 rad limit** — a face 37% steeper than the limit it
was supposed to respect.

So each attempt measures the surface it would actually leave, and if that surface is over the
limit, lengthens the ramp in proportion and measures again. Bounded at 8 attempts, in the same
spirit as Section 3C.1's bounded local retry; a request that cannot be satisfied inside the limits
is refused as `FOUNDATION_GRADING_SLOPE_CHANGE_EXCEEDED` with the measurement attached, never
widened forever. The smoothstep peak factor (1.5, the maximum derivative of `3t² - 2t³`) seeds the
first estimate but never decides the answer.

This is the only reason `MaxSlopeChangeRadians` in the report means anything: it is a measurement
of the graded samples, never a restatement of the number used to size them.

### Step 3's "valid ghost placement", and the one exception

Grading is defined from a valid placement, asked of the one shared validator, so a grading can
never be the way a refused placement gets built anyway. **One refusal is treated as gradeable:
`PLACEMENT_SLOPE_TOO_STEEP`.** That is the one thing the operation exists to fix — a grading that
could only ever level ground already flat enough to build on would change no outcome at all, and
Phase 17's own purpose line is letting the Commander build on generated terrain. This is an
interpretation of step 3, and it is disclosed here rather than buried: every other refusal,
including all seven generated-world exclusions, still stands. A river, a cliff, a bridge and a
chokepoint are not made acceptable by levelling.

### Steps 6, 8, 9

Protected features are asked **per sample over the whole affected disc**, not only under the
footprint: the apron is ground the operation moves too, and an apron that reaches into a river has
cut the river's bank. The navigation half of the exclusion set is deliberately not asked —
levelling ground does not remove it from the walkable world, so an apron cannot create a chokepoint
(`Classify_Static_Features` is that half).

After execution: `Build_Collision` refills only the patches the edit marked dirty; the chunk
registry is rebuilt against `Get_Chunk_Max_Buildable_Slope()` — the same cap it was built with, so
no chunk nobody touched changes classification; the far layer was already invalidated by the edit.
Then step 9 revalidates server-side on the ground as it now is, and a grading that left the
footprint unbuildable is reported as a failure rather than a success.

---

## Steps 10-13: reservation, completion, expansion regions

**Step 10 is the one that has to exist for any of the rest to be safe.** Between a placement being
accepted and `BuildingConstructionService` finishing the structure, there is no building in the
world: no collision object, nothing in the spatial index, nothing for the next placement's overlap
query to hit. Two Commanders clicking the same hillside inside that window both get a valid answer
and the second building lands inside the first. `ExpansionRegionRegistry::Reserve` closes that
window on the server, where the sequence of accepted placements is the one that matters.

Step 12's completion sweep rebuilds navigation and the chokepoint set, recalculates the owning
team's power through `TeamPowerGridService::RecalculateTeam`, and releases the reservation **last**,
so nothing can claim the ground between the building appearing and the world agreeing that it has.
The sweep returns a written report of what it actually did, so "nav was rebuilt" is observable
rather than claimed.

Spatial registration is deliberately *not* performed there: a completed building is wrapped in a
`StaticPhysClass` and handed to `PhysicsSceneClass` by the construction path that created it
(`BuildingGameObj::Build_Runtime_Components`). Doing it again would put two collision objects in
the scene for one building.

Step 13's expansion regions are proximity clusters of completed structures, per team, each with a
stable id (`expansion_<team>_<ordinal>`), a running-mean centre, a radius that contains every
member, and an `IsForward` flag. Clustering is single-pass in deterministic server-side completion
order, so every peer replaying the same sequence builds the same regions with the same ids, and the
whole set carries a Section 3B.4-quantized CRC.

---

## Disclosed gaps — none of them implementation shortfalls

- **The tunnel exclusion is thin, and honestly so.** No tunnel system exists anywhere in this tree
  (`bridgeplanner.cpp` says the same about its own stage-3 fallback; Phase 16's steps 7 and 12 are
  unimplemented for the same reason). There is no portal geometry and no interior volume to test.
  What exists is the strategic plan's own `TUNNEL_CANDIDATE` positions, so the ground a tunnel
  mouth was *planned* on is excluded. When a tunnel system arrives it owns this test.
- **Nav rebuild after a construction is whole-world, not incremental.** There is no incremental nav
  rebuild anywhere in this tree — Phase 16's own disclosed gap. A stale graph that says a unit can
  walk through a Power Plant is worse than the cost of rebuilding.
- **`IsForward` is decided by distance from the team's first region.** That is the only real "home"
  signal available without a base-ownership concept, which this tree does not have.
- **The chokepoint analysis is at chunk granularity**, because the sector graph it reads is
  (Phase 16's disclosed granularity gap, inherited rather than added).
- **No UI consumes any of this yet.** The exclusions reach the ghost through the existing validator,
  so a ghost already greys out for them; the expansion regions and the grading report have no
  renderer, the same "real output, no consumer yet" state `MinimapBaseLayer` is in.

## What is proven, and what is not

**Proven by `terrain_placementrules`:** every exclusion has a real name; the articulation analysis
returns exactly the three cut vertices of a dumbbell grid whose answer is known by construction, and
zero on an open grid; malformed input is refused; a fresh process excludes nothing; the chokepoint
analysis refuses honestly with no navigation; over a real chain the boundary band and a position the
water system itself calls water are both excluded; every reported chokepoint is itself navigable; no
cliff chunk is ever navigable; and re-analysing the same graph reproduces the same checksum.

**Proven by `terrain_grading`:** refusals before measurement; a real measurement over a real 10°
ramp with non-zero samples, volumes and apron; the reported area really is the pad-plus-apron disc;
the pad really is wider than the footprint; the measured graded slope really is under the limit;
`Evaluate` changes nothing and executes nothing; each of the four limits refuses when made
impossible; a zero limit is clamped rather than accepted; and after `Execute` the footprint really
is level (within 0.05 world units at its worst of eight sampled points), ground outside the apron
really did not move, and re-grading flat ground moves almost nothing.

**Proven by `terrain_expansion`:** the second claim on the same ground is refused and the first is
kept; clear ground is a separate claim; binding, releasing and double-release behave; a completion
releases its reservation and reports what it did; clustering produces the right number of regions
with the right membership and the right `IsForward`; a point resolves to the region containing it;
and the same completions clustered twice produce the same checksum.

**Not proven here:** that a human has watched a Commander grade a hillside and put a Power Plant on
it. Phase 17's gate is "Commander can expand a base on generated terrain using only canonical v1
building systems plus safe bounded terrain grading", and the last word of that sentence is the only
one a test can reach. The live pass reads one new log line per generation —
`ProceduralWorldManager: expansion rules armed -- N chokepoint(s), grading limits ...` — alongside
Phase 16's four.
