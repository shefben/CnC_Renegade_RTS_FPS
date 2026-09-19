# Dedicated server rules for the Commander/RTS layer

Roadmap Section 47, Phase 41.

## The rule is enforced by one parameter, not a review

Section 47 draws a hard line: the server owns every gameplay-semantic RTS system, and
owns none of the client-only rendering. Rather than a per-subsystem checklist that has to
be re-audited by hand whenever a system is added, this project threads one boolean —
`render_available` — from the process entry point down through every Commander subsystem
that could otherwise allocate a device resource.

`Code/Commando/init.cpp` decides it once: `CombatManager::Init(ConsoleBox.Is_Exclusive()
? false : true)`. A dedicated server runs its console in exclusive mode, so this is the
same expression stock Renegade already used to gate every other renderer-owning subsystem
(`BackgroundMgrClass::Init`, `HUDClass::Init`) — the three Commander renderers added in
Phases 36-38 were wired onto that existing, already-trusted mechanism rather than
inventing a second one:

```cpp
// Code/Combat/combat.cpp, CombatManager::Pre_Load_Level
CommanderSidebarRenderer::Init( render_available );
CommanderGhostRenderer::Init( render_available );
BuildingConstructionRenderer::Init( render_available );
BackgroundMgrClass::Init (BackgroundScene, SoundEnvironment, render_available);
```

Each renderer's `Init` sets its `_Available` flag and returns immediately when it is
false, before any `Render2DClass`, `Render2DSentenceClass`, `DynamicMeshClass` or
`VertexMaterialClass` is allocated — verified directly against each of the three bodies
(`commandersidebarrenderer.cpp`, `commanderghostrenderer.cpp`,
`constructionrenderer.cpp`). None of the three has a static/global constructor that
touches a device outside `Init`. A dedicated server therefore allocates exactly nothing
for any of them, structurally, not by a caller remembering to skip a call.

`CommanderStateClass` (the replication object) allocates no `Render2DClass` or
`DynamicMeshClass` at all — it is pure network state — so it needs no `render_available`
gate; it runs identically on both sides, which is the point of a replicated object.

## Server includes / excludes, checked against Section 47's own list

| Section 47 says the server includes | Where it lives | Server-authoritative because |
| --- | --- | --- |
| Commander authorization/actions | `CommanderProductionService::Get_Team_Commander`, checked first in every `Validate` | client never supplies a verdict, only a request |
| building placement validation | `CommanderPlacementValidator::Validate_Server_Request` | same pure `Validate` the ghost uses, re-run server-side |
| authoritative construction state/timing | `BuildingConstructionService::Update`, driven by server time | client mirrors via `CommanderStateClass`, never decides |
| production queues | `CommanderProductionService` | no client code path reaches it directly |
| AI infantry/ground/air creation and orders | `cCommanderProductionEvent::Act`, `cCommanderOrderEvent::Act` | both `Act` bodies run only where `cNetwork::I_Am_Server()` |
| save/load/network state | `RTSPersistenceClass` | registered as an ordinary `SaveLoadSubSystemClass`, no renderer dependency |

| Section 47 says the server excludes | Confirmed absent from the server path because |
| --- | --- | --- |
| Commander camera rendering | camera render code is gated the same `render_available` way as the HUD (pre-existing, not part of this phase) |
| right-side GUI rendering | `CommanderSidebarRenderer::Init(false)` allocates nothing |
| ghost rendering | `CommanderGhostRenderer::Init(false)` allocates nothing |
| procedural construction GPU meshes | `BuildingConstructionRenderer::Init(false)` allocates nothing; `BuildingConstructionService` itself (the rules/timing half) has no device dependency at all and legitimately runs on both sides |

## What Phase 41 found, and did not need to change

The audit behind this document is a read, not a rewrite: every Commander subsystem added
in Phases 36-40 already followed the `render_available` contract `HUDClass` and
`BackgroundMgrClass` established, because that contract was written into each of their
`Init` signatures from the start (`docs/zerohour/CommanderSidebar.md`,
`CommanderPlacement.md`, `BuildingConstruction.md` all note "no-ops when initialised
without a device"). No code changed for this phase; this document records the audit that
confirmed it, per Section 47's acceptance line "no renderer/UI dependency is pulled into
server code merely to support Commander Mode."

## What this phase does not build

- No new server-only integration test. The existing `fds_shader_*` ctest twins
  (`docs/zerohour/TestCoverageCatalog.md`) already run every Commander self-check family
  under `renegadeserver`, device-less, which is the coverage this section's acceptance
  line calls for; a live listen-server-plus-client match (Section 46's own open item) is
  what would additionally prove the *content* is right, not just that nothing crashes
  without a device.
