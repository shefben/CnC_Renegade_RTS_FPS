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
