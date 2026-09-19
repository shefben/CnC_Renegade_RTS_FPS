# Finite team power grid + strategic building capabilities

Roadmap Section 40, Phase 34.

`Code/Combat/teampowergridservice.{h,cpp}`, reading `Code/Combat/buildingdescriptor.h` power/
capability blocks (Phase 33) and driving `Code/Combat/building.h`'s existing
`Enable_Power`/`Is_Power_Enabled` and `Code/wwphys/worldshroudmanager.h`'s sensor-source API
(Section 30).

## The rule this phase is shaped by

*"Forbidden: LegacyPoweredFlag + RTSPowerFlag + translation glue. Audit the latest OpenW3D +
integrated TT owner and extend it."* The audit found that owner:
`BuildingGameObj::Enable_Power`/`Is_Power_Enabled` is the one canonical per-building flag, and its
current decider is `BaseControllerClass::Power_Base` (`Code/Combat/basecontroller.cpp:521-552`):
a coarse, whole-base, all-or-nothing rule -- *"the base is NOT powered if all power-plants are
destroyed, however the base IS powered if there are no power plants"* -- applied with one uniform
loop calling `Enable_Power(BasePowered)` on every building in the base regardless of what it
actually needs. That is exactly the coarse-whole-base-flag case the roadmap names, and exactly why
Renegade never had finite generation/demand: one surviving Power Plant powers everything, with no
concept of how much power anything actually draws.

`TeamPowerGridService` is the deterministic replacement decision. It does not add a second flag --
its entire output is a sequence of calls to the same `Enable_Power` that already exists.

## Wired into `BaseControllerClass`, and into every `BuildingGameObj`

A follow-up pass closed both gaps named above. `BuildingGameObjDef` now carries an editable
`DescriptorName` string (`Code/Combat/building.h`/`.cpp`, saved/loaded as its own microchunk,
`Get_Descriptor_Name()`), the key a preset names into `data/building_descriptors/*.json`. Empty
by default -- a preset that names none behaves exactly as it always did.

`BuildingGameObj::Initialize_Building()` (called from both the server's own component-collection
path and the client's `Import_Creation`) now calls `TeamPowerGridService::RegisterBuilding` for
*every* building unconditionally, using `Get_ID()` (the network object id -- already stable and
unique) as `member_id`. This is deliberately unconditional rather than gated on a descriptor
existing: `RegisterBuilding`/`Peek_Descriptor` already treat an empty or unresolved
`DescriptorName` as a neutral member that neither generates, consumes, nor gets shed, so
registering everything makes every future preset's `DescriptorName` take effect immediately with
no further code change, while changing nothing about how an undescribed building behaves today.
The destructor calls `UnregisterBuilding` unconditionally (a safe no-op if it was never
registered); registration/unregistration are pure bookkeeping (no `Enable_Power` call inside
either), so they run identically on client and server.

`On_Destroyed`/`On_Revived` call `TeamPowerGridService::OnBuildingDestroyed`/`OnBuildingRevived`,
and `Initialize_Building` calls one initial `RecalculateTeam` after registering -- all three
guarded by `CombatManager::I_Am_Server()`, matching `Power_Base`'s own existing rule that only the
server ever decides `Enable_Power`; a client receives it mirrored through `BIT_RARE`, exactly as
before this phase.

`BaseControllerClass::Power_Base`'s coarse loop (`basecontroller.cpp:521-552`) was *not* deleted
or replaced wholesale -- most building types still name no descriptor, so it remains their sole
authority, unchanged. Instead the loop now calls the new
`TeamPowerGridService::Has_Resolved_Descriptor(member_id)` query and skips any building it
answers true for, so the two allocators never both call `Enable_Power` on the same building.
`Check_Base_Power`'s own power-plant scan (which feeds `BasePowered`/`Is_Base_Powered()`, read
elsewhere for HUD/announcements/production gating) is untouched and still scans every power
plant regardless of descriptor, so that summary signal keeps meaning exactly what it always meant.
The net effect: a building preset that names no `DescriptorName` (still nearly everything, since
only four JSON descriptors exist) is powered exactly as before; a preset that does name one is
powered by this service's deterministic allocation from the moment a level places an instance of
it, with no further code change on either side.

**Correction, a later pass.** The skip above was only half the rule, and the missing half was a
real defect rather than a refinement. `Power_Base` was taught to leave this service's buildings
alone, but `RecalculateTeam`'s own `Enable_Power`-driving loop was *not* taught the converse: it
called `Enable_Power` on every registered, non-destroyed member of a team, descriptor or no
descriptor. Since registration is unconditional, that meant every undescribed building on a team
had its power flag overwritten by this service the next time anything on that team was destroyed,
revived or registered -- silently overriding whatever `Power_Base` had decided for it, and making
the paragraph above false in exactly the case it claims to protect.

The fix is the mirror of the `Power_Base` skip: `RecalculateTeam`'s driving loop now `continue`s on
a member whose `Peek_Descriptor` returns null, before it touches `Enable_Power` or the shroud. The
allocation passes above it were already correct -- an undescribed member contributes zero
generation and zero consumption to both -- so only the write was wrong. Both halves of the rule now
live within a few lines of each other, each with a comment naming the other, because a one-sided
skip is exactly the shape of bug that reads as complete.

## The algorithm

Per team, each `RecalculateTeam` call:

1. **Generators** (`requires_power == false && generation > 0`): each alive, not manually
   disabled one contributes its `generation` to `AvailablePower` and is always `Enable_Power`d on
   while alive -- a generator needs no grid allocation of its own, exactly the roadmap's "Typical
   Power Plant" case.
2. **Consumers** (`requires_power == true`, or no descriptor resolved yet -- see below): destroyed
   or manually disabled consumers are excluded entirely, both from allocation *and* from
   `RequestedPower` -- *"manual disable immediately frees demand."* The rest are sorted by
   effective priority (a manual override if one is set, else the descriptor's own
   `power.priority`), descending, with `MemberId` ascending as the stable, deterministic tie-break
   the roadmap names. A plain insertion sort; team rosters are small.
3. Greedy allocation in that order: a consumer is powered if its consumption would not push the
   running allocated total past `AvailablePower`; otherwise it is shed. Nothing is randomly shed,
   and nothing about the order depends on iteration order over an unordered container.
4. `Enable_Power` is called on every non-destroyed, non-null-`Building` member with the decision
   just made. A member with no resolved descriptor (an unrecognized or not-yet-loaded
   `DefinitionName`) is treated as a zero-generation, zero-consumption, `requires_power = true`
   member -- the missing-config case behaves sanely rather than blocking or asserting, the same
   rule this project applies to missing art.

Because `RecalculateTeam` always recomputes from the full current member list, "new generation or
reduced demand restores them" is not a separate code path -- it is just what happens the next time
the algorithm runs with different inputs. Destroying the last generator drives `AvailablePower` to
zero, which sheds every `requires_power = true` survivor through the same loop; there is no special
case for "the last one."

## Communications coverage

For every member whose descriptor declares `capabilities.communications.enabled`, the same
recalculation decides `online_eligible = !destroyed && (allocated || !requires_building_online)`
and registers, updates, or unregisters that member's `WorldShroudManager` sensor source to match --
Section 30's own `RegisterStrategicSensorSource`/`UpdateStrategicSensorSource`/
`UnregisterStrategicSensorSource`, exactly as that section specified for communications buildings,
not a second shroud implementation. *"Coverage disappears when the building is destroyed... or
powered down when its capability requires online state. Coverage returns automatically when
restored"* is proved end to end in the self check: destroying a communications building drops its
sensor source, and a `WorldShroudManager::Timestep` afterward shows the cell falling out of
`SHROUD_VISIBLE`; reviving it re-registers and the cell returns to `SHROUD_VISIBLE`.

## Tiberium Silo wallet expansion

`Get_Wallet_Capacity_Bonus(team)` sums `wallet_capacity_bonus_per_player` over every
`tiberium_silo`-capable member that is online-eligible by the same rule communications uses --
computed fresh on every call from current grid state, so it is automatically correct after a
destruction, a manual disable, or a total power loss, rather than a value that has to be remembered
to be recomputed. **What this does not do**: apply the number to an actual player credit cap. This
codebase has no wallet-cap enforcement point today -- no `MaxMoney`/`WalletCap`/`CreditCap` concept
exists anywhere in `Code/` before this phase. The bonus is real, computed, stacking, and queryable;
wiring it into wherever player credits are actually capped is a separate, smaller follow-up once
that enforcement point is identified or built.

## Manual control

`Set_Manual_Disabled`/`Set_Priority_Override`/`Clear_Priority_Override` each call
`RecalculateTeam` for the affected member's team immediately, rather than leaving the effect for
whatever next calls it -- *"manual disable immediately frees demand"* and *"manual priority/on-off
control is authoritative"* are read here as taking effect this call, checked as an immediate state
change in the self check, not merely asserted.

## What this phase does not build

- **Wallet-cap enforcement.** See above -- still open; needs a `BaseWalletCap` design value this
  codebase has never had (see `awaiting_answers.md`).
- **Live-tunable reload of anything beyond generation/consumption/priority.** `OnDescriptorReload`
  recalculates every team touched by a changed definition, which is sufficient for every numeric
  field this service reads (`power.*`, `communications.*`, `tiberium_silo.*`) since they are all
  re-read fresh from `BuildingDescriptorManager::Get_Descriptor` on every recalculation -- there is
  no cached copy inside a `PowerGridMember` to go stale.
- **Save/load.** No persistence code exists for grid membership or manual overrides yet; that is
  Phase 40 (save/load + multiplayer pass), out of this range.
- **Network replication of manual overrides or grid state.** No network code exists in this file.

## Checks

`renegade -shaderselfcheck powergrid`, registered as ctest `shader_powergrid` and
`fds_shader_powergrid`. Device-less -- every member is registered with a `nullptr`
`BuildingGameObj*`. Covers, against the roadmap's own acceptance line order: finite generation/
demand; deterministic overload shedding by priority; restoration on reduced demand; the final
generator powering down every dependent consumer, and reviving it restoring them; manual on/off
freeing demand immediately and restoring it; priority override changing the winner immediately and
`Clear_Priority_Override` reverting to the descriptor priority and `MemberId` tie-break;
communications coverage tracked live through `WorldShroudManager`; Silo bonuses stacking, dropping
under manual disable, and dropping to zero when the only generator is destroyed; and a live JSON
edit reaching `Get_Generation` through `OnDescriptorReload`.

**Not covered by any automated check**: a real `BuildingGameObj*` actually receiving an
`Enable_Power` call (needs a live combat scene), and everything named in "what this phase does not
build."
