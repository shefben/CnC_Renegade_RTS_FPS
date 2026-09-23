# OpenW3D / Renegade Shared Project Rules

This file is the shared execution contract for all coding agents working on the OpenW3D / Renegade RTS/FPS project. Agent-specific files such as `CLAUDE.md` and `AGENTS.md` may refine tool usage and local workflow, but they must not contradict this file, the active roadmap, or explicit user decisions.

## 1. Authority and precedence

When two sources disagree, use this order:

1. current explicit user instruction or decision;
2. recorded architectural/user decision in `DECISIONS.md` or `docs/adr/` when present;
3. the active roadmap's product, gameplay, and architecture requirements;
4. this file for repository-wide workflow and anti-drift rules;
5. persistent project state files for current implementation status;
6. subsystem documentation under `docs/`;
7. source comments;
8. archived/superseded plans and historical donor material.

Category ownership matters:

- roadmaps own required product and architectural behavior;
- this file owns shared workflow and anti-drift behavior;
- `DECISIONS.md` / ADRs own resolved user decisions when those files exist;
- lifecycle state files own implementation status only;
- the repository source owns what currently exists;
- donor code is evidence/reference, never destination architecture.

Never allow a stale status note, source comment, archived plan, or donor implementation to override a current roadmap requirement or explicit user decision.

## 2. Roadmap activation and stage gating

The current staged roadmaps are:

- Stage V1: `openw3d_tt484_zerohour_commander_integration_roadmap_v1.1.0.md`
- Stage V2: `openw3d_commander_procedural_runtime_world_roadmap_v2.2.0_claude.md`

V1 is the prerequisite implementation stage. V2 work must not begin until the V1 final/hard gate required by the V2 roadmap is satisfied, unless the user explicitly changes that dependency.

If `PROJECT_MANIFEST.toml` exists, it becomes the machine-readable authority for which roadmap stage is active, but it may not override an explicit user decision or roadmap prerequisite. If it does not exist, infer the active stage from the roadmaps and persistent project state; do not invent a new stage.

Archived or superseded plans are reference-only. Never implement directly from an archived plan when an active roadmap covers the same area.

## 3. Mandatory read set before coding

At a fresh session, after context loss/compaction, or before resuming unfamiliar work, read:

1. this file;
2. the active roadmap;
3. any prerequisite roadmap needed to understand the active stage;
4. `completed_features.md`;
5. `WIP_features.md`;
6. `unstarted_features.md`;
7. `awaiting_answers.md`;
8. `DECISIONS.md` and relevant `docs/adr/` entries if present;
9. `docs/architecture/OWNERSHIP.md` and `docs/architecture/AUTHORITY_MATRIX.md` if present;
10. current Git status/HEAD.

Use repository state instead of chat/session memory whenever both are available. Internal todos and conversation summaries are temporary aids only.

## 4. Non-negotiable architecture

- TT 4.8.4 is integrated first and natively.
- Where TT patched, hooked, overrode, or replaced stock behavior, merge the TT result into the canonical OpenW3D owner and remove the superseded runtime path.
- Never keep duplicate stock-vs-TT implementations selected by flags, wrappers, compatibility switches, or parallel managers.
- Zero Hour donor means **TheSuperHackers `GeneralsGameCode` configured as `Core + GeneralsMD`**.
- Do not use TheSuperHackers `Generals/` as an implementation donor stage.
- EA `GeneralsMD` may be consulted only to validate historical shipped Zero Hour behavior.
- OpenW3D remains the destination architectural authority.
- Prefer extending/canonicalizing existing OpenW3D owners over importing a parallel SAGE subsystem.
- Keep OpenW3D/WWPhys. Do not introduce Jolt or another replacement physics engine.
- Preserve Renegade FPS world geometry, interiors, tunnels, free-form structures, and non-heightfield gameplay spaces.
- Do not reintroduce historical TT DLL-hook/proxy architecture or an unnecessary binary plugin compatibility layer.
- No network compatibility with stock Renegade, TT 4.8.4, or other clients is required. OpenW3D clients and servers only need to interoperate with each other, so wire protocol, packet layout, net class ids, and app packet types may change when the architecture benefits.
- Asset compatibility is separate: existing `.w3d`, models, maps, textures, `always.dat`, and other intended Renegade assets remain valid inputs unless the roadmap/user explicitly changes that requirement.

## 5. Canonical ownership and no duplicate authorities

Before creating any new manager, service, registry, controller, subsystem, or authoritative cache:

1. search the repository for the existing canonical owner;
2. inspect `docs/architecture/OWNERSHIP.md` if present;
3. extend the existing owner when practical;
4. create a new owner only when the active roadmap requires a genuinely new responsibility or the existing architecture cannot correctly own it.

If a new owner is necessary, document:

- canonical class/module;
- public integration boundary;
- server/client authority;
- persistence/network responsibility;
- donor/reference source if applicable;
- which competing/legacy path is removed or explicitly non-authoritative.

Do not create two authoritative implementations of the same state or behavior.

## 6. Mandatory current Commander/RTS scope

The V1 roadmap includes all of the following and they must not be collapsed into a generic "Commander Mode" task:

- FPS-first gameplay invariants and an embodied/vulnerable Commander soldier body;
- Commander intelligence firewall: strategic camera movement is not a sensor;
- finite JSON-driven team building power generation/consumption with deterministic load shedding;
- validated server-authoritative live building-JSON tuning/reload;
- Communications Center/Advanced Communications Center tactical coverage;
- Tiberium Silo team-wide player wallet-capacity bonuses;
- right-side two-row Commander sidebar with `BUILDINGS`, `GROUND_VEHICLES`, `AIR_VEHICLES`, and `INFANTRY` modes;
- power generation/consumption/overload bars;
- contextual production by clicking the physical ground factory, Air Pad/air factory, Barracks, or Hand of Nod;
- Commander purchases create normal Renegade AI infantry/vehicles/aircraft, not fake human players;
- Commander selection and server-authoritative orders for produced AI;
- deterministic batched procedural construction blocks before a placed building becomes operational;
- automatic PCT/MCT placement from W3D semantic nodes;
- human-player movement/right-of-way and FPS responsiveness take priority over distant Commander AI work.

Required dependency order:

```text
building descriptors
-> finite power/capabilities
-> embodied Commander camera
-> Commander sidebar
-> ghost placement
-> procedural construction
-> AI production/control
-> persistence/network/dedicated-server/regression
```

Do not skip ahead because a later feature looks easier or more interesting.

## 7. Definition of Ready

A substantive implementation item is ready when the agent can determine, from repository evidence and the roadmap, the following where applicable:

- stable work-item identity or an unambiguous roadmap item;
- prerequisite/dependency gate is satisfied;
- canonical source owner is identified;
- existing implementation has been searched before creating a new one;
- applicable TT behavior has been located/understood;
- applicable Zero Hour donor behavior has been located/understood;
- server/client authority is known;
- input/output and ownership contracts are known;
- persistence/network impact is known;
- deterministic behavior requirements are known;
- failure/fallback behavior is known;
- missing-asset behavior is known;
- applicable acceptance/hard gate is known.

Not every unknown is a blocker. A task is blocked only when continuing would require guessing externally visible behavior, user intent, authoritative ownership, or architecture that cannot be resolved from source/data/reference evidence.

## 8. Questions, decisions, and investigation order

Do not ask the user for information that can reasonably be discovered from the repository, asset data, runtime introspection, donor/reference source, generated manifests, existing docs, or targeted tests.

Use this resolution order:

```text
repository/source
-> generated content/definition manifests if available
-> runtime introspection/diagnostic command
-> canonical donor/reference material
-> existing project decision records
-> user question
```

`awaiting_answers.md` is for **unresolved user-blocking questions only**.

- Add a precise `Q-###` only when a genuine user/architecture decision blocks progress.
- State what work it blocks and the smallest decision required.
- Do not use questions as a substitute for source investigation.
- Once answered, remove it from the open-question section and record the answer in `DECISIONS.md` or an ADR when those files exist.
- If `DECISIONS.md` does not yet exist, keep a clearly marked resolved record in `awaiting_answers.md` only as a transitional fallback; do not treat resolved entries as open blockers.

Major, durable architecture decisions should use `docs/adr/ADR-###-name.md` when the ADR directory exists.

## 9. Stable work-item identifiers

Prefer immutable roadmap/work IDs such as:

```text
V1-P38-T004
V2-P10-T015
```

When stable IDs already exist, preserve them across backlog, WIP, completion records, docs, tests, TODOs, and commits.

Do not silently rename an existing work ID. If an item is split or merged, record the relationship so conservation is auditable.

If the current roadmap has not yet assigned IDs, do not invent a competing numbering scheme inside source code. Continue using the roadmap's existing phase/task labels until the roadmap/state files are normalized.

## 10. Persistent project state

The current lifecycle files are:

- `completed_features.md`
- `WIP_features.md`
- `unstarted_features.md`
- `awaiting_answers.md`

These are status, not design specifications.

### Write timing

Read them at session start. Keep implementation progress in working context and reconcile lifecycle files in one deliberate pass immediately before handing control back to the user.

The only normal mid-session exception is `awaiting_answers.md`: add a blocking `Q-###` as soon as the genuine blocker is established so it cannot be lost.

Do not continuously rewrite lifecycle files after every micro-step.

### Entry size

- `completed_features.md`: 1-2 sentences per completed item, naming what finished and concise evidence.
- `WIP_features.md`: 2-3 sentences max per in-flight item: current state and exact next action.
- `unstarted_features.md`: delete-only unless the user explicitly changes the format. Do not regenerate/reformat it.
- `awaiting_answers.md`: unresolved questions only, plus transitional resolved records only when no decision ledger exists.

Long findings, design explanations, logs, and rationale belong under `docs/`.

### Conservation rule

Nothing disappears silently.

1. Remove an item from `unstarted_features.md` only when an equivalent item is added to WIP or completed in the same reconciliation pass.
2. Remove an item from `WIP_features.md` only when an equivalent item is added to completed in the same pass.
3. If an item becomes impossible, superseded, or user-cancelled, record it in completed as `N/A:` with the reason rather than deleting it.
4. A combined WIP/completed entry must explicitly name every absorbed work item or stable ID.
5. At handoff, every tracked item must exist in exactly one lifecycle state.

If `project_state.yaml` is introduced later, it should become the single machine-readable lifecycle source and Markdown views should be generated from it rather than manually synchronized. Until that migration is explicitly performed, the existing three lifecycle files remain authoritative for status.

## 11. Definition of Done

Do not mark a substantive item complete merely because code was written.

Where applicable, completion requires:

- implementation exists in the canonical owner;
- no duplicate runtime/authority path was introduced;
- obsolete competing path is removed or explicitly non-authoritative;
- required schema/configuration/data contract exists;
- server/client authority is correct;
- persistence/network behavior is handled when applicable;
- deterministic requirements are satisfied when applicable;
- failure/fallback behavior is safe;
- missing assets fail gracefully rather than blocking engine code;
- directly affected target(s) build;
- directly relevant focused validation passes;
- the item's applicable acceptance/hard gate is satisfied;
- relevant docs, asset requirements, decision records, and state are reconciled;
- no unresolved TODO/FIXME created by the change exists without a roadmap/work-item reference.

## 12. Validation economy

Follow the active roadmap's validation section, including V1 Section 0B where applicable.

Normal mode:

```text
implement
-> build directly affected target(s)
-> run directly relevant cheap existing test(s), if useful
-> perform one focused smoke check when runtime behavior warrants it
-> if PASS and no escalation trigger exists: STOP VALIDATING AND KEEP IMPLEMENTING
```

A green targeted result is sufficient evidence to continue.

Do not automatically:

- build every client/server/tool target together;
- run full regression or every golden scenario;
- benchmark every change;
- create exhaustive tests for trivial wrappers/getters/forwarders;
- reread successful logs line-by-line;
- rerun unchanged passing suites for reassurance;
- start broad regression merely because the session is ending.

Treat roughly 10-15% of a healthy ordinary implementation session as a soft validation budget. Escalate only for concrete failures, cross-cutting/high-risk changes identified by the roadmap, serialization/network ownership changes, deterministic contract changes, or acceptance/hard gates.

If `docs/development/VALIDATION_MAP.md` or a machine-readable validation map exists, use it to select targets/checks rather than rediscovering them.

## 13. Determinism contract

Authoritative gameplay and procedural systems must be reproducible where the roadmap requires determinism.

- No wall-clock-derived randomness in authoritative gameplay/generation.
- No process-randomized seeds.
- No pointer/address-derived ordering.
- Do not depend on unordered-container iteration for authoritative results; stabilize ordering first.
- Use named/derived RNG streams rather than one shared mutable global RNG where subsystem isolation matters.
- Bound retry counts.
- Define deterministic fallback behavior after retries are exhausted.
- Persist generator/schema/version identity when reconstruction depends on it.
- Use explicit tolerance/quantization rules when authoritative floating-point comparisons require stability.
- A procedural generation failure should be reproducible from its seed/manifest/version/reproduction data.

Presentation-only randomness may be nondeterministic only when it cannot affect authority, collision, navigation, persistence, or gameplay-visible state.

## 14. Authority/data lifecycle

For every new important state type, determine:

- authoritative owner;
- server/client ownership;
- whether it is replicated;
- whether it is persisted;
- whether it is reconstructed deterministically;
- whether it is presentation-only.

Use `docs/architecture/AUTHORITY_MATRIX.md` when present and update it for new authoritative state categories.

Do not let presentation/client convenience code become gameplay authority by accident.

## 15. Units, geometry, and shared physical constants

Do not independently invent or scatter physical gameplay constants across subsystems.

Use or establish a canonical location for shared definitions such as:

- coordinate handedness and up axis;
- world distance units;
- angle/slope representation;
- vehicle clearance envelopes;
- Humvee/reference-vehicle width;
- lane/shoulder/road widths;
- bridge deck width and vertical clearance;
- maximum road grade;
- minimum turn radius;
- tunnel clearance;
- spawn clearance;
- building setbacks;
- roadside-prop clearance.

Roads, bridges, tunnels, navigation, spawning, placement, and procedural structures must consume the same definitions rather than maintaining near-duplicate constants.

## 16. Missing assets are not code blockers

Textures, models, sounds, and other art that do not exist yet never justify skipping engine/system implementation.

Write the system so it requests the intended asset and fails safely when the asset is absent: untextured geometry, an empty optional layer, a model slot that draws nothing, or another non-crashing behavior appropriate to the subsystem.

Never check in a fake stand-in asset merely to hide a missing-art requirement unless the user explicitly requests one.

Record required assets under `docs/assets/` using the repository's current asset-documentation format. Each requirement should capture:

- exact name passed to the asset manager;
- human-readable description;
- code location/consumer;
- behavior when absent;
- geometry/material constraints such as tiling, projection, aspect, UV layout, pivots, nodes, clearances, or collision requirements.

Invented asset names use the `ow_` prefix so they cannot shadow stock Renegade assets.

If a structured `assets_required.yaml` is later introduced, prefer making it the machine-readable source and generating indexes/docs from it rather than maintaining duplicated inventories manually.

## 17. No opportunistic side quests

While implementing a roadmap item, do not perform unrelated:

- refactoring;
- renaming;
- formatting sweeps;
- modernization;
- API redesign;
- dependency replacement;
- hierarchy cleanup;
- architecture cleanup;
- test-suite expansion.

Only perform such work when it is strictly necessary for the active item's acceptance gate or to fix a concrete defect blocking that item.

If useful cleanup is discovered but not required, record a separate roadmap/WIP candidate or documentation note and continue the current task.

## 18. Source-reading and context discipline

- Use targeted `rg`/symbol searches before opening large trees/files.
- Read the smallest source region needed to establish ownership/behavior.
- Do not repeatedly reread unchanged large files/logs without new evidence.
- Summarize large donor files internally and retain only symbols/behavior required by the active task.
- Do not recursively audit unrelated subsystems after a focused check passes.
- Do not turn implementation tasks into open-ended research or test-writing projects.
- When the roadmap gives a concrete architecture/precedence rule, follow it instead of generating alternatives unless a real conflict is discovered.

## 19. Content/preset discovery and runtime introspection

Do not guess `always.dat` preset names, definition names, model semantic nodes, animation identifiers, or other runtime content identifiers when they can be enumerated.

Prefer or create narrow developer tooling/console diagnostics that can export the relevant data, ideally in machine-readable form, such as:

```text
content_manifest.json
definition_manifest.json
```

Useful categories include:

- `BuildingGameObjDef` presets;
- other game-object presets;
- vehicle/soldier/weapon presets;
- W3D model names;
- semantic nodes/hooks;
- animation names;
- texture/material references.

A missing runtime data string should normally produce a targeted introspection task, not a guessed descriptor that merely loads and fails later.

## 20. Git discipline

Before declaring work complete or handing off:

- inspect `git status` and the relevant diff;
- preserve unrelated user changes;
- ensure generated/temp/debug files are not accidentally included;
- do not reset, rewrite, discard, or overwrite user work unless explicitly instructed;
- ensure roadmap state, lifecycle files, docs, and repository behavior agree.

## 21. Handoff reconciliation

Immediately before handing control back:

1. inspect relevant diff/status;
2. move finished work to `completed_features.md`;
3. leave partial work in `WIP_features.md` with the exact next action;
4. delete corresponding completed/started backlog lines from `unstarted_features.md` while obeying conservation;
5. update open `awaiting_answers.md` questions;
6. record resolved durable decisions in `DECISIONS.md` / ADR when available;
7. run the cheap project-state validator when available;
8. do not launch broad tests merely because handoff is occurring.

The final repository state must be sufficient for another agent to resume without relying on private chat context.
