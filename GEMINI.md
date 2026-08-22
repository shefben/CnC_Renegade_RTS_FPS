# OpenW3D / Renegade Project Context for Gemini CLI

Use this file as repository-level Gemini context. The authoritative project specification is `openw3d_tt484_zerohour_commander_integration_roadmap_v1.1.0.md`.

Whenever a session begins or context is refreshed/compacted, load the roadmap and persistent project-state files before changing code.

## Required context load order

1. `openw3d_tt484_zerohour_commander_integration_roadmap_v1.1.0.md`
2. `completed_features.md`
3. `WIP_features.md`
4. `unstarted_features.md`
5. `awaiting_answers.md`

Do not infer current implementation status from old conversation text when these repository files can answer it.

## Architectural constraints

- Implement TT 4.8.4 natively before SAGE/Zero Hour donor work.
- TT replacement/patch/hook semantics must end as one canonical OpenW3D implementation, not parallel stock and TT paths.
- The Zero Hour donor is **TheSuperHackers `GeneralsGameCode`, specifically `Core + GeneralsMD`**.
- Do not use TheSuperHackers `Generals/` as an implementation donor.
- EA `GeneralsMD` is optional historical validation only.
- OpenW3D owns the final architecture and naming.
- Keep OpenW3D/WWPhys. Do not introduce Jolt/replacement physics.
- Preserve Renegade FPS map geometry, buildings, interiors, tunnels, and arbitrary W3D geometry.
- Do not import obsolete DirectX8/proxy/hook architecture merely because donor code historically used it.
- Do not create a second SAGE-style subsystem when the capability can be integrated into an existing canonical OpenW3D service.

## Gemini working style for this repository

- Search narrowly first, then expand only when necessary.
- Avoid loading large directory trees or long logs into context without a specific reason.
- When inspecting `Core + GeneralsMD`, follow symbols and build ownership rather than assuming file location from old SAGE layouts.
- Prefer implementation over extended speculative planning once the roadmap and relevant source provide enough direction.
- Keep donor/provenance decisions in roadmap-prescribed matrices.
- Research code/documentation questions before asking the user.
- Put genuine unresolved user decisions in `awaiting_answers.md` with stable `Q-###` IDs.

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

## Validation and token discipline

Roadmap Section 0B is mandatory.

Default normal-development validation:

1. Build only directly affected target(s).
2. Run cheap directly relevant existing tests when applicable.
3. Use one focused smoke/runtime check when the change needs one.
4. On PASS with no escalation trigger, **STOP VALIDATING AND CONTINUE IMPLEMENTATION**.

Do not automatically run the complete test suite, all client/server/tool builds, all golden scenarios, broad performance benchmarks, or unrelated subsystem tests.

Do not create tests merely because a function was changed. Prefer existing coverage and add new tests only for meaningful behavior/risk identified by Section 0B.

For successful builds/tests, consume only status and concise summary. For failures, inspect filtered/relevant error output first and expand only as necessary.

Full regression belongs at designated roadmap gates or after a concrete cross-cutting failure.

## Persistent project state

The following files must remain synchronized with actual repository state:

```text
completed_features.md
WIP_features.md
unstarted_features.md
awaiting_answers.md
```

Before substantive work, identify/resume the WIP item and ensure lifecycle state is correct.

Before any intentional session end/handoff/context reset:

- inspect Git status and relevant changes;
- update all four state files;
- preserve an exact next action for every WIP item;
- resolve/remove answered `Q-###` items;
- run the lightweight feature-state validator when available;
- record concise targeted validation already obtained;
- do not trigger broad regression merely because work is stopping.

Never mark a feature complete until its applicable acceptance gate has passed.
