# World Bounds and Boundary Cliffs (V2)

Required output of roadmap Section 34 Phase 2. This document currently covers the **boundary**
half only; the **cliff geometry** half is Phase 3 and appends to this same file rather than
starting a second one.

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

## Phase 3 (not started)

The cliff **geometry** — `BoundaryCliffGenerator`, cliff spline, segment mesh, collision, and the
triplanar rock material — is Phase 3 and is what makes the boundary visible and physically
explained (Sections 5.3-5.5). Phase 2 deliberately stops at the authoritative topology: the cliff
band is reserved and protected, but nothing renders in it yet.
