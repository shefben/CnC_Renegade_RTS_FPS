# Sectional bridge system

Roadmap Section 20. Donor: Zero Hour's `W3DBridgeBuffer` and the bridge half of
`TerrainRoadType`.

Acceptance: *bridges support variable length, damage, broken spans, collision, and
multiplayer state.*

## What is here

| File | Holds |
|------|-------|
| `Code/WWPhys/bridgesection.h/.cpp` | `BridgeSectionKindType`, `BridgeDamageStateType`, `BridgeStateRuleClass`, `BridgeDefinitionClass`, `BridgeSectionClass` |
| `Code/WWPhys/bridgesystem.h/.cpp` | `BridgeClass`, `BridgeDebrisClass`, `BridgeSystem` |
| `Code/WWPhys/dynamicmeshbuilder.h` | `DynamicMeshBuilderClass`, shared with the road system |
| `Code/Commando/terrainselfcheck.cpp` | `Check_Bridges`, run as `terrain_bridges` and `fds_terrain_bridges` |
| `Code/Commando/consolefunction.cpp` | `bridge_test`, `bridge_break`, `bridge_clear` |

## How a bridge is described

A bridge is **two points and the name of a kind**. Everything else is derived.

A kind is a `BridgeDefinitionClass`: a width, a deck thickness, a length for each of the
five section kinds, a pier spacing and clearance, an optional decoration spacing, and a
table of `BridgeStateRuleClass` — one per section kind per damage state. A rule is exactly
the list Section 20 gives of what a state may change: the model, whether it collides,
whether the path still runs through it, and what debris comes off on arriving in that state.

Three kinds are defined by default, and their widths are Section 19's three road classes'
widths so a road runs onto a bridge without a step:

| Definition | Width | Span | Piers every | For |
|-----------|-------|------|-------------|-----|
| `ow_bridge_timber` | 4 m | 5 m | 10 m | `ROAD_CLASS_TRACK` |
| `ow_bridge_concrete` | 8 m | 8 m | 24 m | `ROAD_CLASS_STREET` |
| `ow_bridge_steel_truss` | 16 m | 16 m | 48 m | `ROAD_CLASS_HIGHWAY` |

None of them names a model. See [../assets/BridgeSections.md](../assets/BridgeSections.md).

## Variable length

`Layout_Bridge` takes the two abutment lengths off the gap, divides what is left by the
number of the definition's nominal spans that best fits, and stretches those spans to land
exactly on the far bank. A bridge is never short of the far side and never overshoots it,
and no length of gap needs a mesh made for it — which is what the acceptance means by
variable length.

Two edges are handled rather than assumed away. A gap too short to hold both abutments
scales them down **together**, so a short bridge is still a bridge with two ends and
something between them rather than one abutment and a stub. And the span count is clamped to
at least one, so the shortest possible bridge still has a span to break.

Piers go under the joints between spans, at the definition's spacing, and only where the
ground is far enough below the deck to be worth standing on — measured per pier through
`WorldTerrainSystem::Sample_Height`. Where there is no terrain to ask, the pier is built at a
nominal drop, because a bridge stands on something whether or not the check knows what.

## Damage, and broken spans

Damage is **per section**, not per bridge. That is the one real generalisation over the
donor, whose `W3DBridge` has a single `BodyDamageType` and three meshes, and it is forced by
the acceptance saying "broken spans": the interesting thing to do to a bridge is put a hole
in the middle of it and leave both ends standing.

So:

- `Set_Section_State` changes one section, notes its debris, and rebuilds **that one
  section's** geometry. A shell landing on a span costs one mesh.
- `Set_Damage_State` is the whole-bridge convenience over it.
- `Break_Span_At` finds the span nearest a point and breaks that — what a shell does.
- `BridgeClass::Get_Damage_State` reports the worst state any load-bearing section is in,
  because a bridge with one broken span is a broken bridge whatever the rest looks like.

A section that neither collides nor is traversable and names no model is not built at all,
so a broken span is a genuine hole with nothing standing in it.

## Traversal and pathing

- `Is_Traversable(id)` — can the far side be reached.
- `Find_Break(id, &section)` — where it stops.
- `Get_Traversable_Extent(id, &from_start, &from_end)` — how far onto it you can get from
  each end, which is what a unit that has to turn round needs.
- `Conform_Point(x, y, &height, &bridge)` — where the deck is. This is the counterpart of
  `RoadSystem::Conform_Point` and the answer to the opposite question: a road conforms to
  the ground, and over a bridge the deck **is** the ground. It returns false over a broken
  span, which is what makes a hole a hole rather than an invisible floor, and picks the
  higher deck where two bridges cross.

A bridge naming a kind nobody defined stays in the list with no sections: it draws nothing,
carries nothing, reports itself untraversable and can be named in a log. A map error is not
a crash.

## Collision

Unlike a road, a bridge collides. `RoadSystem` deliberately sets collision type zero,
because the ground under a road already collides and a second surface a few centimetres
above the first is a thing to trip on. A bridge is the opposite case — there is nothing
under it but the drop — so every section whose state says it collides becomes a
`StaticPhysClass` in the physics scene with physical, projectile, vehicle and camera
collision.

## Multiplayer state

The server owns the states. `Get_State_Block` writes a bridge count, then per bridge an id,
a section count and **one byte per section**; `Apply_State_Block` reads it back. Nothing
about the geometry travels, because a client holding the same definition and the same two
endpoints lays out exactly the same sections in exactly the same order — which is what
Section 20 means by replicating state rather than raw geometry where deterministic
reconstruction is possible.

Applying the server's state is not a local change to send back out, so `Apply_State_Block`
leaves the dirty flag as it found it. A block describing a bridge whose section count does
not match is skipped and logged rather than scribbled onto the wrong sections — that
mismatch means the definitions or the endpoints differ, which is a real problem worth seeing
rather than one to paper over.

The transport is not here. There is no packet, no net object and no send site: that is the
network phase's business, and this phase's job was to make the state small, deterministic
and separable, which it is.

## What was taken from the donor and what was not

**Taken.** The shape of a bridge as cap, repeated section, cap; the width scale; the model
per damage state; the idea that a bridge is described by two points and a template name.

**Not taken.** The donor packs every bridge in the world into one shared vertex and index
buffer with a 200-bridge, 12000-vertex cap, culls per bridge and rebuilds the buffer when
visibility changes. That is a draw-call optimisation for an RTS drawing a whole map at once,
and it is the wrong trade here: it caps how many bridges a map may have, and it makes
changing one span's state rewrite a shared buffer. Sections are separate static objects
instead, which is also what lets damage cost one mesh.

**Not taken.** `BodyDamageType`'s `REALLYDAMAGED` and `RUBBLE` are renamed `CRITICAL` and
`BROKEN` to match the roadmap's own list, which is the naming authority here.

**Not taken.** Bridge tower objects and repair scaffolds. Those are gameplay objects with
health, not sections, and they belong to whatever builds gameplay objects.

## Shared with the road system

`DynamicMeshBuilderClass` was `RoadMeshBuilderClass`, private to `roadsystem.cpp`. Both
systems build a `DynamicMeshClass`, which is sized at creation and therefore has to have its
triangles counted before any are written, so both run their emitter twice against the same
helper. It was moved to its own header rather than written twice.

The box a missing section is built as went the same way: it was `Emit_Box` here, and Section
21's foliage system needs the same box for a tree's collision post, so it is now
`DynamicMeshBuilderClass::Box` and this file's copy is gone. See
[FoliageSystem.md](FoliageSystem.md).

## Not done

- **Nothing has been seen.** The checks run device-less, so there is no physics scene and
  `Build_Geometry` correctly declines. What a bridge looks like, and whether a soldier walks
  onto one from a road and falls through a broken span, is `bridge_test` in a running
  client.
- **No bridge art exists.** Every section is a slab of its declared size until it does;
  the sixty model names and the constraints they have to be built to are in
  [../assets/BridgeSections.md](../assets/BridgeSections.md).
- **Nothing repairs a bridge.** The states go both ways and `Set_Damage_State` will take a
  bridge back to `PRISTINE`, but nothing in the game calls it.
- **Debris is recorded, not drawn.** `Peek_Pending_Debris` has no consumer until Section 35.
- **Nothing damages a bridge from gameplay.** `Break_Span_At` exists and only the console
  calls it; wiring it to weapon damage needs a gameplay object over the bridge, which is the
  same missing piece as the towers.
- **The state block has no transport.** See above.
- **Pathing does not know.** `Is_Traversable` and `Conform_Point` answer the questions a
  path-finder would ask; no path-finder asks them yet.
