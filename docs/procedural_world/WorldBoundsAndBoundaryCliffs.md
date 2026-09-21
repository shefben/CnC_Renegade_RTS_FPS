# World Bounds and Boundary Cliffs (V2)

Required output of roadmap Section 34 Phases 2 and 3. The **boundary** half is Phase 2 and runs
to "Debug drawing"; the **cliff geometry** half is Phase 3 and starts at "The cliff ring".

Design sources: Sections 1.3-1.6 (one finite map, one absolute boundary; the skybox is not the
boundary; boundary cliffs are mandatory), 5.1 (topology), 5.2 (protected zones), 5.6 (physics
enforcement), 5.7 (air boundary), 5.8 (Commander camera), 5.9 (feature interaction), 5.10
(debugging).

## Ownership, and why it is split across two layers

| Concern | Owner |
| --- | --- |
| Boundary data + every query against it | `WorldBoundsService` (`Code/wwphys/worldboundsservice.h/.cpp`) |
| Deciding what the boundary *is* for a profile | `ProceduralBoundsBuilder` (`Code/Combat/proceduralboundsbuilder.h/.cpp`) |

The service lives in `wwphys` because Phase 2 step 14 ("integrate actor boundary checks at the
canonical movement/authority layer") lands in `MoveablePhysClass::Post_Timestep_Process`, which
is below Combat and cannot include it. Generation lives in Combat because it needs Phase 1's
`ProceduralWorldProfile` and `WorldSeedService`, which live there. The two meet at exactly one
`WorldBoundsService::Configure` call — one boundary handed across a layer edge, never two
representations. This is the same split `CommanderModeController::Set_World_Bounds` already used
before v2 existed.

## Legacy maps are unaffected

Directive 1.10. The service is **inactive until something calls `Configure`**, and nothing in the
stock game does. While `Is_Configured()` is false:

- `Contains_Horizontal_Point` / `Contains_Gameplay_Point` / `Is_Inside_Air_Volume` return true;
- `Classify_Point` returns `PLAYABLE_INTERIOR`;
- `Can_Place_In_Region` returns true;
- `Closest_Valid_Point` / `Clamp_Commander_Camera` return their argument untouched;
- `Segment_Crosses_Hard_Boundary` returns false;
- `Recover_Out_Of_Bounds_Actor` does nothing.

So every integration point below costs one `bool` test on a hand-authored map and changes no
behaviour. The `worldbounds` self check asserts this directly rather than leaving it to review.

## Zones

Section 5.2's five zones, measured by **inward distance** from the boundary polygon (positive
inside, negative outside — `WorldBoundsService::Get_Inward_Distance`, the single primitive every
classification derives from):

| Inward distance `d` | Zone | Meaning |
| --- | --- | --- |
| `d < 0` | `OUTSIDE_WORLD` | never valid gameplay space |
| `0 <= d < hard_barrier_offset` | `HARD_BOUNDARY` | authoritative non-crossable barrier |
| `... < hard_barrier_offset + cliff_band_width` | `CLIFF_BAND` | tall perimeter cliffs; no roads/buildings/tunnels/resources |
| `... + approach_band_width` | `BOUNDARY_APPROACH` | terrain rises/changes toward the perimeter |
| deeper | `PLAYABLE_INTERIOR` | ordinary terrain and features |

Two different legality rules are derived from this, and the difference is deliberate:

- **Actor legality** (`Contains_Horizontal_Point`): `d >= hard_barrier_offset`. The cliff band is
  *inside* the world — its geometry is what normally stops anyone standing there, and the hard
  boundary is the backstop for when geometry fails.
- **Construction legality** (`Can_Place_In_Region`): `d >= hard_barrier_offset + cliff_band_width
  + footprint_radius`. Section 5.9 forbids building in the cliff band, and passing the footprint
  radius is what stops a building whose centre clears the band from putting a corner inside it.

### The approach band width is derived, not configured

`cliff_band_width` and `hard_barrier_offset` come straight from the profile's `boundary` block.
The `BOUNDARY_APPROACH` band has **no profile field** — nothing in Section 5 or Section 4A asks
for one — so it is derived as one more cliff-band width inward. This is a deliberate choice not
to bump the Phase 1 schema for a value the roadmap never asked to be tunable; replacing it with a
real field later is a one-line change in `Configure` plus a schema-version bump.

## The vertical axis

`Contains_Gameplay_Point` tests the horizontal polygon **plus the configured air ceiling**, and
nothing else. There is deliberately no world floor: Section 5.7 asks for a flight ceiling, no
section defines a floor, and "fell through the terrain" is a collision failure owned by the
physics scene rather than a boundary escape. Inventing a floor depth with no profile field behind
it would be a guess.

The air volume is not a separate volume — `Is_Inside_Air_Volume` *is* `Contains_Gameplay_Point`.
"Aircraft uses unrelated bounds" is named on Phase 2's own do-not-proceed list, so the two cannot
diverge.

## Integration points

All five, and there are only five:

| Step | Where | What |
| --- | --- | --- |
| 14-17 | `MoveablePhysClass::Post_Timestep_Process` (`Code/wwphys/movephys.cpp`) | every moving actor's boundary check, last-safe tracking, and server-side recovery |
| 11 | `CommanderModeController::Set_Focus` (`Code/Combat/commandermodecontroller.cpp`) | camera clamp (Section 5.8) |
| 10 | `CommanderPlacementValidator::Validate` (`Code/Combat/commanderplacement.cpp`) | construction rejection, reported as the existing `PLACEMENT_OUT_OF_BOUNDS` |
| — | `CombatManager` init (`Code/Combat/combat.cpp`) | `Init` + `Set_Server_Authoritative(I_Am_Server())` |
| 18 | `WorldDebugDisplay` (`Code/Combat/worlddebugdisplay.cpp`) | `debug_layer world_bounds 1` |

### One actor check, not three

Infantry (`HumanPhysClass` → `Phys3Class`), ground vehicles (`MotorVehicleClass` →
`VehiclePhysClass` → `RigidBodyClass`) and aircraft (`VTOLVehicleClass` → same chain) all derive
from `MoveablePhysClass`. One call in `Post_Timestep_Process` therefore covers every gameplay
actor type, which is what makes "different subsystems maintain different boundary polygons" and
"aircraft uses unrelated bounds" impossible by construction rather than by review — both are on
Phase 2's own do-not-proceed list.

### Server authority

`Recover_Out_Of_Bounds_Actor` refuses to act unless `Set_Server_Authoritative(true)` was called.
`wwphys` cannot ask `CombatManager::I_Am_Server()` (wrong layer), so Combat sets it once at
startup. A client therefore never corrects its own position — it receives the corrected one
through the replication it already uses. Default is false, so an unconfigured or client-side
build enforces nothing.

Recovery prefers the actor's own last safe position (it was reachable, so it is certainly legal
ground) and falls back to `Closest_Valid_Point` when the actor has not had a legal frame yet.
Velocity is zeroed on recovery, or an actor pushed outward by an exploit would simply be
recovered again every frame. Section 5.6 is explicit that **death is not the boundary mechanic**;
this restores, it never kills. Every recovery logs actor identity, prior transform, attempted
transform and the restored position (step 17).

`MoveablePhysClass::LastSafePosition`/`HasLastSafePosition` are **not** persisted by Save/Load:
pure runtime recovery state with no gameplay meaning, re-seeded on the actor's first legal frame
after a load, and not worth a save-format change.

## Generation

`ProceduralBoundsBuilder::Build_Boundary_Polygon` builds the ring deterministically from the
profile and `WorldSeedService::Derive_Subsystem_Seed(seed, "boundary")`. The world is centred on
the origin (X spans `[-width/2, +width/2]`, Y likewise) — nothing in the roadmap fixes an origin
convention, and centring keeps the ring symmetric about the axes, which is what makes the derived
band rings well behaved.

Shapes (`boundary.shape`), matching Section 5.1's allowed list:

| Tag | Shape |
| --- | --- |
| `irregular_ring` | ellipse with deterministic per-vertex inward jitter (the default) |
| `rounded_ring` | the same ring with no jitter |
| `rectangle` | the explicit opt-in Section 5.1 says should not be the default |

An **unrecognised tag is a validation failure**, not a silent fallback: a profile asking for a
shape this generator does not have should say so rather than quietly producing a different world
than its author asked for.

Jitter only ever **shrinks** a radius, never grows it. That buys two guarantees for free: the
ring always fits inside the profile's declared world extent, and a star-shaped ring built from
strictly positive radii around one centre cannot self-intersect — so `Configure`'s
self-intersection check can never fail on builder output, and a failure there would indicate a
real bug rather than unlucky noise.

## Validation performed by `Configure`

Phase 2 step 3's four named rules, plus finiteness. Nothing is installed unless **all** pass — a
half-applied boundary is worse than none, because the rest of the engine would start trusting it.

- at least 3 vertices, all finite;
- minimum vertex spacing, including the implied closing edge (which is what catches a caller that
  closed its own ring by repeating the first vertex);
- non-self-intersecting (proper crossings only, so adjacent edges sharing a vertex are fine);
- minimum enclosed area.

Winding is normalized to clockwise on install, so downstream consumers — Phase 3's cliff mesh in
particular, which needs a consistent outward normal per edge — can rely on the direction without
re-deriving it.

## Debug drawing

`debug_layer world_bounds 1` draws each band's ring in its own colour at the configured air
ceiling: outer boundary (purple), hard boundary (red), cliff band (orange), approach (yellow).
Drawn at the ceiling rather than on the ground because the rings are a topological fact with no
terrain height to follow, and the ceiling is itself one of the values worth seeing.

`Get_Zone_Ring`'s inset is toward the ring's own centre rather than a true polygon offset. This is
debug drawing only — nothing queries these rings for legality, `Classify_Point` is the sole
authority — so the cheap approximation is correct here and exact for the roughly-radial rings the
builder produces. A true straight-skeleton offset would be needed only if a band ring ever became
load-bearing.

## What is proven, and what is not

Proven by the `worldbounds` self check (`Code/Commando/shaderselfcheck.cpp`, `Run_World_Bounds`),
all pure geometry, no device/scene/actors:

- unconfigured service answers permissively for every query (the legacy-map guarantee);
- each of `Configure`'s four validation rules fails on its own, with a reason, installing nothing;
- every zone edge lands exactly where the band widths say;
- actor legality vs. construction legality differ correctly at the cliff band;
- a footprint radius reaching into the cliff band is refused though its centre is legal;
- the air volume respects both the horizontal polygon and the ceiling;
- `Closest_Valid_Point` returns a point that is actually legal, leaves an already-legal point
  untouched, and pulls an over-ceiling point down;
- the camera clamp leaves an inside focus alone and brings an outside one back in;
- segment crossing detects a span that leaves the world and accepts one that does not;
- recovery restores the last safe position, counts the violation, ignores an in-bounds actor,
  falls back correctly when there is no last-safe position, and **refuses to act when not
  server-authoritative**;
- the builder is deterministic (same profile → identical polygon), seed-sensitive (different seed
  → different polygon), stays inside the declared world extent, and rejects an unknown shape tag.

Not proven here, and needing a real physics scene and a running match — the same reason every
other Commander/RTS live check in this tree carries (`awaiting_answers.md` Q-012): Phase 2's
targeted-validation list phrased as live statements. Infantry, a ground vehicle and an aircraft
each actually failing to cross; the camera actually refusing to pan out in game; a deliberately
teleported actor actually being restored by a live server. The arithmetic each of those reduces
to is checked above; the integration call sites are single-line and reviewed, but have not been
executed. Registered as `shader_worldbounds` / `fds_shader_worldbounds` and covered by the same
`ctest -C Debug -R "shader_|fds_shader_|terrain_shroud" --output-on-failure` sweep Q-012 names.

## The cliff ring

Phase 3. Design sources: Sections 5.3 (cliff geometry), 5.4 (cliff material), 5.5 (dimensions),
5.9 (feature interaction), 28 (asset reuse).

### Four of the five named components already existed

Phase 3's text names `BoundaryCliffGenerator`, `CliffSpline`, `CliffSegmentMeshBuilder`,
`CliffCollisionBuilder` and `CliffMaterialProjector`, and says to *create or adapt something
equivalent*. Adapting was the right half of that instruction, because this tree already had a
system whose header opens "the vertical face a heightfield cannot hold":

| Phase 3 component | What actually does it |
| --- | --- |
| cliff spline | `EscarpmentClass`'s polyline of `EscarpmentPointClass` stations |
| segment mesh builder | `EscarpmentSystem::Build_Escarpment_Geometry`, on `DynamicMeshBuilderClass` — the shared runtime-mesh helper the road, bridge, foliage and water systems all already use |
| collision builder | the `StaticPhysClass` that same function adds to the physics scene, carrying `ESCARPMENT_COLLISION_BITS` |
| material projector | `TERRAIN_LAYER_CLIFF` plus `MATERIAL_PROGRAM_TERRAIN`, which escarpments already resolve |

So `BoundaryCliffGenerator` (`Code/wwphys/boundarycliffgenerator.h/.cpp`) is one generator and
nothing else. A second cliff mesh path beside the existing one would have been the parallel
implementation this project's rules forbid, and would have produced two answers to "is there a
wall at x,y".

**There is no triplanar material.** Section 5.4 *prefers* triplanar rock projection; grepping the
whole of `Code/` finds no triplanar path anywhere, and this renderer has no programmable shader
tier to add one on (the same finding `docs/zerohour/VisualFidelityProgram.md` records — no
`CreatePixelShader` exists in this tree). The face is therefore dressed by the existing cliff
terrain layer, which is Section 5.4's other instruction — *use the v1 terrain/material/shader
system* — satisfied in full. Scree material near the toe is geometry here rather than a second
material, for the same reason.

### Why the ring is many escarpments

Phase 3 refuses to proceed if "the ring depends on one monolithic unbounded mesh", and
`EscarpmentSystem` builds exactly one mesh and one `StaticPhysClass` per escarpment. Handing the
whole perimeter over as a single escarpment would have been precisely that monolith: one draw
call and one collider the size of the world, culled all-or-nothing.

The ring is cut into chunks of `StationsPerChunk` segments, each its own escarpment. The seam
between two chunks is not "close enough" — chunk *k*'s last station is the **same array entry**
as chunk *k+1*'s first, read out of one canonical station array built once. Two chunks cannot
drift apart because neither owns the number. The last chunk closes onto station 0, which is why
the station array is deliberately left open rather than carrying a duplicate of its first entry:
a duplicate is a second copy of a number that must never differ from the first.

### Which way the face points

An escarpment drops to the **right of its direction of travel**. `WorldBoundsService` normalizes
its polygon to clockwise, and the interior of a clockwise polygon lies to the right of every
edge — so walking the ring in polygon order drops the face inward, leaving the high ground
outboard and the playable interior at the foot of the cliff looking up at it. Walking it the
other way would build the map as a mesa to fall off rather than a basin to be contained by. The
direction is load-bearing, not incidental.

### Plan, then realize

`Build_Ring` is `Plan_Ring` followed by `Realize_Ring`, and the split is what makes the ring
checkable:

- **`Plan_Ring`** decides what the ring *is* — the canonical stations, the chunk division, the
  escarpments carrying them, the checksum. It touches no render object and no physics scene.
- **`Realize_Ring`** turns that into meshes and colliders, and needs a `PhysicsSceneClass`.

Every property Phase 3 actually cares about — same seed means same ring, seams share stations, no
station is climbable, no face is inverted — is a property of the plan. The triangles are a
function of the plan and cannot introduce a fault the plan does not already contain. So a
device-less check can hold the ring to all four, which is why the `worldcliffs` self check is
worth more than its lack of a scene costs it.

Realization failing leaves **no** ring: `Realize_Ring` clears on failure. Half a cliff ring is
worse than none, because the gap is exactly the traversal leak this phase exists to prevent and
it would be invisible from inside the world.

### Determinism

The rim height at each station is `mid ± half_range * rim_roughness * noise`, where noise is
`Random4Class` seeded from `WorldSeedService::Derive_Subsystem_Seed(seed, "boundary_cliffs")`,
drawn in station order and then smoothed three times circularly.

`boundary_cliffs` is a **new seed domain**, deliberately not shared with `boundary`. Sharing would
have drawn the rim noise from the stream that already shaped the polygon, correlating the two so
that a bump in the outline always arrived with a matching bump in the cliff height. Separate
domains is what per-subsystem derivation is for.

The smoothing is not cosmetic. Raw per-station white noise puts a tall station directly beside a
short one, which is the shape a climbable notch has; three circular passes give the rim a
wavelength of several stations instead.

A `rim_roughness` of 0 puts every station on the **midpoint** of the min/max band rather than on
the minimum, so turning roughness off yields a level rim of sensible height rather than the
shortest cliff the profile permits.

### Schema version 2

Section 5.5 names seven profile controls. Three already existed; Phase 3 added the other four to
`ProceduralWorldBoundarySettings`, plus two fields the fitted-world path needs:

| Field | Meaning |
| --- | --- |
| `rim_roughness` | 0..1, how far the rim wanders inside the min/max band |
| `face_roughness` | 0..1, how much the face inflects between toe and rim |
| `toe_width` | world units of scree apron at the cliff foot, must be narrower than the band |
| `collision_simplification` | see below |
| `world.fit_to_level` | size the world from the host level's measured extents |
| `world.center_x` / `center_y` | where the world sits; a stock map is not built around the origin |

A schema-1 profile still loads — the new fields take their documented defaults. The generator
version moved to 2 at the same time, and independently: the same seed and the same schema-1
profile now produce a world with cliffs in it where before it produced an invisible boundary, so
the algorithm identity had to move even though a profile need not.

`collision_simplification` is **parsed, validated, hashed into the profile identity, and not
consumed.** Being honest about that is better than a fake use: an escarpment's collider *is* the
mesh it draws, so there is no separate collision representation to simplify. Simplifying it would
mean building a second, coarser geometry — and then "is there a wall here" would have two answers
that could disagree, which is the exact failure mode this phase's component reuse avoided. The
field stays because Section 5.5 names it and a later phase that introduces a real collision LOD
should find it already in the schema rather than having to bump it again.

### Escarpment extensions

`EscarpmentClass` gained three things, all additive and all defaulting to exactly its previous
behaviour:

- `FaceRowCount` (default 1) — how many bands of quads the face is built from. The generator asks
  for 3 only when face roughness is non-zero.
- `EscarpmentPointClass::FaceInflection` (default 0) — how far the middle of that station's face
  bulges out from the straight batter line, scaled by a bulge that is zero at both ends. With one
  row the only samples are t=0 and t=1, so the term vanishes and the geometry is byte-identical
  to what the class built before.
- `ToeWidth` (default 0) — the scree apron, drawn as part of the face so it shares the face's
  material. It is deliberately **not** a separate collision surface: it lies flat on the lower
  bench, the ground under it already stops everything it would stop, and a second surface a few
  centimetres above the first is how z-fighting and "standing on nothing" bugs start.

An existing map with hand-placed escarpments is unaffected, and `Clear_Ring` preserves them —
a procedural ring appearing is not a reason for an authored cliff to disappear.

### The cliff band is now immutable

Step 11. `WorldTerrainSystem`'s three mutation entry points — `Modify_Height_Region`,
`Apply_Road_Grade`, `Apply_River_Cut` — now refuse edits that fall in the cliff band, asking
`WorldBoundsService::Can_Place_In_Region`, the same question building placement asks, rather than
inventing a second notion of where the band is. Ground that can be raised inside the band is a
ramp waiting to be built.

The two polyline mutators skip the offending **stamps** rather than rejecting the whole call, so
a river approaching the perimeter simply stops at the band — which is Section 5.9's "terminate it
naturally" rather than a hard error.

With no boundary configured this always answers yes, so terrain editing on a hand-authored map
behaves exactly as it did before.

Steps 12 and 13 were already satisfied by Phase 2: `Can_Place_In_Region` has excluded the band
from construction since then, and `EscarpmentSystem::Is_Segment_Blocked` answers the navigation
question against the same faces. **Step 14 has nothing to reject yet** — procedural roads,
tunnels and bridges arrive in Phases 8 to 10, and `WorldBoundsService::Segment_Crosses_Hard_Boundary`
is already the query they will be refused with.

### Seam diagnostics

Step 15, all on the canonical stations rather than on the built mesh, because the stations are
what the mesh is a function of: a fault there *explains* a bad triangle where finding it in the
mesh only reports one.

- `Check_Seam_Continuity` — every chunk boundary shares a station, position and both benches, and
  the last chunk closes onto station 0.
- `Check_Inverted_Triangles` — no station's rim is at or below its own toe.
- `Check_Traversable_Ramps` — no station's face is short enough to climb.

**Render gap and collision gap are the same question**, asked once, because an escarpment's
collider is the mesh it draws. There is no third geometry that could disagree with either. That
is a property worth having, not a corner cut.

`Check_Traversable_Ramps` is about the world *looking and behaving* as though it were enclosed,
not about whether it is — Phase 2's hard boundary still stops an actor that gets past the
geometry, which is exactly why Phase 3's own do-not-proceed list forbids cliff geometry being the
only boundary enforcement.

### Debug drawing

`debug_layer boundary_cliffs 1` draws the rim line, the toe line, and one riser per station.
Rim and toe are drawn separately because a ring whose rim closes while its toe does not is a real
and otherwise invisible fault. Each riser is coloured green for a wall and red for a face short
enough to walk up — the same question `Check_Traversable_Ramps` answers numerically, on screen,
because a single short station in a ring of hundreds is a way out of the world and a count does
not say where it is.

Kept separate from `world_bounds` on purpose: the boundary is a topological fact and the ring is
geometry that is supposed to explain it, and seeing one without the other is how a mismatch
between them would be spotted.

## The procedural world owner

Phases 0 to 2 produced services and no one thing that decides a world exists and drives them in
order. Phase 3 step 1 asks for a cliff component "under the procedural-world/terrain owner",
which is the first time that owner is actually needed: `ProceduralWorldManager`
(`Code/Combat/proceduralworldmanager.h/.cpp`).

| Call | When |
| --- | --- |
| `Request_World(seed, profile, error)` | before a level load, from the menu button |
| `Generate_World(error)` | `CombatManager::Post_Load_Level`, once |
| `Destroy_World()` | `CombatManager::Pre_Load_Level`, dropping the previous world |

Directive 1.10 is the whole shape of it. With no request outstanding `Generate_World` returns
immediately having configured nothing, which is every hand-authored map.

Generation runs in `Post_Load_Level` rather than before the load for two reasons: a world fitted
to its host level cannot be sized until that level's extents exist, and the cliff ring needs a
physics scene to put geometry into.

**Boundary first, then cliffs.** Not cosmetic: the ring's purpose is to explain a boundary that is
already authoritative. If the cliff build fails, the boundary stays configured and the world is
still correctly closed — invisible, but closed.

**Generation is not replicated**, because Section 3B exists so it does not have to be. Every
client deriving the same world from the same seed and profile is the determinism contract
working. What must agree is the seed and the profile; the manifest is what makes disagreement
detectable rather than silent.

### Fitting a world to a hand-authored level

`world.fit_to_level` takes the world's size from `PhysicsSceneClass::Get_Level_Extents` — the
existing answer to "how big is this level", not a second extent computed here.

The world is made **larger** than the level, not fitted to it. Every band is measured inward from
the boundary polygon, so a ring drawn on the level's own outline would stand its cliffs on the
outermost ground a player can already walk on. Growing outward by the full inset instead leaves
the entire authored level inside `PLAYABLE_INTERIOR` and puts the cliffs on the empty ground
beyond it. The inset is two cliff-band widths plus the hard barrier, because Phase 2 derives the
approach band as one more cliff-band width — if that derivation ever becomes a real profile
field, `Fit_Profile_To_Level` is the second place that has to learn about it.

The cliff foot is set to the level's own floor (`min.Z`), not to an abstract sea level, or a ring
fitted to a map whose ground sits well above or below zero would float or be buried.

The manifest records the profile **as used**, fit-to-level included. Recording it as authored
would produce a manifest describing a different world than the one it came from.

## The main menu button

"Procedural Skirmish", between Practice and Options on the main menu
(`IDC_MENU_START_PROCEDURAL_GAME_BUTTON`). It takes the same skirmish map the Practice button
picks and places a world request before starting it.

A procedural world is laid **over** a real level rather than replacing one. The generator owns
the boundary and the ring that encloses the map; everything else a match needs — lighting,
spawners, presets, the physics scene itself — still comes from the level being loaded. Generating
a whole level from nothing is later phases' work.

Two details worth recording:

- The button's label is a **literal string**, not an `IDS_` id. `DialogBaseClass`'s
  `TranslateDialogString` only translates text containing `IDS_` and passes anything else through
  untouched, so a new label needs no new string-table entry. The cost is that this one label is
  not localized — the string table ships inside the game data and this project does not author
  new entries into it. A real `IDS_` id is the fix if that data is ever regenerated.
- A seed of 0 means "choose one". The chosen value is fixed immediately, logged, and carried into
  the manifest, so a world worth revisiting can be asked for again by number rather than being
  unrepeatable.

The profile is loaded and validated at the moment of the **request**, before the level starts
loading, so a bad profile is refused while the player is still looking at a menu rather than
after a load with the reason only in a log. It is read again at generation time, so a profile
edited between the click and the load is still picked up.

## What is proven, and what is not

Proven by the `worldcliffs` self check (`Run_World_Cliffs`), all on the ring plan, no device or
scene:

- an uninitialised generator claims no ring, no plan, no stations and no checksum;
- each parameter rule refuses on its own, with a reason, leaving no stations behind;
- station count and chunk count match the perimeter and the requested spacing exactly;
- the ring is divided into more than one chunk — Phase 3's own monolith condition;
- all three seam diagnostics report zero faults on a valid ring;
- every station's face height lands inside the profile's own min/max band;
- the same seed and polygon give the same checksum, a different seed gives a different one;
- a rim roughness of zero gives a level rim at the mid height;
- realizing with no physics scene **refuses** and leaves no ring behind;
- clearing returns the generator to its legacy-map state.

Not proven, and needing a display and a running match — the same reason every other check family
in this tree carries (`awaiting_answers.md` Q-012). Phase 3's targeted-validation list is phrased
as live statements, and the arithmetic each reduces to is checked above, but nothing here has
been on a screen: **no cliff has ever been drawn or collided with.** Specifically unseen are
whether the ring actually renders as rock rather than as untextured geometry (it depends on a
`TERRAIN_LAYER_CLIFF` layer naming a real texture, and the default layers name none — see
`docs/assets/TerrainLayers.md`), whether a vehicle can crest it, whether the toe apron reads as
scree or as a visible skirt, and whether the face inflection improves the look or merely costs
triangles. The menu button itself has never been clicked.

Registered as `shader_worldcliffs` / `fds_shader_worldcliffs` and covered by the same
`ctest -C Debug -R "shader_|fds_shader_|terrain_shroud" --output-on-failure` sweep Q-012 names.

## Phase 4 (not started)

Macro terrain generation and chunk ownership: the bounded playable landform *inside* this
immutable perimeter. Phase 3 deliberately stops at the rim — the band is reserved, protected and
now visible, and the ground it encloses is still whatever the host level brought with it.
