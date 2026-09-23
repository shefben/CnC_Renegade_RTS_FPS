# Commander AI infantry / ground / air production + unit control

Roadmap Section 45, Phase 39.

`Code/Combat/commanderproduction.{h,cpp}`.

## "Do not make bots emulate human PT GUI input"

That line decides the whole file's shape. A Commander purchase is a server-side request against a
validated queue, and the thing that eventually creates the unit is canonical Renegade production —
so this service owns the queue, the validation and the ordering, and owns **no spawning code at
all**. Completed items come out of `Get_Completed_Item` for the caller that knows how to build a
`SoldierGameObj` or a `VehicleGameObj`.

That is also what makes *"Commander-purchased infantry are normal AI `SoldierGameObj` paths, not fake
human players"* and *"do not consume human player slots or scoreboard identities"* provable by grep:
there is no player-slot, scoreboard or `cPlayer` code in this file, because there is no object
creation in it.

## Queues are bound to a facility, not a team

A queue is keyed by the facility's power-grid member id — the building's own network id. Section
45's *"Multiple facilities may have independent queues"* and *"Do not silently redirect to another
building"* are therefore properties of the data structure, not rules someone has to remember.

One item progresses per facility: the first item in request order whose facility id matches. The
shared queue keeps request order global and exact, which is what lets a save/load restore the same
queue in the same order.

## Validation

`Validate` is the single rule set, and `Request_Production` calls it — so a UI greying a card out
and the server refusing a purchase can never disagree. The order is the order a player would fix:

1. **Commander authority** — one Commander per team, named by player id. `0` means the team has none
   and every request from it is refused. Deliberately not a flag on a player object: the check has to
   be answerable on the server with nothing but a team and an id, which is exactly what a network
   handler has.
2. **The facility** — exists, is the right kind, alive, construction complete, powered. Each with its
   own reason, because the sidebar has to draw the reason.
3. **Availability** — the purchase entry's own tech level and script flags, passed in from
   `PurchaseAvailabilityClass` by the caller that read the page.
4. **Queue capacity** — the descriptor's `production.queue_capacity`.
5. **Population** — per-kind cap and an overall AI budget.
6. **Credits** — last, because it is the only one that resolves by waiting.

`CommanderFacilityContext` is passed in rather than looked up, the same way
`CommanderModeController` takes `star_is_alive`: it makes every rule checkable with no
`BuildingGameObj`, no power grid and no scene, and it keeps this service from growing a second
opinion about whether a building is alive.

## Caps count the queue, not just the living

A cap that counted only live units would let a Commander queue thirty tanks against a cap of twenty
and receive them all, because none of them exists yet when the twenty-first is asked for. Counting
pending items against the cap is what makes *"production is bounded"* true rather than eventually
true.

## Interruption policy, stated once

Section 45 requires *"one documented canonical pause/cancel policy"* for a facility that loses power
during production. **This codebase pauses.** An item whose facility is unpowered stops accumulating
time and reads `WAITING_FOR_POWER`; restoring power resumes it where it was. Cancelling on a power
cut would refund-and-requeue a whole queue every time a Power Plant took a hit.

A **blocked exit** holds a *finished* item at `BLOCKED_EXIT` rather than releasing it — and because
only the head item of a facility advances, the line behind it does not pass it either. That is
Section 45's *"do not release the next vehicle while the exit reservation remains blocked"*.

## Refund rule, stated once

A cancelled or destroyed item refunds its **full cost**, because the unit never existed. A partial
refund would need a partially-built-unit concept this game does not have. `Cancel_Item`,
`Cancel_Facility_Queue` and `On_Facility_Destroyed` all return the refunded total, so the caller can
credit it back to the same wallet it debited — this service tracks cost but owns no wallet.

`On_Facility_Destroyed` also drops *completed-but-undelivered* items for that facility and refunds
them with the rest. Section 45's *"prevents invisible completion"* means the unit must not appear
after its factory is gone, and a caller draining the completed list has no way to know the factory
died between the two calls.

## Selection and orders

Selection is a bounded set of **network object ids**, not pointers: a selected unit can die between
the click and the order, and a dead id is a miss where a dangling pointer is a crash.

`Can_Order_Unit` refuses an order to a unit on another team and to any unit that is not Commander AI
— *"human-controlled units receive priority"* (Section 3A.4), and more fundamentally are never the
Commander's to drive.

`Get_Order` returns `ORDER_HOLD_POSITION` for a unit this service has never been told about, which is
exactly what Section 45 says a newly produced unit does until ordered. The order list is Section 45's
own; no second SAGE AI model is introduced, and Phase 37's `GuardStateControllerClass` already owns
the guard/retaliation half.

## Governance — Roadmap Section 25.1 (V2 Phase 23)

*"No first-player-wins permanent rule."* A vacant seat with no other connected teammate still
grants a claim instantly — there is no one to contest it, and it is what keeps a solo or
skirmish match's Commander assignment costing zero latency. A vacant seat with a real teammate
on the same team instead opens a real "initial volunteer/candidate period": the claimant becomes
an `ELECT` candidate and the team gets `Get_Vote_Window_Seconds()` (15s) to object, rather than an
instant, undisputable grant. A seated Commander is challenged by a `REMOVE` vote, never re-elected
over. Resignation and re-claiming a seat already held remain instant, unchanged from before this
phase — the one behavior change is what a claim against a *contested vacant* seat does.

`Call_Commander_Vote` / `Cast_Commander_Vote` / `Update_Commander_Vote` / `Transfer_Commander` are
pure functions of plain ids, bools and a team — the same `CommanderFacilityContext` split this
file already uses for `available_credits` — so the whole vote engine is checkable with no
`GameObj`, no scene and no `cPlayer`. The one real roster walk,
`Count_Connected_Teammates`/`Is_Client_Connected_On_Team`, mirrors `Find_Soldier_Of_Client_ID`'s
own `Get_Control_Owner` test and filters `Is_Human_Controlled` first — every bot on a team shares
the same `SERVER_CONTROL_OWNER` sentinel, so skipping that filter would collapse an entire bot
roster into one phantom "voter".

**Resolution is asymmetric on purpose.** `ELECT` passes on any plurality of cast ballots
(`yes > no`) — silence defaults to granting the lone candidate, the same "the only claimant just
gets it" spirit the solo/skirmish fast path already relies on, just after a real waiting period
instead of instantly. `REMOVE` needs a strict majority of the *whole eligible team*
(`yes * 2 > EligibleVoters`), not just of who bothered to vote: unseating an incumbent must not be
one unopposed caller and silence from everyone else. The Commander being challenged does not get a
ballot on their own removal — resignation is the door already open to them.

A `REMOVE` vote whose seat changes hands for any other reason mid-window (resignation, the
existing vacancy timeout, another call) is caught **internally**, with no roster walk: 
`Update_Commander_Vote` compares `Get_Team_Commander` against the vote's captured candidate every
tick. An `ELECT` vote whose candidate disconnects before the window closes is caught by the one
real check the caller does supply, `subject_still_eligible`, computed once per active vote per
server tick in `combat.cpp` — not every tick regardless, so a match with no open vote pays nothing
beyond an `Is_Vote_Active` check.

`Transfer_Commander` is a direct handoff by the sitting Commander, Section 25.1's own separate
item from a vote — no ballot, refused unless the sender really holds the seat and the target is a
real, still-connected, different teammate.

Net path: `cCommanderVoteCallEvent` (open an `ELECT`/`REMOVE` vote), `cCommanderVoteCastEvent`
(cast a ballot), `cCommanderTransferEvent` (the direct handoff) — `Code/Commando/`, same
client-request/server-`Act()` shape as `cCommanderClaimEvent`, sender's team always read from
their own soldier, never trusted from the packet. Console: `commander_vote
<elect [id]|remove|yes|no|status>`, `commander_transfer <id>`, and a two-line summary folded into
`commander_rts_status`.

**"Disconnect recovery"** is `CommanderProductionService::Handle_Player_Reconnect(old_id,new_id)`,
called from the one place a reconnect actually happens server-side,
`cGod::Create_Player` (`Code/Commando/god.cpp`). `_Commanders[]` is keyed by network client id, and
that function hands a rejoining, name-matched `cPlayer` a *new* one whether it found them via
`cPlayerManager::Find_Player` (the crash-and-immediately-rejoin race) or
`Find_Inactive_Player` (an ordinary disconnect/rejoin) — without this, the only path back for a
Commander who drops is racing `COMMANDER_VACANCY_GRACE_SECONDS` (20s) under an id the server is
about to stop recognising, and losing even that race leaves an ordinary vacancy with no special
standing to reclaim it. The remap is a pure integer rewrite of whichever team slot (if any)
currently reads `old_id`, resetting that team's grace clock the same way `Set_Team_Commander`
does — a no-op for anyone who was not actually seated. Not migrated: a ballot already cast on an
active vote under the old id (disclosed in `WIP_features.md`, not fixed — the window a
disconnect/rejoin would have to land inside is 15 seconds, and one stray extra ballot cannot flip
a majority or plurality outcome on its own).

**Section 25.2 (skirmish)** — *"Human player is Commander of selected team"* falls out of the
general rule with no special case: a skirmish team's one connected human is the `eligible <= 1`
fast path above. *"Enemy receives AI Commander"* is not built here — there is no AI Commander yet
to assign one to; that is V2 Phase 24, and this claim/vote layer is not the mechanism a bot
Commander will use to hold its seat (an autonomous Commander does not need to out-vote anyone).

**Section 25.3 (Commander death)** — already correct before this phase and unchanged by it:
`CommanderModeController::Update` exits the strategic camera on `!star_is_alive` and touches
nothing in `CommanderProductionService`, so death "exits Commander Mode but does not necessarily
remove the team Commander role" exactly as required. Mode policy (whether the same player can
re-enter after respawn) is `Toggle_Commander_Mode` re-claiming a seat it still holds, which the
`current == SenderId` branch in `cCommanderClaimEvent::Act` grants instantly with no vote.

## What this phase does not build

- **No unit is ever created.** By design — but it does mean the acceptance lines (*"Barracks/Hand
  produces AI infantry that exits and holds"*, and its ground and air twins) are not met by this
  phase alone. The consumer that turns a completion into a canonical AI unit, walks it out through
  the descriptor's `infantry_spawn`/`vehicle_exit`/`aircraft_spawn` anchors and stages it, is the
  remaining work; recorded in `WIP_features.md`.
- **No order is ever executed.** `Get_Order` is a query; nothing yet feeds it into a Renegade AI
  state. The orders are durable, validated and persisted — they are not yet obeyed.
- **No drag-box or control groups.** Single select, add and remove are real. Box selection needs a
  cursor and a projection, which is renderer work; control groups have no caller.
- **No spawn/exit occupancy query.** `ExitClear` is reported *to* this service by the caller that can
  see the world, which is the same "pass in the fact, get back a decision" split the rest of the
  Commander layer uses.
- **Distant-AI update-rate budgeting** (Section 3A.6) is not here; it belongs with the AI that runs
  the units, not with the queue that ordered them.

## Checks

`renegade -shaderselfcheck production`, registered as ctest `shader_production` and
`fds_shader_production`. Device-less. Pins: a team with no Commander and a non-Commander player both
refused; each of the four facility states refusing with its own distinct reason; credits and
availability; the accepted path returning a ticket; the per-facility queue cap; a second facility
keeping an independent queue; one item at a time with the head advancing and the rest waiting; power
loss pausing rather than cancelling, and resuming where it was; a blocked exit holding a finished
unit and releasing it when cleared; destruction refunding the exact queued value and leaving no
queue; population caps counting live plus pending; selection add/remove/dedupe; and orders refused
for a human-driven unit, a foreign team and a non-Commander, defaulting to hold position, replacing
rather than stacking, and being forgotten with the unit.

**Not covered by any automated check**: everything that needs a real unit — that a completion becomes
a canonical AI soldier or vehicle, that it clears a door, that it stages and holds, and that it never
occupies a player slot. Those need a live match and are named in `WIP_features.md`.

`renegade -shaderselfcheck governance`, registered as ctest `shader_governance` and
`fds_shader_governance`. Device-less, same "pure ids and bools" split as the rest of this file's
checks. Pins: every `Call_Commander_Vote` refusal (no candidate, zero eligible voters, `REMOVE` on a
vacant seat, `ELECT` on a seated team, a Commander `REMOVE`-ing themself, a second vote while one is
active); the accepted `ELECT` path recording the caller as its first ballot; one team's vote leaving
another untouched; `Cast_Commander_Vote` refusing a repeat ballot from the same voter (including the
initiator) and a `REMOVE` target's self-ballot; no resolution before the window elapses; `ELECT`
granting on `yes > no` at the window and clearing the vote; an `ELECT` candidate disconnecting
cancelling rather than seating a ghost; a solo `ELECT` (no other ballots) still granting after its
window; a `REMOVE` vote with one unopposed initiator and silence from the rest of a 5-person team
*not* unseating the Commander; a real 3-of-5 majority unseating one; a `REMOVE` vote whose target
resigns mid-window cancelling with no roster walk; `Transfer_Commander` refusing a
non-Commander sender, a same-id transfer and a caller-supplied ineligible target, then accepting a
valid handoff; and `Handle_Player_Reconnect` carrying a held seat to its new id, leaving an
untouched team alone, and treating a zero id or an old-id-equals-new-id call as the no-op it is.

**Not covered by any automated check**: `Count_Connected_Teammates`/`Is_Client_Connected_On_Team`,
the one real `GameObjManager` roster walk — it needs a live `SoldierGameObj` this harness has no
way to construct, the same gap Phase 22's `Govern_Subsystem_Speed` documents. Also not covered: the
actual net events (`cCommanderVoteCallEvent`/`cCommanderVoteCastEvent`/`cCommanderTransferEvent`)
exercised over a real connection, `cGod::Create_Player` actually being called by a reconnecting
client, and the console commands' output formatting. All named in `WIP_features.md`.
