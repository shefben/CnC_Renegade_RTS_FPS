# Save/load and networking for the RTS layer

Roadmap Section 46, Phase 40.

`Code/Combat/rtspersistence.{h,cpp}`, registered as a `SaveLoadSubSystemClass` under
`CHUNKID_RTS_PERSISTENCE`.

## One subsystem for four services

Not a convenience. Section 46's rules are almost entirely about what must **not** be written, and
several of them are cross-service:

- *"Do not serialize a second independent powered/unpowered truth."*
- *"Rebuild building-provided communications sensor registrations from live buildings/descriptors
  after load."*
- *"Do not save strategic camera position as if it were sensor coverage."*

A single owner is the only place those can be enforced as code rather than as four separate good
intentions.

## What is written, and why it is the authoritative form

| Service | Persisted | Why not the other thing |
|---|---|---|
| Power grid | manual disable, priority override, per member id | generation / requested / allocated / shed are **recomputed** — Section 46 says so, and it is also the only way the loaded state can be consistent with buildings that did not survive |
| Construction | the compact replication state Section 44 already defined, plus the model bounds | no block is written; blocks regenerate from `(bounds, seed)` — the same path a late joiner takes |
| Production | the queues, with elapsed time and ticket ids | — |
| Orders | durable Commander orders per unit id | not the selection set: a selection is a cursor, not gameplay state |

Deliberately absent: the placement ghost (*"transient and not saved"*), the strategic camera, and any
block vertex, mesh or render handle.

`Contains_Data` returns false when nothing is registered anywhere, so a match that never entered
Commander Mode writes no chunk at all — an ordinary FPS save file is what it was before this phase
existed. Similarly, a power-grid member with neither a manual disable nor a priority override writes
nothing, which is the overwhelmingly common case: a base full of untouched buildings costs zero
bytes.

## Progress is not saved, and that is the point

The construction chunk carries start time and duration, never progress. Progress is
`(server time - start) / duration`; writing it as well would be a second truth that could disagree
with the clock.

**The model bounds are written**, and that deserves a note, because Section 46 forbids serializing
renderer-only implementation details. The bounds are six floats derived from the *model*, not from
the renderer — and persisting them is what lets a load regenerate an identical layout without needing
the W3D resident at load time. They are a reconstruction input, in the same category as the seed.

## Rebuilding rather than restoring

`On_Post_Load` calls `Rebuild_Derived_State`, which is one call per team to
`TeamPowerGridService::RecalculateTeam`. That single call rebuilds generation, requested and
allocated demand, the load-shed set, every building's `Enable_Power` flag, and — because the
allocator owns them — the communications sensor registrations Section 46 separately requires to be
rebuilt from live buildings and descriptors. One call, because there is one allocator; a second
recomputation path here would be exactly the second independent truth the section forbids.

Team Silo wallet capacity needs no rebuild step either: `Get_Wallet_Capacity_Bonus` is computed on
demand from the same surviving members, so it is correct the instant that returns. *"Preserve
existing player balances even when they currently exceed a reduced reconstructed cap"* is free, since
nothing in this codebase clamps a balance yet — see **Q-014**.

`Rebuild_Derived_State` is public so a late joiner, or a descriptor reload, can ask for the same
reconstruction without pretending to have loaded a file.

## Restore bypasses validation on purpose

`CommanderProductionService::Restore_Item` skips every rule `Request_Production` enforces. A saved
queue was already validated when it was requested; re-validating it at load — against a credit
balance that has since been spent and a facility whose power state has not been recomputed yet —
would silently delete a legitimate queue. Section 46's own instruction is to persist the queues and
revalidate *navigation*, not purchases.

Items come back as `QUEUED` whatever they were. `PRODUCING`, `WAITING_FOR_POWER` and `BLOCKED_EXIT`
are all conclusions about a world that has not been rebuilt yet; the next `Update` re-reaches
whichever is true now.

The ticket counter is pushed past everything restored, so a purchase made after a load cannot be
handed a ticket id a queued item already owns.

An order record is a destination and a target id, never a path — so *"do not restore stale raw
navigation pointers, transient reservations, or invalid path handles"* holds because there is nothing
in the record that could be a stale handle. The AI replans from the order the first time it runs.

## Failures are reported, never fatal

A member id that no longer exists is not an error: the building it named may have been destroyed by
the same save's game-object state, and a manual override on a dead building is meaningless. A
construction whose descriptor no longer loads logs a `WWDEBUG_SAY` naming the building and the
descriptor — Section 46's *"report incompatible material changes clearly"* — and is counted in
`Get_Last_Load_Failure_Count` rather than aborting the load.

## Late join

Section 46: *"Late joiners receive compact construction state and regenerate the current stage."* A
joining client runs exactly the load path for construction — `Begin_Construction_From_State`, the
same function — rather than a second one that could drift from it. That is why the late-join counter
lives on this class and not on a network handler.

## Network authority

Section 46's server-owned list is satisfied structurally rather than by a permission check in this
file:

- **Commander permission** — `CommanderProductionService::Get_Team_Commander`, checked first in
  `Validate`.
- **Placement acceptance** — `CommanderPlacementValidator::Validate_Server_Request`.
- **Construction completion** — `BuildingConstructionService::Update`, driven by server time.
- **Power allocation and manual controls** — `TeamPowerGridService`, which contains no network code
  at all and whose `Enable_Power`-driving calls are gated behind `CombatManager::I_Am_Server()` at
  every call site in `BuildingGameObj`.
- **Production purchases/queues, AI orders** — this service, which no client code reaches.
- **Shroud/radar intelligence** — `WorldShroudManager`, Section 30.

## What this phase does not build

- **No network messages.** Nothing yet puts a placement request, a production request or a
  construction state on the wire. Directive 0 removes every compatibility constraint on that wire
  format, so this is an unconstrained addition — but it is not written, and until it is, the
  Commander layer is single-machine. This is the largest remaining gap and is recorded in
  `WIP_features.md`.
- **No round-trip through a real file.** `Save` and `Load` are written against the same
  `ChunkSaveClass`/`ChunkLoadClass` micro-chunk pattern `MapMgrClass` uses, but `-shaderselfcheck`
  cannot stand up a save file, so the byte-level round trip is unverified. Every rule the two bodies
  depend on is checked directly instead.
- **Shroud exploration state** is not persisted. Section 46 says to persist it *"only when the game
  mode requires it"*, and no mode here does.

## Checks

`renegade -shaderselfcheck rtssaveload`, registered as ctest `shader_rtssaveload` and
`fds_shader_rtssaveload`. Device-less. Pins: an empty RTS layer claiming no save data (so an ordinary
FPS save is untouched) and a populated one claiming it; the member enumeration and the two
persisted power facts reading back, with a member that has no override correctly reporting none; the
post-load recalculation producing a state consistent with the restored manual controls rather than
with whatever was there before; a restored production item keeping its ticket and elapsed time and
*resuming* rather than restarting; a post-load ticket not colliding with a restored one; and a
restored order reading back, with a unit id of zero refused.

**Not covered by any automated check**: nothing here writes or reads an actual chunk, because
`ChunkSaveClass` wants a file and `SaveLoadSystemClass` wants a running game. A real round trip needs
a live match.
