# AI state/path/guard-retaliation + human-yield hooks

Roadmap Section 37, Zero Hour / SAGE feature 24. Donor audit points: `AI`, `AIGuard`,
`AIGuardRetaliate`, `AIPathfind`, `AIPlayer`, `AISkirmishPlayer`, `AIStateMachine`, `AITNGuard`.

`Code/Combat/guardstatecontroller.{h,cpp}`.

## What the audit found

Before writing anything, this pass searched `Code/Combat` and `Code/Scripts` for an existing
"guard" or "retaliate" concept. There is none at the engine level: `grep`-ing `Code/Combat` for
`Guard`/`Retaliat` finds only a UI string table entry. What Renegade calls guarding is entirely a
per-mission scripted behaviour, hand-authored in `Code/Scripts/Mission*.cpp` for whichever unit a
level designer placed and wired up. That is a real, working answer for a level someone wrote by
hand, and it is exactly the thing that cannot exist for a unit a Commander player purchases at
runtime (Phase 39) -- there is no mission script watching it. So this phase's actual job was not
"replace Renegade's AI with something ported from Zero Hour" -- the section's own rule forbids
exactly that -- it was "give OpenW3D a canonical engine-level guard/retaliate capability that
never existed", built by adapting the roadmap's named *behaviours* (state semantics, attacker
memory, a leash boundary, stuck recovery, human-priority scheduling) rather than by porting SAGE's
`Thing`/`AIPlayer` machinery.

## A pure state machine, not a GameObj

`GuardStateControllerClass` knows nothing about `GameObj`, `ActionClass`, or the scene. It is fed
damage reports and a position every tick and hands back an *intent*
(`GUARD_INTENT_ATTACK`/`GOTO`/`STAND`/`NONE`); a caller -- Phase 39's Commander AI unit, not built
this pass -- is the one that turns an intent into a real `ActionClass::Attack`/`Goto` call on a
real `SmartGameObj`. That split is what makes the state machine itself exercisable with no device,
no scene and no live unit, which is why this phase could implement the roadmap's own seven named
test scenarios exactly rather than approximating them:

| Roadmap scenario | Where it is proved |
|---|---|
| guard -> retaliate -> return | `Run_Guard_AI`, block 1 |
| guard -> lose target -> reacquire | block 2 (combined with multiple threats) |
| guard area boundary | block 4 |
| path blocked -> replan | block 5 |
| stuck unit recovery | block 5 (same mechanism) |
| multiple simultaneous threats | block 2 |
| invalid/dead target transition | block 3 |

## The four states

```text
GUARDING      at (or returned to) the anchor, nothing to do
RETALIATING   engaging the current target
REACQUIRING   target just lost; deciding what happens next
RETURNING     walking back inside the leash
```

`Report_Damage(attacker_id, position, now)` remembers the attacker (bounded ring buffer,
`GUARD_ATTACKER_MEMORY_MAX` = 4, oldest evicted) and, if nothing is currently being fought,
engages immediately -- "retaliation after hostile damage." A second attacker hitting while one is
already being fought is remembered but does not steal the target -- "multiple simultaneous
threats" resolved as memory depth, not as an interrupt. `Report_Target_Lost` (called explicitly,
or noticed by `Update` itself when it is handed `target_valid == false`) removes that specific
attacker's memory entry -- so a confirmed-dead target can never be immediately reselected -- and
moves to `REACQUIRING`, which the next `Update` resolves: the freshest still-remembered attacker
within `GUARD_ATTACKER_RETENTION` (8 seconds) if there is one, otherwise a walk home if outside the
guard radius, otherwise straight back to `GUARDING`.

## The leash

A retaliating unit chasing a target that carries it past `guard_radius * 1.5` is forced into
`RETURNING` even though the target is still alive -- "guard area boundary" and "return-to-guard-
area behaviour" -- because a guard is a guard, not a unit assigned to a target permanently.

## Stuck detection and replan, budgeted

Every `Update` that proposes a `GOTO` (the return leg, or the leash break-off) compares the
guarded object's real position against where it was the last time this controller thought; less
than 25 world units of movement for two seconds running counts as stuck. A stuck controller with
budget available proposes a new `GOTO` -- a perpendicular offset from the blocked line, the honest
scope of "path retry/replan improvements" this class actually owns; there is no pathfinder behind
it, and pretending there was would be a much larger claim than what this pass built. The budget
itself is shared across every controller (`Set_Max_Replans_Per_Tick`, default 8) and reset once a
tick by `Begin_Tick()`, so many simultaneously stuck units stagger rather than all replanning on
the same frame -- "expensive path replans are staggered/budgeted" proved as a counter that can
actually refuse, not just exist.

## Human priority (Section 3A.4/3A.6)

Two inputs to `Update`, not a separate system. `nearest_human_distance` scales how often the
*expensive* part of a think runs at all: near a human (or while actively retaliating) every tenth
of a second, otherwise once a second -- serving both "distant/non-combat AI may use lower-
frequency decision updates" and "AI yields... around human-controlled units where practical" from
the same signal, since both want more attention paid near a human and less paid far from
everything. Separately, any proposed `GOTO` (or the position an `ATTACK`/`STAND` would hold) that
would land within `GUARD_HUMAN_YIELD_RADIUS` (5 units) of `nearest_human_position` is suppressed
for that tick -- `GUARD_INTENT_STAND` instead -- which is the concrete form of "do not solve
congestion by disabling collision or teleporting through players": this class never *proposes* a
position on top of a human, so nothing downstream has to un-teleport one.

## Population bound

`Set_Max_Active_Controllers`/`Get_Active_Controller_Count`/`Get_Refusal_Count` are the one number
every controller instance shares -- "Commander AI population is bounded by configurable
team/map/server limits." No production system exists yet to actually fill it (Phase 39); the
bound refuses past its ceiling and a freed slot (`Deactivate`) is reusable, checked directly.

## Server authority

Nothing in this class touches the network. It is meant to be driven only from server-authoritative
`Think` code, the same rule every other gameplay decision in this project already follows;
enforcing that is the caller's responsibility once a caller exists.

## Deliberately not built this pass

- **Formation/group path ideas** -- the roadmap names these as "only where they fit existing
  Renegade AI", and there is no group/movement concept in this codebase to extend; inventing one
  to have something to extend would be a new system, not an adaptation, so it is left out.
- **A real pathfinder behind the replan nudge** -- see above; the roadmap's own "improvements" are
  what is claimed, not a new planner.
- **Any real caller.** No `GameObj` creates a `GuardStateControllerClass` yet. The mission-script
  layer's existing hand-authored guard behaviour is untouched and still works exactly as before --
  this is additive, not a replacement, consistent with the section's own rule. Phase 39's
  Commander-produced AI units are the first intended real caller.

## Checks

`renegade -shaderselfcheck guardai`, registered as ctest `shader_guardai` and
`fds_shader_guardai`. Device-less by construction -- the class needs nothing else. Covers all
seven roadmap-named scenarios (table above), the replan budget actually refusing at zero and
recovering after `Begin_Tick`, human-yield suppression as a state change once a human is placed on
the proposed destination, and the population bound refusing past its cap and reusing a freed slot.

**Not covered by any automated check**: everything past the state machine itself -- there is no
real `SmartGameObj`, no real `ActionClass::Attack`/`Goto` call, and no visible unit anywhere in
this pass, because nothing calls this controller from gameplay code yet.
