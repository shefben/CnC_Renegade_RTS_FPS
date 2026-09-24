# Optional Vehicle Subsystem Damage (V2 Phase 22)

Roadmap Section 23. Canonical owner: `VehicleGameObj` (`Code/Combat/vehicle.h/.cpp`). Schema:
`ProceduralWorldProfile::VehicleSubsystems` (`Code/Combat/proceduralworldprofile.h`, schema
version 8).

This file sits in `docs/procedural_world/` although vehicle damage is not world-generation
content. The alternative was a new directory holding one file. The block that gates this feature
is a `ProceduralWorldProfile` block, its per-field table is in `WorldProfileSchema.md` next door,
and every other V2 phase doc is here — so co-locating with the schema it extends beats splitting
the schema's documentation across two trees.

## The rule this phase exists to not break

Section 23's first line is the whole design: "Do not alter default Renegade vehicle handling
unless enabled by game profile." Phase 22's own gate repeats it — "behaviorally absent when
disabled."

That is stronger than "off by default," and it is why the toggle is a `ProceduralWorldProfile`
field rather than an `SSGMSettingsClass` one. `ssgm.ini` is read for *every* map, procedural or
stock; a profile is loaded only when a procedural world is actually generated. A legacy match
therefore never even constructs a non-default settings block, so the claim "canonical Renegade
vehicle handling is untouched" is a property of which object the value lives on, not a rule
someone has to remember to check. `SSGMSettingsClass` would have been the right owner for a
server-operator gameplay knob; this is a generated-world feature, and it belongs with the rest of
the generated-world configuration.

Proven, not asserted: `terrain_vehiclesubsystems` walks six health fractions from full down to
zero with a default-constructed settings block and checks all four accessors return the canonical
answer (`1`, `1`, can-fire, not-immobilized) at every one of them.

## Four subsystems, two of them real, two disclosed as gaps

Section 23.1 lists four possible subsystems. Here is what each one actually became:

| Section 23.1 subsystem | Status | What it does |
|---|---|---|
| `engine damage -> reduced acceleration/top speed` | Real | Scales `ANALOG_MOVE_FORWARD`/`ANALOG_MOVE_LEFT` in the live `Apply_Control`, plus a speed governor so top speed really falls |
| `track/wheel damage -> reduced mobility` | Real | Scales `ANALOG_TURN_LEFT` in the same place |
| `turret/weapon module damage -> reduced weapon behavior` | Real, continuous | Lengthens the per-instance refire interval in `WeaponClass::Set_State`; an optional hard cut-off is available but off by default |
| `sensor damage -> reduced AI targeting` | **Not implemented** | Nothing to hook into — see below |

**Sensor damage is not attempted.** No sensor, perception, or detection-radius concept exists
anywhere in `Code/Combat` — confirmed by searching the tree, not assumed. Implementing "reduced AI
targeting" would mean first inventing a targeting-range system for this engine and then degrading
it, which is building an unrequested subsystem rather than wiring an existing one. No `sensor_*`
profile field is offered, because a field nothing reads is worse than an honest absence.

## Where the effect is applied, and why there

For movement: in `VehicleGameObj::Apply_Control`, after the existing target-steering and turn-clamp
work, immediately before `SmartGameObj::Apply_Control()`. That is the one place per frame where a
vehicle's final control state exists regardless of whether it came from a human driver, a gunner,
or `VehicleDriverClass`'s own AI output — so one hook covers every driver, and no second
AI-specific path can drift from the human one. The whole block is additionally gated on the
profile's own master toggle, so a match with the feature off does no extra work at all rather than
merely reaching an accessor that returns 1.

Deliberately **not** the `wwphys` layer. `MotorVehicleClass`'s torque/RPM simulation is where
"engine power" physically lives, and scaling there would have been the more literal reading of
"engine damage". It was rejected: Section 1.9 forbids replacement physics, and modifying the shared
integrator to serve one optional gameplay toggle would put a gameplay concern inside the layer
every vehicle in the game — procedural or stock — runs through. The physics keeps receiving
ordinary control values and running the stock model; only this one damaged vehicle's effective
commands change before they get there.

## The speed governor: why scaling throttle was not enough by itself

Reducing the throttle number is not automatically a reduction in *performance*, and the two vehicle
families differ sharply:

- **Tracked vehicles degrade correctly on throttle alone.** `TrackedVehicleClass` sets track torque
  strictly proportional to throttle, and `wheel.cpp`'s track resistance is linear in forward speed.
  Balancing drive force against that resistance gives a terminal velocity directly proportional to
  throttle — half throttle really is half top speed.
- **Wheeled vehicles do not.** `MotorVehicleClass::Get_Engine_Torque` scales torque by throttle, but
  the resulting top speed is set by where the engine torque curve falls off at redline — an RPM that
  does not depend on throttle — and the speed-proportional friction penalty saturates at a tiny
  constant rather than setting terminal velocity. The genuine quadratic drag term exists but its
  coefficient is data-driven per surface and defaults to `0.0f`. A damaged wheeled vehicle would
  therefore accelerate noticeably worse and still reach essentially its normal top speed, which is
  exactly where "engine damage" should bite hardest.

`VehicleGameObj::Govern_Subsystem_Speed` closes that gap without touching the integrator. While the
engine is undamaged it only watches, recording the fastest ground speed this vehicle instance has
actually reached; once damaged it withholds propulsion whenever ground speed exceeds
`engine_factor` × that reference, letting the stock physics do all the deceleration itself.

Two deliberate details. Reverse and braking input is never touched — the governor removes
propulsion, not control, which is step 5 again. And vertical velocity is excluded, so a vehicle
falling or descending a slope is not throttled for motion it did not ask for.

**Why the reference is learned rather than derived.** There is no authoritative per-vehicle top
speed to read: `SmartGameObj::Get_Max_Speed` is a hardcoded placeholder that `VehicleGameObj` never
overrides, and everything the physics actually uses to determine top speed — gear ratios, the
torque curve, redline — is owned by the shared definition. Re-deriving top speed from those would
put a second copy of the integrator's own model inside Combat, free to silently disagree with it.
A vehicle damaged before ever being seen at speed simply has no reference, and the governor stands
down rather than inventing a cap; the throttle scaling still applies. `SubsystemPeakSpeed` is not
saved or networked — after a load the reference re-learns, the same transient treatment
`SubsystemRetreating` gets.

## Weapon damage is continuous, per weapon instance

`WeaponClass::Set_State` computes a weapon's refire interval as `StateTimer = 1.0f / RateOfFire`.
`RateOfFire` lives on `AmmoDefinitionClass` and is **shared by every weapon of that type on every
object in the world**, so scaling it would slow that preset's weapon everywhere at once.
`StateTimer` is the per-instance value the shared rate has just become — and the adjacent
`WEAPON_RATE_CHEAT` block already performs exactly this shape of per-instance, owner-conditional
multiply on it.

So the modifier divides `StateTimer` by the owning vehicle's weapon efficiency, immediately after
it is computed and only for the two firing states. Reloading and weapon switching are handling
speeds belonging to whoever carries the weapon, not to a damaged turret, and are left alone. The
multiply is owner-gated on `As_VehicleGameObj()` because `WeaponClass` is shared by soldiers and
vehicles alike and nothing here may touch an infantry weapon's rate of fire.

No new state is introduced: the efficiency is a derived read of health computed at the moment the
timer is set, and the already-persisted `StateTimer` simply carries whatever interval was current,
exactly as it does today.

The binary hard cut-off still exists as `weapon_disable_fraction`, but it now defaults to `0`,
which turns the rule off. A damaged weapon's normal fate is to fire ever more slowly; falling
silent is an explicit opt-in, the weapon-side counterpart of `allow_immobilize`.

## `CanDrive`/`CanFire` were left alone on purpose

`VehicleGameObj` already carries `CanDrive` and `CanFire` — real, saved, network-replicated
fields with real TT script callers (`Set_Can_Drive`, `Set_Scripts_Can_Fire`). They are exactly the
shape this phase needed, and the commented-out `Apply_Control` at `vehicle.cpp:1604-1667` shows
the original intended consumption.

They are still not used here. Those fields are script-owned state: a mission script sets
`CanFire` false for a reason, and an automatic health-driven write to the same field would fight
that script's intent the instant the vehicle took a hit, with no way for either side to tell who
last wrote it. Phase 22's answers are instead pure derived reads of health that own no state at
all, so the two mechanisms compose rather than collide — a script can still disable a weapon
independently of damage, and damage can disable one independently of a script.

(That the live `Apply_Control` never reads `CanDrive`/`CanFire` at all, while the dead block above
it does, is a real pre-existing gap in this tree. It is **not** fixed here: no bug was
demonstrated by Phase 22's own work, the scripts that set those fields are TT donor content whose
intended behavior is not this phase's to decide, and Section 0 permits an out-of-scope fix only
for a demonstrated incompatibility. Recorded in `WIP_features.md` rather than silently repaired.)

## Health is the only state, so repair is free

Every accessor is a pure function of `DefenseObject.Get_Health() / Get_Health_Max()` — the same
health-fraction-threshold pattern Phase 20 established for building subcomponents. There is no
second health pool, no per-subsystem hit points, and no new save-game or network state, because
health is already saved and already replicated.

Section 23.2 asks that "canonical Repair Facility/Repair Bay and existing repair mechanics restore
subsystems". They already do, with zero new code: `RepairBayGameObj::Repair_Vehicle` restores
`DefenseObject` health, the derived reads immediately answer differently, and the vehicle drives
normally again. The same is true of handheld repair-gun healing (step 7) — it also moves health,
so it also restores subsystems. This is the identical story Phase 20's building subcomponents
already told, and it is the main reason the derived-read design was chosen over real subsystem
state.

## Step 5, the rule most worth getting right

"Never remove basic drivability from a human player without an explicit configured rule."

Two independent guarantees implement it. First, the engine and mobility curves bottom out at
configured *floors* (`min_engine_factor`, `min_mobility_factor`), not at zero — engine damage
alone always leaves a vehicle moving. Second, full immobilization requires `allow_immobilize`,
which is a separate opt-in that stays false even when `enabled` is turned on.

`terrain_vehiclesubsystems` proves all of it: that an enabled-but-not-`allow_immobilize` profile
never immobilizes even at zero health, that `allow_immobilize` with `enabled` false stays inert
(so "disable the profile" is the total bypass step 11 requires), and that the curves land exactly
on the configured floor at zero health rather than somewhere the implementation chose.

## Step 8: AI retreat-to-repair

`VehicleGameObj::Think_Subsystem_Retreat`, ticked once per `Think`, server-only. A vehicle at or
below `retreat_to_repair_fraction` health walks to the nearest same-team, capability-enabled
repair bay via `ScriptEngine::Action_Goto` — the same call
`commanderorderevent.cpp`'s own `ORDER_RETREAT_TO_REPAIR` case already makes for a human
Commander's click, triggered by the vehicle's own health instead of by an order.

It never acts on a vehicle a human is driving. That is step 5 again: a human's vehicle driving
itself away would be a far worse violation of "never remove basic drivability" than slowing it
down ever was.

The repair-bay search is a direct `GameObjManager` walk rather than a
`LogisticsGraphService` REPAIR-node query, deliberately: Phase 21's own doc records that
`Rebuild_Building_Nodes` has never run at runtime, and an AI response that ticks every server
frame should not depend on a pipeline stage the roadmap already discloses as unwired.

`SubsystemRetreating` only suppresses re-issuing the order every tick. It is intentionally not
saved or networked — the worst case after a load is one extra frame deciding again, the same
transient-AI-state treatment `CommanderGuardAI`'s own registry gets.

## Step 9: recovery/support vehicles

Section 23.3 lists repair vehicle, recovery vehicle, escort behavior, retreat-to-repair as
*optional*. Retreat-to-repair is built (above). The other three are not, and the reason is the
same content gap every recent phase has hit: step 9 requires them to be added "only through normal
vehicle/AI APIs", which means a real vehicle preset to add them to, and no buildable
repair/recovery vehicle preset exists in this tree yet. The AI half would be a `CommanderGuardAI`-
shaped controller; the content half is P33's backlog. Recorded, not stubbed.

Fuel is not implemented at all, per Section 23's own closing line ("Do not make fuel
micromanagement mandatory in normal v2 gameplay") — nothing asked for optional fuel either.

## What is proven, and what is not

**Proven, device-less, by `terrain_vehiclesubsystems`/`fds_terrain_vehiclesubsystems`** (both
green): every accessor is canonical at every health while disabled; nothing is scaled above the
start threshold; all three curves are bounded, monotonic, and land exactly on the configured floors
at zero health; a per-subsystem threshold of zero switches that one subsystem off without touching
the other two; weapon efficiency stays strictly positive across the default curve (it divides the
refire interval, so a zero would be an infinite one); the optional weapon cut-off is off by default,
fires at and below its threshold once armed, and is itself overridden by `enabled`;
`allow_immobilize` gates immobilization in both directions and is likewise gated by `enabled`.
`shader_worldprofile` additionally proves a fraction above 1 or below 0 is rejected rather than
clamped, that the block round-trips through JSON, that a schema-1..7 profile that never mentions
the block still loads with vehicle handling untouched, and that enabling the feature implicitly
authorizes neither immobilizing nor silencing a weapon.

**What is not proven**: no real `VehicleGameObj` has ever executed this code. A `-terrainselfcheck`
run brings up no `GameObjManager` or physics scene, so the instance accessors, the `Apply_Control`
hook, the speed governor, the `WeaponClass::Set_State` modifier and `Think_Subsystem_Retreat` have
never run against a live vehicle — the same standing harness limit Phase 20's subcomponents and
Phase 21's building nodes already carry, which is why the response curves were made testable as
pure functions rather than left untested behind an un-constructible object. No vehicle has been
observed actually slowing down, firing more slowly, or driving itself to a repair bay on screen.

**The speed governor in particular deserves a live pass.** Its correctness argument rests on a
reading of the tracked and wheeled physics paths, and its reference speed is learned at runtime, so
the two things most worth confirming by hand are that a damaged wheeled vehicle now genuinely tops
out lower (not merely accelerates worse), and that the governor does not visibly oscillate or judder
as it holds a damaged vehicle at its cap. `Find_Nearest_Repair_Bay` has never found anything either,
because no buildable Repair Bay descriptor exists yet (P33's own gap) — correct code with no content
to act on.

## Next exact action

Rolled into the standing live-pass backlog every other device-less phase carries: run
`renegade --gamedir "C:\Westwood\Renegade_full"`, start **Procedural Skirmish** with a profile
setting `vehicle_subsystems.enabled` true, then:

1. In a **wheeled** vehicle, note its healthy top speed on a straight, take it to roughly half
   health, and confirm it now tops out visibly lower — this is the governor, and it is the single
   most important thing to verify by hand.
2. Repeat in a **tracked** vehicle, which should degrade on throttle scaling alone.
3. Confirm turning is worse and that reverse/braking still respond normally while damaged.
4. Fire while damaged and confirm the gap between shots has grown, then run `vehicle_subsystems` at
   the console and check the reported fire-rate factor matches what it feels like.
5. Confirm an infantry weapon's rate of fire is unaffected at any vehicle health.

Immobilization needs `allow_immobilize` true, the weapon cut-off needs `weapon_disable_fraction`
set above 0, and AI retreat needs a repair bay — the last is blocked on P33's descriptor backlog,
a content gap rather than a bug to chase.

## D-001 updates

- **Weapon accuracy.** At fire time a damaged vehicle's spray angle is multiplied by the inverse
  of the same weapon efficiency that already slows its rate of fire (`WeaponClass` bullet loop).
  The shared `AmmoDefinitionClass` is never touched. A weapon with no spray stays exact.
- **Sensor.** `SmartGameObj::Get_Sensor_Range_Factor` scales the sight range the AI's enemy
  acquisition uses. It is 1 for everything but an AI-driven vehicle, whose factor follows the
  weapon module's curve over `vehicle_subsystems.sensor_damage_start_fraction` /
  `min_sensor_factor` (schema 9, defaults 0.66 / 0.3, like the weapon module's). A vehicle a player
  drives is never impaired. Checked in `terrain_vehiclesubsystems`.
