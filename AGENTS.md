# OpenW3D / Renegade Project Instructions for Codex

This root `AGENTS.md` contains Codex-specific execution rules. **Read `PROJECT_RULES.md` first.** The shared rules, active roadmap, and explicit user decisions are authoritative; this file only refines Codex behavior and must not contradict them.

A nested `AGENTS.md` may refine local implementation workflow, file conventions, build commands, or component-specific constraints, but it may not override root/shared architecture, donor rules, roadmap requirements, authority, dependency order, or explicit user decisions unless the user explicitly changes them.

## Mandatory bootstrap

Before modifying code in a fresh session or after meaningful context loss:

1. read `PROJECT_RULES.md` completely;
2. determine the active roadmap stage using `PROJECT_MANIFEST.toml` if present, otherwise the roadmap prerequisite/state rules in `PROJECT_RULES.md`;
3. read the active roadmap and any prerequisite roadmap needed for context;
4. read `completed_features.md`, `WIP_features.md`, `unstarted_features.md`, and `awaiting_answers.md`;
5. read `DECISIONS.md`, relevant ADRs, ownership/authority docs, and validation map when present;
6. inspect current Git status/HEAD;
7. resume existing WIP before starting unrelated work unless dependency order requires otherwise.

Repository state is authoritative over prior chat/session assumptions. Internal plans/todos are temporary helpers only.

## Codex execution behavior

- Inspect only repository areas relevant to the active task before broad searches.
- Use `rg` and targeted symbol/code searches before opening large files or trees.
- Prefer small, reviewable canonical changes over speculative rewrites.
- Do not stop at analysis when the requested change can be implemented safely from existing evidence.
- Do not silently alter roadmap scope, stage order, architecture, or user decisions.
- Research source/content/runtime questions before asking the user.
- Do not guess preset/model/definition identifiers that can be enumerated.
- Establish the existing canonical owner before creating a new manager/service/registry/controller.
- Do not add a parallel implementation just to avoid modifying an existing owner.
- Do not perform unrelated refactors, renaming, formatting sweeps, modernization, dependency replacement, or test-suite expansion while implementing a roadmap item.
- Keep edits scoped to what is required for the active acceptance gate and concrete blocking defects.

## Blocking questions and decisions

Use `awaiting_answers.md` for unresolved user-blocking questions only.

Create a `Q-###` only when continuing requires a genuine user/architecture decision that cannot be resolved from:

```text
repository/source
-> generated manifests/content inventories
-> runtime introspection
-> canonical donor/reference material
-> existing decision records
-> user
```

A blocker entry must identify the exact unknown, affected work item, evidence already checked, and smallest decision required.

When resolved, record durable decisions in `DECISIONS.md` or `docs/adr/` when present rather than leaving answered questions mixed with open blockers.

## Work readiness

Before substantive implementation, apply the shared Definition of Ready in `PROJECT_RULES.md`.

At minimum establish, where applicable:

- dependency gate;
- canonical owner;
- existing implementation search result;
- applicable TT / Zero Hour donor behavior;
- server/client authority;
- persistence/network impact;
- deterministic contract;
- fallback/missing-asset behavior;
- acceptance gate.

Do not elevate ordinary implementation uncertainty into a user blocker when source evidence can resolve it.

## Implementation discipline

- Extend/canonicalize OpenW3D rather than importing duplicate SAGE ownership structures.
- Preserve FPS-first movement, world geometry, interiors, tunnels, and human-player priority.
- Preserve server authority for gameplay state.
- Preserve deterministic behavior where required by the roadmap/shared contract.
- Reuse centralized units/geometry/clearance constants rather than introducing local equivalents.
- Missing assets do not block code; request/document the intended `ow_` asset and provide a safe absent-asset behavior.
- For content identifiers stored in `always.dat` or similar runtime data, prefer reusable dump/introspection tooling and machine-readable manifests over guessed hard-coded strings.

## Validation economy

Follow `PROJECT_RULES.md` and the active roadmap validation section.

During normal implementation:

1. build only directly affected targets;
2. run directly relevant cheap existing tests when useful;
3. run one focused smoke check when runtime behavior warrants it;
4. if green and no escalation trigger applies, stop validating and continue implementation.

Do not automatically run full regression, every golden scenario, every target, broad benchmarks, or unrelated tests. Do not repeatedly rerun unchanged passing tests. Do not create tests by default for trivial wrappers/getters/forwarders or behavior already adequately covered.

For failures, inspect the smallest relevant error region first. Do not read giant successful logs line-by-line.

## Session-state protocol

Do not continuously mutate lifecycle files during implementation. Keep progress in working context and reconcile repository state once immediately before intentional handoff.

The normal mid-session exception is a newly discovered blocking `Q-###`.

Before handoff:

- inspect relevant Git status/diff;
- reconcile `completed_features.md`;
- reconcile `WIP_features.md` with the exact next action for partial work;
- reconcile `unstarted_features.md` while obeying the conservation rule;
- reconcile `awaiting_answers.md` and decision records;
- run the cheap feature/project-state validator when available;
- record targeted build/test evidence already obtained;
- do not launch broad regression merely because the session is ending.

Never mark an item complete merely because code was written. Apply the shared Definition of Done and the item's applicable acceptance gate.

## Git discipline

Before declaring work complete or handing off:

- inspect `git status` and relevant diff;
- ensure generated/temporary/debug files are not accidentally included;
- preserve unrelated user changes;
- do not reset, rewrite, discard, or overwrite user work unless explicitly instructed;
- ensure repository state, active roadmap, docs, decisions, and lifecycle files agree.


<!-- headroom:memory-instructions -->
## Memory

Use the `headroom_memory` MCP server for persistent cross-session knowledge.

**Before** answering questions about prior decisions, conventions, project context,
architecture, user preferences, org info, codenames, debugging history, or anything
from past sessions — call `memory_search` first.

**After** making durable decisions, discovering conventions, or learning important
facts — call `memory_save` to persist them for future sessions.

Memory is your first source of truth for anything not visible in the current conversation.
