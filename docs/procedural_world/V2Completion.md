# V2 Completion: Section 38 Definition of Done, item by item

V2 roadmap Phase 30, step 12. This is the final audit of the V2 roadmap's 67 Section 38 items.
Evidence is a named self check where one exists. The full suite runs with
`ctest --test-dir build/win32 -C Debug`, and every `terrain_`/`shader_` check has an `fds_`
dedicated-server twin.

**Legend**

| Mark | Meaning |
| --- | --- |
| **✔** | Implemented, and a self check asserts it. |
| **✔ code** | Implemented and inspectable, but nothing asserts it automatically. |
| **◐ live** | Implemented, but only a person in a running client can confirm the end-to-end behaviour. No game-client automation exists in this environment (see `docs/integration/Phase28IntegrationGate.md`). |
| **✘** | Not met. |

## World generation

| # | Requirement | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Same seed/profile/version reproduces the world | ✔ | Determinism asserts in `terrain_macro`, `terrain_hydrology`, `terrain_strategic`, `terrain_roadgraph`, `terrain_bridgeplan`, `terrain_tunnels`, `terrain_coastline`, `terrain_biomedecoration`; manifest/profile hashing in `shader_worldprofile`; handshake refusal in `terrain_worldsync`. `terrain_hydrology`'s determinism assert failed for a long time because it regenerated over its own carved terrain; fixed at V2 close. |
| 2 | World is finite | ✔ | `WorldBoundsService`; `shader_worldbounds` |
| 3 | Tall procedural cliffs continuously encircle the map | ✔ / ◐ live | `BoundaryCliffGenerator`; `shader_worldcliffs`. How it looks needs a client. |
| 4 | No traversable seams or gaps in the boundary | ✔ | `BoundaryCliffGenerator::Check_Seam_Continuity`, `Check_Inverted_Triangles`, `Check_Traversable_Ramps`; `shader_worldcliffs` |
| 5-8 | Infantry, ground vehicles, air vehicles and the Commander camera cannot leave | ✔ / ◐ live | `Closest_Valid_Point`, `Recover_Out_Of_Bounds_Actor`, `Is_Inside_Air_Volume` and `Clamp_Commander_Camera`, asserted in `shader_worldbounds`. Enforcement on live actors needs a client. |
| 9 | Roads cannot leave | ✔ | `RoadGraphGenerator::Check_No_Cliff_Band_Crossing`; `terrain_roadgraph` |
| 10 | Tunnels cannot leave | ✔ | `TunnelSystem::Breaches_Boundary`; `terrain_tunnels` (boundary refusal and `Check_No_Boundary_Breach`) |
| 11 | Bridges cannot leave | ✔ | `CROSSING_FAILURE_OUT_OF_BOUNDS`; `terrain_bridgeplan` |
| 12 | Runtime terrain mutation cannot breach the boundary | ✔ | `WorldOperationJournal`: the boundary band is never mutable; `terrain_worldops` |
| 13-15 | Hills, mountains/ridges, interior cliffs/ravines | ✔ | `terrain_macro`, `terrain_interior_cliffs` |
| 16-17 | Rivers/streams, lakes/ponds | ✔ | `terrain_hydrology` |
| 18 | Coastlines inside the fixed boundary | ✔ | Built at V2 close: `water.coastline` consumed, generator 7; `terrain_coastline`. Beach and shore dressing is art/material work on top (`Hydrology.md`). |
| 19 | Roads/highways generated | ✔ | `terrain_roadgraph` |
| 20 | Street lamps populate appropriate roads | ✔ | Built at V2 close: `ow_street_lamp`; `terrain_biomedecoration`. No model art yet and no night light (`BiomeDecoration.md`). |
| 21 | Bridges generated from crossing needs | ✔ | `terrain_bridgerequired`, `terrain_bridgefallback` |
| 22 | Tunnels/underpasses where useful | ✔ | Built at V2 close (Phase 10): `terrain_tunnels`. The settlement-underpass and strategic-alternate candidate sources produce nothing; see `RoadsBridgesTunnels.md`. |
| 23 | Forests/vegetation | ✔ | `terrain_biomedecoration` |
| 24 | Settlements/city/industrial regions | ✔ | `terrain_settlements` |
| 25 | Existing building models populate settlements | ✔ code | The Section 28A asset registry resolves real descriptors (`Settlements.md`). Connector/street-facing metadata is a disclosed Phase 13 gap. |
| 26 | Tiberium fields with Harvester access | ✔ | `terrain_tiberiumfields` |
| 27 | Base sites generated fairly | ✔ | `terrain_baseselection` |
| 28 | Generated world supports Commander construction | ✔ | `terrain_placementrules`, `terrain_grading`, `terrain_expansion` |

## FPS quality

| # | Requirement | Status | Evidence |
| --- | --- | --- | --- |
| 29 | Infantry has usable cover and routes | ✔ | Sightline/cover analysis in `terrain_combatspace` |
| 30 | Vehicle routes are viable | ✔ | `TransportationNetworkValidator` required base routes; `terrain_bridgerequired` |
| 31 | Meaningful alternate approaches where feasible | ✔ | Approach counts: `terrain_baseselection`, `terrain_combatspace` |
| 32 | Procedural terrain does not degrade FPS controls | ◐ live | Needs a person playing |
| 33 | Exposed infantry travel is bounded | ✔ | `max_mandatory_exposed_infantry_run_time_seconds`; `terrain_combatspace` |
| 34 | Human units keep priority over AI congestion | ✔ / ◐ live | Guard replan budgets in `shader_guardai`, which was failing for a long time on a unit placed past its own leash and was fixed at V2 close. Needs a live crowd. |
| 35 | Local FPS responsiveness over distant RTS simulation | ✔ code / ◐ live | Budget mechanisms catalogued in `PerformanceBudgets.md`; no measured baseline yet |

## Dynamic world

| # | Requirement | Status | Evidence |
| --- | --- | --- | --- |
| 36 | Runtime terrain operations are authoritative and deterministic | ✔ | `terrain_worldops` |
| 37 | Boundary cliff band is immutable | ✔ | `terrain_worldops`; `terrain_macro`'s per-vertex cliff-band check (corrected at V2 close from a per-chunk rule no generator promised) |
| 38 | Terrain, collision and nav dependencies rebuild correctly | ✔ / ◐ | Journal invalidation in `terrain_worldops`. Nav rebuild queueing was not found (`PerformanceBudgets.md` gap); traversability rebuilds on demand. |
| 39 | Bridge destruction updates navigation | ✔ code / ◐ live | `WorldTraversabilityGraph` reads live bridge state at build time; `terrain_bridges` covers damage state. A mid-match rebuild needs a client. |
| 40 | Construction grading works | ✔ | `terrain_grading` |
| 41 | Crater/deformation budgets are bounded | ✔ | Crater rate limit in `terrain_worldops`. Only craters are rate-limited (`PerformanceBudgets.md`). |
| 42 | Save/load preserves world mutations | ✔ / ◐ live | Snapshot and journal round trips in `terrain_worldsync`. A real save file needs a client. |
| 43 | Late join reconstructs the world | ✔ / ◐ live | `terrain_worldsync`; live run in the Phase 28 procedure |
| 44 | Checksum repair fixes a bad client region | ✔ | `terrain_worldsync` |
| 45 | Snapshots compact long operation history | ✔ | `terrain_worldsync` |

## AI Commander

| # | Requirement | Status | Evidence |
| --- | --- | --- | --- |
| 46 | Uses normal Commander APIs | ✔ | `shader_commanderai`; `docs/zerohour/CommanderAI.md` |
| 47 | No hidden-information cheat | ✔ | Shroud/intel firewall. `terrain_shroud` was failing because a sensor did not see its own cell on a coarse grid; `WorldShroudManager::Stamp_Circle` was fixed at V2 close. |
| 48-53 | Manages power; harvests Tiberium; expands its base; produces, defends and attacks | ✔ | `shader_commanderai`, `shader_powergrid`, `shader_production` and `shader_construction` all pass after V2-close fixture/timeline fixes |
| 54 | Replans after bridge/path changes | ✔ code / ◐ live | Planning interval re-reads the traversability graph; a live bridge break needs a client |
| 55 | Uses roads, tunnels and bridges intelligently | ✔ code | Traversability carries `TRAVERSE_EDGE_BRIDGE`/`ROAD`/`TUNNEL`; tunnels were added at V2 close |
| 56 | Obeys population/performance budgets | ✔ | Difficulty settings and planning interval in `shader_commanderai` |

## Advanced systems

| # | Requirement | Status | Evidence |
| --- | --- | --- | --- |
| 57 | Building subcomponents integrate with canonical building state | ✔ | `shader_subcomponents` |
| 58 | Logistics creates physical strategic objectives | ✔ | `terrain_logistics` |
| 59 | Advanced vehicle damage does not change default gameplay when disabled | ✔ | `terrain_vehiclesubsystems`; `shader_worldprofile` (off by default) |
| 60 | Toolchain can reproduce/debug a seed | ✔ | `shader_worldtools`; `procedural_world status`, `world_tools repro` |
| 61 | Legacy authored maps still work | ✔ code / ◐ live | No request means no generation and unconfigured bounds (`Architecture.md`). A live load of a stock map is Q-006 / the P00 runtime matrix. |
| 62 | Dedicated server has no client renderer dependency | ✔ | `FREEDEDICATEDSERVER` guard. `renegadeserver` builds and runs every `fds_` check headless. |
| 63 | Network state stays server-authoritative | ✔ | `NetworkingAndPersistence.md` wire/save matrix |
| 64 | No Jolt/replacement physics | ✔ | No reference anywhere in `Code/` |
| 65 | No duplicate V1/V2 runtime | ✔ | One owner per concern (`Architecture.md`); the tunnel exclusion placeholder was replaced, not kept alongside |
| 66 | Documentation is complete | ✔ | Section 33 crosswalk in `docs/RequiredDocumentationIndex.md` |
| 67 | Persistent state files correct and reconciled | ✔ | Reconciled at V2 close (`completed_features.md`, `WIP_features.md`) |

## What is not met, and what needs a person

No item is marked **✘**. What stands between this audit and "V2 verified end to end" is live
play:

- Every **◐ live** row above.
- The Phase 28 multiplayer, save-load and late-join procedure (`docs/integration/Phase28IntegrationGate.md`).
- The Phase 29 performance and soak measurement (`PerformanceBudgets.md`).
- Q-006 and Q-012 in `awaiting_answers.md`.

Every self check now passes. The two that failed at V2 close were fixed under D-001 priority 1
(`completed_features.md`):

- `shader_commandermode` now uses a team-taking entry that never dereferences its sentinel star.
- `shader_visualsettings` exposed a real bug: the anisotropy and LOD-bias setters asserted when
  called before a device existed.

## Regression baseline (after D-001 priorities 1-4)

- 199 ctest entries, all passing. This adds `terrain_worldsynctransport` and its `fds_` twin.
- At V2 close there were 197 entries, of which 4 failed; at the start of that stretch, 24 failed.
- Builds: `renegade`, `renegadeserver` and `leveledit`, with zero warnings.

## Baseline identity

- Profile schema 8, generator 7, handshake 1, snapshot 1, world-op format 1, campaign schema 1.
  See `NetworkingAndPersistence.md`.
- Tag: `v2.0-baseline`, on the commit that closes V2 Phases 10 and 30.
