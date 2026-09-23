# Runtime Terrain Mutation and the WorldOperation Journal (V2 Phase 18)

Phase 17 let the Commander change the ground. Phase 18 makes every runtime change to the ground
**deterministic, ordered, authoritative, dependency-aware and replayable** — and makes that claim
checkable rather than asserted.

| Owner | File | Steps |
|---|---|---|
| `WorldOperationClass` — the versioned envelope | `Code/Combat/worldoperation.*` | 1, 3, 4, 14 |
| `WorldOperationJournal` — revision owner, journal, apply pipeline | `Code/Combat/worldoperationjournal.*` | 2, 5-13, 15, 16 |
| `HeightfieldClass::Smooth_Height_Region` | `Code/wwphys/heightfield.*` | 3 (SMOOTH) |
| `WorldTerrainSystem::Apply_Flatten` / `Apply_Smooth` / `Apply_Height_Patch` | `Code/wwphys/worldterrainsystem.*` | 7 |
| `worldop` console command | `Code/Commando/consolefunction.cpp` | 16 |

Self-check: `terrain_worldops` and `fds_terrain_worldops`. `ctest -N` is now 175, was 173.

---

## The one design decision: operations travel, heights do not

A peer never receives terrain. It receives the request that produced the terrain and performs it
itself. A crater is a fixed ~56 bytes whatever its radius; the only thing in an operation that
scales with anything is the polyline of the two types that carve along one, capped at 64 points.

That is what makes a checksum comparison meaningful rather than hopeful: two peers that ran the
same ordered list of operations from the same base terrain **have** the same terrain, and
`Get_Terrain_Checksum()` disagreeing proves that one of them did not.

`Begin` takes one snapshot of the heightfield, and from then on the world is defined as *the base,
plus these operations, in this order*. That single decision pays for three separate features:

- **RESTORE_REGION** has something real to restore from, with no per-operation undo blob.
- **Replay** has a defined starting point, so step 16 is a function rather than a hope.
- **Replication** stays compact, because nothing ever needs to send the heights.

## Step 1: the versioned envelope

`FormatVersion` travels with every operation rather than once per stream, so a journal that
outlives a build can be read back operation by operation instead of being all-or-nothing.

The wire format is written field by field, little-endian, never by copying the struct: a `memcpy`
of a class would tie the format to one compiler's padding and one machine's idea of an enum's
width. Floats go across as their bit patterns; every checksum quantizes to millimetres
(Section 3B.4) so two peers who agree about the ground to the last millimetre produce the same
number.

`worldoperation.h` contains no terrain code at all. It defines what an operation *is*, what a
well-formed one looks like, and how to write one down. That is what lets a client validate an
envelope it will never apply.

### Field semantics worth stating once

- `Center.Z` **is** the target height for the three absolute types — FLATTEN, GRADE_FOUNDATION and
  RECLAIM_SHALLOW_WATER. One field rather than a `TargetHeight` that would have to be kept in step
  with it.
- `Depth` is always stated **positive**. The sign belongs to the type, so a LOWER with a negative
  depth is malformed rather than a quiet RAISE.
- `Blend` is overloaded by type and validated by type: a 0..1 pull for FLATTEN, a 0..1 strength for
  SMOOTH, a 0..1 grade blend for CARVE_ROAD, and **the apron radius** for GRADE_FOUNDATION — which
  is why it is validated against `Radius` rather than against 1.

## Step 2: the world revision

`Begin` sets it to **1**, not 0. Zero means "no journal", so an operation submitted against
revision 0 is a submitter that never read one — a different mistake from a stale one, deserving a
different answer.

## Steps 3-4: ten operation types, and what each one invalidates

Dependencies are a static per-type mask (`World_Operation_Dependencies`), not a chain of ifs at the
apply site. Every type moves the ground, so every type invalidates render, collision and nav. What
differs is everything downstream:

| Type | + water | + roads | + materials |
|---|:-:|:-:|:-:|
| RAISE, LOWER, SMOOTH, CREATE_CRATER | | | |
| FLATTEN, GRADE_FOUNDATION | | ✓ | |
| CARVE_ROAD | | ✓ | ✓ |
| CARVE_CHANNEL | ✓ | ✓ | |
| RECLAIM_SHALLOW_WATER | ✓ | | |
| RESTORE_REGION | ✓ | ✓ | ✓ |

RESTORE_REGION invalidates everything any operation could have, because it can undo any of them.
Assuming less would leave whichever system the undone operation had touched holding a view of
ground that no longer exists.

### LOWER and CREATE_CRATER are deliberately the same arithmetic

A crater *is* a bounded lowering. Giving it its own dish would be a second falloff to keep in step
with the first. What makes it a crater is step 15's rules, not the shape.

### Protected-region rules are per type

Which features forbid an operation depends on the operation. Carving a channel is *supposed* to
move water; reclaiming shallow water is supposed to fill it. Both are still forbidden to touch a
bridge abutment, a tunnel mouth, a resource route or an immutable cliff. RESTORE_REGION is
permitted everything except the boundary band, because putting ground back cannot damage the
feature it is putting back.

This is why `GeneratedWorldPlacementRules` gained `Get_Static_Feature_Mask`: asking "is anything
here", being told "water", and then having to ask again without the part that was allowed is not a
question the old first-hit classifier could answer. `Classify_Static_Features` is now the lowest
set bit of that mask, which is what keeps the two from ever disagreeing about what is present.

## Step 5: transaction reservation

The affected chunks are held from before the mutation until after the last dependency has
committed — the window in which the ground is inconsistent with the systems that describe it.
Reservation is **all or nothing**: a half-taken reservation is worse than a refused one, because
the caller would hold some of the ground it needs and none of the right to the rest.

The grid is the terrain's own patch grid, which is the grid the chunk registry is built on, so a
reservation and a chunk-registry entry mean the same square of ground. It is available whenever
terrain is, where the chunk registry only exists once something has built one.

## Step 6: stale revisions, "where ordering matters"

The roadmap hedges, and the hedge is real. RAISE, LOWER, SMOOTH and CREATE_CRATER are **relative** —
each one's result depends on the ground it found, so running one against a revision other than the
one the submitter measured produces a different world, and it is rejected. The other six drive the
ground to an absolute height, so a stale revision does not change their outcome in their own
region, and they are accepted. Journal order still decides what happens where two of them overlap.

## Steps 8-12: dependencies, and why the revision moves last

Step 12 says a new revision may be committed "only after authoritative dependencies reach required
commit point". The reason is visible the moment you try it the other way: a peer that sees revision
N+1 and asks the pathfinder a question is entitled to an answer about revision N+1's ground. So the
revision is the last thing to move, and a result with `Committed` false names the dependency that
did not commit.

Each dependency is driven through the system that owns it — `Build_Collision`, `Build_Chunk_Registry`
(against the same slope cap it was built with), `NavMeshGenerator::Build`,
`WaterSystem::Stamp_All_Masks`, `RoadSystem::Apply_Grades`, `BridgeSystem::Conform_Abutments`,
`EscarpmentSystem::Conform_To_Terrain`, `RoadSystem::Stamp_Road_Mask`. Nothing is recomputed in the
journal.

**A dependency whose system is not present reaches its commit point trivially** and says so in the
detail string rather than pretending work was done. That is a real state, not a test artefact: a
dedicated server has no render data, a headless tool has no physics scene. The operation is
journalled either way — it really did change the ground, and a journal that omitted it could not
replay the world.

## Step 15: bounded crater rules

Max radius, max depth, a rate limit, and no cliff-band mutation. The rate limit reads the **journal**
rather than a counter, because the journal is the one record that survives a replay — a replay that
hit the rate limit differently from the original run would not be a replay. The band check is
applied to *every* type, not only craters: `WorldTerrainSystem`'s own edit gate already refuses
silently, and refusing here first is what turns a no-op into an answer the submitter can read.

## Step 16: the replay command

```
worldop status                        -- revision, counts, journal and terrain checksums, last 8 ops
worldop begin                         -- open a journal over the terrain as it stands
worldop replay                        -- restore base, re-apply everything, print checksum before/after
worldop crater X Y RADIUS DEPTH       -- submit a real CREATE_CRATER and print its verdict
worldop restore X Y RADIUS            -- submit a real RESTORE_REGION
```

`worldop replay` prints the terrain checksum before and after, and whether they MATCH. That is the
phase's own "save/replay produces same checksum" acceptance line, made runnable by a person.

Replay applies one dependency pass at the end rather than one per operation: replay reproduces a
state, and the state every system needs to see is the one after the last operation. Rebuilding nav
nine times to arrive at the same graph would only make replay slower. Replay also leaves the
revision and the journal untouched — it reproduces a state, it does not invent history.

---

## One thing the self-check taught, worth writing down

**Smoothing a plane is correctly a no-op.** The first version of the check ran SMOOTH over the
uniform test ramp and asserted the ground changed; it did not, and the code was right — a linear
ramp is already its own four-neighbour mean. The check now digs a real dent first and asserts that
the dent gets *shallower*, which is what the operation actually promises. An assertion that some
bytes changed would have passed on a smoother that did the wrong thing.

The smoother itself reads every neighbour from the heights as they were before the operation began.
Filtering in place would make each vertex's result depend on the sweep order — the difference
between a filter and a smear, and the difference between an operation that replays identically on
every peer and one that does not.

## Disclosed gaps — none of them implementation shortfalls

- **RECLAIM_SHALLOW_WATER refuses a partly dry disc** rather than levelling it. Raising a partly dry
  region to the waterline would *lower* the dry part, which is not reclamation, it is FLATTEN by
  another name and without FLATTEN's own checks. It is also off by default (step 3's "if enabled"):
  reclaiming water changes a map's navigable area, which a profile opts into rather than something
  a weapon does by accident.
- **Replication is the format, not the transport.** `Write_Block`/`Read_Block` produce and consume
  the compact operation and are proven by round trip; no packet type sends one yet. Wiring it to the
  network layer is Phase 19's business, where the snapshot and late-join formats it has to sit
  beside are defined.
- **Nav rebuild is whole-world**, inherited from Phase 16's disclosed absence of an incremental
  rebuild, not added here.
- **The journal lives in memory.** Persisting it to a save file is Phase 19 step territory; what
  exists here is the ordered, checksummed, replayable structure a save format would write.
- **No profile block for the crater limits yet.** They are set through
  `WorldOperationJournal::Set_Crater_Limits`, defaulted conservatively, and not yet sourced from
  `ProceduralWorldProfile` — the roadmap does not ask for one at this phase, and inventing a schema
  field ahead of its own phase is what Section 34.0's phase discipline exists to avoid.

## What is proven, and what is not

**Proven by `terrain_worldops`:** every type, result and dependency mask is real and self-consistent;
the ordering rule splits relative from absolute types correctly; a default operation is malformed
rather than a no-op; a negative depth is refused; an apron inside its own pad is refused; a
path-less CARVE_ROAD is refused; affected bounds cover the whole polyline and reach a grade's apron
rather than its pad; the compact form round-trips every field including `Blend`, uses exactly the
bytes it reported, does not grow with the size of the region, and refuses a truncated block; a stale
revision is refused for a relative type and not for an absolute one, and the refusal moves no
ground; a real RAISE moves the ground, gets id 1, affects real chunks, advances the revision to 2,
releases its reservation and is journalled; ground held by an explicit reservation cannot be taken,
and the *same* operation succeeds once it is released; each crater limit refuses when broken and the
rate limit is a rate rather than a total; a zero limit is clamped; reclamation is refused while
disabled and refused honestly when enabled over dry ground; LOWER, FLATTEN, GRADE_FOUNDATION,
CARVE_ROAD and CARVE_CHANNEL each really move ground and each commit; SMOOTH makes a real dent
shallower; RESTORE_REGION puts a raised region back; replaying the journal from the base snapshot
reproduces the same terrain checksum, twice, without changing the revision, the operation count or
the journal checksum; a crater inside the boundary band is refused and moves nothing; and `Close`
leaves nothing behind.

**Not proven here:** that two processes agreed. The self-check runs one journal in one process, so
what it demonstrates is determinism under replay, which is the necessary half. "Server applies,
client reconstructs, collision matches" needs two peers and a transport, and the transport is
Phase 19's. Phase 18's gate is met for everything a single authoritative process owns; the
cross-peer half of its targeted validation is carried forward and stated as such rather than
claimed.
