# OpenW3D / Renegade Persistent Project Instructions for Claude Code

This file contains Claude-specific execution rules. **Read `PROJECT_RULES.md` first.** The shared rules, active roadmap, and explicit user decisions are authoritative; this file only refines Claude Code behavior and must not contradict them.

## Mandatory bootstrap

At a fresh session, after context compaction/restoration, or after substantial context loss:

1. read `PROJECT_RULES.md` completely;
2. determine the active roadmap stage using `PROJECT_MANIFEST.toml` if present, otherwise the roadmap prerequisite/state rules in `PROJECT_RULES.md`;
3. read the active roadmap and any prerequisite roadmap needed for context;
4. read `completed_features.md`, `WIP_features.md`, `unstarted_features.md`, and `awaiting_answers.md`;
5. read `DECISIONS.md`, relevant ADRs, ownership/authority docs, and validation map when present;
6. inspect Git status/HEAD;
7. resume existing WIP before unrelated work unless dependency order requires otherwise.

Claude's internal todo list, context summary, plan, memory, or previous conversation does not replace repository state.

## Claude execution behavior

- Use targeted searches and narrow reads before opening large files or source trees.
- Prefer symbol-level/source-region inspection over repeatedly dumping entire donor files into context.
- Summarize donor/reference behavior internally and retain only facts needed for the current implementation.
- Do not repeatedly prove already-green code.
- Do not recursively audit unrelated systems after the focused implementation check passes.
- Do not turn a coding task into an open-ended test-writing or architecture-research project.
- Do not stop at analysis when the requested change is safe and sufficiently specified to implement.
- Do not silently alter roadmap scope, dependency order, authority, or user decisions.
- Follow concrete roadmap architecture rather than inventing alternatives unless source evidence reveals a real conflict.
- Investigate repository/runtime/reference data before asking the user.
- Do not guess content/preset/model identifiers that can be enumerated or dumped from runtime data.
- Do not create parallel managers/services merely because the existing owner is inconvenient; establish canonical ownership first.
- Do not perform opportunistic refactors, renames, formatting sweeps, modernization, or dependency replacement outside the active work item.

## Blocking questions

Only create a `Q-###` when continuing would require guessing a genuine user/architecture decision that cannot be resolved from repository evidence, runtime introspection, generated manifests, existing decisions, or canonical donor/reference material.

A blocking question must state:

- the exact unknown;
- what work item it blocks;
- what evidence was checked;
- the smallest user decision needed.

Write a genuine blocker to `awaiting_answers.md` immediately so it cannot be lost. Do not use that file as a general decision log. Resolved durable decisions belong in `DECISIONS.md` or an ADR when those files are available.

## Work readiness

Before substantial implementation, mentally apply the shared Definition of Ready from `PROJECT_RULES.md`:

- dependency gate satisfied;
- canonical owner identified;
- existing implementation searched;
- applicable TT / Zero Hour behavior understood;
- authority, persistence, networking, determinism, failure, missing-asset, and acceptance-gate requirements known where relevant.

Unknown implementation detail is not automatically a blocker. Continue when source evidence makes the required behavior clear.

## Implementation discipline

- Prefer the smallest canonical change that satisfies the active roadmap item.
- Preserve FPS responsiveness and player control priorities described by the roadmap.
- Preserve server authority for gameplay state.
- Preserve deterministic behavior where required.
- Reuse shared physical/geometry constants instead of inventing local copies.
- Missing art never blocks engine implementation; request the intended `ow_` asset and document the requirement while writing the code that constrains it.
- Where runtime content data such as `always.dat` is needed, prefer a reusable definition/content dumper over hard-coded guesses or repeated manual questions.

## Validation economy

Follow `PROJECT_RULES.md` and the active roadmap validation section exactly.

Normal behavior:

```text
implement
-> build directly affected target(s)
-> run directly relevant cheap existing checks when useful
-> perform one focused smoke check when runtime behavior warrants it
-> if green and no escalation trigger applies: STOP VALIDATING AND CONTINUE IMPLEMENTING
```

Do not run broad regression, all targets, all golden scenarios, broad benchmarks, or exhaustive new tests merely for reassurance. Escalate only for concrete failures, designated hard gates, or genuinely cross-cutting/high-risk changes.

## State-write protocol

Repository lifecycle state is reconciled **once immediately before handoff**, not continuously after every task/subtask/build.

The normal mid-session exception is a newly discovered blocking `Q-###` in `awaiting_answers.md`.

At handoff:

1. inspect relevant Git status/diff;
2. update `completed_features.md` with concise completed entries;
3. update `WIP_features.md` with concise current state and exact next action;
4. delete corresponding lines from `unstarted_features.md` only when conservation is satisfied;
5. reconcile open questions and durable decisions;
6. run the cheap state validator if available;
7. do not start broad validation simply because the session is ending.

Keep the exactly-one-lifecycle-state invariant. Nothing disappears silently.

## Completion rule

Never mark an item COMPLETE merely because code exists. Apply the shared Definition of Done from `PROJECT_RULES.md`, including canonical ownership, applicable authority/determinism/persistence behavior, safe fallback behavior, targeted build/validation, acceptance gate, docs/assets/decision updates, and lifecycle reconciliation.

## Git safety

- Preserve unrelated user changes.
- Never reset/rewrite/discard user work unless explicitly instructed.
- Keep generated/temp/debug artifacts out of commits unless intentionally required.
- Before handoff, verify that repository behavior, roadmap requirements, docs, and state records agree.
