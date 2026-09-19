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

## Not wired into `BaseControllerClass` this pass

Replacing `Power_Base`'s coarse loop with a call into `TeamPowerGridService::RecalculateTeam` is
the precise, identified next step -- but doing it needs two things this pass does not have: a
`BuildingGameObj` that knows its own `DefinitionName` (no production code assigns one yet -- Phase
33's own honest gap), and a careful pass through `BaseControllerClass`'s networking/save-load
paths (`Set_Base_Powered`'s dirty-bit marking, the packet read/write at
`basecontroller.cpp:1055-1090`) to avoid breaking a heavily-tested, already-networked class under
time pressure this late in a long pass. What is built for real instead is the algorithm itself,
driven and checked exactly the way production code would drive it -- registrations may simply
carry a `nullptr` `BuildingGameObj*`, which is what makes the deterministic core testable with no
device, no scene, and no live building.

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

- **`BaseControllerClass` integration.** See above.
- **Wallet-cap enforcement.** See above.
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
