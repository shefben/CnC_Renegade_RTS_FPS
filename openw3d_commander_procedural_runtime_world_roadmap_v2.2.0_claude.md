# OpenW3D Command & Conquer: Renegade Procedural Runtime World Roadmap
## Procedural Battlefield Generation + Dynamic World Mutation + Autonomous Commanders + Persistent Strategy

**Roadmap version:** **2.2.0**  
**Plan type:** implementation roadmap for **Claude Code / Claude Opus**  
**Primary target:** the OpenW3D codebase produced by `openw3d_tt484_zerohour_commander_integration_roadmap_v1.1.0.md`  
**Prerequisite:** **v1.1.0 MUST be complete before this roadmap begins**  
**Engine:** OpenW3D / W3D, native TT 4.8.4 integration, existing OpenW3D/WWPhys physics  
**Renderer foundation:** existing OpenW3D D3D9-backed renderer  
**Primary gameplay identity:** Renegade first-person combat augmented by Commander RTS systems  
**World model:** finite, seed-generated, server-authoritative procedural battlefields with runtime mutation inside an immutable outer boundary  
**Archived design reference:** `CNC_RENEGADE_COMMANDER_RUNTIME_WORLD_MASTER_PLAN.md` is reference-only. Do not import its obsolete OmniBlade target, duplicate compatibility layers, or parallel subsystem architecture.

---

# 0. CLAUDE: READ THIS BEFORE DOING ANYTHING

This roadmap begins **after** the v1.1.0 roadmap has reached its final gate.

Do not reopen or redesign completed v1.1.0 systems unless:

1. the v2 requirement genuinely cannot be implemented through the canonical v1 API;
2. an actual bug/incompatibility is demonstrated; or
3. the user explicitly requests a redesign.

The following v1 systems are assumed to exist and are reused:

```text
native TT 4.8.4 integration
canonical OpenW3D/TT scripts
Zero Hour-derived terrain/world renderer improvements
WorldTerrainSystem
runtime roads
sectional bridges
water/rivers renderer
foliage batching
spatial query/index layer
shroud/radar tactical intelligence
AI state/path/guard improvements
building semantic JSON descriptors
finite JSON-driven team power grid
Communications Center sensor coverage
Tiberium Silo wallet-capacity capability
embodied Commander Mode
right-side Commander production UI
building ghost/placement validation
procedural block building construction
Commander AI infantry/ground/air production
Commander unit selection/orders
save/load/networking/dedicated-server support
persistent feature-state files
```

**Do not recreate these under new v2 class names. Extend their canonical owners.**

**Section 34 contains executable phase work packages. Claude must use those detailed steps, not merely the compact phase list in Section 40.**

The v2 project is not "replace Renegade with an RTS terrain engine."

It is:

> **Generate and mutate richer battlefields that create better Renegade FPS combat while giving the Commander meaningful strategic control over a physical world.**

---

# 0A. MANDATORY PERSISTENT SESSION STATE

At the start of every Claude session, read:

```text
CLAUDE.md
openw3d_tt484_zerohour_commander_integration_roadmap_v1.1.0.md
openw3d_commander_procedural_runtime_world_roadmap_v2.2.0_claude.md
completed_features.md
WIP_features.md
unstarted_features.md
awaiting_answers.md
```

The four state files remain authoritative human-readable project state.

## Session start

Before coding:

1. inspect `git status`;
2. read all WIP entries;
3. resume the highest-priority dependency-safe WIP item;
4. do not redo completed work merely because conversation context was compacted;
5. add any newly started v2 feature to `WIP_features.md`.

## During work

Update `WIP_features.md` whenever:

```text
implementation direction materially changes
a blocker is found
an architectural decision is made
the exact next action changes
important targeted validation has completed
```

Put genuine user decisions in:

```text
awaiting_answers.md
```

using stable IDs:

```text
Q-001
Q-002
...
```

Do not ask the user questions that source inspection can answer.

## Before every intentional session end / handoff / context reset

Claude MUST:

1. inspect `git status`;
2. inspect relevant diffs;
3. update `completed_features.md`;
4. update `WIP_features.md` with exact next actions;
5. update `unstarted_features.md`;
6. update `awaiting_answers.md`;
7. run the lightweight feature-state validator if present;
8. record concise targeted build/test state already obtained;
9. **do not** launch full regression merely because the session is ending.

---

# 0B. VALIDATION / TOKEN DISCIPLINE

The validation-economy policy from v1.1.0 remains mandatory.

Normal implementation loop:

```text
implement
-> build only directly affected target(s)
-> run directly relevant cheap existing test(s), if useful
-> one focused runtime/smoke check when warranted
-> PASS with no escalation trigger
-> STOP VALIDATING AND CONTINUE IMPLEMENTING
```

Do not automatically:

```text
run all tests
run all golden maps
benchmark every edit
rebuild all targets
read giant successful logs
create unit tests for trivial wrappers
repeat already-passing tests for reassurance
```

Broad regression is reserved for the explicit v2 integration gates later in this roadmap or a concrete cross-cutting failure.

Implementation should dominate a healthy work session.

---

# 1. NON-NEGOTIABLE V2 DIRECTIVES

## 1.1 OpenW3D remains the final architecture

Do not create a separate runtime-world engine beside OpenW3D.

Extend:

```text
WorldTerrainSystem
WorldSpatialIndex
existing renderer
WWPhys/OpenW3D collision
canonical AI/path systems
canonical shroud/radar
canonical Commander services
canonical building descriptor system
canonical save/load/network layer
```

## 1.2 Preserve FPS-first gameplay

Generated worlds must improve first-person combat.

Do not generate enormous empty RTS landscapes merely because the Commander camera looks impressive from above.

Every generated map must deliberately support:

```text
infantry combat
vehicle combat
mixed infantry/vehicle combat
building infiltration
cover
flanking
tunnels/interiors
short/medium/long sightlines
defensible positions
attackable infrastructure
multiple approaches to important objectives
```

## 1.3 One finite map, one absolute boundary

Every generated battlefield has a finite authoritative playable extent.

The map is **not** an infinite terrain streamer.

The map boundary is generated at world creation and cannot be crossed by:

```text
infantry
ground vehicles
air vehicles
boats/naval vehicles where supported
Commander camera
AI navigation
Commander building placement
runtime terrain operations
tunnels
roads
bridges
```

Projectiles/effects may visually continue beyond the perimeter when harmless, but actors and gameplay objects may not escape the world.

## 1.4 The skybox is not the boundary

The visual skybox remains background presentation.

The actual map boundary consists of:

```text
a visible tall procedural cliff belt encircling the entire playable map
+
an authoritative WorldBoundsService boundary
+
physics/collision enforcement
+
AI/navigation exclusion
+
Commander-camera constraints
+
aircraft flight-volume constraints
+
server-side out-of-bounds fallback protection
```

The cliff ring must visually explain why the battlefield ends.

An invisible wall by itself is not acceptable.

## 1.5 Boundary cliffs are mandatory

Every procedural map must be fully enclosed by tall cliffs.

The outer cliff system must:

```text
form a continuous closed ring
have no traversable gaps
be too steep/tall for infantry and ground vehicles
not expose climbable collision seams
not allow roads to exit
not allow tunnels to exit
not allow bridges to cross outward
not allow terrain deformation to breach it
not allow construction on/in the protected cliff belt
blend visually into the generated terrain
use generated geometry/materials without requiring a unique hand-modeled perimeter
```

Aircraft also remain inside the horizontal boundary through flight-volume enforcement. They may not simply fly over the cliffs and leave the map.

## 1.6 Runtime mutation occurs inside the fixed outer perimeter

Terrain may change during a match, but the absolute v2 map boundary remains fixed.

Allowed runtime mutation can include:

```text
foundation grading
craters
raising/lowering terrain
smoothing
road grading
river/canal cuts
land reclamation where configured
bridge damage/rebuild
destructible structures
rubble
controlled tunnel/structure operations
```

The protected outer boundary/cliff belt is immutable during normal gameplay.

## 1.7 Deterministic generation

A world generated from:

```text
generator version
world seed
world profile
content manifest
```

must produce the same authoritative layout.

Subsystem randomness must use deterministic derived seeds rather than sharing one mutable global random stream.

## 1.8 Server authority

The server owns:

```text
world seed/profile
world bounds
terrain operations
procedural structures
resource locations
base sites
navigation revisions
building placement
AI commander decisions
persistent world mutations
```

Clients reconstruct deterministic presentation and receive compact authoritative operations/state.

## 1.9 No replacement physics

Keep WWPhys/OpenW3D.

No Jolt.

No second physics world.

## 1.10 Legacy/static maps remain supported

A normal hand-authored Renegade map must still work.

Procedural-world code must be activated through explicit world/game configuration.

Do not force existing maps through the procedural generator.

---

# 2. V2 PRODUCT MODES

Support these modes on the same canonical systems.

## 2.1 Procedural multiplayer

```text
seed-generated bounded battlefield
human Commander per team
human FPS players
Commander-produced AI
runtime building expansion
optional runtime world mutation
```

## 2.2 Procedural skirmish

```text
human player is Commander of selected faction
friendly AI follows human Commander
enemy team has autonomous AI Commander
same world/economy/power/building APIs as multiplayer
```

## 2.3 Procedural developer sandbox

Allow:

```text
specific seed loading
seed reroll
world-generation step visualization
terrain brushes
hydrology debug
road graph debug
bridge/tunnel generation debug
boundary cliff debug
navigation debug
AI Commander debug
snapshot/rollback
world-operation replay
```

## 2.4 Legacy map mode

No generated world unless explicitly enabled.

The v1.1.0 Commander systems remain usable on authored maps.

---

# 3. TARGET ARCHITECTURE

Extend canonical v1 services into roughly this responsibility layout:

```text
OpenW3D
|
+-- ProceduralWorldService
|   +-- ProceduralWorldProfile
|   +-- WorldSeedService
|   +-- WorldGenerationPipeline
|   +-- WorldGenerationManifest
|   +-- WorldBoundsService
|   +-- BoundaryCliffGenerator
|   +-- WorldChunkManager
|   +-- WorldOperationJournal
|
+-- WorldTerrainSystem                  [existing v1 owner]
|   +-- ProceduralHeightSource
|   +-- MacroTerrainGenerator
|   +-- CliffFeatureGenerator
|   +-- HydrologyGenerator
|   +-- TerrainMaterialGenerator
|   +-- RuntimeTerrainMutation
|
+-- ProceduralInfrastructureService
|   +-- RoadGraphGenerator
|   +-- RoadGeometryGenerator           [reuse v1 road renderer]
|   +-- BridgePlanner                   [reuse v1 sectional bridges]
|   +-- TunnelPlanner
|   +-- TunnelGeometry/ModuleBuilder
|   +-- StreetLightPlacer
|
+-- ProceduralEnvironmentService
|   +-- BiomeGenerator
|   +-- FoliageScatterer                [reuse v1 foliage system]
|   +-- Rock/DebrisScatterer
|   +-- WaterBodyPlanner                [reuse v1 water]
|   +-- TiberiumFieldGenerator
|
+-- ProceduralSettlementService
|   +-- SettlementPlanner
|   +-- StreetPlanner
|   +-- BuildingModelPlacer
|   +-- AbandonedStructurePlacer
|   +-- POIPlanner
|
+-- ProceduralCombatSpaceService
|   +-- InfantrySpaceAnalyzer
|   +-- VehicleSpaceAnalyzer
|   +-- SightlineAnalyzer
|   +-- CoverAnalyzer
|   +-- ConnectivityValidator
|   +-- LocalRepair/RerollPlanner
|
+-- WorldNavigationService
|   +-- NavTileBuilder
|   +-- DynamicNavInvalidation
|   +-- InfantryGraph
|   +-- VehicleGraph
|   +-- AirNavigationVolume
|
+-- RuntimeWorldPersistence
|   +-- GenerationManifest
|   +-- OperationLog
|   +-- ChunkSnapshots
|   +-- ChecksumService
|   +-- JoinSynchronizer
|   +-- SnapshotCompactor
|
+-- AutonomousCommanderService
|   +-- StrategicDirector
|   +-- EconomyPlanner
|   +-- PowerPlanner
|   +-- BaseExpansionPlanner
|   +-- ProductionPlanner
|   +-- DefensePlanner
|   +-- AttackPlanner
|   +-- ReconPlanner
|   +-- LogisticsPlanner
|
+-- AdvancedBuildingCapabilityService
|   +-- FunctionalSubcomponentController
|   +-- LogisticsCapabilityController
|
+-- RuntimeWorldTools
    +-- SeedPreview
    +-- GeneratorStepViewer
    +-- WorldValidationReport
    +-- BoundaryViewer
    +-- NavigationViewer
    +-- AICommanderViewer
```

These names are illustrative. Reuse/extend existing canonical classes when they already own the responsibility.

Do not create empty interface forests merely to match this diagram.

---

# 3A. NORMATIVE PHYSICAL SCALE, REFERENCE ACTORS, AND TRAVERSAL CONTRACTS

This section is normative. Later generators MUST consume these shared definitions rather than inventing local dimensions, slopes, clearances, or movement assumptions.

## 3A.1 World-unit contract

Before procedural geometry work begins, measure the actual OpenW3D/WWPhys collision geometry used by canonical gameplay actors. Do not assume real-world dimensions, marketing dimensions, or visual-model dimensions are authoritative.

Create one versioned `GameplayScaleProfile` or equivalent canonical data owner containing, at minimum:

```text
WORLD_UNITS_PER_METER, if OpenW3D world units are not already documented as meters
REFERENCE_INFANTRY_HEIGHT
REFERENCE_INFANTRY_RADIUS / WIDTH
REFERENCE_INFANTRY_STEP_HEIGHT
REFERENCE_INFANTRY_MAX_SLOPE

GDI_HUMVEE_WIDTH
GDI_HUMVEE_LENGTH
GDI_HUMVEE_HEIGHT
GDI_HUMVEE_MIN_TURN_RADIUS

APC dimensions/turning envelope
MAMMOTH dimensions/turning envelope
HARVESTER dimensions/turning envelope
MRLS dimensions/turning envelope
largest normal ground-vehicle width
largest normal ground-vehicle height
largest required production-vehicle turning envelope
```

Where possible, populate these from canonical physics/collision definitions at startup or from generated authoritative metadata produced from those definitions. If a value must be authored, document the source and validation method.

Every dimensional generator must use the same unit system.

## 3A.2 Traversal profiles

Define canonical movement capability profiles, equivalent to:

```text
InfantryTraversalProfile
LightVehicleTraversalProfile
MediumVehicleTraversalProfile
HeavyVehicleTraversalProfile
HarvesterTraversalProfile
AircraftTraversalProfile
NavalTraversalProfile, only where canonical naval support exists
```

A traversal profile contains at minimum:

```text
collision width/radius
collision height
required horizontal safety clearance
required vertical safety clearance
preferred slope
maximum normal slope
absolute short-segment slope where applicable
maximum step/ledge
minimum turning radius
minimum road corridor width
minimum tunnel width
minimum tunnel height
minimum bridge usable width
water/wading capability
off-road capability
surface restrictions
```

The generator, navigation builder, combat-space validator, road planner, tunnel planner, bridge planner, Harvester-route validator, and construction-clearance logic MUST consume these same profiles.

No subsystem may carry an unrelated private constant for "Humvee width", "vehicle clearance", "max slope", or equivalent geometry rule when the canonical traversal profile already defines it.

## 3A.3 Reference vehicle policy

The GDI Humvee is the minimum reference vehicle for a normal road lane.

This does NOT mean every route only needs to fit a Humvee. Route classes must also declare which traversal profiles they are required to support. Heavy-vehicle, Harvester, factory-exit, and special strategic routes are separately validated against their required profiles.

## 3A.4 Universal traversal-envelope query

Create/reuse one canonical query equivalent to:

```text
CanTraverseEnvelope(route_or_volume, TraversalProfile)
```

It must evaluate both horizontal and vertical clearance and must be usable for:

```text
road lanes
intersections
bridge decks/railings/beams
tunnel interiors/portals
building overhangs
factory exits
rock arches
street furniture
spawn egress
Harvester routes
```

Generated geometry is not considered valid merely because its centerline is connected.

---

# 3B. DETERMINISTIC NUMERIC RULES, STABLE IDS, AND CANONICAL ORDERING

Derived subsystem seeds are necessary but insufficient for deterministic generation. This section defines additional hard determinism requirements.

## 3B.1 RNG contract

Document and version:

```text
RNG algorithm
seed width
seed hashing algorithm
seed-domain strings
integer-to-float conversion where relevant
```

Do not rely on implementation-defined library RNG behavior if client/server/platform parity is required.

## 3B.2 Stable ordering and tie-breaking

All authoritative candidate collections MUST be put into a stable deterministic order before scoring or selection.

Rules:

```text
never use unordered-container iteration order as gameplay order
sort candidate lists by stable generated ID before scoring
quantize authoritative generated positions before comparison/checksum when necessary
resolve equal scores by a documented stable tie-break key
serialize authoritative maps/sets in canonical key order
```

Every scoring system described as choosing a "best" candidate must specify its tie-break.

## 3B.3 Parallel generation

Worker scheduling MUST NOT alter output.

Do not consume one shared mutable RNG stream from parallel jobs. Prefer deterministic per-stage/per-feature/per-cell seed derivation such as:

```text
Hash(WorldSeed, subsystem_domain, stable_parent_id, stable_feature_index)
```

Merging parallel results must use deterministic ordering.

## 3B.4 Authoritative numeric quantization

Where cross-platform floating-point drift could alter branch decisions, define explicit quantization for authoritative procedural values before:

```text
candidate equality/tie comparison
stable ID derivation from position
checksums
serialized manifests
network comparisons
```

Do not quantize purely visual local detail unless necessary.

## 3B.5 Stable generated object identities

Every authoritative generated object must have a stable deterministic identity independent of pointer address, allocation order, thread completion order, or runtime random UUID generation.

Recommended hierarchical examples:

```text
boundary-cliff:segment:87
road:17
road:17:intersection:3
bridge:road17-crossing2
settlement:4
settlement:4:building:11
tiberium:3
tunnel:road12-crossing1
base:gdi
base:nod
```

Stable IDs are used by:

```text
save/load
network replication
checksums
world operations
bridge damage state
snapshot repair
debugging/reproduction
```

## 3B.6 Generator-version compatibility

A persisted world is identified by at least:

```text
generator version
world seed
profile/schema version + hash
content manifest hash
```

A world created by generator version N must remain loadable/reproducible as version N. Do not silently regenerate an old save using a newer algorithm.

If old generator code is retired, a compatible authoritative snapshot/cache/migration path must preserve that world. Generator upgrades do not silently redefine old seeds.

---

# 3C. GENERATION FAILURE, RETRY, AND REPAIR POLICY

Every generator stage already requires bounded retries. The world-level behavior is now explicitly defined.

## 3C.1 Failure escalation hierarchy

When a candidate fails:

```text
1. attempt bounded local geometric adjustment allowed by that stage
2. try bounded alternate candidate for the same feature
3. invoke the documented deterministic local-repair hierarchy if the failure belongs to combat/fairness validation
4. try the next valid higher-level candidate combination where applicable
5. if a hard required feature still cannot be satisfied, return WORLD_GENERATION_FAILED with exact diagnostics
```

Never silently accept a hard-constraint violation.

## 3C.2 Explicit seed behavior

For an explicitly requested developer/host seed:

```text
failure MUST report the failing seed/profile/version
failure MUST NOT silently substitute another seed
```

For a "random new map" request, the host may advance to another seed only if the world profile explicitly enables automatic seed fallback. That fallback sequence must itself be deterministic and logged.

Developer mode may optionally retain/render a failed world for diagnosis.

## 3C.3 Repair priority

When Phase 15 local combat/fairness repair is needed, use the following default priority unless a profile overrides it:

```text
1. adjust existing route geometry within its corridor
2. add/adjust a local terrain ramp
3. add a short road/service-road spur
4. add an infantry side path
5. add a ford/secondary crossing where hydrology and traversal allow it
6. add a bridge where appropriate
7. add a tunnel where appropriate
8. move a local POI/resource anchor within its allowed candidate region
9. move a base anchor within the selected base candidate region
10. select the next base-pair/candidate combination
11. reject the world
```

Every repair type must declare:

```text
maximum affected radius
maximum terrain displacement
maximum new route length
features it may modify
features it may never modify
cost/priority used when multiple repairs are valid
```

"Apply the smallest repair" means lowest valid deterministic repair cost under this hierarchy, not whatever implementation happens to be easiest.

---

# 3D. PROCEDURAL FEATURE OWNERSHIP AND CLEARANCE CORRIDORS

## 3D.1 Feature ownership

The ordered generation pipeline does not grant later stages unrestricted write access to earlier-stage data.

Default ownership:

```text
WorldTerrainSystem owns terrain height/state.
Road generator may request bounded road grading through WorldTerrainSystem.
Bridge planner may request bounded approach grading through WorldTerrainSystem.
Tunnel planner may request explicit portal cuts through WorldTerrainSystem/cliff owner.
Settlement/base placement may request bounded foundation grading through the canonical grading API.
Decoration may never modify authoritative terrain.
Combat-space repair invokes explicit operations against the owning subsystem.
Runtime mutation uses WorldOperation ownership and validation.
```

No subsystem may directly mutate another owner's authoritative state through private backdoors.

## 3D.2 Canonical protected/clearance corridors

Create one canonical `WorldClearanceCorridor` or equivalent representation used by all generators and runtime placement systems.

Required roles include:

```text
FACTORY_EXIT
HARVESTER_ROUTE
CRITICAL_VEHICLE_ROUTE
CRITICAL_INFANTRY_ROUTE
BRIDGE_APPROACH
TUNNEL_APPROACH
SPAWN_EGRESS
BASE_PRIMARY_ENTRANCE
AIRCRAFT_OPERATION_CLEARANCE where needed
```

A corridor defines:

```text
stable ID
role
2D/3D footprint
required traversal profile(s)
minimum clear width/height
whether construction may cross it
whether decoration may occupy it
whether terrain mutation may modify it
priority/criticality
```

Construction, foliage, rocks, settlements, POIs, runtime mutation, and later local repairs MUST query this shared representation.

---

# 4A. WORLD PROFILE FIELD SEMANTICS, DEFAULTS, AND LIMITS

Every configurable profile value must document:

```text
name
units
meaning
default
minimum
maximum
hard constraint vs soft preference
whether changing it alters authoritative generation/checksum
```

Density fields such as:

```text
hill_density
mountain_density
interior_cliff_density
valley_density
```

must explicitly state what the number represents, for example target area coverage, normalized weighting, feature count per area, or probability. Do not leave density semantics implementation-defined.

World size must additionally be validated against gameplay-time targets, not coordinate dimensions alone.

Recommended profile metrics include:

```text
desired_base_to_base_vehicle_travel_time
max_base_to_resource_travel_time
desired_base_to_front_infantry_time
max_mandatory_infantry_exposed_run_time
```

A nominal 4096 x 4096 world is acceptable only if measured gameplay travel time remains within profile bounds.

---

# 10A. ROAD CROSS-SECTIONS, LANES, CURVATURE, INTERSECTIONS, AND OFF-ROAD TRAVERSAL

This section is normative for Phase 8 and Phase 15.

## 10A.1 Lane width derivation

A normal road lane must be wide enough for the GDI Humvee collision envelope plus lateral clearance.

Let:

```text
H = authoritative GDI Humvee collision width
C = desired lateral clearance on each side
```

Default derivation:

```text
C = max(0.35 meters converted to world units, H * 0.15)
NormalLaneWidth = max(H + 2*C, H * 1.30)
```

Round only through documented profile/geometry quantization. The Humvee measurement comes from the canonical physics representation, not real-world HMMWV dimensions.

This is the minimum normal lane baseline. Required routes must also pass traversal-envelope validation for every vehicle class they promise to support.

## 10A.2 RoadCrossSection

Define a versioned road cross-section descriptor equivalent to:

```text
lane_count
lane_width
travel_directions
shoulder_width_left
shoulder_width_right
median_width
divided
sidewalk_left
sidewalk_right
sidewalk_width
curb/barrier type
required traversal profiles
```

Road geometry, bridge selection, tunnel selection, intersection generation, nav, and minimap data MUST inherit from the same cross section.

## 10A.3 Default lane configurations

Unless the world profile explicitly overrides them:

```text
HIGHWAY
    default: 4 lanes, 2 each direction
    may use median/divider
    supports major strategic/base-to-base traffic

PRIMARY_ROAD
    default: 2 lanes
    supports ordinary two-way vehicle traffic

SECONDARY_ROAD
    default: 2 lanes with reduced shoulders, never below required traversal width

CITY_STREET
    default: 2 lanes
    selected arterial streets may be 4 lanes

SERVICE_ROAD
    default: 2 lanes
    may use 1 wide lane only where explicit one-way/low-traffic policy permits

DIRT_TRACK
    1 wide lane or 2 loose wheel-track corridor according to profile
    remains traversable by the declared vehicle classes
```

Four-lane roads are deliberate major infrastructure and must not dominate the battlefield by default. A healthy map should normally combine a limited number of major highway/arterial segments with two-lane roads, secondary routes, off-road terrain, and infantry-only paths.

## 10A.4 Usable width, not mesh width

Validation measures the actual collision/navigation corridor, not merely asphalt mesh width.

For every required lane/route:

```text
sweep the traversal profile along the centerline
sweep through legal lateral lane offsets
verify horizontal clearance
verify vertical clearance
verify curve traversal
verify intersection traversal
```

Guardrails, lamps, rocks, terrain shoulders, bridge railings, tunnel walls, debris, and other collision count against usable width.

## 10A.5 Road geometry constraints

Each road class must define/profile:

```text
preferred grade
maximum normal grade
absolute short-segment grade, if allowed
minimum curve radius
minimum tangent between opposing curves where needed
maximum grade-change rate / vertical-curve constraint
minimum segment length
minimum intersection spacing
shoulder/blend width
terrain grading width
```

Required route geometry is rejected if the designated traversal profiles cannot physically follow it.

## 10A.6 Intersections and lane transitions

Generate explicit geometry and nav behavior for:

```text
T junction
crossroads
base entrance
four-lane to two-lane merge
two-lane to four-lane expansion
city arterial junction
roundabout-like layout where enabled
```

Every intersection must validate required vehicle turning sweeps.

A four-lane road may narrow to a two-lane bridge/tunnel only through an explicit merge/taper segment of sufficient length. Never terminate lanes directly at the bridge/tunnel entrance.

## 10A.7 Road graph topology targets

Unless the profile explicitly requests a chokepoint-heavy map:

```text
major strategic nodes should normally have at least 2 meaningful route choices
bases should normally have at least 2 distinct vehicle approaches
bases should normally have at least 2 distinct infantry approaches
road generation should prefer useful loops over excessive dead ends
```

Route independence is defined in Section 17A; multiple graph paths that immediately merge into the same choke do not count as independent approaches.

## 10A.8 Off-road traversal

Ground vehicles are not restricted to asphalt.

Classify terrain/navigation regions at minimum as:

```text
ROAD
EASY_OFFROAD
ROUGH_OFFROAD
INFANTRY_ONLY
NON_TRAVERSABLE
CLIFF
WATER
```

Off-road classification considers:

```text
slope
roughness
obstacle density
surface/material
clearance
water depth
```

Vehicle AI may prefer roads by cost while retaining legitimate off-road movement where the canonical vehicle can physically travel.

---

# 11A. BRIDGE GEOMETRY, DECK ELEVATION, SUPPORTS, WIDTH, AND ROAD HANDOFFS

This section is normative for Phase 9.

## 11A.1 Bridge width inherits road requirements

Every bridge candidate receives the required `RoadCrossSection`.

Preferred mapping:

```text
2-lane road -> 2-lane bridge
4-lane highway -> 4-lane bridge
```

If a compatible bridge asset/module set does not exist, the road planner may introduce a validated taper/merge before the bridge and expansion afterward. The resulting narrowed bridge still must support the traversal profiles required by the strategic route.

## 11A.2 Minimum-valid deck elevation

Bridge height is not a random aesthetic variable.

The planner chooses the LOWEST valid deck elevation satisfying all hard constraints.

Conceptually:

```text
MinimumDeckZ = max(
    obstacle_clearance_requirement,
    water_clearance_requirement,
    under-road/underpass_clearance_requirement,
    structural-depth requirement,
    endpoint terrain/rim requirement
)
```

Then validate approach grades and structural feasibility.

If the minimum valid elevation creates impossible approaches, the planner MUST try, in deterministic order as applicable:

```text
shift crossing location locally
lengthen road approach
lower bridge if clearance still permits
choose alternate bridge candidate
choose ford where legal
choose tunnel where legal
route around
reject crossing candidate
```

Do not keep raising/warping terrain into an unreasonable artificial bridge approach.

## 11A.3 River bridge rule

For non-navigable rivers, required clearance is based on:

```text
water surface
+ configured flood/visual/gameplay margin
+ bridge structural/deck depth
```

Do not add vessel clearance when no naval traversal exists.

If navigable water is enabled, clearance must additionally satisfy the largest required vessel traversal envelope plus safety margin.

## 11A.4 Ravine/canyon bridge rule

Ravine depth does NOT raise the deck merely because the ravine is deep.

Deck elevation should connect the usable rim/road elevations with the minimum additional elevation needed for grading and structure. A deep canyon generally produces longer supports, not an absurdly elevated roadway.

## 11A.5 Approach-grade solver

Each connected road class supplies preferred and maximum approach grade.

Bridge approach grade must be no greater than the minimum of:

```text
road-class configured limit
required traversal-profile tested limit
```

The planner computes required approach length from elevation change and grade constraint and rejects candidates that do not have sufficient approach room.

## 11A.6 Bridge scoring

Among valid candidates, prefer lower deterministic cost based on:

```text
unnecessary deck-height penalty
approach-grade penalty
span-length penalty
support-complexity penalty
road-detour penalty
terrain-modification penalty
```

Hard failures include:

```text
insufficient obstacle clearance
required vehicle cannot fit
approach impossible
bridge intersects terrain incorrectly
unsupported section
hard-boundary violation
road/bridge cross-section mismatch without legal taper
```

## 11A.7 Supports/piers

Bridge module metadata/profile must define:

```text
preferred span length
maximum unsupported span
support socket positions
support spacing constraints
support-footprint requirements
allowed support terrain/material
forbidden support zones
```

For water crossings, specify whether piers may occupy the active channel and any bank/channel setback. For ravines, supports must terminate on valid terrain/structure rather than floating or penetrating invalid geometry.

## 11A.8 Bridge asset metadata

Each bridge section/template must expose:

```text
usable deck width
lane capacity
section length
connector transforms
support sockets
structural depth
railing/barrier collision bounds
vertical clearance obstructions
damage-state variants
```

## 11A.9 Broken bridge semantics

Bridge descriptors must state whether damage is:

```text
whole-bridge state replacement
individual sectional destruction
```

For every state define:

```text
render representation
collision representation
nav link availability
vehicle eligibility
infantry eligibility
repair/rebuild behavior if supported
```

A broken bridge never remains counted as required connectivity unless an alternate valid route exists.

---

# 12A. TUNNEL CLEARANCE, MODULE CONNECTORS, AND PORTAL CONTRACT

This section is normative for Phase 10.

## 12A.1 Tunnel class dimensions

Every tunnel class declares required traversal profiles and derives:

```text
minimum internal width
minimum internal height
lateral safety clearance
vertical safety clearance
maximum grade
minimum bend radius
minimum straight distance at portal
```

Defaults by class:

```text
INFANTRY_ONLY
    infantry traversal envelope plus fighting/passing clearance

VEHICLE
    designated road/vehicle traversal profiles

MIXED
    vehicle envelope plus explicit safe infantry-side clearance where the design intends simultaneous mixed use

UNDERPASS
    inherits the cross section and traversal class of the road/path passing through
```

## 12A.2 Module metadata

Reusable tunnel/cave modules must expose:

```text
connector sockets/transforms
internal clearance volume
supported curvature/turn angle
supported grade transition
collision bounds
portal compatibility
lighting anchors where relevant
```

Modules are connected by explicit sockets, not guessed bounding-box alignment.

## 12A.3 Portal rules

Portal generation must preserve:

```text
required width/height
minimum straight approach
terrain/cliff structural separation
road cross-section handoff
spawn/build/decoration exclusion
nav link geometry
```

No portal may intersect the immutable boundary band.

---

# 16A. PROCEDURAL BASE LAYOUT, INITIAL BUILDINGS, SPAWN SETS, AND MATCH BOOTSTRAP

Choosing a base site is not sufficient. This section defines how a final base site becomes a playable Renegade base.

## 16A.1 ProceduralBaseLayoutService

Create/reuse a canonical `ProceduralBaseLayoutService` or equivalent under generated-world/base ownership.

Inputs:

```text
final base site
faction
world profile/game mode
road graph
terrain/buildability
resource/Harvester requirements
canonical building descriptors
```

Outputs:

```text
initial building transforms
internal base road/pedestrian layout
factory-exit corridors
Harvester corridors
infantry spawn sets
fallback team spawn region
defensive/clearance zones
Commander expansion reserve
base connection to external road/nav graph
```

## 16A.2 Initial base composition

The world/game profile MUST explicitly declare which initial structures exist. Do not infer this ad hoc from faction names.

Supported policy may include normal faction equivalents such as:

```text
power generation
refinery/economy
infantry production
vehicle production
repair
communications
base defense
construction/command anchor where the game design uses one
```

Whether a structure starts present, is Commander-buildable only, or is disabled is profile data.

## 16A.3 Building placement inside the base

Base layout must validate:

```text
building footprint
clearance footprint
entrances
pedestrian access
vehicle access
factory production exit
Harvester path
repair approach
weapon/defense firing clearance where applicable
aircraft operation clearance where applicable
future Commander expansion space
```

Do not orient a building solely by visual bounding box. Use semantic connector/entrance metadata from Section 28A.

## 16A.4 Infantry spawn anchors

Prefer semantic spawn anchors from the faction's infantry-production structure, for example Barracks/Hand-of-Nod descriptor anchors.

Building semantic descriptors may expose:

```text
INFANTRY_SPAWN_ANCHOR
INFANTRY_SPAWN_REGION
SPAWN_EGRESS
```

The building transform rotates/translates these into world-space spawn candidates.

## 16A.5 Fallback team spawn region

Every generated base must additionally provide a `TEAM_INFANTRY_SPAWN_ZONE` independent of one specific building asset.

It must be:

```text
inside authoritative world bounds
inside the friendly base/protected region
walkable
within slope limit
outside water
outside Tiberium
outside blocking collision
outside vehicle/factory exits
outside Harvester traffic corridors
outside bridge/tunnel portals
outside current building footprints
connected to infantry navigation
not immediately exposed to the enemy base under initial conditions
```

## 16A.6 Spawn pool, not one coordinate

Generate multiple spawn candidates.

Runtime `ChooseRespawnPoint()` must score currently valid candidates using at least:

```text
collision/occupancy clear
valid nav/ground
world-bounds validity
no vehicle occupying the point
no infantry overlap
spawn-egress corridor clear
building/terrain mutation compatibility
enemy proximity/exposure according to game-mode spawn policy
```

The server owns final selection.

`GenerateBaseSpawnSet()` and `ChooseRespawnPoint()` are separate responsibilities. Generation creates legal candidates; runtime gameplay chooses among them based on current conditions.

## 16A.7 Vehicle spawning

Do not create a generic procedural vehicle-spawn system.

Vehicles continue to use canonical factory production semantics:

```text
base planner places/orients factory
building descriptor supplies production anchor/exit
factory exit corridor connects to generated road/vehicle nav
canonical production system spawns vehicle
```

## 16A.8 Match bootstrap contract

The profile/mode must explicitly define:

```text
starting credits
starting wallet cap
starting power state/production
starting buildings
starting units
starting Harvesters
initial Commander role/state
initial Tiberium state/amount
initial shroud/reveal policy
initial player spawn set
initial AI state
```

The live match clock/gameplay begins only after:

```text
authoritative generation finalized
collision committed
navigation committed
initial bases/buildings instantiated
spawn sets validated
factory/Harvester corridors validated
required clients synchronized to the authoritative baseline
```

## 16A.9 Procedural match victory policy

Default procedural multiplayer preserves canonical Renegade win/loss behavior unless a game profile explicitly selects another ruleset.

By default:

```text
Commander death is not team defeat
loss of an arbitrary procedural POI is not team defeat
loss of an optional Construction Yard/command structure is not automatically team defeat unless canonical/game-mode rules explicitly say so
```

Alternate objective/territory modes must declare their own victory rules rather than leaking them into baseline procedural multiplayer.

---

# 17A. QUANTITATIVE FAIRNESS, COVER, SIGHTLINES, ROUTE INDEPENDENCE, AND ECONOMY METRICS

This section turns qualitative phrases such as "reasonable", "meaningful", and "where feasible" into measurable contracts.

## 17A.1 Required profile thresholds

The world profile must provide documented defaults for at least:

```text
max_initial_resource_travel_time_difference
max_projected_initial_income_difference_percent
max_base_to_front_travel_time_difference
max_base_to_base_travel_time_difference
min_vehicle_approach_count
min_infantry_approach_count
min_base_expansion_area
max_single_bridge_dependency, if allowed
max_mandatory_exposed_infantry_run_distance/time
preferred_cover_interval
min_vehicle_lane_clearance
```

## 17A.2 Route independence

Two graph paths count as independent approaches only if they remain topologically/spatially distinct for a configured meaningful portion of the route and do not immediately merge into the same mandatory choke.

For example, three paths that all require the same single bridge count as one bridge-dependent strategic approach for redundancy analysis.

Implement a documented route-overlap/shared-cut metric rather than merely counting path objects.

## 17A.3 Cover classification

Procedural asset metadata and terrain analysis must classify cover meaningfully, for example:

```text
FULL_INFANTRY_COVER
PARTIAL_INFANTRY_COVER
VEHICLE_COVER
CONCEALMENT_ONLY
VISUAL_ONLY
```

A street lamp or tiny decoration does not count as full combat cover merely because it is an object.

Terrain folds/berms may count when line-of-sight testing confirms the intended protection.

## 17A.4 Sightline analysis

Define/document:

```text
representative eye height(s)
route/grid sampling interval
directional sample count
maximum analysis distance
which terrain/objects count as occluders
```

Report distributions such as:

```text
P50 sightline
P90 sightline
maximum sightline
```

Do not rely on one average sightline number.

## 17A.5 Vehicle-route analysis

Vehicle analysis validates:

```text
actual usable lane/corridor width
turning sweep
vertical clearance
grade
bridge/tunnel compatibility
off-road alternate viability where required
repair/logistics access
```

## 17A.6 Tiberium economic fairness

Field fairness is measured by projected economic throughput, not straight-line distance alone.

Field/profile data must define or derive:

```text
field area/radius range
initial yield range
crystal density/harvest density
harvest anchor count
Harvester load amount
regeneration/growth rate if enabled
expected route time
expected early income per minute
```

Initial team economies should remain within the configured projected-income difference after accounting for route length, route speed, turning, bridge/tunnel delays, and Harvester accessibility.

---

# 18A. HYDROLOGY GAMEPLAY DIMENSIONS AND WATER TRAVERSAL

The hydrology generator must define gameplay-scale geometry, not only drainage topology.

Profile fields must define/document as applicable:

```text
stream width range
river width range
river depth range
bank slope range
shore transition width
lake minimum area
pond maximum area
maximum water coverage
```

Actor-water behavior must be explicit by traversal profile:

```text
can wade?
can swim?
maximum traversable depth?
vehicle water-depth limit?
water causes damage?
Harvester water policy?
```

A ford exists only when all required traversal profiles satisfy:

```text
water depth <= capability
bank grade <= capability
bottom slope/roughness <= capability
usable width >= route requirement
```

Do not label a shallow-looking crossing as a ford without traversal validation.

---

# 18B. TERRAIN FEATURE SCALE CONTRACT

Procedural terrain operates at deliberate scales:

```text
MACRO: world-scale masses, ridges, basins, major valleys
MESO: combat-space hills, berms, local valleys, plateaus, ravines
MICRO: primarily visual/detail variation that should not unpredictably invalidate navigation
```

The profile must define minimum/maximum gameplay-scale dimensions for features such as:

```text
hill footprint/radius
plateau usable area
traversable valley width
ravine width/depth
major-cliff spacing
ridge width
```

Micro noise must not create navigation-significant bumps without going through terrain/traversal validation.

---

# 18C. CANONICAL BUILDABILITY CONTRACT

"Buildable area" is one canonical query shared by candidate-base generation and runtime Commander construction.

Equivalent API:

```text
BuildabilityResult EvaluateBuildability(region_or_footprint, BuildingDescriptor, policy)
```

Inputs include:

```text
slope
height variance
water
cliff proximity
road/corridor exclusions
tunnel exclusions
bridge exclusions
resource exclusions
boundary distance
navigation impact
footprint/clearance overlap
required grading amount
```

The candidate-base planner may not call an area buildable using weaker rules than the runtime building-placement validator.

## 18C.1 Foundation grading policy

Profile/defaults must define:

```text
max cut depth
max fill height
max slope change
max affected area
transition apron width
minimum water setback
minimum road/corridor setback
minimum neighboring foundation setback
retaining-wall policy if generated
```

Critical route policy:

```text
FACTORY_EXIT               never fully obstructable
HARVESTER_ROUTE            guaranteed required clearance
CRITICAL_VEHICLE_ROUTE     cannot be fully severed by ordinary placement
CRITICAL_INFANTRY_ROUTE    cannot be fully severed when it is the last required infantry route
OPTIONAL_ROUTE             may be obstructed if resulting world still passes connectivity rules
```

This preserves strategic construction without allowing a player to accidentally or deliberately invalidate the only mandatory route.

---

# 18D. RUNTIME TERRAIN MUTATION / INFRASTRUCTURE INTERACTION MATRIX

Every `WorldOperation` must consult a canonical dependency policy.

Default policy:

| Feature | Default terrain-mutation policy |
|---|---|
| Boundary cliff/hard boundary | Reject |
| Building foundation/footprint | Reject unless building-owned operation explicitly permits |
| Bridge structural anchor/support | Reject |
| Tunnel portal/interior shell | Reject unless tunnel-owned operation explicitly permits |
| Factory exit corridor | Reject mutations that invalidate required clearance |
| Harvester critical corridor | Reject mutations that invalidate required clearance |
| Ordinary paved road | Profile-dependent bounded mutation; road/collision/nav must invalidate/rebuild |
| Dirt track | Usually mutable within traversal/rebuild limits |
| River bank/channel | Profile-dependent; water/hydrology dependencies must be rebuilt |
| Spawn region/egress | Reject mutations that leave no valid spawn/egress |

The roadmap must not treat "terrain operation succeeded" as sufficient when dependent infrastructure becomes incoherent.

---

# 25A. COMMANDER GOVERNANCE NUMERIC RULES

Multiplayer Commander governance must define profile/default values for:

```text
nomination/volunteer duration
voting duration
minimum participation, if any
plurality vs majority rule
tie behavior
candidate withdrawal behavior
removal-vote threshold
removal-vote cooldown
transfer acceptance behavior
disconnect grace period
re-election timing after disconnect/resignation
```

All tie behavior must use deterministic/team-authoritative rules. Do not leave these values to ad hoc UI implementation.

---

# 25B. INITIAL SHROUD, MAP KNOWLEDGE, AND MINIMAP DISCOVERY POLICY

Procedural mode must explicitly define what is known at match start.

Classify separately:

```text
terrain/topography
roads
bridges
tunnel portals
settlements
neutral POIs
Tiberium fields
own buildings/units
enemy buildings
enemy units
```

A recommended default is that static terrain and public infrastructure may be represented on the strategic/minimap base layer while tactical enemy state obeys canonical shroud/radar, but the final policy must be encoded in the game/world profile rather than invented by the renderer.

Own-base visibility and initial friendly spawn/base information must always be sufficient for normal play.

---

# 28A. PROCEDURAL ASSET METADATA AND CONNECTOR DESCRIPTORS

Reusable assets require machine-readable metadata. Generators must not independently reverse-engineer the same W3D geometry through bounding-box guesses.

## 28A.1 Common asset metadata

Every asset eligible for procedural placement should expose or derive:

```text
stable asset/content ID
category
faction where applicable
render bounding box
collision bounding box
placement footprint
clearance footprint
allowed terrain slope
allowed biomes/settlement types
allowed rotations
whether arbitrary scaling is permitted; default should normally be NO
nav obstacle behavior
cover classification
destructibility
LOD/performance cost class
```

## 28A.2 Building metadata

Building/large-structure descriptors should additionally expose where applicable:

```text
entrance anchors
door anchors
pedestrian approach anchors
INFANTRY_SPAWN_ANCHOR / region
SPAWN_EGRESS
vehicle production anchor
factory exit anchor/corridor
Harvester interface/route anchor
repair approach anchor
road connection anchor
roof access information
interior accessibility
defense firing-clearance volume
aircraft clearance volume
```

## 28A.3 Infrastructure metadata

Road/bridge/tunnel modules expose explicit connector transforms rather than relying on bounding-box adjacency.

Bridge metadata follows Section 11A.
Tunnel metadata follows Section 12A.

## 28A.4 Settlement placement metadata

Settlement-capable structures should expose:

```text
preferred street-facing side
entrance relation to street
minimum setback
minimum neighbor spacing
whether duplicate adjacent instances are allowed
interior/roof combat usefulness
```

Settlement profiles define:

```text
block-size range
street width/cross section
building setback range
density target
minimum alley width
maximum settlement footprint
building repetition limit
minimum/target fraction of interior-capable buildings where assets permit
```

Avoid repeated identical neighboring buildings unless the asset pool forces it.

## 28A.5 Missing-asset policy

Classify asset groups as required or optional.

Typical required groups:

```text
initial faction/base buildings required by selected mode
road material/cross-section support
required bridge/tunnel modules for features the selected profile marks mandatory
Tiberium economy assets
```

Typical optional groups:

```text
extra ruins
decorative vegetation variants
street furniture
cosmetic debris variants
```

Missing optional decoration degrades gracefully and logs the fallback. Missing a required gameplay asset causes profile/world validation failure unless an explicit compatible fallback is defined.

---

# 29A. CONTENT MANIFEST, AUTHORITATIVE DECISION MANIFEST, AND CLIENT/SERVER GENERATION BOUNDARY

## 29A.1 Content-manifest inputs

The generation content manifest hash must include every asset/data source capable of changing authoritative geometry or gameplay, including as applicable:

```text
W3D model/collision content
procedural asset descriptors
building semantic descriptors
bridge module definitions
tunnel module definitions
road cross-section definitions
traversal profiles
terrain/generator tables
Tiberium/economy generation tables
world-profile schema/version
other generation-relevant gameplay metadata
```

Purely visual resources may be excluded only when proven unable to alter authoritative generation, collision, navigation, placement, or gameplay.

## 29A.2 Server/client responsibility

Server:

```text
owns authoritative candidate selection and repair decisions
owns final generated feature graph/manifest
owns gameplay checksums
owns all runtime world operations
```

Client:

```text
may reconstruct deterministic geometry only from the server-accepted world identity and authoritative generation decisions
may not independently select alternate candidates/repairs
```

## 29A.3 Authoritative generation decision manifest

In addition to seed/profile/version, persist enough branch decisions to make reconstruction and diagnosis robust, including at minimum where applicable:

```text
selected base candidate IDs
selected resource-field IDs
selected required road graph edges
selected bridge candidates/templates
selected tunnel candidates/module chains
settlement layout/template IDs
important POI IDs
local deterministic repair operations
initial base-layout IDs/transforms
```

This does not require serializing every terrain vertex when deterministic regeneration is valid. It records consequential choices so that a client does not independently reinterpret ambiguous branches.

---

# 24A. AI COMMANDER STRATEGIC METADATA, KNOWLEDGE AGING, AND DECISION STABILITY

## 24A.1 Unit strategic-role metadata

AI planning must not be a hard-coded collection of unit-name checks.

Canonical unit descriptors should expose/derive strategic roles such as:

```text
ANTI_INFANTRY
ANTI_LIGHT_VEHICLE
ANTI_HEAVY_VEHICLE
ANTI_AIR
SCOUT
ARTILLERY
TRANSPORT
REPAIR
HARVESTER
ASSAULT
DEFENSIVE
```

plus:

```text
mobility class
effective range band
preferred terrain/route type
cost
production source
prerequisites
power/capability dependencies where relevant
```

## 24A.2 Knowledge aging

Last-known enemy information must carry:

```text
last observation time
confidence
position uncertainty
confidence-decay rule
forget/ignore threshold
```

The AI may reason from stale intelligence but may not treat indefinitely old contacts as exact current truth.

## 24A.3 Decision stability

AI planners must define:

```text
stable candidate ordering
stable equal-score tie-break
planning interval
minimum goal commitment time
normal replan cooldown
emergency override conditions
```

This prevents rapid oscillation between equally scored goals.

---


# 4. WORLD PROFILE + SEED CONTRACT

Create a versioned procedural world profile.

Suggested schema:

```json
{
  "schema_version": 1,
  "generator_version": 1,
  "seed": 123456789,

  "world": {
    "width": 4096.0,
    "height": 4096.0,
    "chunk_size": 128.0,
    "sea_level": 0.0
  },

  "boundary": {
    "shape": "irregular_ring",
    "cliff_band_width": 180.0,
    "cliff_min_height": 90.0,
    "cliff_max_height": 180.0,
    "hard_barrier_offset": 20.0,
    "air_ceiling": 260.0
  },

  "terrain": {
    "macro_relief": 0.6,
    "hill_density": 0.45,
    "mountain_density": 0.18,
    "interior_cliff_density": 0.16,
    "valley_density": 0.30
  },

  "water": {
    "rivers": true,
    "lakes": true,
    "ponds": true,
    "coastline": true
  },

  "world_features": {
    "forests": true,
    "settlements": true,
    "tunnels": true,
    "highways": true,
    "tiberium_fields": true
  }
}
```

Values are examples only.

## Derived subsystem seeds

Do not consume one global RNG in unpredictable subsystem order.

Derive stable seeds:

```text
TerrainSeed      = Hash(WorldSeed, "terrain")
BoundarySeed     = Hash(WorldSeed, "boundary")
HydrologySeed    = Hash(WorldSeed, "hydrology")
RoadSeed         = Hash(WorldSeed, "roads")
BridgeSeed       = Hash(WorldSeed, "bridges")
TunnelSeed       = Hash(WorldSeed, "tunnels")
BiomeSeed        = Hash(WorldSeed, "biomes")
SettlementSeed   = Hash(WorldSeed, "settlements")
TiberiumSeed     = Hash(WorldSeed, "tiberium")
BaseSeed         = Hash(WorldSeed, "bases")
DecorationSeed   = Hash(WorldSeed, "decoration")
```

Changing one generator must not unnecessarily reshuffle every unrelated subsystem.

## Generation manifest

Persist:

```text
generator version
seed
world profile identity/hash
asset/content manifest hash
important generated anchors
world bounds
base sites
resource sites
road graph checksum
navigation revision
```

---

# 5. AUTHORITATIVE MAP BOUNDARY + ENCIRCLING CLIFF SYSTEM

This is a core v2 system, not cosmetic polish.

## 5.1 Boundary topology

Generate one closed outer boundary around the entire battlefield.

Preferred representation:

```text
closed clockwise 2D polygon/spline
inner playable limit
protected cliff band
outer hard-boundary line
air flight volume derived from same boundary
```

The boundary may be:

```text
roughly rectangular
rounded
irregular natural ring
```

but must never self-intersect.

Avoid a perfectly straight square wall unless the world profile explicitly requests it.

## 5.2 Protected zones

Define:

```text
PLAYABLE_INTERIOR
BOUNDARY_APPROACH
CLIFF_BAND
HARD_BOUNDARY
OUTSIDE_WORLD
```

Example:

```text
playable interior
    -> ordinary terrain/features

boundary approach
    -> terrain naturally rises/changes toward perimeter

cliff band
    -> tall generated perimeter cliffs
    -> no roads/buildings/tunnels/resources

hard boundary
    -> authoritative non-crossable barrier behind/within cliff geometry

outside world
    -> never valid gameplay space
```

## 5.3 Boundary cliff geometry

A heightfield cannot represent a true vertical/overhanging cliff cleanly.

Therefore implement a dedicated procedural cliff mesh path.

Create/adapt something equivalent to:

```cpp
BoundaryCliffGenerator
CliffSpline
CliffSegmentMeshBuilder
CliffCollisionBuilder
CliffMaterialProjector
```

Generate:

```text
upper cliff rim
face vertices
lower toe/blend region
occasional ledges that remain non-traversable
rock variation
deterministic segment noise
simplified collision
```

The cliff ring must connect continuously around the map.

No seam may create:

```text
walkable crack
vehicle ramp
physics hole
AI path leak
camera escape
```

## 5.4 Cliff material

Use the v1 terrain/material/shader system.

Prefer:

```text
triplanar rock projection
macro rock variation
detail normal/material layer
scree/debris material near cliff toe
optional moss/dirt variation by biome
```

Do not require unique UV-authored cliff meshes.

## 5.5 Boundary dimensions

Profile controls:

```text
minimum cliff height
maximum cliff height
band width
rim roughness
face roughness
toe width
collision simplification
```

Boundary cliffs must be tall enough that:

```text
infantry cannot climb them
normal ground vehicles cannot climb them
jump/physics exploits cannot reasonably crest them
```

The hard boundary remains authoritative even if a movement exploit reaches the geometry.

## 5.6 Physics enforcement

Create one authoritative boundary query:

```cpp
bool WorldBoundsService::ContainsGameplayPoint(const Vector3&);
bool WorldBoundsService::ContainsHorizontalPoint(const Vector2&);
Vector3 WorldBoundsService::ClosestValidPoint(...);
```

Integrate with:

```text
infantry movement
vehicle movement
air vehicles
spawn validation
teleports
building placement
terrain edits
AI navigation
Commander camera
```

Normal gameplay should be stopped physically by terrain/cliffs/collision.

Server fallback protection:

```text
remember last valid gameplay transform
if authoritative actor escapes due to exploit/numerical error:
    reject/undo invalid movement
    restore to a safe valid transform
    log the boundary violation
```

Do not use death as the normal boundary mechanic.

## 5.7 Air boundary

Cliffs alone do not contain VTOL/aircraft.

Create a bounded air navigation/flight volume:

```text
same horizontal boundary polygon
configured maximum flight altitude
minimum terrain clearance handled by normal flight logic
```

Near the boundary:

```text
AI aircraft turn/replan before entering exclusion band
human aircraft receive clear boundary feedback
physics/controller prevents continued outward movement
server rejects sustained out-of-volume position
```

The flight ceiling must be below/inside a visually sensible skybox volume.

## 5.8 Commander camera boundary

Commander camera may pan/zoom only inside configured strategic bounds.

It must not fly outside the cliff ring and view the terrain from behind the world.

## 5.9 Boundary interaction with procedural features

Forbidden:

```text
road exits through boundary
highway exits through boundary
tunnel penetrates boundary cliff
bridge spans beyond world
river creates a walkable boundary cut
city overlaps cliff band
Tiberium field overlaps cliff band
base overlaps cliff band
Commander building placed in cliff band
terrain brush modifies cliff band
```

If a river approaches the perimeter, terminate it naturally using one of:

```text
inaccessible waterfall/ravine
enclosed lake/basin
coastal body inside the cliff boundary
subsurface/culvert visual termination that does not create traversal
```

A coastline may exist inside the world, but it must not replace the outer cliff/bounds contract.

## 5.10 Debugging

Render:

```text
playable boundary
cliff band
hard boundary
air volume
camera boundary
forbidden construction band
```

## Acceptance

- entire generated map is visually enclosed by continuous tall cliffs;
- no infantry/vehicle path crosses the perimeter;
- air vehicles cannot fly out;
- Commander camera cannot leave;
- no generated road/tunnel/bridge accidentally exits;
- runtime deformation cannot breach the cliff band;
- server recovers safely from an out-of-bounds exploit;
- no visible cliff seam creates a traversable hole.

---

# 6. PROCEDURAL GENERATION PIPELINE

Use a deterministic staged pipeline.

Required order:

```text
1. validate world profile
2. derive subsystem seeds
3. generate immutable world boundary
4. reserve boundary cliff band
5. generate macro terrain relief
6. choose candidate base regions
7. generate drainage/hydrology
8. classify cliffs/steep terrain
9. establish strategic POIs/resources
10. build high-level traversal graph
11. plan roads/highways
12. resolve bridge crossings
13. resolve tunnel candidates
14. grade roads/bridge approaches/tunnel portals
15. choose final base sites
16. generate settlements/city regions
17. generate Tiberium fields
18. apply terrain materials/biomes
19. scatter forests/rocks/decoration
20. place street lamps and infrastructure props
21. build collision
22. build navigation
23. build shroud/radar/minimap world representation
24. validate FPS/RTS combat quality
25. locally repair/reroll invalid regions
26. finalize generation manifest/checksums
```

Do not place roads before hydrology and then discover that civilization has thoughtfully built six highways through the bottom of a lake.

## Bounded retries

Each generator stage must have:

```text
maximum local retries
deterministic fallback
diagnostic reason
```

Never use unbounded random rerolls.

---

# 7. MACRO TERRAIN GENERATION

Use the v1 `WorldTerrainSystem` as the canonical terrain owner.

## 7.1 Terrain representations

Support:

```text
generated height data
near/detail terrain patches
far/background terrain
separate procedural vertical cliff meshes
separate tunnel/interior geometry
separate W3D building/prop geometry
```

Do not try to force tunnels and vertical cliffs into heightfield-only geometry.

## 7.2 Macro terrain features

Generate deliberate:

```text
plains
rolling hills
large hills
mountain masses
ridges
valleys
basins
plateaus
mesas where profile permits
gullies
ravines
interior cliffs
coastal shelves
```

Terrain should be driven by a combination of:

```text
low-frequency deterministic noise
ridge noise
erosion-like shaping
feature masks
distance fields
hydrology constraints
road/base grading
```

## 7.3 Base candidates

Before final roads/cities, identify several candidate base regions.

Candidate requirements:

```text
enough buildable area
not inside boundary band
not inside major lake/river
multiple possible approach directions
reasonable access to Tiberium
rough symmetry/fairness without requiring mirrored terrain
room for expansion
room for production exits
```

Final base choice occurs after major infrastructure is known.

## 7.4 Terrain chunking

Chunks must support:

```text
generation
rendering
collision
nav rebuild
material rebuild
runtime mutation
save/checksum
```

Chunk boundaries must not produce visible or collision seams.

---

# 8. INTERIOR CLIFFS, HILLS, MOUNTAINS, RAVINES, AND VALLEYS

The user explicitly requires cliffs as normal world features as well as the boundary.

## 8.1 Cliff classification

When generated slope/relief exceeds terrain limits:

```text
classify region as cliff
replace/augment impossible heightfield face with procedural cliff mesh
apply cliff collision
apply cliff material
update navigation
```

Interior cliffs may be:

```text
short escarpments
long canyon walls
mountain faces
quarry-like walls
river gorge walls
coastal cliffs
```

## 8.2 Gameplay rules

Interior cliffs must create:

```text
cover
chokepoints
high ground
valley corridors
bridge opportunities
tunnel opportunities
flanking boundaries
```

Do not create cliff mazes that leave infantry with one absurdly long route.

## 8.3 Traversability

Explicitly classify cliff edges:

```text
NON_TRAVERSABLE
INFANTRY_RAMP_NEARBY
VEHICLE_RAMP_NEARBY
TUNNEL_BYPASS
BRIDGE_CROSSING
```

Every strategically important region must retain required connectivity.

## 8.4 Cliff decoration

Scatter bounded:

```text
rocks
scree
dead vegetation
small ledges
debris
```

without creating accidental climb exploits.

---

# 9. HYDROLOGY: RIVERS, STREAMS, LAKES, PONDS, COASTLINES

Use terrain drainage rather than drawing random blue splines across hills.

## 9.1 Drainage analysis

Compute approximate:

```text
flow direction
flow accumulation
basins/depressions
watersheds
outlets
```

## 9.2 Rivers

Generate rivers from high-flow paths.

Carve:

```text
channel
banks
floodplain/low shelf where applicable
```

Integrate with v1 water renderer.

Support:

```text
main rivers
small streams
tributaries
```

## 9.3 Lakes and ponds

Use natural basins and configured placement.

Provide shore gradients appropriate for:

```text
walkable bank
steep bank
cliff shore
vehicle crossing restrictions
```

## 9.4 Coastlines

A profile may generate ocean/coastal water **inside** the outer bounded world.

Coastlines can include:

```text
beaches
rocky shores
coastal cliffs
harbor/city shoreline
river mouths
```

The outer cliff ring still encloses the absolute map.

## 9.5 Crossings

For every strategically important water barrier, evaluate:

```text
ford
bridge
alternate road
infantry foot crossing
tunnel
long route around
```

Do not accidentally isolate a team because the river generator developed political opinions.

---

# 10. ROAD AND HIGHWAY NETWORK GENERATION

Use the v1 runtime road system.

## 10.1 Road hierarchy

Support:

```text
HIGHWAY
PRIMARY_ROAD
SECONDARY_ROAD
DIRT_TRACK
CITY_STREET
SERVICE_ROAD
```

## 10.2 Graph anchors

Road graph should connect:

```text
bases
Tiberium fields
settlements
major bridges
tunnel portals
strategic POIs
repair/logistics sites
important defensive terrain
```

## 10.3 Path cost

Road planner cost field considers:

```text
slope
cliff
water
river width
forest density
city plan
boundary distance
Tiberium exclusion
base approach
bridge cost
tunnel cost
```

## 10.4 FPS design

Roads are not just vehicle lines.

Where practical include:

```text
ditches
rock/vegetation cover
parallel infantry routes
buildings
terrain folds
side roads
ambush positions
```

Avoid kilometers of completely exposed asphalt.

## 10.5 Intersections

Generate clean:

```text
T junctions
crossroads
roundabout-like spaces when profile permits
city blocks
base entrances
```

## 10.6 Street lamps

Place street lamps using existing lamp models along:

```text
city streets
selected highways
base approach roads
industrial areas
```

Rules:

```text
deterministic spacing
collision where appropriate
do not block sidewalks/vehicle lanes
LOD/culling
destructibility if existing model supports it
night-light integration if used
```

---

# 11. PROCEDURAL BRIDGES

Use v1 sectional bridge infrastructure.

## 11.1 Candidate generation

Create bridge candidates where:

```text
road crosses river
road crosses ravine
highway crosses low terrain
strategic shortcut justifies cost
```

## 11.2 Bridge composition

Use:

```text
START_CAP
SPAN
END_CAP
SUPPORT
OPTIONAL_DECORATION
```

Support varying length.

## 11.3 Bridge gameplay

Bridge state:

```text
PRISTINE
DAMAGED
CRITICAL
BROKEN
```

State affects:

```text
collision
AI traversal
vehicle traversal
Commander map
alternative routing
```

## 11.4 Mixed/broken bridge generation

World profile may deliberately spawn:

```text
intact strategic bridges
damaged but usable bridges
broken abandoned bridges
```

A broken bridge is scenery/strategic obstruction unless repair/rebuild mechanics explicitly make it operational.

## 11.5 FPS consideration

Where a bridge is a major chokepoint, provide at least one meaningful alternate strategy when possible:

```text
infantry path below
tunnel
ford
second bridge
long flank
```

---

# 12. TUNNELS, CAVES, AND UNDERPASSES

Heightfields cannot represent these correctly.

Use separate procedural/modular W3D geometry.

## 12.1 Tunnel use cases

Generate tunnels for:

```text
mountain/ridge road passage
infantry flank
city underpass
base back route
river/road underpass
natural cave-like shortcut when profile permits
```

## 12.2 Geometry strategy

Preferred order:

1. reuse existing modular tunnel/cave W3D sections where available;
2. procedurally join/transform sections;
3. generate simple runtime tunnel tube/portal geometry only where modular assets cannot satisfy the route.

Do not require the user to hand-model every tunnel.

## 12.3 Portals

Portals must:

```text
blend into terrain/cliff face
have correct collision
connect nav graphs
support lighting transition
avoid terrain z-fighting
```

## 12.4 Boundary prohibition

No tunnel may cross the immutable outer cliff/hard boundary.

## 12.5 AI

Navigation must explicitly represent:

```text
entrance
interior
exit
clearance class
vehicle/infantry eligibility
```

---

# 13. BIOMES, TERRAIN MATERIALS, FORESTS, AND NATURAL DECORATION

## 13.1 Biome fields

Derive from:

```text
height
slope
moisture
water distance
temperature/profile
Tiberium contamination where applicable
urban influence
```

## 13.2 Terrain material assignment

Use v1 terrain texture system:

```text
base soil
grass
rock
cliff
wet shore
sand
mud
road shoulder
Tiberium-affected ground
urban dirt
```

## 13.3 Forests

Use existing tree models and v1 foliage batching.

Generate:

```text
dense forest
sparse woodland
tree lines
dead forest
river vegetation
city-edge vegetation
```

Gameplay rules:

```text
do not fully block required routes
create infantry concealment/cover
do not make vehicle navigation unpredictable
use collision only where gameplay-relevant
use LOD/impostor/batching
```

## 13.4 Rocks/debris

Scatter from reusable models or lightweight generated geometry.

Avoid:

```text
blocking factory exits
blocking mandatory roads
clipping through buildings
forming accidental boundary ramps
```

---

# 14. SETTLEMENTS, ABANDONED BUILDINGS, INDUSTRIAL AREAS, AND POIS

## 14.1 Settlement planner

Generate small city/settlement regions around selected road graph nodes.

Support:

```text
small abandoned town
industrial yard
roadside structures
warehouse district
research/communications outpost
ruins
Tiberium-contaminated settlement
```

## 14.2 Buildings

Use existing W3D building/prop models where available.

The generator controls:

```text
transform
spacing
street relation
collision validation
entrance orientation
nav access
interior eligibility
```

Do not crop/modify models in runtime simply to fit.

## 14.3 Street layout

Support:

```text
simple grid
linear roadside settlement
industrial loop
irregular small town
```

## 14.4 FPS purpose

Settlements should create:

```text
infantry cover
interiors
rooftop/high-ground opportunities where supported
vehicle streets
alleys
flanking
defensible intersections
```

## 14.5 POIs

Generate optional:

```text
abandoned bunker
destroyed convoy
Tiberium research site
damaged bridge
repair depot
communications relay
ruins
neutral defensive position
```

POIs must not become mandatory clutter.

---

# 15. TIBERIUM FIELD GENERATION

## 15.1 Field placement

Generate Tiberium fields based on:

```text
distance from bases
road access
harvester accessibility
terrain slope
water exclusion
boundary exclusion
strategic contestability
```

## 15.2 Fairness

Each faction must have:

```text
at least one viable early resource field
harvester route
reasonable contestable resource opportunities
```

Do not require mirrored maps, but avoid seed-based economic suicide.

## 15.3 Field geometry

Generate:

```text
field footprint
Tiberium ground material/mask
crystal/model scatter
harvester navigation area
field center/harvest anchors
```

Reuse canonical Renegade Tiberium damage/harvesting behavior.

## 15.4 Growth/regeneration

If enabled by profile, Tiberium growth uses server-authoritative bounded rules.

Do not let growth invade:

```text
building interiors
boundary cliff band
roads unless deliberately allowed
factory exits
critical spawn zones
```

---

# 16. BASE SITE GENERATION + DYNAMIC BASE EXPANSION

## 16.1 Initial base sites

Choose final GDI/Nod base sites after hydrology and high-level traversal are known.

Consider:

```text
buildable area
resource access
travel time
number of attack approaches
defensible terrain
road access
vehicle exits
air space
distance from boundary
Commander expansion room
```

## 16.2 Fairness without forced symmetry

Evaluate:

```text
base-to-base travel time
early Tiberium income opportunity
number of viable routes
high-ground exposure
bridge/tunnel dependency
available construction area
```

Repair unfair local regions before finalizing the world.

## 16.3 Expansion areas

Mark candidate:

```text
forward base zones
defensive outposts
resource expansions
repair/logistics positions
```

Commander building placement still uses v1 server validation.

## 16.4 Construction terrain grading

A valid Commander placement may request bounded foundation grading:

```text
flatten
smooth
small cut/fill
retaining transition
```

Never grade:

```text
boundary cliff band
major immutable cliff
protected river channel without explicit bridge/water rule
active tunnel
another building
```

---

# 17. FPS / RTS COMBAT-SPACE VALIDATION

Procedural generation is not complete merely because geometry exists.

Create `ProceduralCombatSpaceService` or equivalent validation functions.

## 17.1 Infantry metrics

Evaluate representative routes for:

```text
cover interval
average exposed run length
sightline length distribution
available flank count
interior/tunnel availability
choke severity
```

## 17.2 Vehicle metrics

Evaluate:

```text
road/terrain lane width
turn radius
bridge clearance
slope
open engagement distance
alternate routes
repair access
```

## 17.3 Mixed combat

Important objectives should provide:

```text
vehicle-accessible approach
infantry-accessible approach
cover near objective
at least one alternate attack vector where feasible
```

## 17.4 Travel-time bounds

World profile defines desired ranges for:

```text
base-to-front
base-to-resource
base-to-base
resource-to-resource
major POI access
```

Reject/repair absurd travel-time seeds.

## 17.5 Local repair instead of whole-world reroll

When validation fails:

```text
identify offending region/graph edge
modify local terrain/path/cover/infrastructure
rebuild affected chunks
revalidate
```

Only reroll the whole map when bounded local repair cannot satisfy hard constraints.

## 17.6 Mandatory hard constraints

Generation cannot finalize if any are true:

```text
a faction has no viable resource path
bases are disconnected
required vehicle path is impossible
required infantry path is impossible
a road exits world bounds
a tunnel exits world bounds
boundary cliff ring has a traversal leak
air flight volume leaks
base lies in protected boundary band
```

---

# 18. COLLISION, NAVIGATION, VISIBILITY, LIGHTING, AND MINIMAP

## 18.1 Collision

Generated terrain/cliffs/structures must create WWPhys/OpenW3D collision.

Use simplified collision where render mesh complexity is unnecessary.

## 18.2 Navigation

Maintain separate capability knowledge for:

```text
infantry
ground vehicle
air vehicle
naval where supported
```

Dynamic nav data must respond to:

```text
bridge destruction
new building
building destruction
terrain mutation
road mutation
tunnel availability
construction footprint
```

## 18.3 Navigation revisions

AI may not enter a chunk whose required collision/nav rebuild has not committed.

World operation completion emits:

```text
world revision
terrain revision
nav revision
collision revision
```

as appropriate.

## 18.4 Visibility/culling

Generated objects cannot depend on authored legacy VIS identifiers.

Use:

```text
spatial index
frustum culling
cell/chunk visibility
distance/LOD
occlusion support where practical
```

## 18.5 Lighting

Dynamic/generated geometry uses a runtime-compatible lighting path.

Do not require offline baked lighting for generated terrain/buildings/roads.

Existing authored maps may keep baked behavior.

## 18.6 Minimap / tactical map

Generate a strategic map representation from:

```text
terrain height/material
water
roads
bridges
tunnels/portals
settlements
bases
boundary cliffs
Tiberium
known tactical contacts
```

Commander map must respect shroud/radar intelligence.

---

# 19. RUNTIME TERRAIN MUTATION + WORLD OPERATIONS

Use ordered server-authoritative operations.

## 19.1 Operation types

Support a versioned set such as:

```text
RAISE
LOWER
FLATTEN
SMOOTH
GRADE_FOUNDATION
CARVE_ROAD
CARVE_CHANNEL
CREATE_CRATER
RECLAIM_SHALLOW_WATER where profile permits
RESTORE_REGION from authoritative snapshot
```

## 19.2 Protected areas

Reject operations affecting:

```text
boundary cliff band
hard boundary
critical world metadata
active building footprint unless operation explicitly owns it
bridge structural anchors unless bridge operation owns it
tunnel portal/interior unless tunnel operation owns it
```

## 19.3 Transaction

A world operation:

1. validates against current world revision;
2. reserves affected chunks;
3. computes terrain result;
4. rebuilds terrain render data;
5. rebuilds collision;
6. rebuilds materials as needed;
7. queues nav invalidation/rebuild;
8. updates water/road dependencies when affected;
9. commits one new world revision;
10. replicates compact operation/result checksum.

## 19.4 Craters / battlefield damage

Weapons explicitly configured for terrain deformation may create bounded craters.

Rules:

```text
hard maximum radius/depth
server rate limit
no boundary damage
no base/building invalidation without explicit policy
nav/collision rebuild budget
visual LOD
```

Do not make every bullet deform terrain.

## 19.5 Runtime building rubble

Destroyed generated structures may produce:

```text
bounded debris/rubble visual
optional simplified collision
nav obstacle
```

Keep debris budgets bounded.

---

# 20. WORLD OPERATION JOURNAL, SNAPSHOTS, LATE JOIN, AND ROLLBACK

## 20.1 Baseline + operations

Network/persistence model:

```text
generation manifest
+
deterministic base world
+
ordered runtime world operations
+
periodic compact snapshots
```

Do not transmit complete terrain meshes to every late joiner.

## 20.2 Operation record

Store:

```text
operation id
world revision before/after
operation type
parameters
affected chunk IDs
seed if required
result checksum
server timestamp/tick
```

## 20.3 Snapshot compaction

After configurable operation count/time:

```text
compact affected chunks into authoritative snapshots
discard/rebase obsolete operation history where safe
```

## 20.4 Late join

Late join flow:

1. receive generator/profile/version;
2. generate/load deterministic baseline;
3. receive latest compact snapshots;
4. apply post-snapshot operations;
5. verify checksums;
6. receive dynamic object/building/AI state;
7. enter match.

## 20.5 Repair

If client checksum mismatches:

```text
request affected chunk snapshot
repair only bad region
recompute checksum
```

Do not automatically resend the whole world.

## 20.6 Rollback

Developer/admin rollback may restore a prior authoritative world snapshot.

Gameplay rollback is not required as a casual player feature.

---

# 21. ADVANCED BUILDING FUNCTIONAL SUBCOMPONENTS

Build on v1's complete W3D building semantic descriptors.

Do not replace existing whole-building health/power state.

Add optional function-affecting subcomponents where suitable model parts/nodes exist.

## 21.1 Capability roles

Example JSON semantic roles:

```text
power_generator_module
communications_array
radar_dish
refinery_processor
tiberium_storage
factory_production_module
factory_exit_mechanism
repair_module
defense_targeting_module
defense_weapon_module
```

## 21.2 Effects

Examples:

```text
damaged communications array
    -> reduced communications radius

destroyed radar dish
    -> radar capability disabled while building may remain alive

damaged refinery processor
    -> reduced processing throughput

damaged factory production module
    -> slower production

destroyed factory exit mechanism
    -> production blocked until repaired if canonical geometry supports it

damaged power module
    -> reduced generation

damaged repair module
    -> slower repair rate
```

Only implement effects that can be represented coherently by actual building model parts/semantics.

## 21.3 Repair

Existing Renegade repair remains primary.

Functional subcomponent restoration should integrate with normal building repair rather than introducing a disconnected repair minigame.

---

# 22. LOGISTICS + PHYSICAL INFRASTRUCTURE

The logistics layer must deepen physical battlefield objectives without making ordinary Renegade FPS players perform tedious accounting.

## 22.1 Default philosophy

Keep core player purchases simple.

Logistics primarily affects Commander/AI strategic throughput.

## 22.2 Supported logistics concepts

Potential configurable systems:

```text
Tiberium harvester routes
forward repair/rearm points
construction support radius
supply/repair convoy AI
factory staging capacity
forward operating bases
```

## 22.3 Physical dependency

Where logistics matters, represent it through attackable objects/routes.

Avoid abstract global modifiers with no physical counterplay.

## 22.4 Convoys

Optional AI logistics vehicles may:

```text
carry repair support
move supplies/resources
reinforce forward bases
```

They use canonical vehicle/path/order systems.

## 22.5 Commander information

Commander tactical map displays:

```text
critical logistics routes
blocked routes
damaged infrastructure
forward repair coverage
```

subject to team intelligence rules.

---

# 23. OPTIONAL ADVANCED VEHICLE DAMAGE / RECOVERY

Do not alter default Renegade vehicle handling unless enabled by game profile.

## 23.1 Optional subsystem damage

Possible:

```text
engine damage -> reduced acceleration/top speed
track/wheel damage -> reduced mobility
turret/weapon module damage -> reduced weapon behavior where supported
sensor damage -> reduced AI targeting
```

## 23.2 Repair

Canonical Repair Facility/Repair Bay and existing repair mechanics restore subsystems.

## 23.3 Recovery/support

Optional:

```text
repair vehicle
recovery vehicle
escort behavior
retreat-to-repair AI
```

Do not make fuel micromanagement mandatory in normal v2 gameplay.

---

# 24. AUTONOMOUS AI COMMANDER

The v1 human Commander API is the only action surface the AI Commander may use.

No AI cheats that directly spawn buildings/units or bypass power/credits.

## 24.1 Roles

Create one server-owned AI Commander per autonomous faction.

Strategic responsibilities:

```text
economy
power
base construction
production
defense
offense
recon
repair
resource control
expansion
logistics
```

## 24.2 Strategic world model

AI Commander reasons from legitimate team intelligence:

```text
known enemy buildings
last-known enemy concentrations
friendly strength
resource fields
roads/bridges/tunnels
power headroom
production capability
damaged infrastructure
territory/POI control
```

It does not inspect hidden enemy truth.

## 24.3 Economy planner

Decide:

```text
when another Power Plant is needed
when more Silo capacity is useful
when additional Refinery/resource infrastructure is useful
when production/defense spending should pause
```

## 24.4 Base expansion

Use the same Commander building placement validator.

Evaluate candidate expansion sites based on:

```text
resource proximity
road access
defensibility
enemy pressure
power/logistics
buildable area
distance from boundary
```

## 24.5 Production planner

Select infantry/ground/air mixes based on:

```text
known enemy composition
terrain
bridge/tunnel network
available factories
credits
population caps
current objectives
```

## 24.6 Defense planner

Use:

```text
threat approach graph
high-value buildings
bridge/tunnel chokepoints
Tiberium routes
power infrastructure
communications coverage
```

## 24.7 Attack planner

Create multi-stage plans:

```text
recon
assemble force
choose route
attack objective
retreat/regroup
exploit success
```

Do not merely issue every unit ATTACK_MOVE toward enemy base.

## 24.8 Difficulty

Difficulty should alter:

```text
planning interval
reaction time
risk tolerance
resource reserve
coordination quality
prediction horizon
```

Do not simply grant omniscience or impossible resources unless a separate explicit handicap option is enabled.

## 24.9 Diagnostics

Expose:

```text
current strategic goals
candidate goals
utility/score reasons
known enemy intelligence
planned construction
planned production
power/economy state
attack route
retreat conditions
```

---

# 25. COMMANDER SELECTION / MULTIPLAYER GOVERNANCE

If v1 does not already contain a final Commander assignment policy, complete it here before autonomous/faction campaign work depends on it.

## 25.1 Multiplayer

Support:

```text
initial volunteer/candidate period
team vote
commander resignation
commander removal vote
commander transfer
disconnect recovery
temporary no-commander state
```

No first-player-wins permanent rule.

## 25.2 Skirmish

Human player is Commander of selected team.

Enemy receives AI Commander.

Friendly AI remains subordinate.

## 25.3 Commander death

Because v1 Commander is embodied, death exits strategic camera but does not necessarily remove the player's team Commander role.

Mode policy decides whether the same player can re-enter after respawn.

---

# 26. PERSISTENT CAMPAIGN / STRATEGIC LAYER

This phase begins only after procedural matches and AI Commanders are reliable.

## 26.1 Campaign map

Represent strategic territories/nodes containing:

```text
procedural world profile
seed
faction owner
resource value
tech/resource bonuses
adjacent territories
mission modifiers
```

## 26.2 Persistent progression

Potential persistent state:

```text
territory ownership
campaign resources
unlocked building/unit options
commander upgrades where appropriate
AI faction strength
veterancy rules if enabled
```

Do not make ordinary multiplayer dependent on campaign persistence.

## 26.3 Battle generation

A campaign battle selects:

```text
seed
world profile
terrain/biome style
starting base configuration
faction tech
special objectives
```

then uses the same procedural generator as skirmish/multiplayer.

## 26.4 Save versioning

Campaign persistence uses explicit schema/version migrations.

---

# 27. PROCEDURAL WORLD TOOLCHAIN

The generator will be miserable to tune without tools. Build them.

## 27.1 Seed preview

Tool accepts:

```text
seed
profile
generator version
```

and outputs:

```text
top-down terrain preview
boundary
cliffs
water
roads
bridges
tunnels
bases
Tiberium
settlements
validation scores
```

## 27.2 Step viewer

Allow viewing generation after:

```text
macro terrain
hydrology
roads
bridges
tunnels
settlements
resources
biomes
navigation
```

## 27.3 Validation report

Generate machine-readable and human-readable diagnostics:

```text
hard constraint failures
base fairness
route counts
travel times
cliff leaks
boundary leaks
road exits
nav disconnected components
resource accessibility
FPS cover/sightline metrics
```

## 27.4 Runtime debug overlays

Support:

```text
chunk bounds
terrain LOD
cliff collision
world boundary
air volume
hydrology flow
road graph
bridge graph
tunnel graph
infantry nav
vehicle nav
AI Commander goals
world revisions
dirty chunks
```

## 27.5 Reproduction command

Every procedural-world crash/validation failure should log a concise reproduction string:

```text
generator_version=<n>
seed=<n>
profile=<name/hash>
content_manifest=<hash>
```

---

# 28. ASSET STRATEGY

The system should reduce dependence on custom modeling, not pretend art assets do not exist.

## 28.1 Reuse existing assets

Prefer existing Renegade/OpenW3D-compatible assets for:

```text
trees
street lamps
rocks
Tiberium crystals
abandoned buildings
city/industrial props
bridge detail pieces where available
tunnel modules where available
walls/barriers
debris
```

## 28.2 Procedural geometry

Generate where practical:

```text
terrain
roads
road shoulders
boundary cliff faces
interior cliff faces
simple retaining structures
simple tunnel tubes/portals where modules cannot satisfy route
simple water surfaces
construction/foundation geometry
```

## 28.3 Textures/materials

At minimum the final content set should provide reusable materials for:

```text
soil
grass
rock/cliff
sand
wet/mud
road/asphalt
dirt track
Tiberium contamination
urban ground
tunnel/interior surfaces where generated
```

Use triplanar/procedural blending to reduce UV-authoring requirements.

---

# 29. NETWORKING CONTRACT

## 29.1 Do not replicate raw generated world unnecessarily

Prefer:

```text
seed/profile/version
generation manifest
world operations
snapshots
checksums
dynamic gameplay objects
```

## 29.2 Protocol negotiation

Client/server verify:

```text
generator version
content manifest compatibility
world profile/schema compatibility
required v1/v2 network protocol
```

Reject incompatible clients cleanly.

## 29.3 Server validation

All requests involving:

```text
terrain mutation
building placement
Commander action
AI production
world operation
snapshot/rollback
```

are server validated.

---

# 30. PERFORMANCE + STREAMING / CHUNK BUDGETS

The world is finite but may be much larger than classic Renegade maps.

## 30.1 Chunk lifecycle

Each chunk can independently track:

```text
terrain render resources
far LOD representation
collision
navigation
foliage
surface marks
roads
water dependencies
dirty state
snapshot state
```

## 30.2 Client LOD

Aggressively reduce:

```text
far terrain detail
far foliage
far props
weather
particles
surface marks
AI visual detail
```

before harming local FPS responsiveness.

## 30.3 Server budgets

Budget:

```text
AI think updates
path requests
nav rebuilds
terrain operations
power/Commander updates
snapshot compaction
```

Stagger expensive work.

## 30.4 Generation time

Support:

```text
pre-generation before match start
cached generated-world data keyed by seed/profile/version/content hash
background preparation only when it does not violate session/runtime constraints
```

Do not make clients independently generate authoritative gameplay geometry while already entering live combat unless the protocol explicitly guarantees readiness.

---

# 31. SECURITY / ABUSE RESISTANCE

Validate:

```text
seed/profile supplied by authorized host
world operation permissions
terrain brush limits
Commander build permissions
boundary edits forbidden
snapshot/rollback admin permissions
AI Commander cannot bypass economy
```

Rate limit expensive requests.

Never accept client-supplied:

```text
raw terrain mesh
arbitrary collision geometry
arbitrary world bounds
unvalidated building transform
```

---

# 32. TEST / VALIDATION CATALOG

Section 0B still controls when these run.

These are cumulative gate tests, not per-edit chores.

## 32.1 Boundary

```text
continuous cliff ring
no collision seam escape
no infantry escape
no ground vehicle escape
no aircraft escape
Commander camera confinement
road/tunnel/bridge boundary rejection
terrain mutation boundary rejection
server out-of-bounds recovery
```

## 32.2 Determinism

```text
same seed/profile/version -> same checksums
different subsystem generation order does not alter unrelated outputs
late join reconstructs same baseline
```

## 32.3 Terrain

```text
chunk seams
near/far LOD
collision
cliff collision
base grading
runtime crater
protected cliff-band immutability
```

## 32.4 Hydrology

```text
river flows downhill
lake containment
shore collision
bridge candidate detection
no traversable boundary river leak
```

## 32.5 Roads/bridges/tunnels

```text
connected route graph
road grade limits
bridge clearance
broken bridge nav update
tunnel portal nav/collision
no boundary crossing
```

## 32.6 Resources/base fairness

```text
both factions viable initial Tiberium
harvester route
base approach count
base expansion room
reasonable travel-time difference
```

## 32.7 FPS combat quality

```text
representative infantry cover intervals
multiple objective approaches
no required multi-minute exposed infantry run
vehicle route width
mixed infantry/vehicle access
```

## 32.8 Runtime mutation

```text
ordered operations
collision rebuild
nav rebuild
water/road invalidation
snapshot compaction
checksum repair
save/load
```

## 32.9 AI Commander

```text
builds needed power
responds to power deficit
builds production
harvests resources
defends
attacks through valid route
does not use hidden intelligence
recovers after bridge loss
expands base
```

## 32.10 Soak scenarios

Reserve for explicit hard gates:

```text
large procedural multiplayer world
large skirmish with AI Commander
many AI units
multiple construction sites
bridge destruction
terrain mutations
late joins
save/load
long-running match
```

---

# 33. REQUIRED DOCUMENTATION

Maintain:

```text
docs/procedural_world/Architecture.md
docs/procedural_world/GenerationPipeline.md
docs/procedural_world/WorldProfileSchema.md
docs/procedural_world/WorldBoundsAndBoundaryCliffs.md
docs/procedural_world/TerrainGeneration.md
docs/procedural_world/Hydrology.md
docs/procedural_world/RoadsBridgesTunnels.md
docs/procedural_world/BiomesAndFoliage.md
docs/procedural_world/SettlementsAndPOIs.md
docs/procedural_world/TiberiumGeneration.md
docs/procedural_world/CombatSpaceValidation.md
docs/procedural_world/RuntimeMutation.md
docs/procedural_world/Navigation.md
docs/procedural_world/NetworkingAndPersistence.md
docs/procedural_world/GeneratorDeterminism.md

docs/ai_commander/Architecture.md
docs/ai_commander/StrategicWorldModel.md
docs/ai_commander/EconomyAndPower.md
docs/ai_commander/BaseExpansion.md
docs/ai_commander/ProductionAndCombat.md

docs/gameplay/AdvancedBuildingCapabilities.md
docs/gameplay/Logistics.md

docs/tools/ProceduralWorldTools.md
```

Every generator feature should document:

```text
inputs
outputs
seed derivation
hard constraints
soft constraints
fallback behavior
runtime invalidation dependencies
save/network representation
```

---

# 34. IMPLEMENTATION PHASES

Execute these phases in order.

The earlier sections define architecture and behavior. This section converts those requirements into **implementation work packages**. Claude should be able to begin any phase from this section plus the referenced earlier design section without inventing missing sequencing.

## 34.0 Mandatory phase execution protocol

For every phase below, Claude MUST follow this sequence:

```text
A. Confirm all prerequisite phases are COMPLETE in completed_features.md.
B. Read the referenced design sections for the phase.
C. Search the current repository for the canonical v1/v2 owner symbols named by the roadmap.
D. Record the actual source files/classes/functions discovered in the phase WIP entry.
E. Reuse/extend those canonical owners. Do not create parallel subsystem implementations.
F. Implement the numbered steps in order unless a repository discovery proves a small local reorder is required.
G. Update WIP_features.md after each major numbered step or architectural discovery.
H. Run only the targeted validation listed for the phase, following Section 0B.
I. Satisfy the phase gate.
J. Move the phase to completed_features.md only after the gate is satisfied.
```

### Required WIP phase record

Each active phase entry should contain:

```text
Phase:
Status:
Prerequisite phases:
Design sections read:
Canonical source owners found:
Files currently modified:
Steps completed:
Current step:
Known blocker:
Questions requiring user answer:
Targeted validation already passed:
Acceptance items still outstanding:
Exact next action:
```

### Do not guess rule

If a symbol/path/class named in this roadmap does not exist exactly as described:

1. search for the equivalent current canonical owner;
2. inspect call sites and ownership;
3. document the discovered equivalent in WIP;
4. use the existing owner;
5. only create a new subsystem when no canonical owner exists and this roadmap explicitly requires one.

Do **not** invent a second implementation because a roadmap example class name differs from the current repository.

### v2.2.0 cross-phase contract enforcement

The normative sections added in v2.2.0 are mandatory implementation requirements, not design notes. When a phase below touches one of these areas, Claude MUST read and implement the corresponding contract even if the older phase text uses shorter wording.

Mandatory bindings:

```text
Phase 1  -> Sections 3B, 4A, 29A
Phase 2  -> Sections 3A, 3D
Phase 3  -> Sections 3A, 3D
Phase 4  -> Sections 3A, 18B
Phase 5  -> Sections 3A, 18B
Phase 6  -> Sections 3A, 18A
Phase 7  -> Sections 17A, 18C
Phase 8  -> Sections 3A, 3D, 10A, 17A, 28A
Phase 9  -> Sections 3A, 3D, 10A, 11A, 28A
Phase 10 -> Sections 3A, 3D, 10A, 12A, 28A
Phase 11 -> Sections 16A, 17A, 18C, 28A
Phase 12 -> Sections 3A, 17A, 18A
Phase 13 -> Sections 3D, 17A, 28A
Phase 14 -> Sections 3D, 17A, 28A
Phase 15 -> Sections 3A, 3C, 3D, 10A, 11A, 12A, 17A, 18A, 18B, 18C
Phase 16 -> Sections 3A, 16A, 17A, 25B, 29A
Phase 17 -> Sections 3D, 18C, 18D
Phase 18 -> Sections 3D, 18D, 29A
Phase 19 -> Sections 3B, 29A
Phase 23 -> Sections 25A
Phase 24 -> Sections 24A
Phase 25 -> Sections 24A
Phase 27 -> Sections 3B, 17A, 29A
Phase 28 -> Sections 3B, 16A, 29A
Phase 30 -> all v2.2.0 normative sections
```

Additional phase gates introduced by v2.2.0:

```text
Phase 1 cannot complete until RNG algorithm, stable ordering/tie-break, stable generated-ID rules,
content-manifest inputs, and old-generator-version compatibility behavior are documented.

Phase 7 cannot call a candidate region "buildable" using weaker rules than the runtime
EvaluateBuildability contract.

Phase 8 cannot complete until road cross sections explicitly support two-lane roads and four-lane
highways/arterials, lane width is derived from the measured Humvee collision width, intersections
and lane transitions validate turning envelopes, and off-road traversal remains represented.

Phase 9 cannot complete until bridge width inherits the incoming road cross section, deck elevation
uses the lowest-valid-elevation solver, approach grade is validated, supports are structurally grounded,
and broken-state collision/nav semantics are explicit.

Phase 10 cannot complete until tunnel class width/height/grade/bend dimensions derive from traversal
profiles and modular tunnel pieces use connector metadata rather than guessed bounding-box alignment.

Phase 11 cannot complete merely by selecting two base coordinates. It must also establish the
procedural base-layout inputs required by Section 16A and reserve the base regions/corridors needed
for buildings, factory exits, Harvesters, spawn sets, and expansion.

Before Phase 12/13 content can rely on the bases, the initial base composition and transforms must
be resolved through the Section 16A base-layout contract. If repository architecture makes this
cleaner as an internal Phase 11 substep, do so rather than creating a parallel phase/system.

Phase 15 cannot count routes as redundant unless they satisfy the Section 17A independence rule.
It cannot count arbitrary nearby props as combat cover. Vehicle validation must use swept traversal
envelopes, not road-mesh width.

Phase 16 cannot open the live match until the Section 16A bootstrap barrier is satisfied and valid
server-owned infantry spawn pools/fallback zones exist for both factions.

Phase 17/18 may not invalidate canonical clearance corridors or infrastructure dependencies through
construction grading or terrain mutation.

Phase 19 checksums/snapshots must use stable IDs and canonical serialization/order.

Phase 24/25 AI decision code must consume strategic unit metadata, age stale intelligence, and use
stable decision tie-breaking/commitment rules rather than unit-name special cases or oscillating scores.
```


### Phase-local failure rule

If a numbered step fails:

```text
diagnose the failed step
fix or document the blocker
rerun only the directly relevant check
continue from that step
```

Do not restart the entire phase or rerun broad regression.

---

## Phase 0 - Verify the v1.1.0 prerequisite and establish the v2 baseline

### Purpose

Prove that v2 is starting from the intended canonical v1 architecture without spending a session rerunning the entire v1 regression suite.

### Prerequisites

None. This is the entry gate to v2.

### Read first

```text
v1.1.0 final definition of done
v1 persistent state files
v2 Sections 0, 0A, 0B, 1, 3
```

### Locate in source

Confirm the current repository contains the canonical owners for:

```text
WorldTerrainSystem
WorldSpatialIndex
runtime road system
sectional bridge system
water system
foliage system
shroud/radar intelligence
Commander camera/input
Commander sidebar/production
building semantic descriptors
finite team power grid
Commander construction
Commander AI production/orders
save/load/network integration
dedicated-server target
```

Record actual source locations.

### Implement in order

1. Confirm the v1 roadmap's final phase is marked complete.
2. Read `completed_features.md`, `WIP_features.md`, `unstarted_features.md`, and `awaiting_answers.md`.
3. Inspect Git status and current branch/HEAD.
4. Search for each canonical v1 owner listed above.
5. Create `docs/procedural_world/V2Baseline.md`.
6. Record:
   - repository commit SHA;
   - compiler/build configuration;
   - v1 owner source paths;
   - any known baseline issue relevant to v2;
   - generator feature flags/config entry point if one already exists.
7. Build only the normal client/server targets needed to prove the baseline is not already broken.
8. Do not modify v1 architecture during this phase.

### Required outputs

```text
docs/procedural_world/V2Baseline.md
Phase 0 completed state entry
actual canonical-owner path list
```

### Targeted validation

```text
baseline affected client build -> PASS
baseline dedicated server build -> PASS if v2 server work will begin immediately
```

### Do not proceed if

```text
v1 final state is not complete
required canonical owners are missing without explanation
current branch is already broken in an area v2 must extend
persistent state files contradict repository reality
```

### Gate

Claude can identify the exact v1 canonical source owner for every system v2 depends on and the affected baseline targets build.

---

## Phase 1 - World profile schema, deterministic seed service, generator version, and generation manifest

### Purpose

Create the immutable identity/configuration contract every later generator consumes.

### Prerequisites

```text
Phase 0
```

### Read first

```text
Section 4
Section 6
Section 29
```

### Locate in source

Find the canonical v1 JSON/config loader and serialization/hash utilities.

Reuse those instead of introducing a second configuration stack.

### Implement in order

1. Define `ProceduralWorldProfile` or equivalent canonical structure.
2. Implement schema parsing for:
   - world dimensions;
   - chunk size;
   - sea level;
   - boundary settings;
   - terrain settings;
   - hydrology flags/settings;
   - road/tunnel/settlement/Tiberium/biome feature settings;
   - retry/fallback limits;
   - gameplay validation thresholds.
3. Add strict validation:
   - finite positive world dimensions;
   - valid chunk size;
   - cliff band smaller than usable world;
   - valid min/max cliff heights;
   - air ceiling above terrain limits;
   - nonnegative densities;
   - bounded retry counts.
4. Define `GeneratorVersion`.
5. Implement deterministic world-seed normalization.
6. Implement stable subsystem seed derivation using named seed domains:
   - boundary;
   - terrain;
   - hydrology;
   - roads;
   - bridges;
   - tunnels;
   - settlements;
   - Tiberium;
   - bases;
   - biomes;
   - decoration;
   - combat-space repair.
7. Ensure subsystem seeds do not depend on generator execution order.
8. Implement canonical profile hashing.
9. Implement content-manifest hashing for generation-relevant assets/data.
10. Define `WorldGenerationManifest`.
11. Add manifest serialization/deserialization.
12. Add a reproduction-string formatter:
   ```text
   generator_version=<n> seed=<n> profile=<hash/name> content_manifest=<hash>
   ```
13. Add one deterministic empty-world manifest creation path.
14. Add concise diagnostics for invalid profiles.

### Required APIs/data

Equivalent to:

```text
LoadProceduralWorldProfile
ValidateProceduralWorldProfile
DeriveSubsystemSeed
ComputeProfileHash
ComputeGenerationContentHash
CreateGenerationManifest
SerializeGenerationManifest
DeserializeGenerationManifest
FormatWorldReproductionString
```

### Required outputs

```text
versioned world-profile schema
generation manifest structure
deterministic seed service
docs/procedural_world/WorldProfileSchema.md
docs/procedural_world/GeneratorDeterminism.md initial content
```

### Targeted validation

Verify:

```text
same profile + seed -> identical derived seed set
changing RoadSeed consumer does not change TerrainSeed
save/reload manifest preserves exact values
invalid profile is rejected with actionable reason
```

### Do not proceed if

```text
generator identity is not versioned
profile hash is nondeterministic
subsystems still consume one shared mutable global RNG
```

### Gate

An empty procedural-world description can be created, saved, loaded, hashed, and reproduced exactly from its manifest.

---

## Phase 2 - WorldBoundsService and immutable boundary topology

### Purpose

Create the authoritative finite map before any generator is allowed to place terrain features, roads, rivers, tunnels, buildings, or actors.

### Prerequisites

```text
Phase 1
```

### Read first

```text
Sections 1.3 through 1.6
Section 5.1
Section 5.2
Section 5.6 through 5.10
```

### Locate in source

Find:

```text
world/map bounds ownership
player movement validation
vehicle movement validation
aircraft/VTOL control
Commander camera bound logic
spawn/teleport validation
building placement validation
WorldSpatialIndex
```

### Implement in order

1. Define one `WorldBoundsService` or canonical equivalent.
2. Generate/load one closed clockwise boundary polygon or spline.
3. Validate:
   - closed;
   - non-self-intersecting;
   - minimum vertex spacing;
   - minimum usable interior area.
4. Derive:
   - playable interior polygon;
   - boundary approach band;
   - protected cliff band;
   - hard-boundary line;
   - outside-world classification.
5. Implement point classification:
   ```text
   PLAYABLE_INTERIOR
   BOUNDARY_APPROACH
   CLIFF_BAND
   HARD_BOUNDARY
   OUTSIDE_WORLD
   ```
6. Implement 2D horizontal containment queries.
7. Implement 3D gameplay containment queries.
8. Implement closest-valid-point query.
9. Implement line/segment crossing query for roads/tunnels/bridges.
10. Implement placement rejection against protected bands.
11. Integrate Commander-camera clamp/constraint.
12. Define the air horizontal flight volume from the same boundary.
13. Define configured air ceiling.
14. Integrate actor boundary checks at the canonical movement/authority layer.
15. Track each actor's last safe valid transform for fallback recovery.
16. Implement server-side out-of-bounds recovery for numerical/exploit escape.
17. Log violations with actor ID, prior transform, attempted transform, and reproduction info.
18. Add debug drawing for all boundary zones.

### Required APIs

Equivalent to:

```text
ContainsGameplayPoint
ContainsHorizontalPoint
ClassifyPoint
ClosestValidPoint
SegmentCrossesHardBoundary
CanPlaceInRegion
ClampCommanderCamera
IsInsideAirVolume
RecoverOutOfBoundsActor
```

### Required outputs

```text
authoritative finite world topology
shared actor/camera/placement boundary queries
debug boundary overlay
docs/procedural_world/WorldBoundsAndBoundaryCliffs.md initial boundary section
```

### Targeted validation

In an otherwise empty test world:

```text
infantry cannot cross
ground vehicle cannot cross
aircraft cannot cross horizontally or above ceiling
Commander camera cannot pan outside
building placement rejects cliff/hard-boundary regions
server recovers intentionally teleported out-of-bounds actor
```

### Do not proceed if

```text
different subsystems maintain different boundary polygons
aircraft uses unrelated bounds
Commander camera can leave the world
server trusts client boundary enforcement
```

### Gate

Every gameplay actor and strategic camera is constrained by the same authoritative finite world boundary.

---

## Phase 3 - Continuous procedural boundary cliff ring

### Purpose

Give the hard boundary a natural, visible, collision-backed physical explanation.

### Prerequisites

```text
Phase 2
v1 terrain/material/shader system
WWPhys/OpenW3D collision
```

### Read first

```text
Sections 5.3 through 5.5
Section 5.9
Section 28
```

### Locate in source

Find reusable:

```text
dynamic/runtime mesh builder
terrain material/shader path
triplanar or generated material path
static collision mesh creation
WorldSpatialIndex registration
terrain/chunk ownership
```

### Implement in order

1. Create the canonical cliff-generation component under the procedural-world/terrain owner.
2. Sample the closed boundary spline into stable segment stations.
3. Generate deterministic rim-height variation within configured min/max.
4. Generate boundary-approach terrain targets that rise/blend toward the cliff.
5. Generate the cliff face mesh:
   - upper rim;
   - face;
   - toe;
   - optional non-traversable ledge variation.
6. Ensure adjacent segments share exact seam vertices.
7. Generate simplified collision from the same canonical cliff geometry.
8. Apply rock/cliff material using the v1 material/shader system.
9. Add toe/scree blending where configured.
10. Register cliff render/collision bounds in spatial systems.
11. Mark the cliff band immutable for later terrain mutation.
12. Mark cliff geometry non-buildable.
13. Mark navigation as blocked through the entire band.
14. Explicitly reject procedural roads/tunnels/bridges that intersect the outward side.
15. Add seam diagnostics:
    - render gap;
    - collision gap;
    - inverted triangle;
    - traversable ramp.
16. Add boundary-cliff debug rendering.
17. Add deterministic cliff checksum to generation diagnostics.

### Required outputs

```text
continuous rendered cliff ring
continuous simplified collision ring
immutable cliff-band metadata
cliff material integration
```

### Targeted validation

Use one small fixed seed and inspect:

```text
full ring closes
first/last segment seam matches
no collision gaps
no walkable ramps
vehicle cannot crest
aircraft still stopped by air bounds
Commander camera cannot view from outside
```

### Do not proceed if

```text
the ring depends on one monolithic unbounded mesh
segment seams do not share canonical vertices
cliff geometry is the only boundary enforcement
```

### Gate

The entire test map is visibly enclosed by continuous tall cliffs with no traversal/collision leak.

---

## Phase 4 - Macro terrain generation and chunk ownership

### Purpose

Generate the bounded playable landform inside the immutable perimeter.

### Prerequisites

```text
Phase 3
v1 WorldTerrainSystem
v1 far/background terrain
```

### Read first

```text
Sections 7
Section 30
```

### Locate in source

Find canonical:

```text
WorldTerrainSystem
terrain patch/chunk representation
height sampling
terrain render rebuild
terrain collision rebuild
far-terrain invalidation
terrain material inputs
```

### Implement in order

1. Define procedural terrain chunk indexing from world dimensions/chunk size.
2. Keep boundary/cliff-band chunks explicitly tagged.
3. Create `ProceduralHeightSource` or equivalent that feeds the existing terrain system.
4. Generate low-frequency base elevation.
5. Add deterministic:
   - plains;
   - rolling hills;
   - hills;
   - mountain masses;
   - ridges;
   - valleys;
   - basins;
   - plateaus/mesas where profile permits.
6. Bias terrain toward boundary approach targets without modifying the immutable cliff face.
7. Reserve reasonable interior flat/buildable candidate zones without selecting final bases yet.
8. Enforce global height range.
9. Enforce maximum ordinary terrain slopes where the region is not explicitly classified as cliff.
10. Build near/detail terrain patches.
11. Build/update far/background terrain.
12. Build terrain collision.
13. Register chunks with spatial ownership.
14. Compute per-chunk:
    - height min/max;
    - slope statistics;
    - candidate buildability mask;
    - dirty/revision metadata.
15. Verify chunk seams use shared height samples.
16. Add deterministic per-chunk checksums.
17. Add chunk/height/slope debug overlays.

### Required outputs

```text
bounded deterministic macro terrain
chunk registry
generated height source
terrain render/collision
far terrain representation
buildability/slope metadata
```

### Targeted validation

```text
same seed -> identical chunk checksums
no terrain seam
collision matches sampled height
boundary band remains intact
far terrain matches near terrain at transition
```

### Do not proceed if

```text
generated terrain bypasses WorldTerrainSystem
procedural terrain has a separate collision truth
chunk seams are repaired visually but not physically
```

### Gate

One seed produces a complete bounded terrain surface with working render, collision, sampling, and chunk metadata.

---

## Phase 5 - Interior cliffs, ravines, canyon walls, and cliff reuse

### Purpose

Add true steep/vertical interior landforms that a heightfield alone cannot represent cleanly.

### Prerequisites

```text
Phase 4
boundary cliff mesh/collision builder from Phase 3
```

### Read first

```text
Section 8
```

### Implement in order

1. Classify terrain regions whose requested slope/relief exceeds ordinary heightfield limits.
2. Convert each accepted region into an interior `CliffFeature`.
3. Determine cliff type:
   ```text
   ESCARPMENT
   CANYON_WALL
   MOUNTAIN_FACE
   RAVINE_WALL
   COASTAL_CLIFF
   QUARRY_STYLE
   ```
4. Reuse/generalize the Phase 3 cliff mesh builder instead of creating a second cliff renderer.
5. Cut/shape the surrounding heightfield so it joins the procedural cliff toe/rim without cracks.
6. Build simplified cliff collision.
7. Register cliff bounds in spatial systems.
8. Mark non-traversable cliff edge segments.
9. Identify nearby potential:
   - infantry ramp;
   - vehicle ramp;
   - tunnel bypass;
   - bridge crossing;
   - deliberate no-cross barrier.
10. Do not create actual bridges/tunnels yet. Store candidates for later phases.
11. Apply cliff materials.
12. Scatter bounded decorative rocks/scree without creating climb exploits.
13. Add cliff connectivity/debug overlay.
14. Compute/update chunk checksums and invalidation metadata.

### Required outputs

```text
interior CliffFeature representation
shared cliff mesh/collision path
traversability classification
bridge/tunnel/ramp candidates
```

### Targeted validation

```text
no terrain/cliff cracks
collision matches faces
AI/nav blocker metadata exists
representative canyon/ravine remains connected by intended routes/candidates
```

### Do not proceed if

```text
interior cliffs are implemented as extreme heightfield spikes
a second unrelated cliff renderer exists
critical regions become disconnected with no stored bypass candidate
```

### Gate

Interior cliffs/ravines are render-, collision-, and connectivity-coherent and expose candidate bypass data for later infrastructure phases.

---

## Phase 6 - Hydrology: drainage, rivers, streams, lakes, ponds, and internal coastlines

### Purpose

Create water from terrain topology, then reserve crossings before roads are finalized.

### Prerequisites

```text
Phase 5
v1 water system
```

### Read first

```text
Section 9
```

### Implement in order

1. Compute drainage graph from macro terrain:
   - flow direction;
   - accumulation;
   - basin/depression identification;
   - watershed/outlet information.
2. Exclude protected boundary cliff/hard-boundary zones from traversable water outlets.
3. Generate river centerlines from accumulation thresholds.
4. Generate tributaries/streams where profile permits.
5. Carve river channels/banks into terrain.
6. Rebuild only affected terrain chunks.
7. Generate lakes/ponds in valid basins.
8. Generate optional internal coastal/ocean region while preserving the outer cliff boundary.
9. Classify shore segments:
   ```text
   WALKABLE_BANK
   STEEP_BANK
   CLIFF_SHORE
   VEHICLE_CROSSABLE
   NON_CROSSABLE
   ```
10. Instantiate v1 water regions/surfaces.
11. Record potential crossing candidates:
    - ford;
    - bridge;
    - tunnel;
    - route-around.
12. Where a watercourse approaches the boundary, terminate it using an inaccessible, non-traversable visual solution defined by Section 5.9.
13. Update terrain materials near wet/shore regions.
14. Update collision/spatial metadata.
15. Add flow/basin/shore debug overlays.

### Required outputs

```text
hydrology graph
river/stream definitions
lake/pond/coastal water regions
shore classification
crossing candidate set
```

### Targeted validation

```text
river centerline never climbs materially uphill except controlled smoothing
lake surface remains within basin
water does not open a traversable boundary gap
water regions match terrain collision/shore
```

### Do not proceed if

```text
roads are generated before crossing candidates exist
water modifies boundary cliff band
rivers terminate as unexplained traversable holes
```

### Gate

Generated water is topologically coherent, rendered through the canonical water system, and all strategic water barriers expose crossing candidates.

---

## Phase 7 - Candidate bases, strategic anchors, POIs, and resource-site planning

### Purpose

Create candidate strategic destinations before infrastructure routing.

### Prerequisites

```text
Phase 6
```

### Read first

```text
Sections 15.1
16.1
16.2
17
```

### Implement in order

1. Build a strategic suitability grid/field using:
   - slope/buildability;
   - boundary distance;
   - water/cliff exclusion;
   - reachable area estimate;
   - expansion room;
   - roadability;
   - resource potential.
2. Generate multiple GDI base candidates.
3. Generate multiple Nod base candidates.
4. Reject candidates:
   - in cliff band;
   - in water;
   - without enough buildable area;
   - without production-exit space;
   - with only one unavoidable approach unless profile explicitly permits.
5. Generate candidate Tiberium field regions.
6. Generate candidate settlement nodes.
7. Generate candidate strategic POIs.
8. Generate candidate forward expansion/outpost areas.
9. Build a high-level abstract node set:
   ```text
   BASE_CANDIDATE
   RESOURCE_CANDIDATE
   SETTLEMENT_CANDIDATE
   POI_CANDIDATE
   CROSSING_CANDIDATE
   TUNNEL_CANDIDATE
   ```
10. Compute approximate straight/topographic travel costs among nodes.
11. Eliminate candidate combinations that are obviously economically disconnected.
12. Do not choose final bases yet.

### Required outputs

```text
strategic candidate graph
base candidate list
resource candidate list
settlement/POI candidates
expansion candidates
```

### Targeted validation

For a fixed set of seeds verify each faction has:

```text
at least 2 viable base candidates where world size permits
at least 1 viable initial Tiberium candidate
at least 2 plausible approach directions across candidate combinations where feasible
```

### Gate

The road/tunnel planner has a valid strategic node graph to connect and no team is already doomed before infrastructure generation begins.

---

## Phase 8 - Road and highway graph plus runtime road geometry

### Purpose

Connect strategic destinations with plausible vehicle infrastructure after terrain/hydrology are known.

### Prerequisites

```text
Phase 7
v1 runtime road renderer/system
```

### Read first

```text
Section 10
```

### Implement in order

1. Define road classes:
   ```text
   HIGHWAY
   PRIMARY_ROAD
   SECONDARY_ROAD
   DIRT_TRACK
   CITY_STREET
   SERVICE_ROAD
   ```
2. Build a traversal cost field using:
   - slope;
   - cliff;
   - water;
   - crossing cost;
   - forest placeholder cost;
   - boundary distance;
   - Tiberium exclusion;
   - base approach needs;
   - tunnel/bridge candidate cost.
3. Select required graph edges among strategic nodes.
4. Route coarse road centerlines.
5. Reject any route crossing the hard boundary/cliff band.
6. Detect each water/ravine crossing and convert it to a pending bridge/tunnel/ford dependency rather than forcing road geometry through it.
7. Smooth/grade road centerlines.
8. Apply bounded terrain grading for road beds outside protected zones.
9. Generate runtime road geometry through the v1 road system.
10. Generate shoulders/material transitions.
11. Generate intersections:
    - T junction;
    - crossroads;
    - base entrances;
    - future settlement street junctions.
12. Mark optional parallel infantry-route/cover corridors for later environment placement.
13. Register road graph with spatial/world metadata.
14. Add road graph/cost/debug overlays.
15. Store unresolved crossing IDs for Phase 9/10.

### Required outputs

```text
road graph
road centerlines
road geometry
road terrain grading
intersection metadata
unresolved crossing list
```

### Targeted validation

```text
all required strategic nodes have graph path
no road leaves world
road slope/curvature within profile limits
roads do not run through lakes/cliffs without pending crossing object
```

### Do not proceed if

```text
required nodes remain disconnected without explicit tunnel/bridge pending edge
road renderer has been duplicated
```

### Gate

All major strategic anchors are connected by a plausible bounded road graph with every non-terrain crossing explicitly represented for resolution.

---

## Phase 9 - Sectional bridge planning, generation, damage integration, and rerouting hooks

### Purpose

Resolve road crossings over rivers/ravines using the canonical v1 bridge system.

### Prerequisites

```text
Phase 8
v1 sectional bridge system
```

### Read first

```text
Section 11
```

### Implement in order

1. Consume pending bridge crossing candidates.
2. For each candidate compute:
   - start bank/edge;
   - end bank/edge;
   - span length;
   - deck height;
   - approach grade;
   - support feasibility;
   - clearance.
3. Reject impossible bridges and return the edge to the alternate-route/tunnel planner.
4. Select bridge section template set.
5. Compose:
   ```text
   START_CAP
   SPAN x N
   END_CAP
   SUPPORT x N
   OPTIONAL_DECORATION
   ```
6. Adjust road approaches to exact bridge endpoints.
7. Build/render/collision through the canonical bridge implementation.
8. Register AI/nav traversal metadata.
9. Generate configured initial state:
   ```text
   PRISTINE
   DAMAGED
   CRITICAL
   BROKEN
   ```
10. Ensure broken initial bridges do not count as required connectivity unless an alternate route exists.
11. Link runtime bridge damage state to nav invalidation/replanning.
12. Store bridge strategic importance for AI Commander use.
13. Add bridge debug overlay.

### Required outputs

```text
resolved bridge objects
road-to-bridge handoff
bridge traversal metadata
bridge state/nav hooks
alternate-route metadata
```

### Targeted validation

```text
vehicle traverses intact bridge
broken bridge blocks relevant nav
road/bridge seam does not snag vehicles
bridge destruction emits correct nav invalidation
```

### Gate

Every accepted bridge crossing is physically traversable in valid states, correctly blocks in broken state, and required strategic connectivity still exists.

---

## Phase 10 - Tunnels, caves, underpasses, and portal integration

### Purpose

Resolve appropriate mountain/ridge/city crossings and create infantry/vehicle alternate paths not representable by the heightfield.

### Prerequisites

```text
Phase 9
interior cliff candidates from Phase 5
```

### Read first

```text
Section 12
Section 28
```

### Implement in order

1. Gather tunnel candidates from:
   - cliff/ridge bypasses;
   - unresolved road edges;
   - infantry flank opportunities;
   - settlement underpasses;
   - strategic alternate-route needs.
2. Reject candidates that:
   - touch cliff boundary band;
   - exit world bounds;
   - intersect protected base zones;
   - conflict with major water geometry without explicit design.
3. Classify:
   ```text
   INFANTRY_ONLY
   VEHICLE
   MIXED
   UNDERPASS
   CAVE_SHORTCUT
   ```
4. Prefer existing modular W3D tunnel/cave sections.
5. Build deterministic module chain/transforms.
6. Where modules cannot fit, use the roadmap-approved simple procedural tunnel geometry path.
7. Create terrain/cliff portal cuts.
8. Blend portal geometry into cliff/terrain.
9. Build collision.
10. Add lighting transition/runtime lighting.
11. Register interior spatial bounds.
12. Add explicit nav entrance/interior/exit links.
13. Add clearance metadata.
14. Update road graph if vehicle tunnel resolves a pending road edge.
15. Add portal/tunnel debug rendering.
16. Validate no portal or tube breaches boundary.

### Required outputs

```text
tunnel route definitions
tunnel geometry/modules
portal geometry
collision
nav links
road-graph integration where applicable
```

### Targeted validation

```text
infantry traverses intended tunnel
vehicle traverses vehicle-rated tunnel
portal collision has no snag/gap
AI enters/exits
no tunnel exits world
```

### Gate

All accepted tunnel routes are physically and navigationally coherent and any required strategic edge they replace is considered resolved.

---

## Phase 11 - Final base selection and deterministic fairness repair

### Purpose

Choose actual faction bases only after roads, bridges, tunnels, water, and terrain constraints are known.

### Prerequisites

```text
Phase 10
```

### Read first

```text
Section 16
Section 17.4
Section 17.5
```

### Implement in order

1. Score all GDI/Nod base-candidate pairs using:
   - buildable area;
   - road access;
   - early Tiberium access;
   - base-to-base travel time;
   - number of attack approaches;
   - bridge/tunnel dependency;
   - high-ground exposure;
   - expansion room;
   - boundary distance;
   - air-operating room.
2. Select the highest valid pair under fairness thresholds.
3. Compute each faction's:
   - initial resource route estimate;
   - vehicle route count;
   - infantry route count;
   - expected early contested area.
4. If thresholds fail, run deterministic local repair:
   - add/adjust road edge;
   - add ramp;
   - add tunnel candidate;
   - move resource candidate;
   - move base within candidate region;
   - adjust limited local terrain.
5. Bound repair attempts.
6. If still invalid, choose the next base pair.
7. If all candidate pairs fail, return a generation failure with exact diagnostic.
8. Once selected, lock initial base footprints/approach zones against later settlement/resource placement.
9. Store final base anchors in generation manifest.

### Required outputs

```text
final GDI base site
final Nod base site
base protected regions
fairness metrics
repair log
manifest base anchors
```

### Targeted validation

For representative seeds check:

```text
both bases have legal build area
both bases have viable initial resource access
both have required infantry/vehicle connectivity
travel-time difference remains within configured threshold
```

### Gate

Final base sites satisfy all hard connectivity/economy constraints and configured fairness tolerances.

---

## Phase 12 - Tiberium fields, crystals, harvest areas, and Harvester routes

### Purpose

Turn candidate resource regions into canonical playable Renegade Tiberium economies.

### Prerequisites

```text
Phase 11
canonical Refinery/Harvester/Tiberium behavior from v1
```

### Read first

```text
Section 15
```

### Implement in order

1. Select initial per-team Tiberium fields from candidates.
2. Select contested/secondary fields.
3. Generate field footprints.
4. Reject:
   - cliff band;
   - water;
   - excessive slope;
   - road/factory exit;
   - base spawn;
   - inaccessible region.
5. Apply Tiberium terrain material/mask.
6. Scatter crystal/models deterministically.
7. Register canonical harvestable field data.
8. Generate/validate Harvester path from owning Refinery/base to field.
9. Ensure Harvester turning/clearance is valid.
10. If route fails:
    - adjust field edge/harvest anchor;
    - repair road/terrain locally;
    - choose alternate field candidate.
11. Add optional regeneration/growth only if profile enables it.
12. Bound growth to legal field regions.
13. Add resource debug overlay and route visualization.
14. Store important field anchors/checksums in manifest.

### Required outputs

```text
working canonical Tiberium fields
Harvester routes
field material/crystal placement
resource graph
```

### Targeted validation

```text
Harvester leaves Refinery
reaches field
harvests
returns
credits are produced through canonical economy
both teams have viable early field
```

### Gate

Both teams can earn canonical Tiberium income from generated fields without special-case scripted teleports or fake economy.

---

## Phase 13 - Settlements, abandoned buildings, industrial areas, and strategic POIs

### Purpose

Create authored-looking human-scale combat spaces using existing reusable W3D assets.

### Prerequisites

```text
Phase 12
final roads/bases/resources
```

### Read first

```text
Section 14
Section 28
```

### Implement in order

1. Select settlement/industrial/POI candidates that do not conflict with:
   - bases;
   - boundary;
   - Tiberium;
   - required roads;
   - bridge approaches;
   - tunnel portals.
2. Choose layout type:
   ```text
   LINEAR_ROADSIDE
   SMALL_GRID
   INDUSTRIAL_LOOP
   IRREGULAR_TOWN
   RUINS
   OUTPOST
   ```
3. Generate local street graph tied into the Phase 8 road graph.
4. Place existing W3D buildings/props with collision-aware transforms.
5. Prefer building entrances facing usable streets/alleys.
6. Preserve interior access for models with usable interiors.
7. Generate alleys and infantry gaps.
8. Keep vehicle street clearance.
9. Place optional POIs:
   - bunker;
   - convoy wreck;
   - research site;
   - relay;
   - repair depot;
   - ruins.
10. Register generated static objects with spatial/collision systems.
11. Add settlement/POI nav connectivity.
12. Add settlement debug bounds/entrance markers.
13. Record content asset IDs in generation manifest/content hash as required.

### Required outputs

```text
settlement plans
placed W3D structures/props
street graph
POIs
collision/nav registration
```

### Targeted validation

Walk/drive a representative generated settlement and confirm:

```text
no critical doorway blocked
no vehicle street impossible
interior-capable buildings remain enterable
no object overlaps road/boundary/Tiberium illegally
```

### Gate

Generated settlements provide useful infantry/vehicle combat spaces and do not break strategic connectivity.

---

## Phase 14 - Biomes, terrain materials, forests, rocks, decoration, and street lamps

### Purpose

Finish environmental dressing while preserving routes and performance.

### Prerequisites

```text
Phase 13
v1 terrain-material and foliage systems
```

### Read first

```text
Section 13
Section 10.6
Section 28
```

### Implement in order

1. Compute biome fields from:
   - height;
   - slope;
   - moisture;
   - water distance;
   - profile climate;
   - Tiberium influence;
   - urban influence.
2. Assign terrain materials through v1 terrain texture system.
3. Mark exclusion masks:
   - roads;
   - buildings;
   - factory exits;
   - base spawn zones;
   - cliff band;
   - bridge approaches;
   - tunnel portals;
   - critical nav corridors.
4. Generate forest regions.
5. Scatter existing tree models through v1 foliage batching.
6. Assign gameplay collision only to trees/objects intended to block.
7. Generate sparse woodland/tree lines/dead forests/river vegetation as biome permits.
8. Scatter rocks/scree/debris with slope/route constraints.
9. Place street lamps on eligible road/street classes using deterministic spacing.
10. Ensure lamp transforms do not block road/sidewalk/navigation.
11. Attach runtime lighting/destructibility only through existing canonical systems.
12. Re-run route-clearance checks after decoration.
13. Apply LOD/culling/budget caps.
14. Add biome/foliage/exclusion debug overlays.

### Required outputs

```text
biome map
terrain materials
forest/foliage instances
rock/debris instances
street lamps
exclusion masks
```

### Targeted validation

```text
required routes remain open
factory exits remain open
boundary does not acquire climbable prop ramps
foliage batches/culls correctly
material transitions are coherent
```

### Gate

The world is environmentally coherent without decoration invalidating gameplay routes or budgets.

---

## Phase 15 - FPS/RTS combat-space analysis and deterministic local repair

### Purpose

Make "fun to fight in" a generated-world hard requirement rather than an aesthetic hope.

### Prerequisites

```text
Phase 14
```

### Read first

```text
Section 17
Section 1.2
```

### Implement in order

1. Define profile thresholds for:
   - infantry exposed-run length;
   - cover interval;
   - sightline distributions;
   - flank count;
   - vehicle lane width;
   - objective approach count;
   - base/resource travel time;
   - route redundancy.
2. Sample representative infantry routes:
   - base -> field;
   - base -> contested POI;
   - base -> enemy approach;
   - settlement routes;
   - bridge/tunnel approaches.
3. Measure cover and exposure.
4. Sample vehicle routes and turning/clearance.
5. Evaluate important objectives for mixed infantry/vehicle access.
6. Detect hard failures:
   - disconnected base;
   - no resource path;
   - one impossible choke with no alternate;
   - multi-minute mandatory exposed infantry march;
   - unusable vehicle lane.
7. Create deterministic local repair operations:
   - terrain fold/berm;
   - rocks/cover placement;
   - infantry side path;
   - road spur;
   - ramp;
   - tunnel candidate;
   - second crossing;
   - move POI/resource locally.
8. Apply smallest repair.
9. Rebuild only affected chunks/nav/roads.
10. Recompute metrics.
11. Bound repair attempts.
12. If unrepaired, reject the world with exact diagnostic rather than silently accepting it.
13. Generate human-readable validation report.

### Required outputs

```text
combat-space metrics
hard/soft failure list
deterministic repair log
final validation report
```

### Targeted validation

Run a small fixed seed corpus and confirm:

```text
known-bad synthetic layouts are rejected/repaired
same seed produces same repairs
repair does not modify boundary
```

### Gate

No hard combat-space constraint remains violated, and all repairs are deterministic and reproducible.

---

## Phase 16 - Final generated-world integration: collision, navigation, visibility, lighting, shroud/radar, minimap

### Purpose

Convert all generated geometry/data into one coherent playable world representation.

### Prerequisites

```text
Phase 15
v1 spatial, shroud/radar, renderer, AI/path systems
```

### Read first

```text
Section 18
Section 29
```

### Implement in order

1. Define final generated object/chunk registration lifecycle.
2. Ensure every generated terrain/cliff/road/bridge/tunnel/building/prop has the appropriate:
   - render owner;
   - collision owner;
   - spatial bounds;
   - persistence identity where needed.
3. Build infantry navigation.
4. Build ground-vehicle navigation.
5. Build air navigation/flight volume.
6. Build naval navigation only where canonical support exists and profile enables it.
7. Add portal/off-mesh links for:
   - bridges;
   - tunnels;
   - building/interior transitions where needed.
8. Assign nav revision numbers.
9. Ensure AI cannot enter a chunk whose pending collision/nav rebuild is incomplete.
10. Integrate generated objects into spatial/frustum/cell culling.
11. Use runtime-compatible lighting for generated geometry.
12. Build minimap/tactical-map base representation from:
    - terrain;
    - boundary cliffs;
    - water;
    - roads;
    - bridges;
    - tunnel portals;
    - settlements;
    - bases;
    - Tiberium.
13. Integrate v1 shroud/radar over generated world coordinates.
14. Confirm Commander intelligence firewall remains intact.
15. Add world-generation completion barrier so live match does not start before authoritative gameplay data is ready.
16. Finalize generation manifest checksums/revisions.

### Required outputs

```text
complete generated collision
complete nav datasets
generated-world culling registration
runtime lighting
minimap/tactical base layer
generation-complete synchronization point
```

### Targeted validation

```text
human traverses all intended route types
AI traverses all intended route types
minimap aligns with world
shroud/radar positions align
no actor enters incomplete-nav region
```

### Gate

The generated world is a complete playable Renegade map for both human and AI traversal.

---

## Phase 17 - Commander expansion on generated terrain and bounded foundation grading

### Purpose

Let the v1 Commander build new bases/forward structures on generated terrain without corrupting world topology.

### Prerequisites

```text
Phase 16
v1 building placement/construction/power
```

### Read first

```text
Section 16.3
Section 16.4
Section 19
```

### Implement in order

1. Feed generated terrain/world bounds into the existing v1 building placement validator.
2. Add generated-world placement exclusions:
   - boundary/cliff band;
   - water;
   - immutable major cliff;
   - tunnel portal/interior;
   - bridge structure/approach;
   - protected resource route;
   - invalid nav chokepoint.
3. Define bounded foundation grading request from a valid ghost placement.
4. Compute cut/fill volume inside footprint plus transition apron.
5. Reject if grading exceeds profile:
   - max depth;
   - max height;
   - max slope change;
   - max affected area.
6. Reject if grading intersects protected feature.
7. Execute grading through canonical WorldTerrainSystem.
8. Rebuild affected terrain render/collision.
9. Revalidate placement server-side after grading preview/result.
10. Reserve construction footprint.
11. Run existing v1 procedural construction.
12. On completion update:
    - nav;
    - power;
    - strategic capabilities;
    - spatial registration.
13. Add forward-base/expansion region metadata for Commander and AI use.

### Required outputs

```text
generated-world placement integration
foundation grading operation
protected-region checks
post-construction nav update
```

### Targeted validation

Construct representative:

```text
Power Plant
ground factory
defense
communications building
```

on generated terrain and verify no boundary/nav corruption.

### Gate

Commander can expand a base on generated terrain using only canonical v1 building systems plus safe bounded terrain grading.

---

## Phase 18 - Runtime terrain mutation and ordered WorldOperation journal

### Purpose

Make runtime world changes deterministic, authoritative, dependency-aware, and persistable.

### Prerequisites

```text
Phase 17
```

### Read first

```text
Section 19
Section 20.1
Section 20.2
```

### Implement in order

1. Define versioned `WorldOperation` envelope.
2. Implement operation IDs and monotonic world revision.
3. Implement operation types:
   ```text
   RAISE
   LOWER
   FLATTEN
   SMOOTH
   GRADE_FOUNDATION
   CARVE_ROAD
   CARVE_CHANNEL
   CREATE_CRATER
   RECLAIM_SHALLOW_WATER if enabled
   RESTORE_REGION
   ```
4. For each operation define:
   - parameters;
   - validation;
   - affected chunks;
   - protected-region rules;
   - render invalidation;
   - collision invalidation;
   - nav invalidation;
   - water/road dependencies;
   - checksum output.
5. Implement transaction reservation of affected chunks.
6. Reject stale operation requests against wrong world revision where ordering matters.
7. Apply terrain mutation.
8. Rebuild affected render data.
9. Rebuild affected collision.
10. Queue/rebuild navigation.
11. Update dependent roads/water/materials when applicable.
12. Commit one new world revision only after authoritative dependencies reach required commit point.
13. Append operation record to journal.
14. Replicate compact operation.
15. Add bounded crater rules:
    - max radius;
    - max depth;
    - rate limit;
    - no cliff-band mutation.
16. Add operation debug/replay command.

### Required outputs

```text
versioned WorldOperation format
world revision owner
operation journal
dependency invalidation graph
runtime mutation APIs
```

### Targeted validation

For each core operation:

```text
server applies
client reconstructs
collision matches
nav revision advances when required
boundary remains unchanged
save/replay produces same checksum
```

### Gate

Controlled runtime mutations are deterministic, ordered, compactly replicated, and leave all affected gameplay systems coherent.

---

## Phase 19 - Snapshots, checksums, late join, regional repair, and developer rollback

### Purpose

Make large/mutated procedural worlds practical to join, save, recover, and debug without sending the full world mesh.

### Prerequisites

```text
Phase 18
```

### Read first

```text
Section 20
Section 29
```

### Implement in order

1. Define per-chunk authoritative checksum inputs.
2. Define world-level checksum composition.
3. Define compact chunk snapshot format containing only authoritative mutable/generated state needed beyond baseline regeneration.
4. Define snapshot version.
5. Implement snapshot creation for dirty chunks.
6. Implement operation-log compaction threshold by count/time/size.
7. Implement safe rebase:
   ```text
   baseline + snapshot + post-snapshot operations
   ```
8. Implement late-join handshake:
   - generator version;
   - profile/hash;
   - content hash;
   - manifest;
   - snapshot inventory;
   - current world revision.
9. Client deterministically regenerates baseline.
10. Client applies snapshots.
11. Client applies later operations in order.
12. Client computes checksums.
13. Implement regional mismatch request/repair.
14. Server returns only affected chunk snapshot.
15. Client repairs and reverifies.
16. Load dynamic building/AI state only after world baseline/revisions required by those objects exist.
17. Implement developer/admin rollback to a previous authoritative snapshot.
18. Do not expose casual gameplay rollback unless separately designed.
19. Add metrics:
    - late-join bytes;
    - generation time;
    - snapshot bytes;
    - repair bytes.

### Required outputs

```text
chunk snapshot format
world/chunk checksums
late-join synchronizer
regional repair protocol
snapshot compactor
developer rollback
```

### Targeted validation

```text
join pristine generated world
join heavily mutated world
intentionally corrupt one client chunk -> regional repair
save/reload
rollback developer snapshot
```

### Gate

A late client reconstructs the same current world without downloading complete raw world meshes, and checksum mismatch can be repaired regionally.

---

## Phase 20 - Advanced building functional subcomponents

### Purpose

Add optional damageable functionality to meaningful W3D building parts without replacing canonical whole-building health/power behavior.

### Prerequisites

```text
Phase 19
v1 building semantic JSON
```

### Read first

```text
Section 21
```

### Implement in order

1. Extend building descriptor schema with optional functional subcomponent semantic roles.
2. Resolve each role to real W3D part/node/aggregate.
3. Add capability state owned by the canonical building object.
4. Implement only the first supported capability effects:
   - communications array/radar dish -> communications reduction/disable;
   - power generator module -> generation reduction;
   - refinery processor -> throughput modifier;
   - factory production module -> production-time modifier;
   - repair module -> repair-rate modifier;
   - defense targeting/weapon module -> corresponding canonical defense modifier.
5. Do not implement an effect when the model lacks a coherent semantic part.
6. Integrate damage events from existing building damage system.
7. Integrate repair with existing Renegade building repair.
8. Recompute v1 power/communications/factory capability as necessary.
9. Save/network subcomponent state.
10. Add debug overlay/status report for functional parts.

### Required outputs

```text
descriptor subcomponent schema
canonical functional-state owner
selected capability effects
repair/save/network integration
```

### Targeted validation

One representative building per supported capability demonstrates damage -> degraded function -> repair -> restored function.

### Gate

Functional damage exists without a second building health/state system and remains optional per descriptor.

---

## Phase 21 - Physical logistics layer

### Purpose

Create attackable strategic logistics while leaving ordinary FPS purchasing straightforward.

### Prerequisites

```text
Phase 20
```

### Read first

```text
Section 22
```

### Implement in order

1. Define logistics profile toggles. Default gameplay must remain viable without mandatory micromanagement.
2. Define canonical logistics nodes:
   ```text
   BASE
   REFINERY
   FACTORY
   REPAIR
   FORWARD_BASE
   RESOURCE_FIELD
   LOGISTICS_POI
   ```
3. Build logistics graph from roads/routes.
4. Add forward repair/rearm coverage where existing gameplay supports it.
5. Add optional construction-support radius if enabled.
6. Implement optional logistics convoy order type using normal AI vehicles.
7. Define convoy cargo/capability abstraction. Do not invent physical inventory systems unless required.
8. Make convoys/critical routes visible to Commander tactical view subject to team intelligence.
9. Integrate route blockage detection:
   - broken bridge;
   - destroyed road-critical structure;
   - enemy obstruction where relevant.
10. Feed logistics status to AI Commander future world model.
11. Ensure human player purchases remain canonical and are not blocked by bookkeeping absent an explicit profile rule.

### Required outputs

```text
logistics graph
optional convoy behavior
forward support coverage
Commander logistics visualization
```

### Targeted validation

Break a bridge/route and verify logistics route recomputes or reports unavailable without breaking unrelated normal purchasing.

### Gate

Logistics creates physical strategic objectives and route dependencies without turning basic Renegade purchases into supply-accounting chores.

---

## Phase 22 - Optional advanced vehicle subsystem damage and support

### Purpose

Add profile-selectable vehicle depth without changing default Renegade vehicle behavior.

### Prerequisites

```text
Phase 21
```

### Read first

```text
Section 23
```

### Implement in order

1. Add explicit world/game-profile toggle. Default is OFF unless user later changes policy.
2. Audit canonical vehicle damage/repair ownership.
3. Add optional subsystem state:
   - engine;
   - mobility/track/wheel;
   - weapon/turret where canonical vehicle supports it;
   - sensor for AI targeting where useful.
4. Map subsystem damage to bounded modifiers.
5. Never remove basic drivability from a human player without an explicit configured rule.
6. Integrate Repair Facility/Repair Bay.
7. Integrate handheld/normal repair behavior where appropriate.
8. Add AI `RETREAT_TO_REPAIR` response.
9. Add optional repair/recovery support vehicle only through normal vehicle/AI APIs.
10. Save/network subsystem state.
11. Ensure disabling profile bypasses all modifiers and preserves canonical behavior.

### Required outputs

```text
optional subsystem-damage schema/state
repair integration
AI repair behavior
profile bypass path
```

### Targeted validation

Compare same vehicle with feature OFF vs ON and verify OFF remains canonical Renegade behavior.

### Gate

Optional subsystem damage works when enabled and is behaviorally absent when disabled.

---

## Phase 23 - Multiplayer Commander governance

### Purpose

Ensure human Commander ownership is robust before autonomous/skirmish/campaign systems depend on stable role assignment.

### Prerequisites

```text
Phase 22
v1 Commander Mode
```

### Read first

```text
Section 25
```

### Implement in order

1. Audit whether v1 already implements a final Commander selection policy.
2. Preserve any complete existing implementation.
3. If missing, implement team-scoped Commander state:
   ```text
   NONE
   NOMINATING
   VOTING
   ACTIVE
   TRANSFER_PENDING
   ```
4. Implement candidate/volunteer period.
5. Implement team vote.
6. Implement commander resignation.
7. Implement voluntary transfer.
8. Implement disconnect recovery.
9. Implement team removal vote with anti-spam/cooldown.
10. Define behavior during temporary `NONE`.
11. Define Commander death policy:
    - exits strategic camera;
    - role retention after respawn according to configured mode;
    - no automatic role theft.
12. Save/network role state where required.
13. Ensure AI Commander can later occupy the same abstract team-Commander role only in autonomous modes.

### Required outputs

```text
robust team Commander ownership
vote/transfer/resignation/disconnect flow
role state replication
```

### Targeted validation

Simulate:

```text
normal election
tie/no candidate
resignation
transfer
disconnect
death/respawn
removal vote
```

### Gate

A multiplayer team cannot become permanently stuck with an absent/dead/disconnected Commander and role changes remain team-authoritative.

---

## Phase 24 - Autonomous AI Commander economy, power, and base construction

### Purpose

Make an AI faction capable of building a legitimate functioning base using the exact human Commander action surface.

### Prerequisites

```text
Phase 23
v1 Commander APIs
generated world strategic graph
logistics graph if enabled
```

### Read first

```text
Section 24.1 through 24.4
```

### Implement in order

1. Define server-owned `AutonomousCommander` using normal Commander authority abstraction.
2. Prohibit direct spawn/build cheats at API boundaries.
3. Build legitimate strategic world model from:
   - team shroud/radar intelligence;
   - own buildings/units;
   - known resource fields;
   - roads/bridges/tunnels;
   - power;
   - credits;
   - logistics where enabled.
4. Ensure hidden enemy truth is inaccessible to decision code.
5. Implement planning tick budget/interval.
6. Implement economy goal evaluator:
   - maintain reserve;
   - build/replace Power Plant;
   - prevent power overload;
   - build Silo when wallet pressure warrants;
   - maintain Refinery/resource capability;
   - avoid spending into unrecoverable economy collapse.
7. Implement base-site/expansion evaluation using the same placement APIs as human Commander.
8. Implement building prerequisite plan.
9. Implement construction sequencing:
   ```text
   power
   economy
   infantry/vehicle/air production as available
   defenses
   communications
   repair/logistics
   expansion
   ```
   Exact order remains utility-driven, not rigid, but prerequisites are respected.
10. Issue real building placement/construction actions.
11. Respond to placement rejection by choosing another legal site.
12. Respond to power loss by building generation or shedding optional loads.
13. Respond to destroyed economy building.
14. Add strategic-plan diagnostics explaining current goal and score.

### Required outputs

```text
AI Commander strategic world model
economy planner
power planner
base expansion/construction planner
legitimate Commander action integration
```

### Targeted validation

On several seeds, AI must independently:

```text
establish/maintain power
maintain income
construct at least one production building
recover from one destroyed Power Plant
recover from one rejected placement
```

### Gate

An AI Commander can create and maintain a legitimate powered economy/base without omniscience or bypassing human Commander rules.

---

## Phase 25 - Autonomous AI Commander production, defense, offense, recon, repair, and logistics

### Purpose

Turn the economically functional AI Commander into a coherent opponent.

### Prerequisites

```text
Phase 24
v1 AI production/orders
```

### Read first

```text
Section 24.5 through 24.9
```

### Implement in order

1. Extend strategic world model with:
   - known enemy composition;
   - known enemy buildings;
   - last-known concentrations;
   - friendly force composition;
   - route risks;
   - bridge/tunnel states;
   - repair needs.
2. Implement recon goal generation.
3. Implement unit-production utility scoring for:
   - infantry;
   - ground vehicles;
   - air vehicles.
4. Issue production through selected physical facilities.
5. Implement force assembly/staging.
6. Implement defense planner around:
   - base approach routes;
   - resource routes;
   - bridges/tunnels;
   - power;
   - communications;
   - production.
7. Implement attack objective scoring.
8. Implement route selection respecting current nav and known threats.
9. Implement staged attack:
   ```text
   recon
   assemble
   move
   engage
   assess
   exploit or retreat
   ```
10. Implement repair/retreat rules.
11. Integrate logistics routes/convoys when enabled.
12. Detect bridge/path invalidation and replan.
13. Implement difficulty parameters only through:
   - think interval;
   - reaction delay;
   - scoring sophistication;
   - risk tolerance;
   - reserve policy;
   - coordination.
14. Do not grant hidden vision/resources except separately configured handicap.
15. Add diagnostics for:
   - current objective;
   - candidate objective scores;
   - selected route;
   - expected force;
   - retreat reason.

### Required outputs

```text
recon planner
production planner
defense planner
attack planner
repair/retreat planner
logistics planner
difficulty tuning
```

### Targeted validation

AI must:

```text
defend a base attack
attack through a valid route
use a tunnel/bridge where appropriate
replan after bridge destruction
retreat damaged forces
continue operating without hidden enemy truth
```

### Gate

AI can play a complete coherent skirmish through canonical gameplay systems and legitimate intelligence.

---

## Phase 26 - Persistent campaign foundation

### Purpose

Add an optional strategic layer that launches ordinary deterministic procedural battles without contaminating standard multiplayer.

### Prerequisites

```text
Phase 25
stable procedural match/save infrastructure
```

### Read first

```text
Section 26
```

### Implement in order

1. Define campaign schema/version.
2. Define territory/node structure:
   - ID;
   - adjacency;
   - owner;
   - procedural profile;
   - seed;
   - strategic value;
   - optional tech/resource modifier.
3. Define faction campaign state.
4. Define battle-launch contract:
   ```text
   territory
   seed
   profile
   starting ownership/config
   allowed units/buildings
   objectives/modifiers
   ```
5. Launch battle through the normal procedural-world bootstrap.
6. At battle completion, produce one battle-result summary.
7. Apply result to campaign territory ownership/resources.
8. Persist campaign state.
9. Add explicit schema migration path.
10. Keep campaign progression disabled/irrelevant in ordinary standalone multiplayer.
11. Add campaign reproduction/logging.

### Required outputs

```text
campaign schema
territory graph
battle launch/result contract
campaign save/migration
```

### Targeted validation

Create campaign -> launch territory battle -> complete -> return result -> save -> reload -> relaunch territory deterministically.

### Gate

A campaign territory can reliably launch and recover a deterministic procedural battle using the same underlying match systems.

---

## Phase 27 - Procedural-world toolchain and reproducibility tools

### Purpose

Make the generator inspectable enough that future tuning does not require staring at a bad seed and divining what offended the terrain gods.

### Prerequisites

```text
Phases 1-26 as applicable
```

### Read first

```text
Section 27
Section 33
```

### Implement in order

1. Implement seed/profile preview command/tool.
2. Render/export top-down preview containing:
   - boundary;
   - cliffs;
   - height;
   - water;
   - roads;
   - bridges;
   - tunnels;
   - bases;
   - Tiberium;
   - settlements;
   - validation failures.
3. Implement generation-step viewer/toggles.
4. Implement machine-readable validation report.
5. Implement human-readable validation summary.
6. Implement runtime debug overlays:
   - chunks;
   - boundary zones;
   - cliff collision;
   - air volume;
   - hydrology;
   - road graph;
   - bridges;
   - tunnels;
   - infantry nav;
   - vehicle nav;
   - AI Commander goals;
   - revisions/dirty chunks.
7. Implement reproduction command using manifest string.
8. Implement deterministic seed batch runner for offline validation.
9. Batch runner must stop/report hard failures without trying endless rerolls.
10. Implement descriptor/profile validation invocation.
11. Document exact developer workflows.

### Required outputs

```text
seed preview
step viewer
validation report
runtime overlays
reproduction command
seed batch runner
docs/tools/ProceduralWorldTools.md
```

### Targeted validation

Given one reproduction string, a developer can reproduce the same world and open the relevant debug view without manually reconstructing settings.

### Gate

A bad seed or runtime-world failure can be reproduced, inspected, and diagnosed with dedicated tools rather than ad hoc logging.

---

## Phase 28 - Full multiplayer, save/load, late-join, and dedicated-server integration gate

### Purpose

Run the first intentionally broad integration gate after the complete gameplay stack exists.

### Prerequisites

```text
Phases 0-27
```

### Read first

```text
Sections 20
29
31
32
```

### Implement/fix in order

1. Freeze one gate commit.
2. Start dedicated server with procedural world.
3. Verify manifest/profile/version negotiation.
4. Join client at match start.
5. Verify generated checksums.
6. Exercise:
   - player movement;
   - vehicles;
   - Commander Mode;
   - building construction;
   - power;
   - AI production;
   - AI Commander if mode enabled.
7. Mutate terrain.
8. Destroy one bridge.
9. Build one new structure.
10. Late join another client.
11. Verify late-join world checksum/state.
12. Save.
13. Restart/load.
14. Verify:
    - terrain mutations;
    - buildings;
    - power;
    - AI state;
    - bridge state;
    - world revision.
15. Trigger one regional checksum mismatch and repair.
16. Test disconnect/reconnect.
17. Verify server does not load client renderer/UI dependencies.
18. Fix only integration defects discovered by this gate.
19. Record concise gate evidence.

### Required outputs

```text
multiplayer integration report
save/load integration report
late-join report
dedicated-server dependency confirmation
```

### Gate

A complete procedural match survives multiplayer play, mutation, late join, save/reload, and dedicated-server execution with matching authoritative state.

---

## Phase 29 - Performance, scaling, and soak hardening

### Purpose

Ensure the procedural/RTS stack remains a playable FPS under realistic scale.

### Prerequisites

```text
Phase 28
```

### Read first

```text
Section 30
v1 performance policy
```

### Implement in order

1. Define representative profiles:
   ```text
   SMALL
   MEDIUM
   LARGE_SUPPORTED
   ```
2. Capture baseline metrics for each.
3. Measure:
   - generation time;
   - client frame time;
   - server tick;
   - terrain memory;
   - collision memory;
   - nav memory;
   - foliage count;
   - AI count;
   - path requests;
   - nav rebuild time;
   - world-operation cost;
   - snapshot size;
   - late-join bytes/time;
   - Commander UI cost.
4. Identify actual bottlenecks before optimizing.
5. Apply budget controls in FPS-priority order.
6. Add/stabilize:
   - chunk LOD;
   - far terrain;
   - foliage LOD/culling;
   - AI think-rate LOD;
   - staggered path replans;
   - nav rebuild queues;
   - snapshot compaction;
   - world-operation rate limits.
7. Ensure local input/camera/combat remains prioritized.
8. Run long server/client soak.
9. During soak exercise:
   - AI Commander;
   - multiple construction sites;
   - bridge destruction;
   - terrain mutation;
   - late joins;
   - save/load checkpoint.
10. Check for:
    - memory growth;
    - stale chunk resources;
    - leaked nav objects;
    - runaway operation journal;
    - AI queue growth.
11. Fix demonstrated issues.
12. Record supported profile limits.

### Required outputs

```text
performance report
supported world/AI limits
budget settings
soak result
```

### Gate

The largest declared supported profile remains playable as an FPS and stable as a server for the defined soak scenario without unbounded memory/state growth.

---

## Phase 30 - Final cleanup, documentation, provenance, and persistent-state reconciliation

### Purpose

Finish v2 as one maintainable canonical engine rather than a pile of generator experiments that happen to compile.

### Prerequisites

```text
Phase 29
```

### Read first

```text
Sections 33
38
39
```

### Implement in order

1. Search for temporary:
   - generator prototypes;
   - duplicate systems;
   - migration adapters;
   - debug-only runtime paths;
   - old feature flags that select obsolete implementation versions.
2. Remove superseded scaffolding.
3. Confirm no second:
   - terrain owner;
   - boundary owner;
   - AI Commander action path;
   - building system;
   - power system;
   - navigation truth;
   - persistence truth.
4. Complete all documentation listed in Section 33.
5. Complete generator/profile schema documentation.
6. Complete network/save version documentation.
7. Complete asset/content provenance.
8. Record final generator version and compatibility rules.
9. Confirm legacy authored maps still load.
10. Confirm procedural mode remains explicitly selectable/configurable.
11. Confirm dedicated server has no renderer dependency.
12. Confirm final Definition of Done in Section 38 item-by-item.
13. Resolve/close every answered item in `awaiting_answers.md`.
14. Ensure no unexplained WIP remains.
15. Move complete features to `completed_features.md`.
16. Ensure future/out-of-scope work remains in `unstarted_features.md`.
17. Run only the final designated release-quality gate required by Section 38.
18. Tag/document the final v2 implementation baseline.

### Required outputs

```text
complete docs
complete provenance
final compatibility/version notes
clean persistent state files
final baseline/tag information
```

### Gate

Every Section 38 requirement is satisfied or explicitly documented as user-approved out-of-scope, no duplicate runtime architecture remains, and the repository/state files are ready for a fresh agent with no hidden implementation knowledge.

---

# 35. FIRST V2 VERTICAL SLICE

Before broad world-generation work continues, deliver this playable slice:

1. load one procedural profile;
2. choose one explicit seed;
3. create finite authoritative world bounds;
4. generate continuous tall cliffs around the entire perimeter;
5. generate one basic terrain field inside;
6. generate GDI and Nod base sites;
7. generate one Tiberium field per side;
8. generate one road connecting the bases/resources using an explicit RoadCrossSection and a lane width derived from the measured GDI Humvee collision width;
9. resolve the initial faction base layouts, canonical factory/Harvester clearance corridors, infantry spawn pools, and fallback spawn zones;
10. spawn players server-authoritatively from validated spawn candidates;
11. allow infantry/vehicle movement, including valid off-road traversal where the traversal profile permits;
12. prohibit all actor escape;
13. prohibit Commander camera escape;
14. allow a v1 Commander building to be constructed on generated terrain;
15. save;
16. load;
17. regenerate the exact same world from seed/version/profile/content manifest;
18. verify checksum and stable generated identities.

Do not proceed to cities, complex tunnels, campaign, or advanced AI until this slice works.

---

# 36. SECOND VERTICAL SLICE: COMPLETE PROCEDURAL BATTLEFIELD

Required:

```text
boundary cliff ring
hills
mountain/ridge
interior cliff
river
lake/pond
bridge
tunnel
forest
road/highway
street lamps
small abandoned settlement
Tiberium fields
two bases
multiple infantry and vehicle routes
Commander construction
AI navigation
```

A human team must be able to play a full Renegade match on it.

---

# 37. THIRD VERTICAL SLICE: AUTONOMOUS SKIRMISH

Required:

```text
human Commander
friendly AI
enemy AI Commander
enemy base construction
enemy power management
enemy production
enemy Tiberium economy
defense
attack
bridge/tunnel-aware route planning
legitimate shroud/radar intelligence
```

No omniscient AI.

---

# 38. FINAL DEFINITION OF DONE

v2.0 is complete only when:

## World generation

1. Same seed/profile/version reproduces the same authoritative world.
2. World is finite.
3. Tall procedural cliffs continuously encircle the map.
4. Boundary has no traversable seams/gaps.
5. Infantry cannot leave.
6. Ground vehicles cannot leave.
7. Air vehicles cannot leave.
8. Commander camera cannot leave.
9. Roads cannot leave.
10. Tunnels cannot leave.
11. Bridges cannot leave.
12. Runtime terrain mutation cannot breach the boundary.
13. Hills exist.
14. Mountains/ridges can exist.
15. Interior cliffs/ravines exist.
16. Rivers/streams can exist.
17. Lakes/ponds can exist.
18. Coastlines can exist inside the fixed boundary.
19. Roads/highways are generated.
20. Street lamps can populate appropriate roads.
21. Bridges are generated from crossing needs.
22. Tunnels/underpasses are generated where useful.
23. Forests/vegetation are generated.
24. Small settlements/city/industrial regions are generated.
25. Existing building models can populate settlements.
26. Tiberium fields are generated with Harvester access.
27. GDI/Nod base sites are generated fairly.
28. Generated world supports Commander construction.

## FPS quality

29. Infantry has usable cover/routes.
30. Vehicle routes are viable.
31. Important objectives have meaningful alternate approaches where feasible.
32. Procedural terrain does not degrade ordinary Renegade FPS controls.
33. Huge exposed infantry travel is bounded by profile/validation rules.
34. Human-controlled units retain priority over AI congestion.
35. Local FPS responsiveness remains priority over distant RTS simulation.

## Dynamic world

36. Runtime terrain operations are authoritative and deterministic.
37. Boundary cliff band is immutable.
38. Terrain/collision/nav dependencies rebuild correctly.
39. Bridge destruction updates navigation.
40. Construction grading works.
41. Crater/deformation budgets are bounded.
42. Save/load preserves world mutations.
43. Late join reconstructs world correctly.
44. Checksum repair can fix a bad client region.
45. Snapshots compact long operation history.

## AI Commander

46. AI Commander uses normal Commander APIs.
47. AI has no hidden-information cheat.
48. AI manages finite power.
49. AI harvests/uses Tiberium economy.
50. AI expands base.
51. AI produces appropriate units.
52. AI defends.
53. AI attacks.
54. AI replans after bridge/path changes.
55. AI uses roads/tunnels/bridges intelligently.
56. AI obeys population/performance budgets.

## Advanced systems

57. Optional functional building subcomponents integrate with canonical building state.
58. Logistics creates physical strategic objectives where enabled.
59. Optional advanced vehicle damage does not alter default gameplay when disabled.
60. Procedural toolchain can reproduce/debug a seed.
61. Legacy authored maps still work.
62. Dedicated server has no client renderer dependency.
63. Network state remains server-authoritative.
64. No Jolt/replacement physics exists.
65. No duplicate v1/v2 terrain, AI, building, power, or Commander runtime exists.
66. Documentation is complete.
67. Persistent state files are correct and reconciled.

---

# 39. CLAUDE OPERATING LOOP FOR V2

## Normal work

```text
1. read v1 prerequisite + v2 roadmap + four state files
2. inspect current WIP
3. inspect exact canonical source owner
4. implement smallest dependency-safe piece
5. update WIP
6. build affected target(s)
7. run focused relevant check(s)
8. PASS -> stop validating
9. continue implementation
```

## Procedural-generator work

For every generator feature:

```text
define deterministic inputs
define derived seed
define hard constraints
define soft preferences
define bounded retries
define deterministic fallback
define outputs
define downstream invalidations
define save/network representation
define debug visualization
```

Never add a random procedural feature without a deterministic failure/fallback story.

## Before phase completion

Verify only that phase's gate.

Do not run final soak/regression after every phase.

## Before session end

Reconcile:

```text
completed_features.md
WIP_features.md
unstarted_features.md
awaiting_answers.md
```

and record the **exact next implementation action**.

---

# 40. IMPLEMENTATION ORDER - COMPACT CLAUDE CHECKLIST

This checklist is navigation only. **Section 34 is authoritative for phase execution details.** Do not use this compact list as a substitute for the numbered phase work package.

```text
00 v1 prerequisite
01 seed/profile/manifest
02 authoritative bounds
03 boundary cliff ring
04 macro terrain/chunks
05 interior cliffs/ravines
06 hydrology
07 base/POI/resource candidates
08 roads/highways
09 bridges
10 tunnels
11 final bases/fairness
12 Tiberium
13 settlements/cities
14 biomes/forests/lamps
15 combat-space validation/repair
16 collision/nav/visibility/lighting/minimap
17 Commander expansion/grading
18 runtime mutation/world operations
19 snapshots/checksums/late join
20 building subcomponents
21 logistics
22 optional vehicle subsystems
23 Commander governance
24 AI Commander economy/base
25 AI Commander combat/production
26 campaign foundation
27 tooling
28 multiplayer/save/dedicated gate
29 performance/soak gate
30 final cleanup/docs/state
```

**Do not skip the boundary/cliff vertical slice and then try to bolt the world perimeter on after procedural roads, tunnels, and hydrology already assume they can wander wherever they please.**
