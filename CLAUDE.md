# OpenW3D / Renegade Persistent Project Instructions for Claude Code

This file is a compact persistent-memory bootstrap. The full specification lives in `openw3d_tt484_zerohour_commander_integration_roadmap_v1.1.0.md`.

After a new session, context compaction, context restoration, handoff, or substantial loss of working context, reread this file, the roadmap, and all four project-state files before continuing.

## Always read before coding

1. `openw3d_tt484_zerohour_commander_integration_roadmap_v1.1.0.md`
2. `completed_features.md`
3. `WIP_features.md`
4. `unstarted_features.md`
5. `awaiting_answers.md`

The four state files are persistent project memory. Claude's internal todo list, plan, summary, or conversation context does not replace them.

## Non-negotiable architecture

- TT 4.8.4 is integrated first and natively.
- Where TT patched, hooked, overrode, or replaced stock behavior, merge the TT result into the canonical OpenW3D owner and remove the superseded runtime path.
- Never keep duplicate stock-vs-TT implementations selected by flags or wrappers.
- Zero Hour donor means **TheSuperHackers `GeneralsGameCode` configured as `Core + GeneralsMD`**.
- Do not treat `Generals/` as a donor stage or predecessor implementation source.
- EA `GeneralsMD` may be consulted only when historical shipped-Zero-Hour behavior needs validation.
- OpenW3D remains the destination architectural authority.
- Keep OpenW3D/WWPhys; no Jolt/replacement-physics migration.
- Preserve Renegade FPS world geometry and interiors.
- **No network compatibility with other clients is required.** OpenW3D clients and servers only ever talk to each other, so the wire protocol, packet layout, net class ids, and app packet types may be changed freely. Never constrain a design, keep a legacy field, or add a version/compat path to stay interoperable with stock Renegade, TT 4.8.4, or any other build. Asset compatibility (models, maps, textures, `.w3d`, `always.dat`) is a separate rule and still stands.
- Do not reintroduce historical TT DLL-hook/proxy architecture or an unnecessary binary plugin compatibility layer.

## Missing art is not a reason to skip code

Textures, models and other assets that do not exist yet never block engine work. Write the
system, have it name the asset it wants, make the absent case behave sanely (untextured
geometry, a layer with a rule and no content, a model slot that draws nothing -- never a
crash, a blocking error, or a stand-in file checked into the tree), and record the asset in
`docs/assets/`.

One markdown file per system, listed in `docs/assets/README.md`. Each entry gives the exact
name the code passes to the asset manager, what the thing is in words an artist can act on,
where in the code it is read, what happens today without it, and the constraints the
geometry imposes -- tiling, projection, aspect, UV layout. Write the constraints while the
code that imposes them is being written; that is the only moment they are obvious.

Invented names are prefixed `ow_` so nothing we add can shadow a stock Renegade asset.

## Claude-specific anti-drift rules

- Do not spend a session repeatedly proving already-green code.
- Do not turn an implementation task into an open-ended test-writing project.
- Do not recursively audit unrelated subsystems after a focused check passes.
- Do not repeatedly reread the same large files/logs unless new evidence requires it.
- Prefer targeted searches and narrow file reads over dumping whole source trees into context.
- Summarize large donor files internally and retain only symbols/behavior needed for the active task.
- When the roadmap gives a concrete architecture or precedence rule, follow it instead of generating alternatives unless a real conflict is discovered.
- If a genuine architectural/user decision is blocked, write a precise `Q-###` item to `awaiting_answers.md`. Do not use questions as a substitute for source investigation.

## Mandatory current Commander/RTS scope

The authoritative roadmap explicitly includes all of the following. Do not collapse them into a generic "Commander Mode" task:

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

Follow the roadmap dependency order exactly:

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

## Validation economy: mandatory

Follow roadmap Section 0B.

Normal mode is:

```text
implement
-> build directly affected target(s)
-> run directly relevant cheap existing test(s), if useful
-> perform one focused smoke check when runtime behavior warrants it
-> if PASS and no escalation trigger exists: STOP VALIDATING AND KEEP IMPLEMENTING
```

A green targeted result is sufficient evidence to continue.

Do not automatically build client + server + tools together, run full regression, run all golden scenarios, benchmark every change, add exhaustive unit tests, inspect successful logs line-by-line, or rerun unchanged passing suites for reassurance.

Treat roughly 10-15% of an ordinary healthy session as a soft upper budget for validation. This is not a clock requirement; implementation should dominate effort when nothing is failing.

Escalate only for concrete failures/high-risk changes listed in Section 0B or when closing an acceptance/hard gate.

Test-first development is not the default. Add tests when they protect meaningful deterministic behavior, a real regression, serialization/network contracts, critical ownership/concurrency behavior, or an explicitly required roadmap invariant.

## Persistent status protocol

**This section overrides roadmap Section 0A wherever the two disagree.** Roadmap 0A
demands continuous updating and verbose per-entry templates. Do not follow that. Do
not "restore" 0A's behavior in a later session; this section is the current rule.

### When to write the state files

Read all four at session start. Then **write them only once, immediately before
handing control back to the user.**

Do not touch them after finishing an individual task, phase, build, or subtask. Do
not update them mid-turn to reflect progress. Keep progress in working context and
flush it in one pass at the end.

The only mid-session exception is `awaiting_answers.md`: add a `Q-###` as soon as a
genuine blocking question appears, since the user needs it to act.

### How much to write

Terse. These are status markers, not reports. Detail belongs in `docs/`.

- `completed_features.md` — **1-2 sentences** per finished item. What got done, and
  the evidence in a few words. Nothing else.
- `WIP_features.md` — **2-3 sentences max** per in-flight item: what was started,
  where it stands, the next exact action.
- `unstarted_features.md` — **delete-only.** It is pre-populated with every roadmap
  work item as one line each. Your only operation on this file is removing lines
  that are done. Never rewrite it, reformat it, add prose, or regenerate it.
- `awaiting_answers.md` — only unresolved questions needing the user. Move answered
  ones to a Resolved section.

Do not write per-entry field templates, evidence dumps, build logs, tables, or
rationale into these four files. Long-form findings go in `docs/` and are referenced
by one short line.

### Conservation rule: nothing disappears silently

**This is the most important rule in this section. Work items may only move between
files, never vanish.**

1. A line may be removed from `unstarted_features.md` **only if** an equivalent entry
   is added, in the same pass, to `WIP_features.md` or `completed_features.md`.
2. An entry may be removed from `WIP_features.md` **only if** an equivalent entry is
   added, in the same pass, to `completed_features.md`.

The only permitted third destination is an item that is genuinely **not applicable**
— impossible, superseded, or cancelled by the user. That still gets a one-line entry
in `completed_features.md` marked `N/A:` with the reason. Never delete it outright.

Because WIP and completed entries are terse, one entry may cover several
`unstarted_features.md` lines — but it must **name** the items it absorbs, so a later
session can confirm nothing was dropped. A WIP entry that silently covers work its
text does not mention is a violation.

Before finishing the pass, check the arithmetic: every line deleted from the backlog
this session should be traceable to a specific entry in WIP or completed. If you
cannot point to where a removed line went, restore it.

### Before handing control back

1. move finished work to `completed_features.md` (1-2 sentences);
2. leave partial work in `WIP_features.md` (2-3 sentences, with next exact action);
3. delete completed lines from `unstarted_features.md`, obeying the conservation
   rule above;
4. update/resolve `awaiting_answers.md`;
5. do not start broad tests merely because the session is ending.

Keep the exactly-one-status invariant: a tracked item lives in exactly one of the
three lifecycle files.
