# Snapshots, Checksums, Late Join, Regional Repair and Rollback (V2 Phase 19)

Phase 18 made runtime world changes deterministic and replayable. Phase 19 makes a *mutated* world
practical to join, verify, repair and roll back — **without sending the world**.

| Owner | File | Steps |
|---|---|---|
| `WorldChunkSnapshotClass`, `WorldSnapshotService` | `Code/Combat/worldsnapshot.*` | 1-7, 17-18 |
| `WorldSyncHandshakeClass`, `WorldSyncService` | `Code/Combat/worldsyncservice.*` | 8-16, 19 |
| `WorldOperationJournal::Rebase` / `Adopt_Revision` / base accessors | `Code/Combat/worldoperationjournal.*` | 6-7, 9-11 |

Self-check: `terrain_worldsync` and `fds_terrain_worldsync`. `ctest -N` is now 177, was 175.

---

## The shape of the thing

A snapshot never contains a chunk. It contains **the difference between a chunk and the chunk a
client can already regenerate for itself** from the seed and profile. On a pristine chunk that
difference is nothing and the snapshot is a six-word header. On a chunk somebody cratered it is the
handful of vertices the crater moved.

That is step 3's own wording — "only authoritative mutable/generated state needed beyond baseline
regeneration" — read literally, and the literal reading is both the simplest to implement and the
smallest to send. In the self-check, a world whose raw heightfield is 17 KB reconstructs from under
half that, and one chunk's repair is a fraction again.

## Steps 1-2: what a checksum is allowed to read

**Heights, quantized to the millimetre, and nothing else.**

The obvious extra input is the chunk registry's own entry — and it was in there, until a late join
proved it cannot be. That entry is *derived* from the same heights, so it adds no information a
checksum over the heights does not already have. What it does add is sensitivity to sub-millimetre
noise: a client rebuilds its ground from quantized heights, so its floats land within half a
millimetre of the server's; the registry then derives slopes from those floats and truncates them at
1e-3, and a value either side of a boundary truncates differently on the two machines. The ground
agrees, the entry does not, and the repair protocol — which cannot tell that from real corruption —
sends the client chasing a chunk that needs nothing.

**A derived value in a checksum is fragility without information.** That is the single most useful
thing this phase taught, and it cost three debugging rounds to learn.

The world checksum composes every chunk checksum in *grid order* (not iteration order), then the
world revision and the journal checksum. The revision is in there because two worlds with identical
ground that disagree about how they got there are not the same world for ordering purposes, and a
late joiner should find that out at the handshake rather than at the first operation it cannot place.

### The quantizer has to round-trip exactly

`Quantize(Dequantize(q)) == q`, for every q. A snapshot stores quantized heights and a client applies
them, so a client's stored height is the dequantized value where the server's is the original float.
Truncation breaks this: `2.999f * 1000.0f` is 2998.9998 in float, so a truncating quantizer turns
2999 into 2998 on the way back. Rounding does not, and the division is done in `double`.

## Steps 6-7: compaction and rebase

`WorldOperationJournal::Rebase` folds everything applied so far into the base snapshot and empties
the operation list, keeping the revision exactly where it is — a compaction changes how the world is
*described*, not what it *is*, and a peer holding revision N must not be told the world advanced
because a server tidied its log.

Everything or nothing, deliberately. Folding only the first N operations would be wrong: the base
would then have to be the ground as it was after operation N, and the ground as it *is* includes
every later one, so there is nothing to copy it from without replaying. **A compaction point is a
moment, not a prefix.**

`Compact` captures the dirty chunks **first** and only then rebases, in that order for the obvious
reason: rebasing makes every chunk pristine by definition, so a capture taken afterwards would be
empty and the world's history would be gone with nothing holding it.

Three thresholds (`count`, `age`, `bytes`) because an operation log can become a problem three ways.

## Steps 8-12: the handshake, and what it refuses

Generator version, profile hash, content hash, manifest revision, snapshot inventory, snapshot
revision, current world revision, world checksum. **If the first four disagree the join is refused
there, before a single snapshot byte moves.** A client that regenerates a different baseline and then
patches it with the server's snapshots does not end up with the server's world; it ends up with one
that is wrong everywhere the snapshots did not happen to cover, and no checksum after the fact can
tell it which parts.

`SnapshotRevision` is separate from `WorldRevision` because they differ whenever a server has applied
operations since its last compaction: snapshots at revision R, then the operations that followed. The
client adopts R and replays those operations, arriving at `WorldRevision` by the same arithmetic the
server used.

### Why a client must adopt the revision

`WorldOperationJournal::Adopt_Revision` exists for exactly one caller. A late joiner's ground reaches
the server's without a single operation passing through its journal, so its revision would otherwise
sit at 1 while its ground is at the server's N. That is not cosmetic: step 16 gates building and AI
state on the revision, so a client that under-reports it would refuse state it is actually ready for —
or submit an operation against revision 1 that the server has long passed.

The join is a small explicit state machine (`Begin_Join` → `Apply_Snapshot_Block`* →
`Apply_Operation_Block`* → `Finish_Join`) rather than one call, because each step fails for its own
reason and "the baseline did not match" and "a snapshot was refused" lead to completely different
recoveries.

Operations arriving during a join are **submitted through the journal**, exactly as the server
submitted them, rather than applied directly: a client that shortcut the journal would end up with
the server's ground and its own idea of the revision.

## Steps 13-15: regional repair

Per-chunk checksums let a client say "chunk 7,3 disagrees" instead of "something disagrees". **That
is the entire difference between a repair and a reconnect.**

`Build_Repair_Block` returns **the retained snapshot** for that chunk, not a fresh capture. Step 14's
wording is "returns only affected chunk snapshot", and the definite article is load-bearing: after a
compaction the journal's baseline is the current ground, so every chunk is pristine against it and a
fresh capture comes back empty — a repair block that repairs nothing. The retained set still holds
each chunk's difference from the baseline a *client* regenerates, which is the baseline a repair has
to be expressed against. A fresh capture is the fallback for a chunk the set does not cover.

`Apply_Repair_Block` **reverifies** the chunk rather than trusting the block it just applied.
Trusting it would make a repair that silently failed indistinguishable from one that worked.

## Steps 17-18: developer rollback

Bounded at eight points, oldest dropped — an unbounded history of a mutated world is the full-world
storage this phase exists to avoid. Rollback points are whole-field, unlike snapshots, because they
never leave the server, so the reason to make them compact does not apply.

**The journal is rebased onto the rolled-back ground rather than rewound.** Rewinding would need
every operation since the point to be individually invertible, which they are not — a SMOOTH destroys
the information needed to un-smooth it. Rebasing states the truth instead: this is the ground now, and
nothing has happened to it since.

Step 18 says not to expose casual gameplay rollback, and nothing does: the only caller is the
developer path.

## Step 19: metrics

`LateJoinBytes`, `SnapshotBytes`, `OperationBytes`, `RepairBytes`, `GenerationTimeMilliseconds`, plus
join/repair/mismatch counts. Every byte figure is the size of something that was actually written or
read, never an estimate.

---

## Disclosed gaps — none of them implementation shortfalls

- **Still a protocol, not a transport.** Every message is a byte block a caller writes and reads;
  nothing here opens a socket or knows a packet type exists. That is deliberate — it is what lets the
  whole join/verify/repair sequence be exercised end to end without a network, which is how the
  self-check proves it. Binding these blocks to the existing packet types is a networking work item,
  not a generation one.
- **Snapshots cover terrain heights.** Buildings, vehicles and AI state each already have their own
  replication or save path; step 16's readiness gate is what this phase owes them, and it is
  implemented. Folding them into the chunk snapshot would have made a second owner for state that
  already has one.
- **`GenerationTimeMilliseconds` is recorded, not measured here.** `WorldSyncService` does not run the
  generator, so it takes the number from whoever does (`Note_Generation_Time`). Measuring it inside
  the sync service would mean timing a call it does not make.
- **The profile and content hashes are supplied by the caller.** This service owns neither a profile
  nor an install, and inventing a hash for either would be a number that means nothing.
  `ProceduralWorldManager` and `WorldGenerationManifest` are the real owners.

## What is proven, and what is not

**Proven by `terrain_worldsync`**, end to end in one process — which works because the whole protocol
is byte blocks: build a world, mutate it with real operations, capture what a server would send,
**throw the world away**, regenerate the same baseline deterministically, feed the blocks in, and
compare checksums. That is exactly the sequence steps 9-12 describe, minus a socket.

Specifically: chunk and world checksums are stable, distinguish different chunks, and refuse off-grid
requests; a pristine world is zero dirty chunks and zero snapshot bytes; two real operations dirty
some chunks and not all; every snapshot round-trips byte for byte and refuses a truncated block; the
whole set is under half the raw field; the handshake round-trips and each of its five identity fields
refuses a join when broken; a refused join leaves nothing in progress and accepts no snapshots; the
real join reconstructs a world whose checksum equals the server's exactly; late-join bytes come in
under the raw field; a correctly reconstructed world reports no mismatched chunks; corrupting one
vertex is detected, names the right chunk, is repaired by one chunk's block, and the world checksum
returns to the server's; the readiness gate passes for revisions reached and refuses ones not;
compaction triggers on its threshold, folds the log, leaves the ground and the revision untouched,
and leaves a world with no dirty chunks; a broken policy is clamped; rollback restores the marked
chunk's checksum exactly, rebases the journal, refuses an out-of-range index, and drops the oldest
point at its cap.

**Not proven here:** two processes on a real network. The self-check demonstrates that the protocol
and the formats are correct and that reconstruction is exact; it does not demonstrate that packets
arrive. Phase 19's gate — "a late client reconstructs the same current world without downloading
complete raw world meshes, and checksum mismatch can be repaired regionally" — is met for everything
that is not the wire itself, and the wire is stated as outstanding rather than claimed.
