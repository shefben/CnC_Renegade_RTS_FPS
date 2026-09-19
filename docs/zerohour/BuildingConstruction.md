# Authoritative construction + procedural block build-up

Roadmap Section 44, Phase 38.

`Code/Combat/buildingconstruction.{h,cpp}`.

## Everything follows from one sentence

> Clients deterministically reconstruct blocks.

If a client can rebuild the exact block layout from a handful of replicated numbers, then the
network never carries a block transform, a late joiner needs nothing but the same numbers, and a
save file stores the numbers rather than the geometry. So `Generate_Blocks` is a pure function of
`(model bounds, construction_visual block, seed)` and contains no `rand()`, no clock, no
floating-point accumulation across calls, and nothing read from a device.

The within-row order comes from an **integer hash of `(seed, row, cell)`**, not from a random number
generator. A generator's output depends on how many times it has been called, which would make a
block's order depend on iteration order — and a client that generated rows in a different order
would get a different building. A hash of its arguments depends on nothing else, so there is no
ordering to get wrong. (It is MurmurHash3's 32-bit finalizer, used for its mixing, not its
cryptography.)

Ordering within a row is an insertion sort by hash with the cell index as the tie-break, so two
cells that happen to hash equal still order identically everywhere.

## The gate is the reason this service exists

> A building under construction provides none of those finished-building capabilities.

That is not a presentation rule. `Is_Under_Construction(building_id)` is the query anything with a
capability asks, and it answers `false` for every building this service has never heard of — which
is every building a level placed rather than built. Phase 36's sidebar already asks it before
letting a facility produce.

Section 44's activation list (render visibility, collision, operational state, power grid, Purchase
Terminals, MCT, factory production, repair, defenses, communications, Silo, spawn anchors, base
controller) is deliberately **not** run by this service. Completions are *reported* through
`Get_Completed_Count`/`Get_Completed_Building`, and the caller runs the activation in one place, in
the order the roadmap lists. A callback per subsystem would scatter that order across whichever
subsystems happened to register a listener.

## Block generation

From the model's local bounds, not the footprint — a footprint has no height.

1. Cells and rows from `construction_visual`'s `block_width` / `block_depth` / `row_height`.
2. If the count exceeds `max_blocks`, **the blocks get bigger**, by a fixed 1.4× factor, until it
   fits. Never truncation: a truncated list would build a building with a missing corner, which is
   worse than a coarser one. The factor is fixed and the loop is bounded, so it stays deterministic.
3. The final cell size divides the bounds exactly, so blocks tile without a seam at the far edge.
   The JSON's nominal size is a proportion hint, honoured to within one cell.
4. Rows are generated foundation to roof; each row's blocks are ordered by the hash.
5. Everything is local to the building. The world transform is applied last, which is what makes
   *"rotated buildings must generate rotated visuals"* free rather than a second code path.

## The timeline

Section 44's default staging, with `source_reveal_start` and `block_fade_start` tunable per
descriptor:

```
0.00 - 0.10   foundation
0.10 - 0.80   block rows
0.75 - 1.00   source building reveal
0.90 - 1.00   block fade
```

Each row owns its own slice of `[0.10, 0.80]`. Within a row, the stagger fills the first half of the
slice and each block takes the other half to rise — so a row looks assembled rather than switched
on, and **no block can escape its row's slice**, which is what makes *"rows activate strictly
bottom-to-top"* true whatever the shuffle did.

`Get_Block_Render_State` derives rise, vertical scale and alpha from the same `t`, so they cannot
disagree: a block is never fully opaque while still half-sunk. The block fade at the end runs against
the whole building's progress rather than each block's own, so every block fades out together.

The current size is returned as a separate `HalfExtent` rather than baked into the transform as a
non-uniform scale: a batching renderer emits eight corners from a centre, an extent and an
orientation, and a `Matrix3D` carrying a scale would make every consumer undo it first.

## Progress is server time, never frames

`Update(server_time_seconds)` computes `(now - start) / duration`. A client that hitched for two
seconds arrives at exactly the same progress as one that did not — *"Server time/state owns
completion, never client frame rate"*, as arithmetic rather than as a promise. Calling `Update`
twice with the same time changes nothing, which the self check pins.

The descriptor's reveal and fade values are cached at `Begin_Construction`, not looked up per frame:
a live JSON reload must not make a half-built building jump. Section 44's own replication list
carries a *"descriptor/config revision if required"* for this reason — a construction runs on the
values it started with.

## Cancellation

`Cancel_Construction` drops the record, and the record is the only place any temporary state lives.
That is the whole of *"cancellation/destruction leaves no temporary state"* and *"no half-built
object may provide power/radar/production/defense/wallet capacity"* as far as this service is
concerned; the footprint blocker and the `GameObj` belong to the caller that created them.

`Clear_Completed` also drops the records behind the completions it consumes — a completed building is
a finished building, and `Is_Under_Construction` has to stop saying yes.

## What this phase does not build

- ~~No renderer, and therefore no batching.~~ **Built, a later pass**:
  `Code/Combat/constructionrenderer.{h,cpp}` keeps exactly **two** `DynamicMeshClass` objects per
  building under construction, whatever the block count -- Section 44's own "completed rows batch /
  current animated row batch". The settled batch holds every row below the one assembling and is
  rebuilt only when `Get_Active_Row` advances; the active batch is rebuilt each frame. During the
  final fade every block's alpha moves, so both rebuild together. Each batch is bounded at 512
  blocks and drops-and-counts past that, the same discipline `DebugRenderSystem`'s line pool uses.
  No block is ever a `RenderObjClass` and none is ever a `PhysClass`.

  Blocks are drawn unlit with the tint and per-vertex alpha the service supplies, and built from the
  block transform's own axes rather than world axes, so a rotated building's blocks are rotated too.
  A flat per-face shade makes a block read as a solid rather than a silhouette without needing a
  light.

- **Progress now advances on both sides.** `CombatManager::Think` calls
  `BuildingConstructionService::Update` on client and server alike, against
  `CommanderStateClass::Get_Estimated_Server_Time()` -- the server's clock as last replicated,
  advanced locally since. That extends "clients deterministically reconstruct blocks" to the
  timeline as well as the layout: a client that hitched still lands on the stage the server is at.
- **No footprint blocker.** Section 44 wants one simple authoritative construction-footprint collider
  while a building is going up, removed at completion. That is a `PhysClass` creation, which belongs
  with the caller that creates the real building — the same caller that runs the activation list.
- **No PCT/MCT spawning.** `BuildingModelDescriptor::Resolve_Anchor_World_Transform` already resolves
  a semantic anchor to a real world transform through the model's own `HTree` (Section 39), so the
  hard part exists; what is missing is the completion-time caller that spawns and binds the canonical
  Renegade behaviour at those transforms.
- **No `GameObj` creation at all.** Accepted placement creating a real canonical building in
  `CONSTRUCTING` state is the caller's step; this service owns the timing, the layout and the gate.

## Checks

`renegade -shaderselfcheck construction`, registered as ctest `shader_construction` and
`fds_shader_construction`. Device-less. Pins: the same seed producing a byte-identical layout twice;
a different seed reordering without changing the geometry; rows ordered bottom-to-top with no block
escaping its row's slice and no higher row starting before a lower one; `max_blocks` respected by
coarsening rather than truncation (the count fits *and* the blocks got bigger); `build_time_seconds`
winning over `construction_visual.duration_seconds`; progress driven purely by server time, including
a repeated `Update` at the same time changing nothing; the stage sequence and the reveal alpha; a
visible block landing near its building with a sane alpha and a non-zero height; completion reporting
exactly once and clearing the gate; cancellation leaving no blocks, no active count and no
completion; and a late joiner rebuilding the identical block count from the replicated state alone.

**Not covered by any automated check**: nothing here draws a block, so the batching itself and how
the reveal actually looks are unverified, as is the interaction with a real building's collision and
capability activation.
