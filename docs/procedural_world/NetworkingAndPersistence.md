# Networking and Persistence

Section 33 required document, and V2 Phase 30 steps 6 and 8: every version number the V2 runtime
stamps, its compatibility rule, and how each kind of world state crosses the wire and a save.
The detailed mechanisms are in the phase docs named below. This file is where the numbers and the
rules live together.

## Final version table (V2 baseline)

| Identity | Constant | Value | Where it is checked | Compatibility rule |
| --- | --- | --- | --- | --- |
| World profile schema | `ProceduralWorldProfile::CURRENT_SCHEMA_VERSION` | **8** | `Validate_Profile` | Any schema 1..8 loads. Each field it predates takes its documented default. 0 or >8 is refused. |
| Generator | `ProceduralWorldProfile::CURRENT_GENERATOR_VERSION` | **7** | `Validate_Profile`; `WorldSyncService` handshake | Must equal 7 exactly. Only one generator implementation is kept, so a profile naming another would silently build a different world. It is refused. |
| World-sync handshake | `WorldSyncHandshakeClass::CURRENT_HANDSHAKE_VERSION` | 1 | `WorldSyncService` | Exact match, server and client. |
| Chunk snapshot | `WorldChunkSnapshotClass::CURRENT_SNAPSHOT_VERSION` | 1 | snapshot read/apply | Exact match. A mismatched snapshot is refused rather than applied. |
| World operation record | `WorldOperationClass::CURRENT_FORMAT_VERSION` | 1 | journal read | Exact match. |
| Persistent campaign | `PersistentCampaignService::CURRENT_SCHEMA_VERSION` | 1 | `Deserialize` | Older schemas migrate forward through `Apply_Schema_Migrations`; newer are refused. |
| Building descriptors | per-file `schema_version` | 1 | `BuildingDescriptorManager` | V1 rule, unchanged. |

### Generator version history

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

The shipped profiles (`data/procedural_world/{small,default,large_supported}.json`) are at schema
8 / generator 7. Phase 29 found `default.json` still claiming schema 4 / generator 4 and refused
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
