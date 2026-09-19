# Commander building ghost / placement validation

Roadmap Section 43, Phase 37.

`Code/Combat/commanderplacement.{h,cpp}`.

## One function, called twice

Section 43's real requirement is not the ghost. It is this:

> Server reruns placement checks. Never trust client green/red state.

The only way to make that *true* rather than promised is for the red/green the client draws and the
accept/reject the server decides to be **the same function**. So `Validate` is one static function
over one request struct, with no client or server in it at all, and both sides call it: the client
every time the cursor moves, the server once when the placement arrives. They cannot drift, because
there is nothing to drift from.

`Validate` is pure with respect to this class — it reads the descriptor registry, the spatial index
and the terrain, and writes nothing. Calling it a hundred times as a cursor moves costs a hundred
queries and changes no state, which is what makes it safe to be both the ghost's colour and the
server's verdict.

`Validate_Server_Request` is `Validate` plus bookkeeping: it records the sequence id of an
acceptance and counts rejections, which makes *"the server independently rejected the same bad
placement"* observable rather than merely true. It is deliberately **not** where the building gets
created — that is Phase 38 — because that is how a validator grows a side effect.

## The ghost is state, not an object

Section 43: *"render-only, translucent, cursor-following, no gameplay collision/network/save
object."* This file contains no code that creates a `GameObj` of any kind. The ghost is a
`CommanderPlacementRequest` plus its last `CommanderPlacementResult`, recomputed by `Move_Ghost` and
`Rotate_Ghost` so a renderer never has to decide when to revalidate.

The `RED TRANSLUCENT` state Section 43 names in capitals is `Is_Ghost_Valid()` coming back false.
There is no separate "invalid" flag that could disagree with the validator.

Rotation is kept in `[0, 360)` so a ghost rotated for a long time cannot accumulate a value that
loses precision, and so a replicated rotation has one representation per orientation.

## The footprint is oriented, and it is the same box both consumers see

`Build_Footprint_Box` produces an `OBBoxClass`, not an AABB: a rotated building is tested rotated.
Section 16's `WorldSpatialIndex::Query_Placement_Overlap` already has an OBB overload — the Commander
ghost is the caller it names — so the *"use better OBB/compound footprint when available rather than
forcing AABB-only tests"* line costs nothing extra.

The box is exposed because Section 43 also asks for the footprint to be shown *"through
status/waypoint renderer where useful"*. One box, built once, tested and drawn — not the validator's
box and a second one drawn near it.

**The footprint block is two-dimensional.** Section 39's schema gives a width and a depth; there is
no height, because it is a ground footprint. The overlap test needs a third dimension, so it uses a
fixed generous `FOOTPRINT_TEST_HEIGHT`. That choice cannot produce a false *accept*: a box that is
too tall can only ever find more overlaps, never fewer.

## Which check answers which Section 43 bullet

| Section 43 asks for | Where it comes from |
|---|---|
| world bounds | `WorldTerrainSystem::Query_Placement_Surface`'s `IsOnTerrain` |
| surface support | the same query, plus `placement_rules.requires_foundation_support` |
| configured slope limits | the same query's `MaxSlope` against `placement_rules.max_slope_degrees` |
| building overlap | `WorldSpatialIndex::Query_Placement_Overlap` (OBB, `COLLISION_GROUP_WORLD`) |
| placement-blocking geometry | the same query |
| production-exit clearance | a second OBB in front of the footprint, `placement_rules.exit_clearance` |

Building overlap and blocking geometry are one query and one reason on purpose: at this level both
are `COLLISION_GROUP_WORLD` physical objects, and splitting them would mean a second query whose
only purpose is to relabel the same hit.

`placement_rules` is a new block on the building descriptor this phase adds — see
`docs/zerohour/BuildingDescriptors.md`. Keeping the slope limit and the exit clearance in JSON is
Section 39's *"do not hard-code ... in C++"* rule applied to placement.

## A level with no heightfield is not a refusal

`WorldTerrainSystem::Has_Terrain()` is false on every stock Renegade level: they are authored
geometry, not generated heightfields. Refusing every placement on one would make the whole feature
untestable on real content, so in that case the ground is whatever the request said, the slope check
is skipped, and the static-geometry overlap test does the real work. This is a deliberate,
documented behaviour, not an oversight — it is also why the self check's baseline placement is
*valid*, which in turn means every refusal it checks is caused by the one thing being tested.

## What this phase does not build

- **No ghost renderer.** Nothing draws the translucent model or the red tint. The state, the
  transform and the validity are all live and checked; the drawing is not written, and is recorded
  in `WIP_features.md` alongside the sidebar renderer it belongs with.
- **No network message.** The request struct is exactly the four fields Section 43 says the client
  sends, and `Validate_Server_Request` is the server's entry point — but nothing yet puts the struct
  on the wire. Because no network compatibility with other clients is required (directive 0), this
  is a small, unconstrained addition whenever the UI that produces a placement exists.
- **Compound footprints.** One OBB per building. A multi-part footprint would be a descriptor schema
  change and has no caller yet.

## Checks

`renegade -shaderselfcheck placement`, registered as ctest `shader_placement` and
`fds_shader_placement`. Device-less. Pins: the definition-level refusals (unknown descriptor, not
buildable, wrong faction) with their own distinct reasons; the footprint box matching the
descriptor's dimensions and genuinely rotating with the request; the ghost tracking `Validate`
through moves and rotations, including the wrap past zero; an unbuildable ghost being active and
permanently red; cancel leaving nothing; and — the acceptance line — the server refusing exactly
what the client's own validator refuses, counting the rejection, and not recording a rejected
sequence id as accepted.

**Not covered by any automated check**: the overlap, slope, world-bounds and exit-clearance branches
all need a live `PhysicsSceneClass` or a generated `WorldTerrainSystem` heightfield, neither of which
`-shaderselfcheck` can stand up. What is proved is that the definition-level rules hold, that the
footprint box those world queries are handed is built correctly and rotates, and that client and
server reach the same verdict.
