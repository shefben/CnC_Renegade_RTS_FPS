# OpenW3D Command & Conquer: Renegade Expansion Roadmap
## Native Tiberian Technologies Scripts 4.8.4 Integration + Zero Hour SAGE Donor Systems + FPS-First Commander RTS Mode

**Roadmap version:** **1.1.0**  
**Revision date:** **2026-08-21**  
**Plan type:** coding-agent implementation roadmap  
**Primary target:** latest `w3dhub/OpenW3D` main branch  
**TT baseline:** Tiberian Technologies Scripts **4.8 Update 4, Revision 9000**  
**Sole SAGE implementation donor:** the Zero Hour build configuration of `TheSuperHackers/GeneralsGameCode`, consisting of shared `Core` + `GeneralsMD`  
**Historical validation reference:** EA `CnC_Generals_Zero_Hour/GeneralsMD` only; the EA `Generals/` tree is not part of the donor workflow  
**End state:** one integrated OpenW3D engine. No TT binary-patching runtime, no `scripts2.dll` dependency, no parallel legacy/new subsystem implementations, and no imported physics replacement.

This document **supersedes the previous Renegade/SAGE/TT transplant roadmap**. Treat this as a fresh project. Do not carry forward assumptions, code, collision-layer plans, physics plans, or architectural decisions from the old roadmap except for the Commander/RTS gameplay, building, UI, AI-production, and procedural-construction ideas explicitly retained and re-specified in this document.

---

# 0. NON-NEGOTIABLE PROJECT DIRECTIVES

The coding agent MUST obey all of the following.

1. **OpenW3D is the target codebase.**
   - Repository: `https://github.com/w3dhub/OpenW3D`
   - Do not implement against EA's original `CnC_Renegade` tree and later try to forward-port the result.
   - EA Renegade may be used only as a historical reference when resolving ancestry or behavior.

2. **Always start from the latest OpenW3D `main`.**
   - The repository state audited while this roadmap was written was:
     - SHA: `f6f9db43149d01367c8e0f9d0c30b0a5d7a4552d`
     - observed 2026-08-21
   - At implementation start, run `git fetch`, record the current `origin/main` SHA, and use the newer SHA if `main` has advanced.
   - If the target is newer than the audited SHA, re-run the source inventory in Phase 0 before modifying code.

3. **Tiberian Technologies 4.8.4 is integrated FIRST.**
   - No Zero Hour donor work.
   - No Commander Mode work.
   - No terrain framework work.
   - No renderer transplant.
   - No building JSON system.
   - until the TT-native integration gate passes.

4. **TT 4.8.4 must replace superseded OpenW3D/Renegade behavior in place.**
   - Do not reproduce the historical closed-binary architecture with hard-coded addresses, trampolines, byte patches, injected DLLs, proxy DLLs, executable-version tables, detours, wrapper subclasses, or parallel stock/TT implementations.
   - For every TT patch, hook, override, replacement, or behavior modification, identify the canonical OpenW3D implementation that TT was changing and update that implementation directly.
   - When TT supersedes stock behavior, the superseded stock implementation must no longer be compiled or reachable. There must not be a runtime selector, fallback path, compatibility branch, or `StockFoo`/`TTFoo` pair implementing the same behavior.
   - TT additions that have no stock predecessor become new native OpenW3D functionality.
   - Git history and provenance documentation are the archive for the old implementation; the shipping source tree does not need two live versions of it.

5. **There is one canonical implementation and one canonical built-in script catalog.**
   - The shipping game must not require or load `scripts.dll`, `scripts2.dll`, `tt.dll`, `shaders.dll`, or any TT engine-patch DLL for built-in functionality.
   - Existing stock scripts that TT does not replace remain native scripts.
   - TT-only scripts are added as native scripts.
   - If TT modifies or replaces a stock script, merge the TT behavior into the canonical script implementation and remove the superseded stock implementation from the build/registry.
   - Duplicate built-in script names or stock/TT variants of the same script are forbidden.
   - Do not provide a binary-plugin compatibility adapter as part of this roadmap. Binary compatibility, if ever wanted, is a separate future project and must not influence canonical engine architecture.

6. **Do not regress OpenW3D's renderer.**
   - OpenW3D already uses a Direct3D 9-backed renderer even though many source filenames and classes retain historical `DX8*` names.
   - Do not import TT's old D3D8-to-D3D9 translation DLL.
   - Do not introduce a second D3D8 path.
   - Zero Hour renderer code must be adapted to OpenW3D's existing D3D9-backed `ww3d2` implementation.

7. **Do not add Jolt or any other replacement physics engine.**
   - `wwphys` / OpenW3D's existing physics and collision system remains canonical.
   - New terrain, bridges, buildings, water-edge semantics, placement tests, and debris must integrate with the current OpenW3D physics/collision APIs.
   - Do not carry over any previous Jolt layer mappings, ragdoll plans, rope physics, vehicle migrations, or collision-group repurposing.

8. **Preserve OpenW3D portability.**
   - TT code originally written around Windows/x86 and binary hooking must be rewritten to fit OpenW3D's CMake/C++20 and client/server/tool targets.
   - Do not introduce unconditional Win32-only code into engine-core paths.
   - Renderer-specific code may remain D3D9-specific where OpenW3D already is.
   - Dedicated-server builds must not acquire renderer dependencies.

9. **Preserve Renegade as an FPS while adding RTS capabilities.**
   - Keep arbitrary static geometry.
   - Keep interiors.
   - Keep tunnels, caves, overhangs, elevators, vehicles, infantry, bullets, doors, and conventional W3D maps.
   - Heightfield terrain is an additional world representation, not a replacement for arbitrary W3D geometry.
   - Preserve canonical Renegade infantry movement, aiming, weapon handling, vehicle handling, repair, Purchase Terminal interaction, MCT/building combat, infiltration, and ordinary first-person HUD/gameplay unless this roadmap explicitly changes one.
   - Commander Mode must create additional physical objectives and tactical choices for FPS players rather than turning normal Renegade play into a secondary or degraded mode.
   - Section 3A defines the mandatory FPS-first gameplay invariants.

10. **Zero Hour is the sole SAGE donor configuration; do not stage work through the Generals game tree.**
    - The implementation donor is the **Zero Hour build configuration** of `TheSuperHackers/GeneralsGameCode`.
    - Treat that configuration as one codebase composed of shared `Core` plus Zero Hour-specific `GeneralsMD`.
    - The repository's top-level build adds `Core` for the shared engine and adds `GeneralsMD` when `RTS_BUILD_ZEROHOUR` is enabled. Code physically located in `Core` is therefore valid Zero Hour donor code even when it is shared with the Generals target.
    - **Do not use `Generals/` as an implementation donor, staging donor, predecessor donor, or required comparison pass.**
    - EA `GeneralsMD` may be used only to validate original shipped Zero Hour behavior when a community change needs historical confirmation. EA `Generals/` is outside this roadmap.
    - Do not import SAGE globals wholesale.
    - Do not import the SAGE `Thing` hierarchy as a replacement for Renegade `GameObj`.
    - Do not import SAGE's complete game loop, networking, command system, INI ownership model, campaign shell, DRM, GameSpy stack, or legacy DirectX 8 device architecture.
    - Port useful Zero Hour algorithms, fixes, renderers, data structures, systems, and architectural ideas into OpenW3D ownership.

11. **The 25 named Zero Hour/SAGE systems are a mandatory floor, and the complete Zero Hour capability audit is mandatory.**
    - They are enumerated in Section 3.
    - All twenty originally requested SAGE systems must be sourced from the current Zero Hour configuration (`Core` + `GeneralsMD`), not from the `Generals/` tree.
    - Five additional clearly reusable Zero Hour systems are also required.
    - Phase 7 must audit the full relevant Zero Hour build surface for additional engine-generic functionality, fixes, performance changes, renderer improvements, AI improvements, stability work, memory-safety work, and tooling improvements worth adapting to OpenW3D.
    - The 25 named systems are therefore **not a ceiling**. Any additional Zero Hour capability that satisfies the inclusion rules is required even if it is folded into an existing subsystem rather than given a new feature number.
    - Do not silently omit a donor improvement because another implementation seems "close enough".
    - If OpenW3D already contains equivalent or better behavior, document that in the capability matrix and extend/canonicalize the OpenW3D implementation instead of duplicating it.
    - Never stage through any predecessor game implementation merely to reach the Zero Hour implementation. Adapt directly from the selected Zero Hour code.

12. **The explicitly retained custom Commander/RTS project ideas are:**
    - Commander Mode strategic camera/view while preserving normal FPS mode.
    - The commander's real soldier remains physically present and vulnerable while using the strategic camera.
    - Commander Mode building placement/construction.
    - Ghost building model at the cursor, with invalid overlap rendered red.
    - Automatic placement of Purchase Terminals and MCTs from building-model nodes.
    - Building-model JSON/semantic parsing sufficient for the engine to understand the complete building hierarchy, parts, anchors, roles, transforms, power definitions, and selected strategic capabilities.
    - A persistent right-side Commander production sidebar using a two-row cell/grid presentation.
    - A default BUILDINGS sidebar mode containing buildable structures.
    - Context-sensitive GROUND_VEHICLES, AIR_VEHICLES, and INFANTRY sidebar modes selected by clicking an operational production building in the world.
    - Clicking a Weapons Factory/War Factory or faction-equivalent ground vehicle factory switches the sidebar to AI ground-vehicle purchasing from that selected facility.
    - Clicking a GDI or Nod Air Pad/air-production building switches the sidebar to AI air-vehicle purchasing from that selected facility.
    - Clicking a Barracks or Hand of Nod switches the sidebar to AI infantry/character purchasing from that selected facility.
    - Commander-purchased infantry and vehicles are normal server-authoritative Renegade AI GameObjs, not fake human players or scoreboard/player-slot entries.
    - Commander selection and orders for those AI infantry, ground vehicles, and air vehicles.
    - Team power generation/consumption bars and overload/offline feedback in the Commander UI.
    - Finite per-team building power capacity driven by values parsed from each building's JSON descriptor.
    - Procedural runtime construction blocks that assemble a newly placed building from the foundation upward before the real building becomes operational.
    - No other old project feature is implicitly in scope unless explicitly reintroduced later in this roadmap.

13. **Every destructive phase has an acceptance gate.**
    - Client must build.
    - Dedicated server must build.
    - Tools/editor targets must build when affected.
    - Required tests must pass.
    - Do not continue by accumulating broken phases and hoping the final linker experiences personal growth.

14. **Temporary migration adapters are permitted only during a phase.**
    - They must be removed or converted into intentional compatibility interfaces before that phase is accepted.
    - The final runtime must not contain user-selectable "old/new" feature switches for the requested replacements.

15. **Keep provenance and licensing explicit.**
    - Preserve relevant copyright/license headers.
    - Record every substantial TheSuperHackers Zero Hour (`Core`/`GeneralsMD`) donor file/symbol, any EA `GeneralsMD` validation reference used, and every TT source file used, including repository and pinned commit.
    - Add `docs/integration/THIRD_PARTY_PROVENANCE.md`.

16. **The four project-state files are mandatory persistent agent memory.**
    - The repository root must contain `completed_features.md`, `WIP_features.md`, `unstarted_features.md`, and `awaiting_answers.md`.
    - The agent MUST read all four files at the beginning of every work session and keep their current contents in active working context while executing this roadmap.
    - The agent MUST update them whenever feature state changes and MUST perform a final reconciliation before intentionally ending, handing off, or pausing a work session.
    - A work session is not complete until these files accurately reflect the repository and test state.
    - Section 0A defines the required lifecycle, formats, invariants, and end-of-session procedure.


17. **FPS-first Commander gameplay is a hard acceptance requirement.**
    - Commander intelligence is limited by legitimate team sensors/visibility and must never become omniscient merely because the strategic camera can see/render an area.
    - Human players receive physical movement/right-of-way priority over Commander AI where practical.
    - Local FPS input, camera, aiming, firing, nearby collision, and vehicle control have simulation priority over distant AI, telemetry, shroud updates, weather, and cosmetic strategic work.
    - Future procedural-world APIs must support infantry-scale cover/routes and vehicle-scale routes rather than optimizing exclusively for large RTS spaces.
    - Commander actions should create real objects and objectives that FPS players can attack, defend, repair, infiltrate, escort, or disrupt whenever a physical-world equivalent exists.

18. **Building power/capability augmentation must extend canonical Renegade behavior.**
    - Renegade/OpenW3D/TT already supports destroyed and powered-down buildings. Preserve those canonical states and the existing gameplay reactions to them.
    - Do not invent a second building operational-state machine.
    - Add one finite team power-grid allocator that determines which surviving power-dependent buildings receive power, then drive the canonical existing powered/unpowered state.
    - Power Plant output, building draw, and load-shedding priority are read from building JSON descriptors, not hard-coded C++ tables.
    - JSON power/capability values must support validated server-authoritative runtime reload so tuning can change without recompiling or restarting.
    - Communications Center/Advanced Communications Center strategic coverage and Tiberium Silo wallet-capacity bonuses are also configured through the building JSON.
    - Existing Refinery/Harvester, Weapons Factory/Airstrip, Repair Facility/Repair Bay, Obelisk/Guard Tower, Barracks/Hand of Nod, destroyed-building, and powered-down-building behavior is reused and augmented rather than reimplemented.


---

# 0A. MANDATORY PERSISTENT SESSION-STATE PROTOCOL

**This section is always active. It is not a phase that can be completed and forgotten.** The coding agent MUST treat it as persistent operating instructions for every implementation session, regardless of which roadmap phase or subsystem is being worked on. If the agent uses context compaction, handoff notes, resumable sessions, or any other memory mechanism, this section and the four state files MUST be retained or re-read immediately after context restoration.

The purpose is to make repository state externally auditable at all times. The user must be able to open four small Markdown files and determine what is finished, what is actively being worked on, what has not started, and what requires user input without reconstructing state from chat history, terminal scrollback, Git commits, or the agent's internal memory.

## 0A.1 Required files and location

Maintain these files at the **repository root** for the entire project:

```text
completed_features.md
WIP_features.md
unstarted_features.md
awaiting_answers.md
```

If any file does not exist at the beginning of a session, create it before substantive implementation work begins. These files are part of project state and should normally be committed to version control.

Do not rename them, replace them with an issue tracker, hide them under `docs/`, or consolidate them into one status document unless the user explicitly changes this requirement.

## 0A.2 Session-start protocol

Before modifying implementation code in **every** work session, the agent MUST:

1. Read this roadmap, especially Section 0 and Section 0A.
2. Read all four state files in full.
3. Inspect the current Git branch, HEAD, working tree, and relevant recent commits.
4. Reconcile obvious discrepancies between the state files and repository reality before starting new work.
5. Identify the exact feature/subfeature being resumed or started.
6. Move that item into `WIP_features.md` before or at the same time substantive implementation begins.
7. Keep open user questions/concerns from `awaiting_answers.md` in active context and do not silently invent answers for them.

The state files are the authoritative human-readable continuation record between sessions. Chat history may supplement them but must not be required to understand project status.

## 0A.3 Exactly-one-status invariant

Every roadmap feature or implementation unit that is tracked must exist in **exactly one** of these three lifecycle files:

```text
completed_features.md
WIP_features.md
unstarted_features.md
```

A feature must never simultaneously appear as completed and WIP, WIP and unstarted, or completed and unstarted.

When state changes, **move** the entry rather than copying it and leaving stale duplicates behind.

Use feature IDs or stable names that map back to this roadmap. Large roadmap features may be decomposed into independently trackable subfeatures when necessary, but the parent feature must make aggregate status clear. For example, a parent may remain WIP while some child tasks are completed.

## 0A.4 `completed_features.md` requirements

This file contains only work that is actually complete and has satisfied the applicable acceptance criteria. "Code was typed" is not completion.

Each completed entry MUST include, as applicable:

```markdown
## <feature ID/name>
- Status: COMPLETE
- Roadmap phase/section: <reference>
- Completed: <YYYY-MM-DD>
- Summary: <what was implemented>
- Canonical implementation: <main files/classes/systems changed>
- Donor/provenance: <TT / Zero Hour Core+GeneralsMD / native OpenW3D, if applicable>
- Tests/validation: <tests, build targets, runtime checks, results>
- Acceptance gate: PASS
- Relevant commit(s): <SHA(s), if committed>
- Notes: <compatibility, migration, or follow-up facts that remain relevant>
```

Rules:

- Do not mark an item complete while required tests, builds, parity checks, cleanup, documentation, or acceptance gates remain outstanding.
- If a previously completed feature is later found to be incomplete or regressed, move it back to `WIP_features.md` immediately and explain why.
- Completed entries should preserve enough evidence that a later agent can determine why the item was considered done.

## 0A.5 `WIP_features.md` requirements

This file is the live handoff record for every feature currently in progress. It must be detailed enough for a fresh coding-agent session to resume without guessing.

Each WIP entry MUST include:

```markdown
## <feature ID/name>
- Status: WIP
- Roadmap phase/section: <reference>
- Started: <YYYY-MM-DD>
- Last updated: <YYYY-MM-DD>
- Objective: <what this feature must accomplish>
- Completed within this feature:
  - <finished subtask>
- Currently implementing:
  - <precise current work>
- Remaining work:
  - <specific remaining task>
- Files/classes currently involved:
  - <path/symbol>
- Tests already run: <commands/results>
- Tests still required: <commands/gates>
- Blockers: <none, or concrete blocker>
- Awaiting user answer IDs: <none, or references into awaiting_answers.md>
- Next exact action: <the first thing the next session should do>
- Relevant commit(s): <SHA(s), if any>
```

Rules:

- Update the entry whenever the implementation direction, blocker, next action, or test state materially changes.
- Never leave vague handoff text such as "continue working on terrain" when the exact next function/file/task is known.
- If multiple features are genuinely active, keep separate entries. Do not hide parallel WIP under one giant "current work" blob.
- If work cannot continue because user input is required, the feature may remain WIP, but the corresponding question MUST also be in `awaiting_answers.md`.

## 0A.6 `unstarted_features.md` requirements

This file contains the remaining roadmap work that has not begun. It is the forward backlog, not a dumping ground for abandoned WIP.

Each entry SHOULD include:

```markdown
## <feature ID/name>
- Status: UNSTARTED
- Roadmap phase/section: <reference>
- Depends on: <feature/gate IDs or none>
- Scope: <short implementation summary>
- Start condition: <what must be true before work begins>
```

Rules:

- Initialize this file from the complete roadmap so every required feature is represented before implementation proceeds materially.
- Remove/move an entry to `WIP_features.md` as soon as substantive implementation starts.
- Newly discovered mandatory work from TT parity, the Zero Hour capability audit, regressions, or acceptance testing must be added here if it has not started yet.
- Do not quietly drop a feature because it became inconvenient. If it is intentionally excluded, record the exclusion and reason in the appropriate roadmap/capability documentation rather than pretending it never existed.

## 0A.7 `awaiting_answers.md` requirements

This file contains **only unresolved questions, decisions, concerns, ambiguities, or user-attention items** that materially need the user's input or awareness. It is the user's action queue.

Each entry MUST have a stable ID and enough context to answer without reopening an entire implementation session:

```markdown
## Q-###: <short title>
- Status: AWAITING USER
- Raised: <YYYY-MM-DD>
- Related feature(s): <feature IDs/names>
- Question/concern: <precise issue>
- Why it matters: <effect on implementation, compatibility, behavior, assets, etc.>
- Options considered:
  1. <option and consequence>
  2. <option and consequence>
- Agent recommendation: <recommended option, if one exists>
- Safe work that can continue meanwhile: <yes/no and what>
- Blocking: <yes/no>
```

Rules:

- Do not bury questions solely inside `WIP_features.md`; reference the stable `Q-###` entry there.
- Do not add trivial questions the agent can answer by inspecting source, tests, documentation, or the roadmap. Research first.
- Do not invent an answer merely to avoid asking the user when the decision genuinely belongs to the user.
- When the user answers or the concern is otherwise resolved, immediately remove the item from the active awaiting list or move it to a clearly labeled resolved-history section in the same file. The default active view must contain only items still needing attention.
- Record the resolved decision in the affected WIP/completed entry and in permanent project documentation when the decision affects architecture or behavior.

## 0A.8 Mandatory status transitions

Use these transitions:

```text
UNSTARTED -> WIP -> COMPLETE
                 -> UNSTARTED only if implementation is deliberately abandoned/reset and repository changes are reverted
COMPLETE  -> WIP if regression, missing scope, or failed acceptance is discovered
```

For any transition:

1. Update the destination file.
2. Remove the stale source entry.
3. Update related `awaiting_answers.md` references.
4. Preserve meaningful implementation/test evidence.
5. Ensure parent/child feature status still makes sense.

## 0A.9 Mandatory end-of-session reconciliation

**Before intentionally ending, pausing, handing off, or declaring any work session complete, the agent MUST perform this reconciliation even if no feature reached COMPLETE.**

1. Inspect `git status`, `git diff --stat`, and relevant changed files.
2. Review tests/builds run during the session and their final results.
3. Review the roadmap items touched during the session.
4. Update `completed_features.md` with every feature/subfeature that actually passed its acceptance gate.
5. Update `WIP_features.md` with everything partially implemented, including exact next actions and any failing tests.
6. Update `unstarted_features.md` for anything newly discovered but not begun, and remove anything that transitioned to WIP or COMPLETE.
7. Update `awaiting_answers.md` with every unresolved user question or concern, remove resolved items from the active queue, and cross-reference blocking WIP entries.
8. Verify the exactly-one-status invariant across the three lifecycle files.
9. Verify that the four files describe the **actual repository state**, not what the agent intended to finish.
10. Only after this reconciliation may the session be considered intentionally finished.

If the agent is forced to stop unexpectedly and later resumes, the **first task on resume** is to reconstruct and repair these four files from Git state, recent commits, tests, and any surviving session notes before new implementation continues.

## 0A.10 Session completion report

At the end of each session, the agent's user-facing summary MUST be derived from the four files and state at minimum:

- what moved to COMPLETE;
- what remains WIP and the next exact action;
- what significant new UNSTARTED work was discovered;
- which `Q-###` items in `awaiting_answers.md` need user attention;
- the final build/test state.

The chat summary is informational. **The four Markdown files are the persistent source of truth.**

## 0A.11 Automation/checking requirement

Early in the project, add a lightweight repository script or test, for example `tools/validate_feature_state.py`, that can be run locally and in CI to verify at least:

- all four required files exist;
- lifecycle entries use stable unique IDs/names;
- no tracked feature appears in more than one lifecycle file;
- all `Q-###` references from WIP resolve to an active or explicitly resolved question;
- required headings/fields are present;
- no lifecycle file is accidentally empty because of a failed generation/edit operation.

This validator must **not** infer whether implementation is truly complete; acceptance remains based on code/tests/gates. Its purpose is to catch bookkeeping contradictions before they cause incorrect handoffs or stale project status.

---

# 0B. MANDATORY VALIDATION-EFFICIENCY / TOKEN-BUDGET POLICY

**This section is always active and governs all phases unless the user explicitly requests deeper validation for a specific task.** The purpose is to preserve engineering confidence without allowing builds, tests, log inspection, benchmarking, or speculative validation work to consume a disproportionate amount of implementation time or model context.

The primary objective of normal work sessions is to **implement the roadmap**. Validation exists to provide enough evidence to continue safely, not to maximize the number of tests that can be imagined.

## 0B.1 Default validation mode: targeted and proportional

For ordinary implementation work, use the smallest validation set that provides reasonable evidence the affected code is sound:

1. Build **only the target or targets directly affected by the change**.
2. Run **existing, directly relevant, inexpensive tests** when they exist.
3. Perform **one focused smoke/runtime check** when runtime behavior materially changed and a cheap check is available.
4. Inspect only the concise success/failure result.
5. If those checks pass and no escalation trigger in Section 0B.3 applies, **STOP VALIDATING AND CONTINUE IMPLEMENTATION**.

A successful targeted check is not an invitation to keep searching for more ways to prove the same thing.

During normal development, DO NOT automatically:

- build the client, dedicated server, and tools when only one is affected;
- run the full unit/integration test suite;
- run all golden runtime scenarios;
- run unrelated subsystem tests;
- rerun an unchanged passing test solely for additional confidence;
- create exhaustive tests for straightforward code that is already covered adequately;
- benchmark ordinary changes;
- perform broad regression testing after every feature or subfeature;
- inspect or reproduce full successful build/test logs;
- repeatedly reopen successful output after PASS has already been established;
- spend substantial time proving invariants already enforced by the compiler, static types, existing tests, or a narrowly targeted runtime check.

## 0B.2 Validation effort budget

When there is no known failure and no high-risk escalation trigger:

- treat approximately **10-15% of normal session effort** as a soft upper budget for validation;
- prefer one targeted build/test invocation over multiple overlapping checks;
- prefer existing tests over inventing new tests;
- prefer a concise PASS/FAIL result over reading complete logs;
- stop validation once there is sufficient evidence to continue;
- if validation begins consuming more effort or context than implementation, pause the validation work and determine whether a concrete risk or failure actually justifies it.

The percentage is guidance, not a requirement to estimate wall-clock time precisely. Its purpose is to prevent validation from becoming the dominant activity in a healthy implementation session.

## 0B.3 Escalation triggers

Broaden validation beyond the default targeted mode **only** when at least one of the following is true:

```text
compilation fails
an existing directly relevant test fails
runtime crash/assert/exception occurs
observed behavior differs from expected behavior
save/load serialization changes
network protocol/replication/dirty-state behavior changes
ABI, binary layout, public TT API, or persistent data format changes
memory ownership/lifetime changes in shared low-level code
threading/concurrency/synchronization changes
shared renderer/physics/container/resource-manager infrastructure changes
a broadly used public interface changes
a bug is difficult to reproduce or has meaningful regression risk
a designated roadmap acceptance/hard-integration gate is being closed
the user explicitly requests deeper validation
```

When escalation is triggered:

1. Start with the **narrowest diagnostic check capable of explaining the problem**.
2. Expand scope only if the previous check does not isolate or resolve it.
3. Stop broadening once the failure is understood and the relevant fix has been validated.
4. Do not convert one failure into an excuse to run every unrelated suite.

## 0B.4 Implementation checks are not acceptance gates

A phase's acceptance criteria describe what must be true **before the phase is moved to COMPLETE**. They do not require every acceptance test to be rerun after every source edit.

During active implementation:

```text
IMPLEMENTATION CHECK
- affected target(s) compile;
- obvious directly affected behavior works;
- directly relevant cheap existing tests pass, when applicable.
```

When the phase or independently completable feature is genuinely ready to close:

```text
ACCEPTANCE GATE
- run the roadmap tests specifically required for that completed scope;
- run the applicable focused runtime scenario if required;
- perform the broader target build required by that gate;
- resolve failures;
- only then move the item to completed_features.md.
```

Several related subfeatures may be implemented and kept in `WIP_features.md` before paying the broader acceptance-gate cost once.

The `Tests still required` field in `WIP_features.md` means **tests/gates still needed before completion**, not tests that must be run immediately after every edit.

## 0B.5 Full regression is rare

Do not run the project's complete regression matrix during ordinary implementation.

Broad/full regression work is reserved for:

```text
Phase 6   TT compatibility / cleanup / hard gate
Phase 40  Full save/load + multiplayer pass
Phase 41  Dedicated-server pass
Phase 42  Tool/editor pass
Phase 43  Performance/regression pass
Final definition-of-done / release-quality gate
```

A concrete cross-cutting failure may justify an earlier broad run, but the reason must be recorded in the current WIP entry.

Phase-specific acceptance tests outside those gates are still allowed and required where the roadmap says so, but they must remain scoped to that phase rather than silently expanding into the entire repository regression suite.

## 0B.6 Test-creation policy

**Do not default to test-first development for this engine-integration roadmap.**

Implement first unless one of these conditions makes a test-first or test-alongside approach clearly valuable:

- fixing a reproducible bug where a regression test will prevent recurrence;
- implementing deterministic algorithmic behavior that is cheap and meaningful to unit-test;
- modifying serialization, networking, protocol, or compatibility contracts;
- modifying critical memory ownership, lifetime, or synchronization behavior;
- the roadmap explicitly identifies a specific test as necessary for the implementation;
- an existing failure needs a minimized reproduction.

Before creating a new test:

1. Search existing tests for equivalent coverage.
2. Prefer extending a focused existing test when appropriate.
3. Do not duplicate coverage for the same invariant.
4. Do not create tests for trivial getters, wrappers, forwarding functions, obvious compile-time relationships, or private implementation details unless they have demonstrated regression risk.
5. A test must protect a meaningful behavior or known risk, not merely increase test count.

## 0B.7 Build-target selection

During normal development:

```text
client-only renderer/UI change
    -> build the affected client/shared target only

server/gameplay change
    -> build the affected gameplay/server/shared target(s)

tool-only change
    -> build the affected tool only

shared public header/core-library change
    -> build the downstream targets reasonably affected by that interface

full client + server + tools build
    -> integration/acceptance gates, or when a concrete cross-target problem requires it
```

Do not build unaffected targets out of habit.

## 0B.8 Log and token discipline

For successful commands:

- inspect exit status and concise summary;
- capture the command and PASS result in project state when useful;
- do not read, summarize, or paste large successful logs.

For failed commands:

- inspect the relevant error region first;
- search/filter the log for the failure;
- expand context only as needed to diagnose the problem;
- do not consume model context reading thousands of unrelated successful lines.

When a command produces very large output, redirect it to a file and inspect targeted portions rather than loading the entire output into the model context.

## 0B.9 Performance measurement discipline

Performance measurement is required when:

- establishing the initial baseline;
- closing a performance-sensitive feature whose behavior could materially affect frame/server cost;
- diagnosing an observed performance regression;
- running Phase 43;
- running the final acceptance gate.

Do **not** benchmark every ordinary code change or every phase merely because a metric exists in Section 49.

## 0B.10 Validation reporting

Validation summaries should be terse:

```text
Validation:
- target: <name> -> PASS/FAIL
- focused tests: <names> -> PASS/FAIL
- smoke check: <name or not needed> -> PASS/FAIL
- escalation: none / <reason>
```

Do not spend user-facing or persistent-state tokens narrating routine successful compiler output.

---

# 1. AUDITED OPENW3D BASELINE

The coding agent must verify these facts against the actual selected OpenW3D SHA before implementation.

## 1.1 Build system

Current OpenW3D uses:

- CMake
- C++20
- separate client and headless-server targets
- optional tools
- shared/common interface library `wwcommon`

Important current options/targets:

```text
W3D_CLIENT
W3D_FDS
W3D_TOOLS

renegade
renegadeserver
```

Important source areas:

```text
Code/Combat
Code/Commando
Code/Scripts
Code/ww3d2
Code/wwphys
Code/wwsaveload
Code/wwnet
Code/wwui
Code/Tests
Code/Tools
```

## 1.2 Renderer baseline

OpenW3D's root CMake includes `dx9`.

On Windows it obtains a D3D9 SDK interface. On non-Windows it can use a DXVK D3D9 library.

Historical names such as:

```text
dx8wrapper.*
dx8renderer.*
dx8vertexbuffer.*
dx8indexbuffer.*
```

remain in `Code/ww3d2`, but the wrapper headers use D3D9 types.

**Rule:** preserve this existing renderer architecture and adapt donor code to it.

## 1.3 Current script architecture that must be replaced

At the audited baseline:

- `Code/Scripts/CMakeLists.txt` builds a separate shared `scripts` target.
- `Code/Combat/CMakeLists.txt` declares that the scripts library is dynamically loaded and adds a build dependency on it.
- `ScriptManager::Init()` calls `Load_Scripts(...)`.
- `ScriptManager::Load_Scripts()` loads `SCRIPTS.DLL` / debug variants through `SharedObject`, looks up create/destroy/setup entry points, and passes a `ScriptCommands` table across the DLL boundary.

This is the exact seam to replace with the native TT architecture described below.

---

# 2. AUTHORITATIVE SOURCE BASELINES

## 2.1 OpenW3D target

```text
https://github.com/w3dhub/OpenW3D
```

At implementation start create:

```text
docs/integration/OpenW3DBaseline.md
```

Record:

```text
OpenW3D SHA
build generator
compiler
architecture
enabled CMake options
vcpkg baseline
client build result
server build result
tool build result
baseline test result
```

## 2.2 TT Scripts 4.8.4

Official download page:

```text
https://www.tiberiantechnologies.org/Downloads
```

Required release:

```text
Version:  4.8 Update 4
Revision: 9000
Release:  2025-02-25
Source:   source-4.8.4.zip
MD5:      5bf9acce0663514ea5e84ff5e0c16fb1
```

Reference documentation:

```text
https://doxy.tiberiantechnologies.org/
```

The agent MUST verify the downloaded source hash before importing it.

Do not use an older TT branch because it is easier to compile.

## 2.3 Sole Zero Hour donor configuration

### Implementation donor: TheSuperHackers Zero Hour target (`GeneralsGameCode`)

```text
https://github.com/TheSuperHackers/GeneralsGameCode
```

The **only SAGE implementation donor** for this roadmap is the repository's Zero Hour build configuration.

Audited `main` while this roadmap was revised:

```text
SHA:      9fee97a1792efc50edfe27300581970b4c022951
Observed: 2026-08-21
```

At implementation start:

1. fetch the latest `main`;
2. record the exact SHA in `docs/zerohour/ZeroHourDonorBaseline.md`;
3. freeze that SHA for the donor audit and implementation phases;
4. build or inspect the repository with `RTS_BUILD_ZEROHOUR` enabled;
5. do not silently chase upstream commits mid-phase;
6. deliberately re-audit if a newer donor revision is adopted later.

### What "Zero Hour source" means in this roadmap

TheSuperHackers has intentionally factored code shared by both games into `Core`. Its top-level CMake structure is effectively:

```text
add_subdirectory(Core)

if(RTS_BUILD_ZEROHOUR)
    add_subdirectory(GeneralsMD)
endif()

if(RTS_BUILD_GENERALS)
    add_subdirectory(Generals)
endif()
```

Therefore, for this roadmap:

```text
ZERO_HOUR_DONOR = Core + GeneralsMD
```

Important Zero Hour donor areas include:

```text
Core/GameEngine/
Core/GameEngineDevice/
Core/Libraries/
Core/Tools/
GeneralsMD/
```

A subsystem located in `Core` is part of the Zero Hour donor configuration whenever the Zero Hour target depends on it, even if that subsystem is shared by both supported games.

The agent MUST search both `Core` and `GeneralsMD` before concluding that a Zero Hour implementation is absent.

### Explicitly out of scope as a donor

Do not use:

```text
TheSuperHackers/GeneralsGameCode/Generals/
EA CnC_Generals_Zero_Hour/Generals/
```

for implementation, staging, ancestry reconstruction, or a "port old version first" workflow.

The goal is **not**:

```text
predecessor-game implementation
    -> patch toward Zero Hour
    -> adapt to OpenW3D
```

The required workflow is:

```text
TheSuperHackers Zero Hour configuration (Core + GeneralsMD)
    -> understand selected current behavior
    -> adapt directly into canonical OpenW3D ownership
```

## 2.4 Historical Zero Hour validation reference

Repository:

```text
https://github.com/electronicarts/CnC_Generals_Zero_Hour
```

Official archived `main` audited for this roadmap:

```text
SHA: 0a05454d8574207440a5fb15241b98ad0b435590
```

Only the Zero Hour tree is relevant:

```text
GeneralsMD/
```

EA `GeneralsMD` is an **optional behavior-validation reference**, not the primary implementation donor. Use it when needed to answer questions such as:

- whether a TheSuperHackers change preserves or intentionally changes shipped Zero Hour semantics;
- whether a behavior is a community fix versus original Zero Hour behavior;
- whether a renamed/refactored shared `Core` implementation still corresponds to the Zero Hour system being ported.

Do not require an EA `Generals` comparison before accepting a feature.

## 2.5 Zero Hour donor-selection rule

For every SAGE-derived subsystem, use this order:

```text
1. TheSuperHackers Core + GeneralsMD at the pinned Zero Hour donor SHA
   -> sole implementation source
   -> includes current community fixes/refactors/improvements

2. EA GeneralsMD, only when historical behavior validation is useful
   -> shipped Zero Hour reference

3. OpenW3D
   -> destination and final architectural authority
```

Do not mechanically copy files. Determine the subsystem's responsibilities, dependencies, data flow, performance assumptions, rendering assumptions, and gameplay semantics, then adapt those responsibilities into the canonical OpenW3D architecture.

## 2.6 Community Zero Hour change policy

Changes present in the pinned TheSuperHackers Zero Hour configuration are candidates for OpenW3D when they provide reusable engine value in categories such as:

```text
NEW_ENGINE_CAPABILITY
ENGINE_ENHANCEMENT
BUG_FIX
RENDER_CORRECTNESS
RENDER_FEATURE
PERFORMANCE
MEMORY_SAFETY
STABILITY
PORTABILITY
AI_IMPROVEMENT
PATHFINDING_IMPROVEMENT
RESOURCE_MANAGEMENT
TOOLING_IMPROVEMENT
DATA_VALIDATION
MAINTAINABILITY_REFACTOR
DEBUG_OR_PROFILING_SUPPORT
```

Do not port merely because code exists when it is only:

```text
ZERO_HOUR_BALANCE_CHANGE
FACTION_OR_UNIT_CONTENT_CHANGE_WITH_NO_REUSABLE_PRIMITIVE
CAMPAIGN_OR_CHALLENGE_CONTENT
RETAIL_BINARY_COMPATIBILITY_HACK
DRM_OR_CD_CHECK
GAMESPY_OR_MATCHMAKING_SPECIFIC_CODE
LEGACY_DIRECTX8_DEVICE_GLUE
OLD_PROPRIETARY_SDK_GLUE
SAGE_GAME_LOOP_OR_THING_HIERARCHY
```

If a community change alters gameplay semantics rather than correcting or extending engine behavior, document why it is accepted or rejected.

## 2.7 Provenance rule

Every adapted Zero Hour implementation must record:

```text
TheSuperHackers repository + pinned SHA
Core or GeneralsMD donor path/symbol
feature/capability classification
TheSuperHackers fix/enhancement classification when known
EA GeneralsMD validation path/symbol, if used
OpenW3D destination path/symbol
adaptation notes
rejected donor-only behavior
```

---

# 3. REQUIRED ZERO HOUR / SAGE CAPABILITIES

The following **25 macro-systems are mandatory**, and **all are sourced directly from the Zero Hour donor configuration (`Core` + `GeneralsMD`)**. There is no predecessor-game implementation pass.

1. Asset exclusion / residency system
2. Sectional bridge renderer/system
3. Runtime road renderer/system
4. Heightmap / terrain framework using the current Zero Hour terrain architecture
5. Terrain texture system
6. Tree / foliage buffer
7. Reflective river / water system
8. Projected / cached shadows
9. Terrain tracks / surface ribbons
10. Particle batching
11. Dynamic-light filtering
12. Tracer / beam / projectile render modules
13. Debris rendering
14. Shader-manager architecture
15. RTS shroud / fog-of-war
16. Radar system
17. Waypoint / status-circle renderers
18. Modular draw-component architecture, including Zero Hour composite/dynamic attachment behavior
19. Spatial partitioning concepts
20. Debug rendering tools
21. Far/background terrain LOD system
22. Surface smudge / decal manager
23. Weather / environment particle layer
24. AI state-machine, pathfinding, guard/retaliation, and threat-response improvements
25. RTS telemetry / statistics instrumentation

## 3.1 Zero Hour implementation rule for all 25 systems

For every item above:

- search `Core` first for shared engine implementation;
- search `GeneralsMD` for Zero Hour-specific extensions, overrides, data bindings, and modules;
- treat `Core + GeneralsMD` together as the authoritative current Zero Hour implementation;
- adapt directly from that selected implementation into OpenW3D;
- do not reconstruct a predecessor-game version first;
- do not add a runtime donor-version selector;
- use EA `GeneralsMD` only when historical behavior validation is materially useful.

Examples of Zero Hour structures the agent must explicitly inspect include:

```text
BaseHeightMap / HeightMap / FlatHeightMap
W3DTerrainBackground
W3DSmudge
W3DSnow and related weather rendering
AIGuardRetaliate
AI / AIPathfind / AIStateMachine
AcademyStats instrumentation concepts
composite/dynamic draw-module attachment systems
```

## 3.2 The list is a floor, not a ceiling

The 25 items are the known high-value macro systems. Phase 7 must audit the **entire relevant Zero Hour build surface** and add every other reusable engine improvement that should reasonably benefit OpenW3D.

An additional Zero Hour capability is required when it is engine-generic and materially improves one or more of:

```text
rendering
terrain/world representation
resource lifetime/residency
culling/spatial queries
AI/pathfinding/threat response
particles/effects
water/weather/environment
roads/bridges/surface marks
RTS tactical visualization
debugging/profiling/tooling
memory use
performance
stability
correctness
portability
data validation
```

Small improvements should normally be folded into the owning macro-system rather than creating dozens of ceremonial feature numbers. Humanity has already invented enough numbering systems.

Zero Hour `Xfer`/snapshot serialization is **not** transplanted as a second save system. New OpenW3D objects still MUST participate in OpenW3D save/load and networking where appropriate.

---


# 3A. MANDATORY FPS-FIRST COMMANDER GAMEPLAY INVARIANTS

These invariants govern all Commander, AI, terrain, radar/shroud, building, power, networking, procedural-world, and performance work.

A technically working RTS feature that violates these invariants is not complete.

## 3A.1 Preserve Renegade first-person gameplay semantics

Unless this roadmap explicitly changes a mechanic, preserve canonical Renegade/OpenW3D/TT behavior for:

```text
infantry locomotion
first-person camera
aiming
weapon handling
projectiles/hitscan
C4/beacons
vehicle entry/exit
vehicle handling
repair
Purchase Terminals
MCT/building damage
building interiors/infiltration
ordinary FPS HUD
ordinary player-vs-player combat
```

Commander/RTS systems must use these mechanics rather than create separate RTS-only approximations.

A player who remains in FPS mode must still be playing recognizable Renegade.

## 3A.2 Commander intelligence firewall

The Commander camera is **not** a sensor.

Commander tactical information may come only from legitimate team information sources such as:

```text
friendly infantry/vehicle observation
friendly building/sensor coverage
powered Communications Center / Advanced Communications Center coverage
existing radar APIs
explicit scripted/objective visibility
legitimate RADAR_DETECTED state
legitimate LAST_KNOWN state
```

Rules:

- moving the strategic camera over an enemy area must not reveal live enemy objects;
- camera position must not mutate shroud state;
- `LAST_KNOWN` is stale information, not a live transform tracker;
- strategically hidden enemy state must not leak into FPS wallhacks, aim assistance, target boxes, or unauthorized overlays;
- use TT/OpenW3D per-client visibility/network semantics where practical to avoid needlessly sending tactically forbidden information;
- ordinary FPS rendering remains physical and is not blacked out because Commander shroud says an area is unexplored.

## 3A.3 Commander remains physically embodied

Commander Mode is a camera/input mode, not a second immortal player entity.

While commanding:

```text
the existing SoldierGameObj remains the player's body
the body remains server-authoritative
the body remains damageable
the body remains targetable/collidable
the body remains physically located in the world
normal body movement/fire input may be suppressed while strategic input is active
```

Death immediately exits Commander Mode.

Do not make the body invulnerable, hidden, non-collidable, or teleported into limbo merely because strategic camera control is active.

An optional `commander_station_anchor` may be exposed by the building semantic system, but no second player GameObj is created.

## 3A.4 Human players have physical priority over Commander AI

AI infantry and vehicles must not make the FPS game worse by clogging:

```text
factory exits
Barracks/Hand exits
air-pad launch/landing areas
doors
tunnels
repair zones
Purchase Terminals
MCT access
narrow base entrances
bridge chokepoints
spawn areas
critical infantry/vehicle routes
```

Requirements:

- AI yields or replans around human-controlled units where practical;
- human-driven vehicles receive right-of-way over AI vehicles where practical;
- produced units clear production exits before the next unit is released;
- AI population/production remains bounded by configurable team/map/server budgets;
- player-induced obstruction is covered by AI stuck detection/replanning;
- do not solve congestion by globally disabling meaningful collision.

## 3A.5 FPS-scale world contract for future procedural generation

The full procedural generator is outside this roadmap, but every world subsystem built here must support it later.

Generated worlds must be able to provide:

```text
infantry cover
short/medium infantry engagement spaces
longer vehicle engagement spaces
alternate infantry routes
flanking routes
tunnels/interiors where appropriate
elevation variation
chokepoints
vehicle lanes
bridge crossings
safe/unsafe traversal choices
meaningful destinations at human-scale intervals
```

Do not design terrain/road/bridge/water APIs around enormous RTS-only empty spaces.

Where a large vehicle corridor exists, the representation must permit parallel/alternate infantry-scale movement instead of forcing infantry into exposed multi-minute marches.

## 3A.6 FPS responsiveness has simulation priority

Latency/frame-time critical work includes:

```text
local input
first-person camera
local movement
aiming/firing
local/nearby collision
nearby combat feedback
vehicle control
```

If budgets are exceeded, reduce/stagger lower-priority work first:

```text
distant AI thinking frequency
distant path replanning
off-screen Commander overlay refresh
far strategic updates that tolerate batching
telemetry aggregation
weather particles
ambient effects
distant cosmetic debris
```

AI and strategic systems must support update-rate/budget LOD where appropriate.

Do not preserve maximum RTS simulation frequency at the expense of FPS responsiveness, first-person frame time, or server tick stability.

## 3A.7 Commander actions must generate physical FPS objectives

Commander Mode should shape **where and why** first-person battles happen.

Prefer physical-world mechanics:

```text
constructed buildings -> real damageable Renegade buildings
production -> real units emerging from physical facilities
repair -> physical Repair Facility/Repair Bay
power -> physical Power Plants and power-dependent buildings
radar/intelligence -> physical communications/sensor buildings
defense -> physical Guard Tower/AGT/Obelisk-type buildings
roads/bridges -> physical traversal routes
economy -> physical Refinery/Harvester/Tiberium infrastructure
AI orders -> units physically navigate the same world as players
```

Destroying, disabling, infiltrating, defending, escorting, or repairing these systems should create meaningful FPS consequences.

The Commander shapes the battlefield. Human players remain the primary high-skill combatants inside it.

---

# 4. TARGET ARCHITECTURE

The desired high-level result is:

```text
OpenW3D
|
+-- ww3d2
|   +-- existing D3D9-backed renderer
|   +-- canonical shader manager
|   +-- asset residency
|   +-- batched foliage
|   +-- shadows
|   +-- surface smudge/decal manager
|   +-- particle/effect buffers
|   +-- weather/environment particles
|   +-- composite/dynamic visual attachments
|   +-- debug drawing primitives
|
+-- wwphys
|   +-- existing OpenW3D physics/collision
|   +-- terrain/static/dynamic queries used by placement/world systems
|
+-- Combat
|   +-- native TT 4.8.4 gameplay/API behavior
|   +-- native script registry
|   +-- TT building/factory classes
|   +-- world terrain/roads/bridges/water integration
|   +-- near/far terrain LOD integration
|   +-- spatial world-query services
|   +-- native AI state/threat-response improvements
|   +-- RTS telemetry/statistics service
|   +-- building semantic descriptors
|   +-- server-authoritative building placement
|
+-- Commando
|   +-- TT client/server game integration
|   +-- Commander Mode camera/input/UI
|   +-- building placement user interface
|   +-- tactical radar/shroud presentation
|
+-- Tools
|   +-- TT-compatible definitions where required
|   +-- building descriptor validator/generator
|
+-- no built-in binary compatibility layer
    +-- canonical native OpenW3D/TT implementation only
```

---

# 5. DEFINITION OF "TT 4.8.4 FULLY INTEGRATED"

"TT integrated" does **not** mean "TT source compiles."

It means all engine-relevant behavior from the TT 4.8.4 source package has been classified and either:

```text
NATIVE_EXISTING
NATIVE_PORTED
NATIVE_REIMPLEMENTED
NATIVE_REPLACEMENT
NOT_APPLICABLE_WITH_DOCUMENTED_REASON
```

Create:

```text
docs/tt484/TTParityMatrix.md
```

Every engine-facing TT source file, public API surface, hook, game-object class, network extension, collision behavior, console feature, script registration unit, shader/HUD integration, and editor-facing definition must receive one of those classifications. `NATIVE_REPLACEMENT` means TT supersedes an existing OpenW3D implementation and only the merged/replaced canonical implementation may remain active or compiled.

**No `UNREVIEWED` rows may remain at the TT gate.**

## Canonical replacement rule

For each parity-matrix item, the agent must also record:

```text
stock owner/file/class/function
TT source/hook/override that changes it
final canonical owner/file/class/function
superseded implementation removed? yes/no
duplicate runtime path exists? must be no
```

A parity row cannot pass when both the old stock implementation and a TT replacement remain compiled/reachable. If TT only adds an event around otherwise-correct stock behavior, that is an addition rather than a replacement; the matrix must explain why no stock code is superseded.

Things that are merely installer/updater/closed-executable deployment machinery may be marked `NOT_APPLICABLE`, but their gameplay behavior cannot be discarded just because their original implementation was a patch.

---

# 6. PHASE 0 — FREEZE AND TEST LATEST OPENW3D

## Goal

Create a reproducible baseline before any TT code is imported.

## Tasks

- [ ] Fetch latest `w3dhub/OpenW3D/main`.
- [ ] Record exact SHA.
- [ ] Build Release client.
- [ ] Build Debug client.
- [ ] Build Release `renegadeserver`.
- [ ] Build with `W3D_TOOLS=ON`.
- [ ] Record CMake cache/options.
- [ ] Run stock Renegade content.
- [ ] Load at least one campaign map.
- [ ] Load at least one multiplayer map.
- [ ] Test:
  - [ ] player spawn
  - [ ] infantry movement
  - [ ] vehicle entry/exit
  - [ ] purchase terminal
  - [ ] building damage/destruction
  - [ ] script creation
  - [ ] map transition
  - [ ] save/load if supported in tested mode
  - [ ] listen/network play
  - [ ] dedicated server startup
- [ ] Capture baseline screenshots and performance counters.
- [ ] Dump the existing script names exported by stock `Code/Scripts`.

## Deliverables

```text
docs/integration/OpenW3DBaseline.md
docs/integration/BaselineKnownIssues.md
docs/integration/BaselineScriptCatalog.txt
```

## Acceptance

The selected OpenW3D SHA builds and runs before TT integration begins.

---

# 7. PHASE 1 — IMPORT AND INVENTORY TT 4.8.4

## Goal

Bring TT source into the repository as reference/import material without yet altering behavior.

## Tasks

- [ ] Download official `source-4.8.4.zip`.
- [ ] Verify MD5 exactly.
- [ ] Preserve the untouched archive outside generated build output.
- [ ] Extract source into a staging area such as:

```text
third_party/tt484_source_reference/
```

This staging copy is for provenance/comparison. Do not compile the historical solution directly as the final architecture.

- [ ] Generate a complete source-file inventory.
- [ ] Classify each file by role:

```text
custom gameplay script
stock-script compatibility
engine wrapper/API
binary hook/patch
network extension
render/shader integration
HUD/UI
building/factory class
physics/collision API
save/load
editor/LevelEdit
server-only
client-only
utility
installer/deployment
```

- [ ] Build `docs/tt484/TTParityMatrix.md`.
- [ ] Compare TT public class declarations with OpenW3D's real classes.

At minimum inspect/reconcile TT declarations and behavior around:

```text
BuildingGameObj
BuildingGameObjDef
SoldierGameObj
VehicleGameObj
AirFactoryGameObj
NavalFactoryGameObj
SoldierFactoryGameObj
ConstructionYardGameObj
SuperweaponGameObj
SamSiteGameObj
PhysicsSceneClass
GridCullSystemClass
AABTreeCull
CullSystemClass
RenderObjClass
SceneClass
NetworkObjectClass
ExtendedNetworkObject
HUDSurfaceClass
ScriptedDialogClass
DefinitionMgrClass
DynamicShadowManagerClass
```

- [ ] Identify every binary-address hook and locate the corresponding OpenW3D source call site.

## Acceptance

100% of TT source is inventoried and every engine-relevant unit has a planned native destination.

---

# 8. PHASE 2 — RECONCILE TT PUBLIC ENGINE CLASSES WITH OPENW3D

## Goal

Make OpenW3D's real classes provide TT 4.8.4 semantics instead of carrying parallel TT shadow declarations.

## Rules

Prefer this pattern:

```text
canonical OpenW3D class
        ^
        |
TT-facing headers/API use the same class
```

Avoid this:

```text
OpenW3D BuildingGameObj
TT BuildingGameObj clone
adapter
second adapter
reinterpret_cast
prayer
```

## Tasks

- [ ] Merge required TT members, virtuals, accessors, definitions, and semantics into canonical OpenW3D classes.
- [ ] Do not preserve obsolete class-layout ABI constraints merely for historical TT binary compatibility.
- [ ] Do not introduce external-plugin façade headers/adapters during this integration. Internal class design follows the canonical native OpenW3D implementation.
- [ ] Port TT building/factory object types into `Code/Combat`.
- [ ] Port TT object visibility/per-client state semantics.
- [ ] Port TT extended network behavior.
- [ ] Port TT collision-group meanings and train/naval/player-building behavior.
- [ ] Port current 4.8.4 changes, including the Revision 9000 additions identified from the source/changelog.
- [ ] Reconcile TT save/load extensions with `wwsaveload`.
- [ ] Reconcile TT definitions with `PARAM_EDITING_ON`/tool builds.

## Collision rule

The canonical collision API remains OpenW3D/WWPhys.

TT's public collision-group semantics must be preserved exactly where TT exposes them.

Do not allocate public TT collision IDs to unrelated new features.

## Acceptance

There is one canonical engine declaration for each core class, with TT 4.8.4 semantics available natively.

---

# 9. PHASE 3 — CONVERT TT BINARY HOOKS INTO NATIVE ENGINE DISPATCH

## Goal

Delete the conceptual need for TT's closed-executable patch architecture.

## Method

For every TT hook/patch/override:

1. identify the exact stock behavior TT was changing;
2. locate the canonical OpenW3D source function/class that owns that behavior;
3. merge the TT behavior directly into that canonical implementation;
4. add a native typed event/callback only when TT exposes a genuine extensibility/event API that must continue to exist;
5. delete the superseded stock branch and all address/trampoline/patch/override machinery;
6. verify there is exactly one reachable implementation of the resulting behavior.

## Required event families

At minimum audit and natively support TT hook behavior for:

```text
game/object creation
object destruction
level load/unload
player join/leave
connection acceptance
chat
radio
host/server lifecycle
purchase
refill
damage
kill
weapon events
dialog events
console output
shader/render notifications
game mode lifecycle
network visibility/dirty-bit updates
```

## Native event design

Create a typed event layer rather than a bag of raw addresses.

Example:

```cpp
class GameEventBus {
public:
    Event<PlayerJoinEvent> PlayerJoin;
    Event<PlayerLeaveEvent> PlayerLeave;
    Event<PurchaseRequestEvent> PurchaseRequest;
    Event<DamageEvent> Damage;
    Event<LevelLoadedEvent> LevelLoaded;
    ...
};
```

The exact implementation may differ, but it must be:

- deterministic;
- explicit;
- testable;
- safe on dedicated server;
- usable by built-in stock/TT scripts through direct native APIs.

## Acceptance

No required TT gameplay feature depends on modifying executable memory or knowing a hard-coded function address.

---

# 10. PHASE 4 — REPLACE DYNAMIC STOCK SCRIPT DLL LOADING WITH A NATIVE SCRIPT REGISTRY

## Goal

Eliminate the `SCRIPTS.DLL`/`scripts2.dll` bootstrap requirement for built-in scripts.

## Current OpenW3D seam

The audited code uses:

```text
ScriptManager::Init()
ScriptManager::Load_Scripts()
SharedObject
ScriptCreateFunct
ScriptDestroyFunct
Set_Script_Commands
```

Replace the default path.

## Required architecture

```text
ScriptManager
    |
    +-- NativeScriptRegistry
          +-- canonical stock scripts not superseded by TT
          +-- canonical stock scripts with TT changes merged in place
          +-- TT-only new scripts
```

There is no stock-vs-TT dispatch layer. A script name resolves to exactly one built-in implementation.

## Tasks

- [ ] Introduce `NativeScriptRegistry`.
- [ ] Give every built-in script a stable registration record:

```text
script name
factory
parameter schema
source/provenance
optional aliases
```

- [ ] Inventory every existing OpenW3D stock script against TT 4.8.4 before registration.
- [ ] Compile unchanged stock scripts only when TT does not supersede them.
- [ ] For stock scripts modified/replaced by TT, merge TT changes into the canonical source and compile/register only that resulting implementation.
- [ ] Compile TT-only new scripts as additional canonical scripts.
- [ ] Fail generation/build on duplicate built-in script names or multiple factories for the same script.
- [ ] Use an OBJECT library or explicit generated registration table so the linker cannot dead-strip unreferenced script translation units.
- [ ] Replace `ScriptManager::Create_Script()` with native registry lookup for built-in scripts.
- [ ] Replace built-in script destruction with native deletion/factory destruction.
- [ ] Keep save/load by script name and parameters compatible.
- [ ] Remove `add_dependencies(combat scripts)` once no longer required.
- [ ] Stop normal `ScriptManager::Init()` from `Load_Scripts("SCRIPTS.DLL")`.
- [ ] Ensure editor/tool builds use the same canonical script catalog.

## Remove the historical `ScriptCommands` DLL abstraction

Do not keep the `ScriptCommands` function-pointer table as an internal compatibility façade merely because the old DLL design used it.

During the TT integration, convert built-in script calls from:

```text
Commands->Create_Object(...)
Commands->Apply_Damage(...)
Commands->...
```

to direct typed OpenW3D APIs/services. Where a stable script-facing service boundary is genuinely useful, create a normal native C++ interface owned by OpenW3D, not a legacy DLL function table.

Rules:

- no dual direct-call/function-table implementations;
- no runtime switch between old and new script APIs;
- no built-in script dependency on exported DLL setup functions;
- no `Set_Script_Commands` bootstrap for built-in scripts;
- no retained facade solely to ease incremental migration at the final TT gate.

Temporary compile-break-fixing shims are acceptable only inside an unfinished commit/branch and must be gone before Phase 6 acceptance.

## Acceptance

With these files physically absent:

```text
scripts.dll
scripts2.dll
tt.dll
shaders.dll
```

the native OpenW3D build can:

- boot;
- load stock maps;
- instantiate stock scripts;
- instantiate TT scripts;
- run a dedicated server.

---

# 11. PHASE 5 — PORT ALL TT 4.8.4 ENGINE-FACING FEATURES NATIVELY

## Goal

Reach TT behavior parity, not just script registration parity.

## Required categories

### 11.1 Game-object/building extensions

Port all TT-supported object types and definitions required by 4.8.4, including applicable:

```text
AirFactoryGameObj
NavalFactoryGameObj
SoldierFactoryGameObj
ConstructionYardGameObj
SuperweaponGameObj
SamSiteGameObj
```

Integrate them with:

- `DefinitionMgrClass`
- object creation
- save/load
- networking
- building lists/base controller
- damage/destruction
- radar/HUD
- tools/editor definitions

### 11.2 Network extensions

Port:

- per-client dirty-state behavior;
- object visibility controls;
- TT network event extensions;
- connection acceptance features;
- server/client scriptable dialog behavior;
- reload/force-reload behavior where supplied by TT;
- GameSpy/broadcaster changes that remain relevant to OpenW3D.

### 11.3 HUD/radar/dialog APIs

Port TT API behavior into actual OpenW3D UI systems.

Do not create a second UI stack solely to mimic old DLL behavior.

### 11.4 Input/controller additions

Integrate TT controller-related behavior using OpenW3D's current input architecture.

Do not reintroduce obsolete DirectInput assumptions where OpenW3D has already abstracted them.

### 11.5 Collision and physics-facing APIs

Preserve TT semantics using WWPhys/OpenW3D collision.

### 11.6 Script library

Compile and register the complete TT 4.8.4 script catalog that is part of the source release.

Do not cherry-pick only JFW/JMG/dp88 scripts used by current test maps.

### 11.7 TT debug/console/admin facilities

Port engine-relevant console commands and debugging/admin functions that exist in the 4.8.4 source and are not obsolete deployment code.

## Acceptance

The TT parity matrix has no `UNREVIEWED` or unexplained `MISSING` entries.

---

# 12. PHASE 6 — TT COMPATIBILITY, CLEANUP, AND HARD GATE

## Goal

Finish TT before touching Zero Hour donor integration.

## No external binary compatibility layer in this project

Do not implement loading or ABI adaptation for historical prebuilt `scripts.dll`/`scripts2.dll`/TT plugin binaries as part of this roadmap. The purpose of this project is a single canonical native implementation, not binary compatibility with the old patch stack.

If third-party binary compatibility is desired later, treat it as a separate project after this roadmap is complete. It must consume public native APIs without changing, duplicating, or constraining the canonical implementation.

## Required tests

- [ ] stock script catalog test
- [ ] TT script catalog test
- [ ] duplicate script-name collision test
- [ ] save/load script recreation
- [ ] TT building/factory object creation
- [ ] TT per-client visibility
- [ ] TT purchase/refill hooks
- [ ] TT damage/kill hooks
- [ ] TT chat/player hooks
- [ ] TT dialogs
- [ ] TT collision groups
- [ ] controller behavior where supported
- [ ] client/server map transition
- [ ] dedicated server
- [ ] clean startup with no scripts-related DLLs
- [ ] tool/editor build

## Hard gate

**Do not begin Zero Hour integration until this phase passes.**

---

# 13. PHASE 7 — FREEZE ZERO HOUR DONOR + BUILD THE COMPLETE CAPABILITY / PORT MATRIX

This phase is mandatory before implementing any SAGE-derived feature.

Create:

```text
docs/zerohour/ZeroHourDonorBaseline.md
docs/zerohour/ZeroHourCapabilityMatrix.md
docs/zerohour/ZeroHourCommunityEnhancementAudit.md
docs/zerohour/OpenW3DPortMatrix.md
```

## 13.1 Freeze the Zero Hour donor revision

Record exact SHAs for:

```text
TheSuperHackers/GeneralsGameCode main
EA electronicarts/CnC_Generals_Zero_Hour main, only as an archival Zero Hour validation reference
OpenW3D target main
```

The implementation donor is always:

```text
TheSuperHackers Zero Hour configuration = Core + GeneralsMD
```

Do not add the `Generals/` tree to the donor inventory.

## 13.2 Inventory the complete Zero Hour build surface

Audit the source that participates in or supports the Zero Hour target, with emphasis on:

```text
Core/GameEngine/Include/Common
Core/GameEngine/Include/GameClient
Core/GameEngine/Include/GameLogic
Core/GameEngine/Source/Common
Core/GameEngine/Source/GameClient
Core/GameEngine/Source/GameLogic
Core/GameEngineDevice/Include
Core/GameEngineDevice/Source
Core/Libraries
Core/Tools
GeneralsMD/Code/GameEngine/Include
GeneralsMD/Code/GameEngine/Source
GeneralsMD/Code/GameEngineDevice/Include
GeneralsMD/Code/GameEngineDevice/Source
GeneralsMD/Code/Libraries
GeneralsMD/Code/Tools
```

Exact paths may differ at the pinned donor SHA. Follow the actual CMake target graph and repository layout rather than assuming an old EA directory layout.

For every reusable subsystem, meaningful engine feature, significant enhancement, or relevant fix encountered, classify it as one of:

```text
PORT_NEW_ENGINE_CAPABILITY
PORT_ENGINE_ENHANCEMENT
PORT_RENDER_FEATURE
PORT_RENDER_CORRECTNESS
PORT_AI_IMPROVEMENT
PORT_PATHFINDING_IMPROVEMENT
PORT_RESOURCE_MANAGEMENT
PORT_PERFORMANCE
PORT_MEMORY_SAFETY
PORT_STABILITY
PORT_PORTABILITY_IDEA
PORT_TOOLING
PORT_DATA_VALIDATION
PORT_DEBUG_PROFILING
PORT_REFACTOR_IDEA
OPENW3D_ALREADY_EQUIVALENT
OPENW3D_ALREADY_BETTER_EXTEND_IF_NEEDED
CONTENT_ONLY_REJECT
BALANCE_ONLY_REJECT
CAMPAIGN_CHALLENGE_REJECT
RETAIL_INFRASTRUCTURE_REJECT
SAGE_ARCHITECTURE_REJECT
NOT_RELEVANT_TO_OPENW3D
```

No relevant Zero Hour engine area may remain `UNREVIEWED` when Phase 7 passes.

## 13.3 Explicitly cover the known macro systems

The capability matrix MUST explicitly map all 25 systems from Section 3, including at least:

```text
asset/prototype residency and exclusion
sectional bridges
runtime roads
BaseHeightMap / HeightMap / FlatHeightMap terrain responsibilities
terrain materials/textures
foliage/tree buffering
water/river rendering
projected/cached shadows
terrain tracks/ribbons
particle batching
light filtering
tracer/beam/projectile visuals
debris rendering
shader management
shroud/fog-of-war
radar
waypoint/status-circle rendering
modular draw components and composite attachments
spatial partition/query systems
debug rendering
W3DTerrainBackground-style far terrain
W3DSmudge-style persistent surface marks
W3DSnow/weather-environment particle architecture
AI / AIPathfind / AIStateMachine / AIGuardRetaliate behavior
AcademyStats-style instrumentation concepts
```

## 13.4 Audit TheSuperHackers-specific Zero Hour improvements

Do not stop at reconstructing the EA 2003-era feature set. The pinned TheSuperHackers Zero Hour code contains years of community maintenance. Inspect current implementation and relevant source history for selected subsystems to identify useful:

```text
bug fixes
performance improvements
memory-safety corrections
renderer correctness fixes
stability fixes
cross-platform improvements
resource lifetime fixes
AI/pathfinding corrections
tooling improvements
validation improvements
useful modern refactors
profiling/debugging additions
```

Record accepted work in `ZeroHourCommunityEnhancementAudit.md`.

A community change becomes part of the same canonical OpenW3D implementation. Do not create an "EA behavior" mode and a "TheSuperHackers behavior" mode.

## 13.5 Optional historical validation against EA Zero Hour

When a current community implementation has materially changed semantics and the desired behavior is unclear, compare it against:

```text
EA CnC_Generals_Zero_Hour/GeneralsMD
```

Use that comparison only to understand original Zero Hour semantics. It is **not** a prerequisite for every port and does not reintroduce the `Generals/` tree into the workflow.

## 13.6 Build the final OpenW3D port matrix

For every required or newly accepted Zero Hour capability record:

```text
Feature/capability ID
TheSuperHackers Core or GeneralsMD source files/symbols + pinned SHA
whether source is shared Core or Zero Hour-specific GeneralsMD
community fix/enhancement classification
EA GeneralsMD validation source, if actually consulted
OpenW3D current equivalent
OpenW3D target module
TT dependency
renderer dependency
physics dependency
AI dependency
network dependency
save/load dependency
client/server applicability
migration class
rejected donor subparts + reason
tests
```

Migration class must be one of:

```text
DIRECT_ADAPT
ADAPTED_BACKPORT
ALGORITHM_ONLY
ARCHITECTURE_ONLY
BUGFIX_ONLY
PERFORMANCE_IDEA_ONLY
ALREADY_BETTER_EXTEND
REJECTED_SUBPART
```

## 13.7 Inclusion rule

Port Zero Hour functionality when it is reusable engine functionality involving any of:

```text
world/terrain representation
rendering correctness/features/batching
resource/asset lifetime
spatial queries/culling
AI state/path/threat handling
tactical display primitives
surface effects/weather
tooling/validation/debugging
performance/memory/stability
```

Reject direct import of:

```text
faction-specific unit behavior with no reusable engine primitive
campaign/challenge shell logic
balance-only changes
INI/content-only additions
Zero Hour mission-specific scripting content
SAGE Thing/game-loop ownership architecture
SAGE networking as a replacement for OpenW3D networking
DRM/CD checks
GameSpy/matchmaking-specific infrastructure
legacy D3D8 device/proxy architecture
proprietary-SDK glue that OpenW3D already replaced
```

Do not copy complete GeneralsGameCode directories. Adapt selected Zero Hour semantics into canonical OpenW3D ownership.

## Acceptance gate

- [ ] TheSuperHackers Zero Hour donor SHA frozen and documented
- [ ] `Core + GeneralsMD` explicitly documented as the sole SAGE implementation donor
- [ ] `Generals/` excluded from the implementation workflow
- [ ] all relevant Zero Hour engine areas classified
- [ ] all 25 known macro-systems mapped to actual `Core`/`GeneralsMD` sources
- [ ] additional reusable Zero Hour capabilities discovered by the audit are added to the port matrix
- [ ] relevant TheSuperHackers fixes/enhancements for selected subsystems are classified
- [ ] every rejected engine-looking capability has a written reason
- [ ] no plan exists to build or stage through a predecessor-game implementation before the Zero Hour implementation

---

## Donor-source rule for Sections 14 through 38

Unless a section explicitly says otherwise, any class/file name shown below is a **symbol family to investigate**, not an instruction to take an old EA file verbatim. The implementation source order is always:

```text
TheSuperHackers Core + GeneralsMD  -> sole SAGE implementation source
EA GeneralsMD                      -> optional historical Zero Hour validation only
OpenW3D                            -> final owner and architecture
```

If the implementation lives in `Core`, use it directly as part of the Zero Hour donor configuration. Do not search `Generals/` for an older copy first.

# 14. ZERO HOUR / SAGE FEATURE 1 — ASSET EXCLUSION / RESIDENCY

## Donor areas

Zero Hour/shared-Core WW3D asset/prototype management, especially behavior analogous to:

```text
Free_Assets_With_Exclusion_List
assetmgr.*
proto.*
```

## OpenW3D target

```text
Code/ww3d2/assetmgr.*
Code/ww3d2/proto.*
```

## Implement

Create a canonical asset-residency service with scopes such as:

```text
PERMANENT
GAME_MODE
WORLD
SECTOR
TRANSIENT
```

Track dependencies for:

- render prototypes;
- hierarchy trees;
- animations;
- textures;
- materials/shaders;
- generated world buffers.

Add diagnostics for:

- live prototypes;
- retained assets;
- dangling references;
- per-scope memory.

## Acceptance

Repeated map/world load/unload does not invalidate retained assets or leak unbounded resources.

---

# 15. ZERO HOUR / SAGE FEATURE 14 — SHADER MANAGER ARCHITECTURE

Implemented early because it is a dependency for many later donor systems.

## Donor

Zero Hour (`Core` + `GeneralsMD`):

```text
W3DShaderManager
W3DDevice shader integration
terrain/water/road shader setup
```

## OpenW3D target

Extend the existing D3D9-backed `Code/ww3d2` renderer.

## Rules

- Do not import a D3D8 wrapper DLL.
- Do not create another rendering backend.
- Historical `DX8Wrapper` class names may remain until a separate cleanup, but all new code must use their actual D3D9 capabilities.

## Implement pipelines/material programs for

```text
terrain
terrain detail
roads
bridges
water
foliage
projected shadows
particles
tracers/beams
status markers
ghost building tint
debug overlays
```

## Acceptance

Existing W3D materials remain functional and new donor systems share one state/shader management layer.

---

# 16. ZERO HOUR / SAGE FEATURE 19 — SPATIAL PARTITIONING CONCEPTS

Implemented early because terrain, building placement, lights, foliage, and Commander Mode need efficient queries.

## Existing OpenW3D/TT baseline

Preserve and build upon:

```text
CullSystemClass
GridCullSystemClass
AABTreeCull
PhysicsSceneClass
```

## Do not

blindly import SAGE `PartitionManager` as a second unrelated world database.

## Implement

Create a stable service such as:

```cpp
class WorldSpatialIndex {
public:
    QueryAABB(...);
    QueryOBB(...);
    QuerySphere(...);
    QueryFrustum(...);
    QueryRay(...);
    QueryPlacementOverlap(...);
    QueryNearbyLights(...);
};
```

Internally it may delegate to multiple specialized structures.

Required consumers:

- rendering;
- game-object proximity;
- area damage;
- AI queries;
- Commander building placement;
- nearby lights;
- foliage cells;
- roads/bridges/water;
- future procedural world generation.

## Acceptance

Common large-world queries no longer require scanning the complete global object list.

---

# 17. ZERO HOUR / SAGE FEATURE 4 — HYBRID HEIGHTMAP / TERRAIN FRAMEWORK

## Primary donor architecture

Use the pinned TheSuperHackers **Zero Hour configuration (`Core` + `GeneralsMD`)** as the terrain implementation source.

The Zero Hour donor exposes terrain responsibilities represented by:

```text
BaseHeightMap
HeightMap
FlatHeightMap
WorldHeightMap
TerrainVisual / TerrainLogic
```

Do not reconstruct or consult a predecessor-game terrain implementation as a required step; the pinned Zero Hour configuration is authoritative for this port.

## Canonical OpenW3D interpretation

Do not recreate several independent SAGE terrain engines. Extract the newer responsibility split into one OpenW3D `WorldTerrainSystem` with shared terrain data/query ownership and renderer-side near/detail/far LOD components.

## World model

OpenW3D becomes:

```text
World
+-- heightfield terrain
+-- arbitrary static W3D meshes
+-- interiors
+-- tunnels/caves/overhangs
+-- roads
+-- sectional bridges
+-- water bodies
+-- dynamic GameObjs
```

## Implement service

```cpp
WorldTerrainSystem
```

Required APIs:

```text
SampleHeight
SampleNormal
SampleSlope
RayIntersectTerrain
GetMaterial
GetBounds
GetPatch
InvalidatePatch
ModifyHeightRegion
ApplyRoadGrade
ApplyRiverCut
QueryPlacementSurface
BuildCollision
SelectRenderLOD
GetVisibleTerrainPatches
BuildFarTerrainRepresentation
InvalidateFarTerrainRegion
```

## Zero Hour terrain architecture requirements

- Centralize common terrain resources and queries rather than duplicating them between render modes.
- Use Zero Hour `BaseHeightMap` concepts for common ownership/lifetime.
- Use `HeightMap` / `FlatHeightMap` algorithms only as useful rendering/tiling strategies beneath the single OpenW3D service.
- Feature 21 adds the dedicated low-resolution/background terrain layer derived from `W3DTerrainBackground`.
- Terrain collision, placement queries, roads, bridges, water, shroud, foliage, surface marks, and generated terrain must all refer to the same canonical world coordinates and source terrain data.

## Physics

Use existing OpenW3D/WWPhys collision.

Rendered terrain and terrain collision must derive from the same source data.

## Future procedural-generation preparation

The terrain service must accept height data from an in-memory/generated source, not only files.

The terrain/query representation must expose enough traversal, slope, region, and connectivity information for a later procedural generator to satisfy Section 3A.5 at both infantry and vehicle scale.

Do **not** implement the full procedural world generator in this roadmap.

## Acceptance

FPS and vehicle gameplay works on runtime-created heightfield terrain while arbitrary W3D geometry remains supported.

---

# 18. ZERO HOUR / SAGE FEATURE 5 — TERRAIN TEXTURE SYSTEM

## Donors

Zero Hour/shared-Core terrain texture/tile/material systems such as:

```text
TerrainTex
TileData
terrain shader logic
detail/noise logic
```

## Implement deterministic material selection using

```text
height
slope
curvature
biome
moisture
water distance
road mask
river mask
city mask
Tiberium mask
cliff mask
manual override mask
```

Support layers for:

```text
base
detail
macro variation
cliff/slope
wet shoreline
road
biome
scorch/damage if retained
```

Expose a runtime build API so a future seed-based generator can create terrain materials with no hand-painted full-map texture.

## Acceptance

A generated heightfield can obtain coherent terrain materials entirely from runtime data/masks.

---

# 19. ZERO HOUR / SAGE FEATURE 3 — RUNTIME ROAD SYSTEM

## Donor

Zero Hour/shared-Core road buffers and `TerrainRoads` algorithms, including:

```text
W3DRoadBuffer
```

## Representation

```cpp
struct RoadSpline {
    controlPoints;
    width;
    shoulderWidth;
    material;
    roadClass;
    gradePolicy;
    bridgeConnections;
    tunnelConnections;
};
```

## Implement

- spline tessellation;
- intersections;
- shoulders;
- texture coordinates;
- material batching;
- terrain conforming;
- optional local terrain grading;
- bridge endpoint handoff;
- tunnel portal handoff;
- navigation/path metadata.

## Acceptance

Road geometry can be generated at runtime from centerlines without a unique modeled mesh for every road segment.

---

# 20. ZERO HOUR / SAGE FEATURE 2 — SECTIONAL BRIDGE SYSTEM

## Donor

Zero Hour (`Core` + `GeneralsMD`):

```text
W3DBridgeBuffer
TerrainRoad bridge definitions
```

## Support modular sections

```text
START_CAP
SPAN
END_CAP
SUPPORT
OPTIONAL_DECORATION
```

## Damage states

At minimum:

```text
PRISTINE
DAMAGED
CRITICAL
BROKEN
```

Each state may change:

- visual model;
- collision;
- traversal;
- path connection;
- debris emission.

## Integration

- roads connect to bridge endpoints;
- WWPhys provides collision;
- destruction updates pathing;
- server owns gameplay state;
- clients replicate state, not arbitrary raw geometry when deterministic reconstruction is possible.

## Acceptance

Bridges support variable length, damage, broken spans, collision, and multiplayer state.

---

# 21. ZERO HOUR / SAGE FEATURE 6 — TREE / FOLIAGE BUFFER

## Donor

```text
W3DTreeBuffer
```

## Implement

```text
FoliageSystem
+-- cells
+-- instances
+-- visibility
+-- distance LOD
+-- material batches
+-- optional collision proxies
```

Categories:

```text
tree
bush
dead tree
grass clump
decorative rock
```

Separate:

```text
visual-only
infantry blocker
vehicle blocker
destructible
```

Do not literally preserve inefficient donor sorting where a better C++20 implementation fits OpenW3D.

## Acceptance

Large forests render with substantially fewer submissions than individual `RenderObj` instances.

---

# 22. ZERO HOUR / SAGE FEATURE 7 — REFLECTIVE WATER / RIVER SYSTEM

## Donors

Zero Hour/shared-Core water systems including:

```text
W3DWater
W3DWaterTracks
```

## Support

```text
ocean/coast
lake
pond
river
stream
```

## Rendering

- reflection;
- animated normals/noise;
- flow direction;
- shore edges;
- wakes;
- ripples;
- projectile disturbances;
- LOD.

## Gameplay/collision

Use TT/OpenW3D water and naval collision semantics where applicable.

Integrate:

- infantry;
- hover/amphibious/naval units;
- bullets;
- TT naval factories;
- future procedural shoreline data.

## Acceptance

Water works visually and physically without a competing collision system.

---

# 23. ZERO HOUR / SAGE FEATURE 9 — TERRAIN TRACKS / SURFACE RIBBONS

## Donor

```text
W3DTerrainTracks
```

## Generalize into

```cpp
SurfaceRibbonSystem
```

Initial consumers:

```text
tank tracks
tire tracks
harvester tracks
scorch trails
drag marks
other temporary ground ribbons
```

Requirements:

- terrain/road/bridge conform;
- capped ring buffer;
- fade/lifetime;
- pooling;
- batched rendering;
- teleport reset;
- network derivation from motion where possible.

## Acceptance

Vehicles can leave stable marks without unbounded allocations or per-mark heavyweight objects.

---

# 24. ZERO HOUR / SAGE FEATURE 8 — PROJECTED / CACHED SHADOWS

## Donors

Zero Hour/shared-Core shadow systems such as:

```text
W3DProjectedShadow
W3DShadow
W3DVolumetricShadow
W3DBufferManager
```

## TT/OpenW3D integration

TT exposes dynamic-shadow behavior. Preserve its public semantics but implement one canonical OpenW3D shadow manager.

## Implement

- shared static shadow resources;
- per-object cached projectors;
- invalidation by transform/light/model changes;
- terrain reception;
- arbitrary receiver support where practical;
- batching.

## Acceptance

There is one shadow implementation serving both TT-facing APIs and new world systems.

---

# 25. ZERO HOUR / SAGE FEATURE 11 — DYNAMIC-LIGHT FILTERING

## Goal

Stop passing all dynamic lights to all renderable content.

## Implement

Use `WorldSpatialIndex` to produce small relevant light sets for:

```text
terrain chunks
foliage cells
bridges
roads
vehicles
infantry
buildings
particles/effects
Commander ghost building
```

## Acceptance

Lighting cost scales mainly with nearby/relevant lights instead of total world light count.

---

# 26. ZERO HOUR / SAGE FEATURE 10 — PARTICLE BATCHING

## Donor

```text
W3DParticleSys
```

## Implement

Shared buffers/pools for:

```text
sprite particles
point particles
streaks
```

Per-particle data:

```text
position
size
color
alpha
angle
lifetime
```

Budget classes:

```text
CRITICAL_GAMEPLAY
COMBAT_NEAR
COMBAT_FAR
ENVIRONMENT
AMBIENT
```

## Acceptance

Large firefights do not explode draw calls or allocations linearly with particle count.

---

# 27. ZERO HOUR / SAGE FEATURE 12 — TRACER / BEAM / PROJECTILE RENDER MODULES

## Donors

Zero Hour/shared-Core modules analogous to:

```text
W3DTracerDraw
W3DProjectileStreamDraw
W3DLaserDraw
W3DRopeDraw
```

## Implement

- ballistic tracer;
- hitscan visual;
- laser/beam;
- projectile trail/stream;
- purely visual rope/cable strip where needed by a donor visual.

**No rope physics subsystem is in scope.**

Use Renegade weapon/projectile definitions and optional new visual fields.

## Acceptance

Weapons can use pooled/batched render modules without script-spawned effect objects for every tracer.

---

# 28. ZERO HOUR / SAGE FEATURE 13 — DEBRIS RENDERING

## Donor

```text
W3DDebrisDraw
```

## Implement two tiers

```text
GAMEPLAY_DEBRIS
VISUAL_DEBRIS
```

Gameplay debris may use existing WWPhys objects when collision matters.

Visual debris must use lightweight pooled rendering and bounded lifetime.

Consumers:

- buildings;
- bridges;
- vehicles;
- future generated city props.

## Acceptance

Large destruction events do not require every visible fragment to be a complete networked `GameObj`.

---

# 29. ZERO HOUR / SAGE FEATURE 18 — MODULAR DRAW-COMPONENT ARCHITECTURE

## Donor concepts

```text
W3DModuleFactory
DrawModule
W3DModelDraw
specialized vehicle/object draw modules
```

## Rule

Do not replace OpenW3D `GameObj` with the SAGE `Thing` hierarchy.

## Implement composable components such as

```cpp
IRenderComponent
IClientUpdateComponent
IRTSOverlayComponent
ISelectionVisualComponent
IWorldInteractionVisualComponent
```

Initial components:

```text
selection/status renderer
waypoint/order renderer
ghost-building renderer
tracer renderer
debris renderer
procedural-world visual attachment
special vehicle visual controller
composite child-model attachment controller
```

## Zero Hour composite/dynamic attachment requirement

Inspect Zero Hour's expanded specialized draw-module set, including the additional Overlord attachment-oriented render modules. Do **not** port those unit classes literally. Extract reusable mechanics for:

```text
child model attached to named W3D node/bone
runtime add/remove child visual
independent child animation state
parent-relative transform propagation
turret/module attachment transforms
visibility/damage-state propagation
LOD/culling propagation
```

Where practical, make this attachment layer consumable by the building semantic descriptor system as well as vehicles. There must be one generalized attachment implementation, not one special class per donor unit.

## Acceptance

New rendering behavior can be attached without continuously expanding every `GameObj` subclass.

---

# 30. ZERO HOUR / SAGE FEATURE 15 — RTS SHROUD / FOG OF WAR

## Donor

```text
W3DShroud
```

## States

```text
UNKNOWN
EXPLORED
VISIBLE
RADAR_DETECTED
LAST_KNOWN
```

## FPS / Commander intelligence rule

Do not black out the physical first-person world because the Commander intelligence layer has not explored it.

Shroud applies to Commander Mode, tactical radar/map, strategic overlays, and strategic contacts.

The Commander camera itself is never a reveal source. Moving it must not alter any shroud/intelligence state.

Only legitimate team visibility/sensor events may update strategic intelligence.

`LAST_KNOWN` is stale information and must not track an unseen object's live transform.

Integrate TT per-player/object visibility semantics and avoid needlessly replicating tactically forbidden information where existing network visibility controls can suppress it.

## Sensor-source API

Expose canonical registration suitable for Phase 34 building capabilities:

```text
RegisterStrategicSensorSource
UpdateStrategicSensorSource
UnregisterStrategicSensorSource
```

A source can provide:

```text
team
world transform
vision/exploration radius
radar-detection radius
last-known retention policy
online/offline state
```

Communications buildings later use this API instead of creating another shroud implementation.

## Acceptance

- Commander camera movement alone reveals nothing;
- Commander/tactical intelligence is independent of normal FPS physical rendering;
- strategically hidden information does not leak into ordinary FPS targeting/HUD behavior;
- sensor sources can be added/removed dynamically.

---

# 31. ZERO HOUR / SAGE FEATURE 16 — RADAR SYSTEM

## Donor

```text
W3DRadar
```

## Implement one shared tactical intelligence model

Represent:

```text
infantry
vehicles
buildings
resources
objectives
waypoints
Commander placement markers
radar-only contacts
last-known contacts
communications-building sensor coverage
```

Existing Renegade and TT radar APIs feed the same model.

Communications Center/Advanced Communications Center coverage from Phase 34 feeds this model and the shroud sensor-source API.

Commander camera position is never radar coverage.

Do not expose strategic radar-only/last-known information as live FPS wallhacks.

## Acceptance

- FPS HUD radar and Commander presentation use consistent canonical intelligence state;
- powered communications buildings can expand configured strategic coverage;
- disabling/destroying those buildings removes their coverage;
- camera movement does not create radar contacts.

---

# 32. ZERO HOUR / SAGE FEATURE 17 — WAYPOINT / STATUS-CIRCLE RENDERERS

## Donors

```text
W3DStatusCircle
W3dWaypointBuffer
```

## Implement batched world overlays for

```text
selection
target
move point
defend area
repair target
patrol point
construction footprint
building placement validity
```

These visuals must be reusable by Commander Mode but are not permission/gameplay logic themselves.

## Acceptance

Large numbers of markers are batched and respect shroud/per-player visibility.

---

# 33. ZERO HOUR / SAGE FEATURE 20 — DEBUG RENDERING TOOLS

## Donors

```text
W3DDebugDisplay
W3DDebugIcons
```

## Implement developer overlays for

```text
terrain chunks
terrain materials
road splines
road triangles
bridge sections
water volumes
foliage cells
spatial-index cells
dynamic-light radii
shadow projectors
WWPhys collision bounds
building semantic nodes
building footprints
placement overlap tests
purchase-terminal anchors
MCT anchors
Commander cursor ray
shroud cells
radar contacts
```

Development-only debug controls are allowed. They are not legacy/new feature selectors.

## Acceptance

Every new world/Commander subsystem has enough visualization to diagnose placement, culling, transform, and rendering errors.


---

# 34. ZERO HOUR / SAGE FEATURE 21 — FAR / BACKGROUND TERRAIN LOD

## Donor

Primary donor:

```text
TheSuperHackers Zero Hour (`Core` + `GeneralsMD`) equivalent of `W3DTerrainBackground`
```

Canonical historical reference:

```text
EA Zero Hour `GeneralsMD/Code/GameEngineDevice/Source/W3DDevice/GameClient/W3DTerrainBackground.*`
```

The Zero Hour implementation describes this layer as backup terrain rendered at lower resolution. Treat the current Zero Hour implementation as authoritative; no predecessor comparison is required.

## OpenW3D target

Integrate this into Feature 4's single `WorldTerrainSystem`; do not create a second independent terrain database.

Implement:

```text
near/detail terrain patches
mid-distance tiled terrain as useful
far/background terrain representation
camera-distance LOD selection
seam-safe transitions
region invalidation after terrain edits
shared material/texture selection
shared shroud/water/world coordinates
```

The far layer may use lower-resolution geometry and textures, but it must derive from the same terrain source as near terrain.

## Future procedural-world requirement

Runtime/generated terrain changes must be able to invalidate and rebuild the affected far/background representation without reloading the complete map.

## Acceptance

- no visible cracks at near/far boundaries under normal camera ranges;
- far terrain uses substantially fewer vertices/draw submissions than near terrain;
- terrain edits invalidate the correct background regions;
- one canonical terrain query/collision representation remains authoritative.

---

# 35. ZERO HOUR / SAGE FEATURE 22 — SURFACE SMUDGE / DECAL MANAGER

## Donor

Primary donor:

```text
TheSuperHackers Zero Hour W3DSmudge implementation
```

Historical reference:

```text
EA Zero Hour `GeneralsMD/.../W3DSmudge.*`
```

`W3DSmudge` demonstrates batched persistent/semi-persistent surface marks in the Zero Hour donor.

## Canonical OpenW3D design

Create one bounded surface-mark service rather than separate scorch, smudge, construction-mark, and terrain-track decal databases.

Suggested service:

```cpp
WorldSurfaceMarkManager
```

Support mark types such as:

```text
SCORCH
BLAST
DIRT
OIL_OR_STAIN
CONSTRUCTION
IMPACT
DECORATIVE_WORLD_MARK
```

Feature 9 terrain tracks/ribbons may share allocation/material/batching infrastructure while preserving ribbon-specific geometry behavior.

Required behavior:

- batched dynamic/static vertex/index buffers;
- bounded memory and deterministic eviction policy;
- material/texture grouping;
- terrain projection with normal/slope handling;
- optional projection onto eligible static W3D surfaces where practical;
- cell/region culling through the spatial index;
- cleanup on world unload;
- client-only rendering unless a mark has explicit gameplay semantics.

## Acceptance

Hundreds or thousands of marks can exist without one draw call or one network object per mark, and mark lifetime/eviction cannot grow memory without bound.

---

# 36. ZERO HOUR / SAGE FEATURE 23 — WEATHER / ENVIRONMENT PARTICLE LAYER

## Donor

Primary donor:

```text
TheSuperHackers Zero Hour W3DSnow implementation and subsequent fixes
```

Historical reference:

```text
EA Zero Hour `GeneralsMD/.../W3DSnow.*`
```

The Zero Hour snow renderer supports a dedicated particle buffer and multiple rendering strategies. Port the reusable architecture, not a snow-only gameplay feature.

## OpenW3D target

Create:

```cpp
WeatherEnvironmentRenderer
```

Initial visual modes:

```text
SNOW
RAIN_LIKE_PRECIPITATION
ASH
DUST
TIBERIUM_ATMOSPHERIC_PARTICLES
GENERIC_FALLING_PARTICLES
```

Required capabilities:

- camera-centered or world-region emitters;
- bounded particle budgets;
- batch rendering using the current D3D9-backed OpenW3D renderer;
- distance/visibility culling;
- optional wind vector;
- density/intensity controls;
- resource recreation after device reset where relevant;
- no dedicated-server renderer dependency.

Do not reproduce the donor's Direct3D 8 device calls or point-sprite assumptions literally. Use OpenW3D's current renderer abstractions and choose the best available D3D9 path.

## Acceptance

Weather can cover a large outdoor scene at a stable bounded particle count without creating a `GameObj` per particle.

---

# 37. ZERO HOUR / SAGE FEATURE 24 — AI STATE / PATH / GUARD-RETALIATION IMPROVEMENTS

## Donor audit

Inspect the complete AI implementation in the pinned TheSuperHackers Zero Hour configuration (`Core` + `GeneralsMD`) and adapt the reusable behavior directly.

Known required audit points include:

```text
AI
AIGuard
AIGuardRetaliate
AIPathfind
AIPlayer
AISkirmishPlayer
AIStateMachine
AITNGuard
```

Treat `AIGuardRetaliate` together with the surrounding Zero Hour AI/state/pathfinding implementation as one connected behavior family rather than cherry-picking one class.

## OpenW3D rule

Do **not** replace Renegade AI with SAGE AI wholesale.

Extract and adapt reusable behavior into OpenW3D's canonical AI ownership:

```text
guard state semantics
retaliation after hostile damage
threat evaluation
attacker memory / target reacquisition
return-to-guard-area behavior
attack-move/guard transition ideas where compatible
path retry/replan improvements
stuck detection/recovery improvements
state transition safety
formation/group path ideas only where they fit existing Renegade AI
```

TT 4.8.4 AI/script behavior remains authoritative wherever TT already changes Renegade semantics. Zero Hour algorithms must be adapted around that result rather than creating a parallel SAGE AI runtime.

## Human-player priority and scheduling

Feature 24 also implements Section 3A.4 and Section 3A.6:

- AI infantry/vehicles yield or replan around human-controlled units where practical;
- production exits use reservation/clearance logic;
- stuck recovery detects player-induced obstruction;
- Commander AI population is bounded by configurable team/map/server limits;
- distant/non-combat AI may use lower-frequency decision updates;
- expensive path replans are staggered/budgeted;
- local player input, aiming, movement, nearby collision/combat, and vehicle control take scheduling priority over distant RTS AI work.

Do not solve congestion by disabling meaningful collision or teleporting units through players.

## Networking/server

All gameplay-relevant AI decisions remain server-authoritative. Clients receive normal replicated object results, not their own independent AI simulation.

## Tests

Create deterministic scenarios for:

```text
guard -> retaliate -> return
guard -> lose target -> reacquire
guard area boundary
path blocked -> replan
stuck unit recovery
multiple simultaneous threats
invalid/dead target transition
```

## Acceptance

AI behavior gains the selected Zero Hour state/path improvements without introducing the SAGE `Thing`/AIPlayer world model as a second game simulation.

---

# 38. ZERO HOUR / SAGE FEATURE 25 — RTS TELEMETRY / STATISTICS INSTRUMENTATION

## Donor concept

Zero Hour adds `AcademyStats`, which records a broad set of player/RTS behaviors such as production, upgrades, income, captures, garrisoning, selection use, power outages, guard use, mines, salvage, and related events.

Do **not** port the Academy advice/tutorial UI or Zero Hour-specific coaching logic.

## OpenW3D target

Create a generic instrumentation service:

```cpp
RTSTelemetryService
```

It should consume native OpenW3D/TT events and maintain inexpensive counters/timers useful for development, balancing, automated tests, Commander Mode, and later procedural-world evaluation.

Initial categories:

```text
building construction/destruction
procedural construction start/cancel/complete
unit/vehicle/aircraft creation/destruction
team economy/income where available
power generation/demand/allocation/load-shed transitions
Commander manual power overrides
communications coverage transitions
Silo wallet-capacity changes
Commander placement attempts/rejections/successes
PCT/MCT/factory usage
Commander production queue outcomes
AI orders/state transitions
blocked factory/air-pad exits
kills/damage by category
world/terrain feature usage where measurable
path failures/replans
asset/particle/foliage/terrain pressure counters
```

## Rules

- disabled or low-overhead in normal release mode unless explicitly enabled;
- no personally identifying telemetry and no external upload requirement;
- deterministic counters should be queryable by tests;
- server owns gameplay-authoritative statistics;
- client renderer metrics remain local profiling data;
- do not couple core gameplay correctness to analytics availability.

## Acceptance

The engine can produce a structured local/session statistics snapshot useful for tests and balancing without carrying Zero Hour's Academy UI or advice rules.

---

# 39. PHASE 33 — BUILDING MODEL SEMANTIC JSON + LIVE BUILDING DEFINITIONS

This phase is the data foundation for power, strategic building capabilities, Commander UI, placement, construction, and AI production.

## Goal

A Commander-buildable building definition resolves to one JSON descriptor supplying:

```text
complete W3D model semantics
anchors/transforms
placement footprint/rules
damage-part annotations
power generation/consumption/priority
optional strategic building capabilities
optional construction visual tuning
```

The W3D remains the geometry/hierarchy source. Do not put raw mesh vertices into JSON.

## JSON implementation

Prefer `nlohmann-json` through the existing vcpkg/CMake dependency system unless OpenW3D has adopted a canonical JSON library.

Do not hand-roll a parser.

## Canonical descriptor

Use one resolved descriptor API equivalent to:

```cpp
class BuildingModelDescriptor {
public:
    std::string DefinitionName;
    std::string ModelName;
    uint32_t SchemaVersion;

    BuildingHierarchy Hierarchy;
    std::vector<BuildingPart> Parts;
    std::vector<BuildingAnchor> Anchors;
    BuildingFootprint Footprint;
    BuildingPlacementRules PlacementRules;
    BuildingDamageLayout DamageLayout;

    BuildingPowerDefinition Power;
    BuildingCapabilityDefinition Capabilities;
    BuildingConstructionVisualDefinition ConstructionVisual;
};
```

Exact class splitting may follow OpenW3D conventions.

Two presets may share a W3D model without being forced to share gameplay values.

## Complete model understanding

On load:

1. resolve the building definition/preset;
2. load its W3D prototype/hierarchy;
3. enumerate the complete hierarchy/subobject graph;
4. capture object/subobject name, hierarchy node/bone, parent, local transform, resolved model transform, bounds, and render/collision type;
5. apply JSON semantic annotations/gameplay configuration;
6. validate;
7. produce one complete resolved descriptor.

## Required power block

Every Commander-buildable building JSON must explicitly contain or inherit:

```json
{
  "power": {
    "requires_power": true,
    "generation": 0.0,
    "consumption": 140.0,
    "priority": 50
  }
}
```

Semantics:

```text
requires_power
    true  -> normal functionality depends on grid allocation
    false -> no grid allocation required

generation
    finite units supplied while alive/operational/eligible

consumption
    finite units requested while alive/operational/eligible

priority
    deterministic load-shedding priority; higher values remain powered first
```

Do not hard-code Power Plant output or building draw in C++.

Typical Power Plant:

```text
requires_power = false
generation > 0
consumption = 0
```

Typical consumer:

```text
requires_power = true
generation = 0
consumption > 0
```

## Communications capability

Normal and Advanced Communications Centers use one data-driven implementation.

Example:

```json
{
  "capabilities": {
    "communications": {
      "enabled": true,
      "vision_radius": 180.0,
      "radar_radius": 300.0,
      "last_known_retention_seconds": 20.0,
      "requires_building_online": true
    }
  }
}
```

Values are examples only.

The Advanced Communications Center obtains greater capability through larger JSON values rather than a duplicate radar/shroud implementation.

## Tiberium Silo capability

Example:

```json
{
  "capabilities": {
    "tiberium_silo": {
      "enabled": true,
      "wallet_capacity_bonus_per_player": 2500.0,
      "requires_building_online": true
    }
  }
}
```

The number is example tuning only.

Each active applicable Silo increases the maximum credit/wallet size of **every player on the owning team**.

Multiple Silos stack.

## Optional construction visual block

```json
{
  "construction_visual": {
    "duration_seconds": 15.0,
    "block_width": 2.0,
    "block_depth": 2.0,
    "row_height": 1.5,
    "source_reveal_start": 0.75,
    "block_fade_start": 0.90,
    "max_blocks": 2048
  }
}
```

These values tune Phase 38's visual presentation.

## Semantic roles

At minimum:

```text
structure
interior
exterior
damageable_part
destruction_part
factory_door
purchase_terminal_anchor
mct_anchor
infantry_spawn
infantry_staging
vehicle_spawn
vehicle_exit
vehicle_staging
aircraft_spawn
aircraft_staging
naval_spawn
repair_zone_anchor
harvester_dock
construction_origin
placement_footprint
pathing_portal
camera_interest
commander_station_anchor
```

## Node resolution

```text
worldTransform =
    buildingWorldTransform *
    nodeModelTransform *
    optionalJsonOffset
```

Never replace model-node semantics with hard-coded mapper world coordinates.

## Runtime hot reload

Power/capability tuning must be editable **on the fly**.

Provide explicit developer/admin reload commands:

```text
reload_building_descriptors
reload_building_descriptor <definition>
```

A development-only file watcher is optional. Correctness must not depend on one.

Reload transaction:

1. parse changed JSON into a temporary descriptor;
2. fully validate it;
3. if invalid, keep the last known-good descriptor active and report the error;
4. if valid, atomically publish the new descriptor revision;
5. apply live-tunable values to existing server-authoritative building instances;
6. trigger affected recomputation:
   - team power;
   - communications/radar/shroud coverage;
   - Silo wallet capacity;
7. replicate resulting authoritative state/config revision as required.

Required live-tunable fields:

```text
power.requires_power
power.generation
power.consumption
power.priority
communications coverage/retention
tiberium_silo wallet bonus
capability online requirements
construction visual tuning for future/new construction sites
```

Structural changes such as model identity, anchors, hierarchy semantics, or footprint topology may apply only to future spawns unless safe live rebinding is implemented.

Clients never gain gameplay authority by editing local JSON.

## Validation

Reject/warn on:

```text
missing required W3D object/node
duplicate semantic ownership
invalid parent
non-finite transform
empty footprint
unsupported schema
negative/non-finite generation or consumption
invalid power priority
negative/non-finite communications radius
negative wallet bonus
invalid construction visual dimensions/counts
incompatible capability/building role
```

## Caching

Cache by:

```text
building definition identity
+ model identity
+ model content hash
+ JSON content hash
```

## Core descriptor coverage

Cover the applicable GDI/Nod equivalents of:

```text
Power Plant
Communications Center
Advanced Communications Center
Refinery
Tiberium Silo
Weapons Factory / War Factory
Airstrip
Air Pad / air-production facility
Repair Facility / Repair Bay
Advanced Guard Tower / Guard Tower
Obelisk of Light
Barracks
Hand of Nod
```

Where Renegade/TT already implements gameplay, descriptor data augments/configures it instead of creating a duplicate implementation.

## Tooling

Add:

```text
dump-building-model <model.w3d>
validate-building-json <descriptor.json>
generate-building-json-skeleton <model.w3d>
validate-all-building-descriptors
```

## Acceptance

- W3D + JSON resolves to one validated semantic descriptor;
- power/capability values are queryable from that descriptor;
- valid runtime reload changes live tuning without recompilation/restart;
- invalid reload preserves last known-good values;
- core Commander-buildable definitions have valid descriptors.

---

# 40. PHASE 34 — FINITE TEAM POWER GRID + STRATEGIC BUILDING CAPABILITIES

This phase occurs after building JSON and before Commander UI/construction.

## Goal

Add a finite per-team power budget while preserving canonical Renegade/OpenW3D/TT destroyed and powered-down building behavior.

Also implement:

```text
Communications Center / Advanced Communications Center strategic coverage
Tiberium Silo team-wide player wallet-capacity expansion
```

Do not reimplement existing Refinery, factory, repair, defense, Barracks/Hand, destruction, or powered-down behavior.

## Canonical power state

The new system decides:

```text
team generation
team requested demand
which consumers receive allocation
```

Then it drives the **existing canonical per-building powered/unpowered state**.

Forbidden:

```text
LegacyPoweredFlag
+
RTSPowerFlag
+
translation glue
```

Audit the latest OpenW3D + integrated TT owner and extend it.

## Accounting

Create one server-authoritative service equivalent to:

```cpp
class TeamPowerGridService {
public:
    RegisterBuilding(...);
    UnregisterBuilding(...);
    OnBuildingCreated(...);
    OnBuildingDestroyed(...);
    OnBuildingOperationalStateChanged(...);
    OnDescriptorReload(...);
    RecalculateTeam(...);

    float GetGeneration(TeamId) const;
    float GetRequestedDemand(TeamId) const;
    float GetAllocatedDemand(TeamId) const;
};
```

Per team:

```text
AvailablePower =
    sum(JSON generation of alive operational eligible generators)

RequestedPower =
    sum(JSON consumption of alive eligible dependent consumers)
```

`CONSTRUCTING` buildings neither generate nor consume finished-building power until Phase 38 completion.

## Allocation

```text
AvailablePower >= RequestedPower
    -> all eligible consumers powered

0 < AvailablePower < RequestedPower
    -> deterministic load shedding until AllocatedDemand <= AvailablePower

AvailablePower == 0
    -> every surviving requires_power=true building enters canonical powered-down state
```

## Multiple Power Plants / generator loss

Generators add.

Example:

```text
Plant A 500
Plant B 500
Total   1000
```

Destroying one removes only its output and rebalances.

Destroying the **last operational generator** takes every power-dependent building offline.

Destroying a non-generator removes its demand and can automatically restore previously shed buildings.

Completing a new Power Plant adds its JSON generation and can restore buildings.

Do not special-case a fixed number of plants.

## Deterministic load shedding

Use JSON `power.priority`.

Higher priority stays powered before lower priority.

Within equal priority use a stable server-owned persisted `PowerGridMemberId` or equivalent deterministic tie-break.

Recalculate only on relevant state/config changes, not every frame.

Never randomly shut buildings down.

## Commander manual power control

Server-authoritatively allow the Commander to:

```text
manually disable/enable an eligible consumer
set a temporary priority override
clear override back to JSON priority
```

Manual disable immediately frees demand.

Overrides are visible in Commander UI, network-authoritative, and saved/restored where appropriate.

## Reuse existing Renegade building behavior

Audit/reuse:

```text
Refinery / Harvester economy
Weapons Factory / War Factory
Airstrip
Repair Facility / Repair Bay
Advanced Guard Tower / Guard Tower
Obelisk of Light
Barracks
Hand of Nod
destroyed state
powered-down state
```

If an existing subsystem already reacts to canonical building power state, leave it alone.

If it only checks a coarse whole-base flag, refactor that check to canonical per-building operational/power state instead of reimplementing the subsystem.

## Communications Center / Advanced Communications Center

Register each active communications-capable building with Sections 30-31's sensor-source service.

JSON defines:

```text
vision/exploration radius
radar-detection radius
last-known retention
requires-building-online policy
```

Coverage disappears when the building is destroyed, under construction, manually disabled, or powered down when its capability requires online state.

Coverage returns automatically when restored.

Normal and Advanced Communications Centers share one implementation with different JSON values.

The intelligence firewall still applies.

## Tiberium Silo wallet expansion

Start from the canonical existing player wallet cap:

```text
EffectivePlayerWalletCap =
    BaseWalletCap
    + sum(active Silo wallet_capacity_bonus_per_player)
```

Apply to every player on the owning team.

Multiple Silos stack.

If capacity falls below an existing player balance:

- preserve already-earned credits;
- do not grant further positive credit above the reduced cap;
- normal spending can bring balance back under the cap.

Recalculate on Silo completion, destruction, online/offline transition, team change if supported, descriptor reload, player join/team change, and save/load reconstruction.

## Hot reload

Changes to:

```text
generation
consumption
priority
requires_power
communications values
Silo wallet bonus
```

immediately trigger transactional recomputation on the server.

## Debug/telemetry

Expose:

```text
generation
requested demand
allocated demand
headroom/overload
powered count
shed count
manual-off count
generator list
consumer list + draw + priority + state
```

## Acceptance

- finite JSON-defined generation/demand works;
- overload deterministically sheds buildings;
- new generation or reduced demand restores them;
- destroying the final generator powers down every dependent building;
- canonical Renegade powered-down behavior is used;
- live JSON reload recalculates correctly;
- communications coverage follows online state;
- Silo wallet bonuses stack for every team player;
- manual priority/on-off control is authoritative.

---

# 41. PHASE 35 — COMMANDER MODE CAMERA / EMBODIED PLAYER

## Goal

Add a strategic camera without replacing/degrading FPS controls.

## State

```text
FPS_MODE
COMMANDER_MODE
```

Entering Commander Mode:

- redirects camera/input to strategic controls;
- keeps the same player/session;
- keeps the same physical `SoldierGameObj`;
- activates Commander overlays/sidebar;
- does not spawn a second player object.

## Embodied Commander

The physical soldier remains:

```text
damageable
targetable
collidable
server-authoritative
in the world
```

Death immediately exits Commander Mode.

Returning to FPS restores camera/input to that same surviving soldier.

An optional `commander_station_anchor` may be supported without creating a second player entity.

## Camera

Support:

```text
pan
zoom
configurable tilt
rotation if desired
map/world bounds
cursor raycast
terrain + arbitrary W3D geometry picking
```

## Intelligence

Use Sections 30-31 and Section 3A.2. Camera movement itself reveals nothing.

## Authority

Camera presentation is client-side. Strategic gameplay actions remain server-authoritative and require Commander permission.

## Acceptance

- FPS -> Commander -> FPS works in one runtime;
- same player body remains physical/vulnerable;
- death exits Commander Mode;
- camera movement alone reveals no hidden enemy;
- FPS behavior outside Commander Mode is unchanged.

---

# 42. PHASE 36 — COMMANDER RIGHT-SIDE UI + CONTEXTUAL PRODUCTION SIDEBAR

Restore the full Commander production UI.

## Persistent layout

While in Commander Mode, show a right-side panel visually compatible with Renegade's existing UI/purchase imagery.

Visible item area:

```text
two rows
target 2 x 4 cells = 8 visible slots
pagination/scrolling for more entries
```

Eight is the visible page size, not a total-content cap.

## Required modes

```text
BUILDINGS
GROUND_VEHICLES
AIR_VEHICLES
INFANTRY
```

`BUILDINGS` is default.

Provide an obvious button/hotkey/input to return to BUILDINGS.

## BUILDINGS mode

Show faction-appropriate buildable structures.

Cards derive/display:

```text
icon
name
cost
availability
construction time/state
power generation (+)
power consumption (-)
prerequisite/disabled reason
queue state
```

Selecting a building enters Phase 37 ghost placement.

## Clicking physical production buildings changes context

Click/select an allied operational building in the world.

### Ground factory

A canonical:

```text
Weapons Factory
War Factory
Airstrip configured as ground factory
other ground_vehicle_factory
```

switches to `GROUND_VEHICLES` and binds the panel to **that selected facility**.

### Air production

A canonical:

```text
GDI Air Pad
Nod Air Pad
other air_vehicle_factory
```

switches to `AIR_VEHICLES` and binds to that facility.

### Infantry production

A canonical:

```text
Barracks
Hand of Nod
other infantry_factory
```

switches to `INFANTRY` and binds to that facility.

Use canonical building definition/capability plus descriptor semantics, not raw model filenames alone.

## Unit cards

Reuse existing Renegade Purchase Terminal character/vehicle images whenever available.

Display:

```text
name
cost
availability
selected facility state
power/operational state
queue state
population/production limit
disabled reason
```

The UI calls a server-side production service rather than emulating human Purchase Terminal GUI clicks.

## Power display

Commander UI must show at minimum:

```text
POWER CAPACITY / GENERATION BAR
POWER CONSUMPTION / ALLOCATED LOAD BAR
numeric generation
numeric requested demand
numeric allocated demand
headroom or overload
load-shed/offline building count
```

When overloaded, make it immediately obvious.

Building cards show their own `+generation` or `-consumption`.

For a selected allied building expose current power state, JSON default priority, manual override, and manual enable/disable when eligible.

## Facility state

If selected facility is destroyed, under construction, loses power, or becomes non-operational, immediately disable/update the contextual panel.

Never allow stale purchases from a dead/offline facility.

## FPS isolation

The sidebar exists in Commander Mode only. Do not replace/clutter the ordinary FPS HUD.

## Acceptance

- BUILDINGS sidebar appears on Commander entry;
- visible production/building area is two rows with paging;
- power bars reflect server state;
- click ground factory -> GROUND_VEHICLES;
- click Air Pad -> AIR_VEHICLES;
- click Barracks/Hand -> INFANTRY;
- return to BUILDINGS is immediate;
- existing PT icons are reused where possible;
- dead/offline selected facilities cannot produce.

---

# 43. PHASE 37 — COMMANDER BUILDING GHOST / PLACEMENT VALIDATION

## Goal

Show the real selected building model at the Commander cursor.

## Ghost

Render-only, translucent, cursor-following, no gameplay collision/network/save object.

Valid placement uses normal/team/neutral translucent tint.

Invalid placement, including overlap, uses the required:

```text
RED TRANSLUCENT
```

state.

Show footprint through status/waypoint renderer where useful.

## Controls

Support rotation, cancel, and return to sidebar.

## Placement query

Use:

```text
BuildingModelDescriptor.Footprint
WorldSpatialIndex
WWPhys/OpenW3D collision queries
WorldTerrainSystem
```

At minimum check:

```text
building overlap
placement-blocking static/dynamic geometry
surface support
configured slope limits
world bounds
required production-exit/anchor clearance
```

Use better OBB/compound footprint when available rather than forcing AABB-only tests.

## Server authority

Client sends:

```text
building definition/preset
position
orientation
placement sequence id
```

Server reruns placement checks.

Never trust client green/red state.

## Acceptance

Real ghost follows cursor, invalid overlap turns it red, and server independently rejects the same bad placement.

---

# 44. PHASE 38 — AUTHORITATIVE BUILDING CONSTRUCTION + PROCEDURAL BLOCK BUILD-UP + AUTO PCT/MCT

Restore the original procedural construction presentation using the current semantic-building architecture.

## Authoritative lifecycle

Accepted placement creates the real canonical TT/OpenW3D building immediately in:

```text
CONSTRUCTING
```

state.

Initially:

```text
real GameObj exists server-side
finished-building gameplay disabled
final collision disabled
real model hidden
construction footprint reserved
procedural visualizer active on clients
```

Do not create a separate Commander-only building class when canonical building classes can represent construction state.

## Completion activation gate

At completion activate in controlled order:

```text
final render visibility
final collision
canonical building operational state
power-grid generation/demand
Purchase Terminals
MCT/building aggregates
factory production
repair functionality
defenses
communications capability
Silo capability
spawn/exit anchors
base-controller operational registration
```

A building under construction provides none of those finished-building capabilities.

## Procedural visual

No authored construction model/animation/new texture is required.

Generate from:

```text
building transform
model bounds
JSON construction_visual
existing team/material facilities
deterministic seed
authoritative normalized progress
```

Sequence:

```text
foundation
procedural blocks assemble bottom-to-top
real building reveals/fades in
temporary blocks fade out
building becomes operational
```

## Batched geometry

Do not create one RenderObj or physics object per block.

Use one or a small bounded number of dynamic meshes, e.g.:

```text
completed rows batch
current animated row batch
optional inactive/future batch
```

Each logical block stores deterministic final center, size, row, order, and progress interval.

Respect `max_blocks`.

## Grid / row generation

From local model bounds:

1. compute X/Y cells and Z rows;
2. keep useful approximate block proportions within max count;
3. generate rows from foundation to roof;
4. assign stable order using seed;
5. transform through building world transform.

Rotated buildings must generate rotated visuals.

## Animation

Default normalized staging:

```text
0.00-0.10 foundation
0.10-0.80 block rows
0.75-1.00 source building reveal
0.90-1.00 block fade
```

JSON can tune these.

Blocks rise from slightly below final location, vertically scale into place, fade in, then settle.

Rows activate strictly bottom-to-top.

Whole-building alpha reveal is the reliable first implementation. Height-band/clipping reveal is optional later.

## Collision / occupancy

Use one simple authoritative construction footprint blocker, not per-block collision.

At completion verify occupancy, remove temporary blocker, then enable final canonical collision safely.

## Networking

Never transmit every block transform.

Replicate compact state equivalent to:

```text
building network id
definition id
transform
server construction start
duration
seed
construction state
descriptor/config revision if required
```

Clients deterministically reconstruct blocks.

Late joiners generate layout, set current progress, and continue.

Server time/state owns completion, never client frame rate.

## Save/load

Persist only authoritative construction state/timing/seed/definition/revision data.

Rebuild GPU block meshes after load.

## Cancellation/destruction

Clean:

```text
procedural meshes
footprint blocker
pending terminals/MCTs
pending production
partial strategic-capability registrations
network state
queue/accounting state
```

No half-built object may provide power/radar/production/defense/wallet capacity.

## PCT/MCT and semantic children

At completion, for each Purchase Terminal/MCT anchor resolve the W3D node transform and spawn/bind canonical Renegade/TT behavior.

Also activate prepared semantic anchors for factory doors, infantry/vehicle/air exits/staging, harvester docks, repair zones, etc.

Support multiple terminals/MCTs.

## Acceptance

- building constructs visibly from bottom-to-top procedural blocks;
- no new authored construction asset is required;
- blocks are batched/non-physical;
- network/save/load reconstructs deterministic stage;
- finished behavior remains disabled until completion;
- final collision activates safely;
- PCT/MCT nodes work;
- completed Power Plant enters grid only at completion;
- cancellation/destruction leaves no temporary state.

---

# 45. PHASE 39 — COMMANDER AI INFANTRY / GROUND / AIR PRODUCTION + UNIT CONTROL

Depends on TT integration, Feature 24 AI, waypoint/status rendering, descriptors, finite power, contextual UI, and completed canonical production buildings.

## AI units are normal Renegade objects

Commander-purchased infantry are normal AI `SoldierGameObj` paths, not fake human players.

Commander-purchased vehicles/aircraft are normal canonical `VehicleGameObj`/derived objects with server-authoritative AI control.

Do not consume human player slots or scoreboard identities.

## Shared production service

Validate server-side:

```text
Commander authority
team
credits
selected facility
facility alive
construction complete
facility powered/operational
unit availability/prerequisites
queue capacity
population limits
spawn/exit availability
```

Do not make bots emulate human PT GUI input.

## Context-bound queues

Purchase is bound to the selected physical facility:

```text
selected ground factory -> GROUND_VEHICLES queue
selected Air Pad         -> AIR_VEHICLES queue
selected Barracks/Hand   -> INFANTRY queue
```

Multiple facilities may have independent queues.

Do not silently redirect to another building unless a future explicit policy is added.

## Infantry production

Use descriptor:

```text
infantry_spawn
infantry_staging
```

New AI infantry:

1. is created through canonical production;
2. clears/walks out of the facility;
3. moves to staging outside;
4. defaults to `HOLD_POSITION` until ordered/assigned.

## Ground vehicles

Use canonical factory production and:

```text
vehicle_spawn
vehicle_exit
vehicle_staging
```

Vehicle clears exit then stages/holds.

Do not release the next vehicle while the exit reservation remains blocked.

Human-controlled units receive priority per Section 3A.4.

## Air vehicles

Use selected GDI/Nod Air Pad/air factory and:

```text
aircraft_spawn
aircraft_staging
```

Aircraft clears launch/landing volume before next release.

Reuse canonical OpenW3D/TT VTOL/flying behavior.

## Commander selection

Support:

```text
single select
drag-box multi-select where appropriate
add/remove selection
selection by type/group
control groups where practical
```

Use batched waypoint/status-circle rendering.

Server validates which units the Commander can control.

## Orders

At minimum:

```text
MOVE
ATTACK_TARGET
ATTACK_MOVE
GUARD
HOLD_POSITION
PATROL
FOLLOW
REPAIR_TARGET when role supports it
RETREAT_TO_REPAIR
```

Use canonical Renegade/TT AI plus Feature 24 improvements.

No second SAGE AI simulation.

## Facility interruption

Power loss prevents new production and uses one documented canonical pause/cancel policy for active production.

Destruction cancels/resolves queue, applies documented refund rules, clears stale UI binding, and prevents invisible completion.

## Limits / performance

Configure:

```text
team infantry AI cap
team ground vehicle AI cap
team air AI cap
overall server AI budget
per-facility queue cap
```

Use Section 3A.6 for distant AI update-rate budgeting.

## UI feedback

Show queue states such as:

```text
queued
producing
blocked exit
waiting for power
population cap
insufficient credits
completed/staging
```

## Acceptance

- Barracks/Hand produces AI infantry that exits and holds;
- ground factory produces AI vehicle that clears exit and holds;
- Air Pad produces AI aircraft that clears pad;
- AI consumes no fake human player slots;
- Commander can select/order units;
- orders use canonical AI;
- dead/offline facilities cannot produce invalid units;
- production is bounded and does not persistently block human players.

---

# 46. SAVE/LOAD AND NETWORKING REQUIREMENTS

This corresponds to **Phase 40** in the implementation order.

## General rule

Gameplay state is server-authoritative.

Render-only caches/batches are reconstructed.

Do not serialize raw:

```text
GPU buffers
D3D resources
shader handles
render cache pointers
procedural construction block vertices
donor pointers
reconstructable AI pathfinder internals
```

Persist authoritative semantic state and rebuild transient runtime/render state.

## TT/OpenW3D

Preserve native OpenW3D/TT object serialization and network ownership.

## Terrain/world

Persist only runtime-modified terrain/road/bridge/water state that cannot be reproduced from normal map/configuration/generated data.

## Shroud/radar/intelligence

Persist exploration/last-known state only when the game mode requires it.

Rebuild building-provided communications sensor registrations from live buildings/descriptors after load.

Do not save strategic camera position as if it were sensor coverage.

## Building descriptor revisions

Save enough descriptor identity/revision information to diagnose/reconstruct a game whose live JSON tuning changed.

Do not serialize an entire parsed JSON document into every building.

On load, resolve the appropriate/current descriptor according to the selected compatibility policy and report incompatible material changes clearly.

## Finite power grid

Persist:

```text
stable PowerGridMemberId or deterministic equivalent where required
Commander manual enable/disable state
Commander power-priority overrides
```

Recompute after load:

```text
generation
requested demand
allocated demand
load-shed result
```

from surviving buildings and their effective descriptors.

Do not serialize a second independent powered/unpowered truth.

## Communications + Silo capabilities

On load:

- rebuild Communications Center/Advanced Communications Center sensor registrations;
- rebuild team Silo wallet caps;
- preserve existing player balances even when they currently exceed a reduced reconstructed cap.

## Commander state

Persist only durable state appropriate to the mode:

```text
Commander identity/permission where required
Commander/FPS mode where safe/useful
manual power controls
production queues
Commander AI control ownership
durable AI orders
control groups where useful
```

The physical Commander soldier remains ordinary player/soldier state.

## Building construction

Persist authoritative:

```text
CONSTRUCTING state
building definition
building transform
construction start/remaining time or normalized authoritative progress
construction seed
duration/config revision needed for deterministic reconstruction
queue/accounting state
```

Reconstruct procedural block meshes locally.

Late joiners receive compact construction state and regenerate the current stage.

## Commander AI production/orders

Persist:

```text
facility production queues
AI ownership/control association
durable Commander orders
guard/hold/patrol state as appropriate
population accounting inputs
```

Do not restore stale raw navigation pointers, transient reservations, or invalid path handles. Revalidate/replan.

## Building placement

The placement ghost is transient and not saved.

Only accepted real building/construction state is authoritative.

## Network authority

Server owns:

```text
Commander permission
building placement acceptance
construction completion
building power allocation
manual power controls
communications/Silo capability state
production purchases/queues
AI spawn/control/orders
shroud/radar authoritative intelligence
wallet-cap changes
```

Clients render UI/ghost/procedural visuals from replicated authoritative state.

## Acceptance

Save/load and late join preserve/reconstruct the complete RTS state without serializing renderer-only implementation details or granting clients gameplay authority.

---

# 47. DEDICATED SERVER RULES

This corresponds to **Phase 41**.

Dedicated server owns gameplay-semantic RTS state but never initializes client rendering/UI resources.

Server includes:

```text
TT gameplay/script behavior
WorldTerrainSystem gameplay queries
road/bridge traversal state
water gameplay/collision state
spatial queries
shroud/radar authoritative intelligence
AI state/path/order behavior
building semantic descriptors needed for gameplay
finite power grid
communications/Silo capabilities
Commander authorization/actions
building placement validation
authoritative construction state/timing
production queues
AI infantry/ground/air creation and orders
save/load/network state
```

Server excludes:

```text
Commander camera rendering
right-side GUI rendering
ghost rendering
procedural construction GPU meshes
weather rendering
surface-mark rendering
visual particles
status-circle GPU buffers
other client-only renderer resources
```

The server may load JSON descriptors and W3D-derived semantic/bounds data needed for gameplay without creating D3D resources.

## Acceptance

- dedicated server can host a complete Commander/RTS match;
- power, production, AI orders, construction timing, placement validation, wallet caps, and tactical intelligence remain authoritative;
- no renderer/UI dependency is pulled into server code merely to support Commander Mode.

---

# 48. TEST STRATEGY

Section 0B controls **when** tests are written and run. This section defines cumulative expected coverage and acceptance-gate assets.

Use `Code/Tests` only where tests provide meaningful regression protection.

## 48.1 Cumulative catalog, not a per-session checklist

The families below describe coverage expected by the owning feature's acceptance gate or final acceptance.

**Do not run, recreate, or re-prove every listed test after every edit.**

Rules:

- existing equivalent tests count;
- do not duplicate adequate coverage;
- normal development uses Section 0B targeted validation;
- run only tests directly relevant to current code;
- broader phase checks run when the phase is actually ready to close;
- full regression runs only at Section 0B.5 gates or after a concrete cross-cutting failure;
- after a focused PASS with no escalation trigger, continue implementation.

## 48.2 Expected cumulative automated coverage

### TT

```text
native script registry
script name lookup
TT script registration count/manifest
script save/load
TT event dispatch
TT collision groups
TT per-client visibility
TT network dirty state
TT building definitions
TT factory definitions
```

### Zero Hour / SAGE donor systems

```text
asset residency dependency retention
terrain sampling/raycast
near/far terrain LOD + seam continuity
far-terrain invalidation
road tessellation determinism
bridge generation/damage transition
foliage culling
water region queries
surface ribbon bounded memory
surface smudge batching/eviction
shadow cache invalidation
nearby-light query
particle/weather budgeting
weather device-resource recreation
effect/debris lifetime
spatial-index correctness
shroud state transitions
radar contact transitions
waypoint batching
composite attachment transforms
AI guard/retaliate transitions
AI path replan/stuck recovery
RTS telemetry determinism
```

### FPS-first Commander invariants

```text
Commander camera movement does not reveal hidden enemy
LAST_KNOWN does not follow unseen live transform
Commander body remains damageable
Commander death exits strategic mode
normal FPS HUD/gameplay remains ordinary outside Commander Mode
AI yield/stuck recovery around human obstruction
AI/update budgeting does not starve local-player simulation
```

### Building JSON / power / capabilities

```text
building JSON schema parse
W3D node resolution
complete hierarchy capture
anchor transform resolution
power definition parse
invalid power reload preserves last-known-good descriptor
live power reload triggers deterministic reallocation
multiple generators add correctly
overload load-shedding deterministic tie-break
last generator destruction powers down all dependents
consumer destruction can restore shed consumer
manual disable/priority override
communications online/offline coverage
advanced communications differing radius from same implementation
Silo bonuses stack for all team players
reduced wallet cap preserves already-earned balance
```

### Commander UI / placement

```text
BUILDINGS default sidebar
two-row page layout
context click -> GROUND_VEHICLES
context click -> AIR_VEHICLES
context click -> INFANTRY
offline/destroyed facility disables contextual production
power bars reflect authoritative state
ghost footprint generation
placement overlap
server/client placement agreement
invalid ghost red
```

### Procedural construction

```text
CONSTRUCTING gates all finished capabilities
deterministic block grid from seed
bottom-to-top row order
rotated building visual
block cap
compact network reconstruction
late-join reconstruction
save/load mid-construction
cancellation/destruction cleanup
footprint blocker -> final collision
PCT anchor spawn
MCT anchor spawn
power registration only at operational completion
```

### Commander AI production/control

```text
AI infantry from Barracks/Hand
AI ground vehicle from selected factory
AI aircraft from selected Air Pad/facility
produced unit clears exit/staging
no fake human player slot
single/multi-selection
MOVE
ATTACK_TARGET
ATTACK_MOVE
GUARD
HOLD_POSITION
PATROL
FOLLOW
REPAIR_TARGET where supported
RETREAT_TO_REPAIR
factory power-loss policy
factory destruction queue cleanup
population caps
```

## 48.3 Golden runtime scenarios

Maintain repeatable scenarios:

1. stock Renegade map;
2. TT-feature map;
3. terrain/road/bridge/water world;
4. foliage/particle/light/weather/smudge stress scene;
5. AI guard/retaliate/pathfinding arena;
6. Commander placement/construction sandbox;
7. finite-power overload/recovery sandbox;
8. contextual production sandbox with Barracks/ground factory/Air Pad;
9. mixed FPS-vs-Commander combat scenario verifying intelligence firewall and embodied Commander.

These are acceptance/regression assets, not per-edit obligations.

## 48.4 Test-result retention

For routine successful tests preserve only:

```text
command/target
PASS
date/commit when useful
```

Retain detailed logs only for failures, difficult regressions, CI artifacts, or explicit debugging evidence.

---

# 49. PERFORMANCE REQUIREMENTS

Section 0B.9 controls **when** measurements are taken.

Record an initial baseline and compare at performance-sensitive acceptance points, observed-regression investigations, **Phase 43**, and final acceptance.

Do **not** run a full performance pass after every ordinary feature/edit.

Metrics available when relevant:

```text
frame time
server tick time
draw calls/submissions
visible object count
particle count
foliage count
dynamic lights
shadow updates
asset memory
terrain memory
far-terrain rebuild time
surface mark count/memory
weather particle count
script count
AI active count
AI decision updates/sec
AI path replans/sec
AI production queue depth
blocked factory/exit count
power-grid recomputation time
power generator/consumer count
Commander UI update cost
procedural construction block/buffer cost
spatial query time
building placement query time
network bytes/sec
telemetry overhead
```

## FPS priority rule

A large Commander battle is unacceptable if it materially degrades ordinary first-person responsiveness.

When budgets are exceeded, reduce/stagger lower-priority work first:

```text
distant cosmetic effects
weather/ambient work
off-screen Commander overlay refresh
distant AI decision frequency
distant path-replan frequency
telemetry aggregation
```

Do not reduce authoritative local combat/movement correctness merely to preserve maximum distant RTS update frequency.

For a feature-specific check, measure only plausibly affected metrics unless evidence suggests a broader regression.

The coding agent must not "optimize" by deleting required gameplay.

---

# 50. REQUIRED DOCUMENTATION OUTPUT

Maintain:

```text
docs/integration/OpenW3DBaseline.md
docs/integration/BaselineKnownIssues.md
docs/integration/THIRD_PARTY_PROVENANCE.md

docs/tt484/TTParityMatrix.md
docs/tt484/NativeScriptArchitecture.md

docs/zerohour/ZeroHourDonorBaseline.md
docs/zerohour/ZeroHourCapabilityMatrix.md
docs/zerohour/OpenW3DPortMatrix.md
docs/zerohour/WorldTerrainSystem.md
docs/zerohour/RendererDonorIntegration.md
docs/zerohour/SurfaceMarkSystem.md
docs/zerohour/WeatherEnvironmentRenderer.md
docs/zerohour/ZeroHourAIAdaptation.md
docs/zerohour/RTSTelemetry.md

docs/commander/CommanderMode.md
docs/commander/FPSFirstGameplayInvariants.md
docs/commander/CommanderUI.md
docs/commander/BuildingDescriptorSchema.md
docs/commander/BuildingAnchorRoles.md
docs/commander/BuildingPlacement.md
docs/commander/BuildingConstruction.md
docs/commander/PowerGrid.md
docs/commander/StrategicBuildingCapabilities.md
docs/commander/AIProductionAndOrders.md
```

`CommanderUI.md` must document:

```text
BUILDINGS
GROUND_VEHICLES
AIR_VEHICLES
INFANTRY
context-switch rules
power bars
facility binding
queue/state presentation
```

`PowerGrid.md` must document:

```text
JSON power schema
generation/demand formula
load-shedding ordering
manual overrides
canonical building power-state integration
hot reload
save/load/network behavior
```

`BuildingConstruction.md` must document the deterministic procedural block visual and compact state reconstruction.

---

# 51. REQUIRED CLEANUP BEFORE FINAL ACCEPTANCE

Remove from the normal runtime path:

- stock dynamic `SCRIPTS.DLL` bootstrap;
- `scripts2.dll` fallback requirement;
- TT executable-address tables;
- TT binary patch/trampoline code;
- TT D3D8-to-D3D9 proxy architecture;
- temporary old/new subsystem toggles;
- duplicate TT/OpenW3D class declarations;
- unused SAGE global/singleton scaffolding;
- temporary donor/OpenW3D comparison implementations;
- all temporary migration shims and donor-version selectors;
- legacy `ScriptCommands` DLL function-table plumbing after direct native script calls are complete;
- any duplicate stock/TT implementation of the same behavior or script;

Do **not** remove:

- native script registry;
- public TT APIs that remain meaningful as native OpenW3D APIs;
- development debug overlays;
- provenance records;
- tests.

---

# 52. FINAL DEFINITION OF DONE

The project described by this roadmap is complete only when all of the following are true.

## TT first

1. OpenW3D boots with no `scripts2.dll`.
2. OpenW3D boots with no required TT engine-patch DLL.
3. Stock scripts not superseded by TT are built into the native registry.
4. TT-modified/replacement scripts exist as one merged canonical implementation; TT-only scripts are added natively.
5. TT 4.8.4 engine-facing functionality is represented in `TTParityMatrix.md`.
6. No unexplained TT parity gaps remain.
7. TT hooks use native source dispatch rather than executable patches.
8. TT extended game objects/buildings/factories work.
9. TT network/visibility/dialog/purchase/damage/player APIs work.
10. TT collision semantics are preserved using WWPhys/OpenW3D.
11. Client, server, and affected tools build.
12. Every TT replacement has exactly one canonical compiled/reachable implementation.
13. No built-in script uses the historical `ScriptCommands` DLL function-pointer abstraction.
14. Duplicate script names/factories are a build/test failure.

## Zero Hour / SAGE donor systems

15. Pinned TheSuperHackers Zero Hour (`Core + GeneralsMD`) is the sole SAGE implementation donor and exact SHA is documented.
16. `Generals/` is not used as implementation donor or predecessor staging pass.
17. No relevant scoped Zero Hour engine capability remains `UNREVIEWED`.
18. Applicable TheSuperHackers fixes/enhancements are classified/integrated.
19. Asset residency is integrated.
20. Sectional bridges are integrated.
21. Runtime roads are integrated.
22. Hybrid terrain uses useful Zero Hour responsibility separation without parallel terrain engines.
23. Runtime terrain texturing is integrated.
24. Foliage batching is integrated.
25. Water/rivers are integrated.
26. Projected/cached shadows are integrated.
27. Terrain tracks/ribbons are integrated.
28. Particle batching is integrated.
29. Dynamic-light filtering is integrated.
30. Tracer/beam/projectile modules are integrated.
31. Debris rendering is integrated.
32. One canonical shader-manager architecture is integrated.
33. RTS shroud/fog-of-war is integrated.
34. Radar is integrated.
35. Waypoint/status-circle batching is integrated.
36. Modular draw components/generalized composite attachments are integrated.
37. Spatial query improvements are integrated.
38. Debug rendering tools are integrated.
39. Far/background terrain LOD is integrated.
40. Surface smudge/decal management is integrated.
41. Weather/environment particles are integrated.
42. Selected Zero Hour AI state/path/guard-retaliation improvements are canonicalized into Renegade/TT AI.
43. RTS telemetry/statistics instrumentation exists without Academy advice UI.
44. No predecessor/staging SAGE implementation remains beside the canonical result.

## FPS-first Commander invariants

45. Normal FPS movement, aiming, weapons, vehicles, repair, PCT/MCT interaction, infiltration, and HUD remain coherent.
46. Commander camera movement alone reveals no hidden enemy.
47. Strategic intelligence does not become FPS wallhacks.
48. Commander uses the same vulnerable physical soldier body.
49. Commander death exits strategic mode.
50. Human players receive practical right-of-way priority over Commander AI.
51. AI/strategic budgets cannot routinely starve local FPS responsiveness.
52. World APIs remain suitable for infantry-scale and vehicle/RTS-scale procedural generation.
53. Commander actions create physical FPS objectives where a physical equivalent exists.

## Building JSON / power / strategic capabilities

54. W3D + JSON resolves to one complete validated semantic descriptor.
55. Power generation, consumption, requires-power, and default priority come from building JSON.
56. JSON power/capability values support validated server-authoritative runtime reload.
57. Invalid runtime reload preserves last known-good values.
58. Multiple Power Plants add finite generation.
59. Excess demand deterministically load-sheds buildings.
60. Destroying the final generator powers down every surviving dependent building.
61. Destroying/removing a consumer can restore a shed building.
62. Existing Renegade/TT destroyed/powered-down building behavior remains canonical.
63. Commander can manually disable an eligible consumer and override/clear priority.
64. Communications Center and Advanced Communications Center use one data-driven sensor implementation.
65. Communications coverage follows online/power state and obeys the intelligence firewall.
66. Tiberium Silo increases wallet capacity for every player on its team.
67. Multiple Silo bonuses stack.
68. Losing Silo capacity never deletes already-earned credits above the new cap.

## Commander UI / placement / construction

69. Commander Mode enters/exits normal play cleanly.
70. Commander cursor raycasts terrain and arbitrary world geometry.
71. Right-side Commander sidebar exists with a two-row visible cell layout and paging.
72. `BUILDINGS`, `GROUND_VEHICLES`, `AIR_VEHICLES`, and `INFANTRY` modes exist.
73. Building cards show costs/availability and power effects.
74. Commander power generation/consumption/overload is visible.
75. Clicking an allied ground factory opens its ground-vehicle context.
76. Clicking an allied Air Pad/air factory opens its air-vehicle context.
77. Clicking Barracks/Hand opens its infantry context.
78. Existing Purchase Terminal icons are reused where available.
79. Building ghost uses the real model.
80. Invalid placement shades the ghost red.
81. Server revalidates placement.
82. Accepted building enters authoritative `CONSTRUCTING`.
83. Construction displays batched procedural blocks from foundation upward.
84. Procedural construction is deterministic and compactly replicated.
85. Late join/save-load reconstructs current construction stage.
86. Finished-building functionality stays disabled until completion.
87. Completion safely swaps footprint blocker to canonical final collision.
88. PCTs auto-spawn from W3D semantic nodes.
89. MCTs auto-spawn/bind from W3D semantic nodes.
90. Completed buildings use canonical TT/OpenW3D classes/capabilities.

## Commander AI production/control

91. Infantry purchases create normal AI SoldierGameObjs, not fake human players.
92. Ground vehicles come from the selected physical canonical factory.
93. Air units come from the selected physical Air Pad/air facility.
94. Produced units clear exits/pads and stage without persistently blocking humans or later production.
95. Commander can select produced AI.
96. Commander can issue MOVE, ATTACK_TARGET, ATTACK_MOVE, GUARD, HOLD_POSITION, PATROL, FOLLOW, and applicable repair/retreat orders.
97. Orders use canonical Renegade/TT AI, not a second SAGE simulation.
98. Unpowered/destroyed production buildings cannot create invalid units.
99. AI population/production is bounded.
100. Save/load/networking preserves/reconstructs durable production/order state.

## Architecture / final quality

101. Existing OpenW3D D3D9 renderer remains the renderer foundation.
102. No D3D8 proxy DLL is reintroduced.
103. OpenW3D/WWPhys remains the physics system.
104. No Jolt or prior replacement-physics plan is present.
105. No requested system has a permanent legacy/new or donor-version runtime selector.
106. Dedicated server has no accidental renderer/UI dependencies.
107. Stock Renegade content still works.
108. Performance/memory regressions are measured/addressed at designated gates.
109. Tests/provenance/documentation are complete.
110. All four persistent project-state files accurately reflect final repository state.

---

# 53. IMPLEMENTATION ORDER SUMMARY

Execute in this dependency order.

The ordering is deliberate:

- TT is canonical before anything depends on its engine behavior;
- shared world/render/AI/intelligence systems exist before Commander consumes them;
- building JSON exists before power/capabilities;
- power/capabilities exist before Commander UI displays/controls them;
- camera/UI exist before placement/production UX;
- placement exists before construction;
- construction exists before Commander-created production buildings participate in AI production;
- persistence/network/dedicated-server/regression passes happen after the gameplay stack exists.

```text
0   Freeze/test latest OpenW3D
1   Import + inventory TT 4.8.4
2   Reconcile TT public engine classes into canonical OpenW3D classes
3   Merge TT hooks/patches/overrides into canonical OpenW3D implementations
4   Build native stock + TT script registry
5   Port remaining TT 4.8.4 engine-facing functionality
6   TT compatibility/cleanup/testing HARD GATE

7   Freeze TheSuperHackers Zero Hour donor; complete capability/community-enhancement/OpenW3D-port matrices
8   Asset residency
9   Shader manager
10  Spatial query/index layer
11  Zero Hour-derived hybrid terrain architecture
12  Zero Hour far/background terrain LOD
13  Terrain texture system
14  Runtime roads
15  Sectional bridges
16  Foliage/tree batching
17  Water/rivers
18  Terrain tracks/ribbons
19  Zero Hour surface smudge/decal manager
20  Projected/cached shadows
21  Dynamic-light filtering
22  Particle batching
23  Zero Hour weather/environment particles
24  Tracer/beam/projectile rendering
25  Debris rendering
26  Modular draw components + generalized Zero Hour composite attachments
27  Shroud/fog-of-war + sensor-source API + intelligence firewall support
28  Shared radar/tactical intelligence
29  Waypoint/status-circle renderers
30  Zero Hour AI state/path/guard-retaliation improvements + human-yield/update-budget hooks
31  Zero Hour-derived RTS telemetry/statistics
32  Debug rendering tools

33  Building W3D semantic JSON/descriptors + live descriptor reload
34  Finite team power grid + communications/Silo strategic capabilities
35  Embodied Commander Mode camera/view
36  Commander right-side UI + BUILDINGS/GROUND/AIR/INFANTRY contexts + power bars
37  Ghost building + server placement validation
38  Authoritative construction + procedural block build-up + final activation + automatic PCT/MCT
39  Commander AI infantry/ground/air production + selection + orders

40  Full save/load + multiplayer pass
41  Dedicated-server pass
42  Tool/editor/descriptor-validation pass
43  Performance/regression pass
44  Remove migration/donor-version scaffolding
45  Final provenance/documentation/state-file reconciliation
```

No later phase may be used as an excuse to leave an earlier phase knowingly architecturally broken.

---

# 54. CODING-AGENT OPERATING LOOP

Section 0A governs persistent project state. Section 0B governs validation scope and token/time discipline.

## 51.1 Normal implementation loop

For ordinary implementation work:

```text
0. read/reconcile completed_features.md, WIP_features.md, unstarted_features.md, and awaiting_answers.md;
1. inspect the current target code needed for the task;
2. inspect the relevant donor/reference code needed for the task;
3. update the relevant parity/donor matrix when donor decisions materially change;
4. move/start the exact feature in WIP_features.md;
5. implement the smallest canonical architecture;
6. update WIP_features.md whenever implementation direction, blocker, or next action materially changes;
7. build only the directly affected target(s), following Section 0B.7;
8. run only directly relevant inexpensive existing tests, if applicable;
9. perform one focused smoke/runtime check only when useful for changed runtime behavior;
10. if those checks pass and no Section 0B.3 escalation trigger exists, STOP VALIDATING and continue implementation;
11. do not run full regression, all golden scenarios, broad benchmarks, or unaffected builds merely because work occurred;
12. do not create tests by default unless Section 0B.6 justifies them;
13. continue implementing related WIP until the feature/phase is genuinely ready for its acceptance gate or the session must end.
```

## 51.2 Acceptance-gate loop

When an independently completable feature or phase is genuinely ready to move to COMPLETE:

```text
1. review the phase's actual acceptance criteria;
2. run the tests specifically required for that completed scope;
3. build the broader target set required by that gate, but no broader;
4. run the applicable focused golden/runtime scenario if the phase requires one;
5. resolve failures using Section 0B's narrow-to-broad escalation policy;
6. remove temporary migration/debug scaffolding that is no longer required;
7. record concise acceptance evidence;
8. commit the feature/phase in a runnable state when repository workflow permits;
9. move the lifecycle entry from WIP_features.md to completed_features.md only after the applicable gate passes.
```

Acceptance criteria are closure requirements, not instructions to rerun the same validation after every edit.

## 51.3 Broad regression loop

The complete regression matrix is reserved for the gates named in Section 0B.5 or for a documented concrete cross-cutting failure.

At those gates:

```text
1. run the specifically required broad build/test matrix;
2. run the applicable golden scenarios;
3. take the performance measurements required by Section 49;
4. diagnose failures from the narrowest relevant evidence first;
5. avoid repeatedly rerunning already-passing expensive suites when unchanged;
6. record final gate evidence succinctly.
```

## 51.4 Session-end loop

Before every intentional pause, handoff, context reset, or session end:

```text
1. stop new implementation work;
2. inspect git status and the actual changed files;
3. reconcile completed_features.md;
4. reconcile WIP_features.md, including the next exact action;
5. reconcile unstarted_features.md;
6. reconcile awaiting_answers.md;
7. run tools/validate_feature_state.py when available because it is cheap bookkeeping validation;
8. do NOT launch a broad build/test pass merely because the session is ending;
9. record the final targeted build/test state already obtained during the session;
10. perform the complete Section 0A.9 reconciliation;
11. only then end or hand off the session.
```

A session ending is **not** itself a validation-escalation trigger.

If the actual latest OpenW3D tree already contains part of a requested feature, **extend and canonicalize that code rather than replacing it merely because Zero Hour contains another implementation**.

The objective is one coherent FPS-first Renegade engine with TT 4.8.4 behavior, the selected Zero Hour/SAGE capabilities from TheSuperHackers `Core + GeneralsMD`, applicable community fixes/enhancements, the finite JSON-driven building power/capability system, the full contextual Commander UI, procedural building construction, and server-authoritative Commander AI production/control.
