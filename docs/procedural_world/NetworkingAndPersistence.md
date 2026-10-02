# Networking and Persistence

Section 33 required document, and V2 Phase 30 steps 6 and 8: every version number the V2 runtime
stamps, its compatibility rule, and how each kind of world state crosses the wire and a save.
The detailed mechanisms are in the phase docs named below. This file is where the numbers and the
rules live together.

## Final version table (V2 baseline)

| Identity | Constant | Value | Where it is checked | Compatibility rule |
| --- | --- | --- | --- | --- |
| World profile schema | `ProceduralWorldProfile::CURRENT_SCHEMA_VERSION` | **12** | `Validate_Profile` | Any schema 1..12 loads. Each field it predates takes its documented default. 0 or >12 is refused. |
| Generator | `ProceduralWorldProfile::CURRENT_GENERATOR_VERSION` | **14** | `Validate_Profile`; `WorldSyncService` handshake | Must equal 14 exactly. Generator 14 is the Q-024/Q-025 stabilization (D-019, D-020); 13 was the Q-022/Q-023 crossing ownership correction (D-017, D-018); 12 was Q-021's cliff talus range `1W..5.25W` (D-016); 11 was the hybrid generator identity; 10 realized terrain from the D-003 map plan. Only one generator implementation is kept, so a profile naming another would silently build a different world. It is refused. |
| World-sync handshake | `WorldSyncHandshakeClass::CURRENT_HANDSHAKE_VERSION` | 1 | `WorldSyncService` | Exact match, server and client. |
| Chunk snapshot | `WorldChunkSnapshotClass::CURRENT_SNAPSHOT_VERSION` | 1 | snapshot read/apply | Exact match. A mismatched snapshot is refused rather than applied. |
| World operation record | `WorldOperationClass::CURRENT_FORMAT_VERSION` | 1 | journal read | Exact match. |
| Persistent campaign | `PersistentCampaignService::CURRENT_SCHEMA_VERSION` | 1 | `Deserialize` | Older schemas migrate forward through `Apply_Schema_Migrations`; newer are refused. |
| Building descriptors | per-file `schema_version` | 1 | `BuildingDescriptorManager` | V1 rule, unchanged. |

### Generator version history

Generator 9 reserves a river crossing in native road plans and rejects generated worlds without a traversable bridge joined to both dry approaches. It also checks that repositioned bridges still cross an obstruction and includes bridge placement in the deterministic checksum. Schema remains 10.

Generator 8 changes native macro relief, slope relaxation, hydrology and road results for a given seed. Schema 10 adds `terrain.land_elevation_bias` and `fairness.min_base_to_base_travel_time_seconds`; older schemas load with zero defaults. A client and server must use the same generator version to reproduce a world.

As recorded in `proceduralworldprofile.h`'s own version comment:

| Version | What changed the output |
| --- | --- |
| 1 | Bounds only (Phase 1-2). |
| 2 | The boundary cliff ring (Phase 3). |
| 3 | Macro terrain (Phase 4). |
| 4 | Hydrology (Phase 6). |
| 5 | Tiberium fields (Phase 12). |
| 6 | The Commander's foundation grading (Phase 17). Schemas 7 and 8 (logistics, vehicle subsystems) added fields only and left the generator at 6. |
| **7** | V2 close. Adds tunnels (Phase 10: bores, ground and cliff portal cuts, road splits at tunnel mouths), the Section 9.4 coastline (`water.coastline` consumed for the first time) and Section 10.6 street lamps. The same seed now yields a different world than under 6, which is why the number moved. |
| **8** | Native terrain relief, slope relaxation, shoreline shaping, and the resulting transportation layout changed for the same seed. Schema 10 adds land elevation bias and a minimum opposing-base route time. |

The shipped profiles (`data/procedural_world/{small,default,large_supported,native}.json`) are at schema
10 / generator 9. Phase 29 found `default.json` still claiming schema 4 / generator 4 and refused
outright. That was corrected, and the validator now enforces the rule above instead of strict
schema equality, which had contradicted `proceduralworldprofile.h`'s own statement that older
schemas load.

## What crosses the wire

| State | Authority | Replicated how | Persisted how |
| --- | --- | --- | --- |
| Generated world (terrain, cliffs, water, roads, bridges, tunnels, bases, fields, settlements, decoration, nav) | Server | **Not sent.** Each client regenerates from `(seed, profile, generator, content manifest)`. The handshake refuses a mismatch in generator version, profile hash or content hash. | Not saved. Regenerated from the manifest. |
| Runtime terrain mutation (craters, grading, channels, restore) | Server (`WorldOperationJournal`) | Ordered journal operations | Journal plus compacted chunk snapshots (`WorldOperationJournal.md`, `SnapshotsAndLateJoin.md`) |
| Late join | Server | Handshake, then snapshot inventory, then snapshots, then the journal tail, then checksum verify | — |
| Drift / bad client region | Server | Regional repair by chunk checksum (`SnapshotsAndLateJoin.md`) | — |
| Bridge damage state | Server | `BridgeSystem` state block | RTS save |
| Buildings, power, Commander, production, AI orders | Server | V1 RTS netcode (`docs/zerohour/RTSPersistence.md`) | `CHUNKID_RTS_PERSISTENCE` |
| Commander governance, AI Commander | Server | Same V1 paths; the AI Commander issues V1 API calls only | Same |
| Campaign (territories, factions, battle results) | Server-side service above a match | Not replicated; a match is launched from it | `PersistentCampaignService::Save_To_File` (JSON, schema 1) |
| Presentation (tunnel lights, debug layers, previews) | Local | Never | Never |

Tunnels in particular have no runtime state to replicate or save. They are indestructible, and
their geometry, holes, cliff cuts, nav links and road splits are all regenerated identically.
`SelectedTunnelCandidateIds` in the manifest records which candidates became tunnels. See
`RoadsBridgesTunnels.md`.

## The transport (D-001 priority 3)

Until priority 3, the rows above described message formats that had never crossed a wire:

- A client connected to a procedural server had no way to learn that a world existed.
- The journal was opened only by the `worldop` console command.
- Saves did not record the world.

`WorldSyncTransport` (`Code/Combat/worldsynctransport.*`) now moves those messages over
Renegade's own reliable, ordered event channel. It uses one net event, `cWorldSyncEvent`
(`Code/Commando/worldsyncevent.*`), that carries one fragment of at most 384 bytes in either
direction. There is no second socket.

| Step | Direction | What happens |
| --- | --- | --- |
| Server generates the world | — | `Post_Load_Level` opens the server's journal on the world as generated (`On_World_Generated`) and sends every client the **identity**: seed, profile path and generator version. |
| Client finishes loading a level | client → server | A pure client never generates from its own request; one left over from an earlier skirmish is dropped. It sends an identity request. The server answers with the identity, or with "no world" for an ordinary map. |
| Client receives identity | — | `Request_World_Exact`: that exact seed, **one attempt, no retries**, so a retry cannot pick a different seed than the one the server kept. The client generates, opens a fresh journal, and sends its join request. |
| Server receives join request | server → client | The join package: handshake, chunk snapshots, the operations after them, and an end marker carrying the last operation id. The client is then on the live stream. |
| Client receives the package | — | `Begin_Join` (identity refusal), snapshots, operations, then `Finish_Join` (world checksum). On a mismatch the client sends a repair request naming the disagreeing chunks. Repair blocks are applied and re-verified. The client then reports its result, which the server logs. A failed client is taken off the live stream. |
| Server commits an operation | server → joined clients | The journal's commit listener sends the operation with its server operation id. A client applies ids strictly in order. A duplicate is ignored. A gap or a refused operation makes the client ask to join again instead of applying out of order. |
| Client connection breaks | — | The server takes it off the live stream (`Server_Broken_Connection_Handler`). |

### Saves

`ProceduralWorldPersistenceClass` (`CHUNKID_PROCEDURAL_WORLD_PERSISTENCE`) writes the world's
identity and the same join package a late joiner would get.

On load:

1. The save subsystem runs before `Post_Load_Level`, so it only calls `Request_World_Exact`.
2. After generation, the package is applied locally through the same join state machine. `Finish_Join` must reproduce the saved world checksum, or the restore is reported as failed.
3. The world is compacted, so later joiners receive snapshots rather than the whole history.

A save made under another generator version is not restored. It is logged, not guessed at.
A save with no generated world writes nothing, so ordinary saves are unchanged.

### A protocol bug this caught

`WorldSyncService::Build_Handshake` set `SnapshotRevision` to the current revision whenever no
snapshot set existed. A joining client adopted that revision and then refused the log's first
operation as stale, because it was based on revision 1. The journal now records its base revision
(`Get_Base_Revision`: 1 at `Begin`, the then-current revision at `Rebase`), and the handshake uses
it. The single-process Phase 19 check always compacted before building a handshake, so it never
reached this case. The new check does.

### Checked

`terrain_worldsynctransport` (and `fds_`) runs the protocol end to end with the network replaced
by a capturing sender:

- the identity round-trips, and a truncated one is refused;
- a join request yields handshake / operation / end, addressed only to the asker;
- a committed operation is broadcast to joined clients only;
- a three-fragment message arriving out of order and with a duplicate reassembles into exactly one;
- the regenerated "client" fed those fragments reaches the server's world checksum;
- a live operation after a gap is not applied, and the client rejoins;
- a save package restores the world by checksum, and a truncated one is refused.

### Still needs two processes

- the identity reaching a real second machine;
- a real late join;
- a live crater seen on both screens;
- repair after deliberately corrupting a client;
- save, quit, reload and compare.

All of these are part of the Phase 28 procedure. Note that live gameplay still submits no
operations of its own. The journal is fed by `worldop`, so the live test drives it from that
console command.

## Reproduction

A world, or a failed attempt, is named by
`generator_version=<n> seed=<n> profile=<hash> content_manifest=<hash>`.

- `world_tools repro` prints it for the active world.
- `procedural_world status` prints it on either binary.
- Generation failures log it.
- The campaign stamps one per launched battle.

Requesting the same seed with the same profile file under the same generator regenerates the
world exactly.

## Not verified without live clients

The mechanisms above are covered by self checks: handshake refusal, snapshot round trip,
journal ordering, regional repair and campaign serialization. What remains is the end-to-end
multiplayer and late-join run in `docs/integration/Phase28IntegrationGate.md`, which needs real
clients.
