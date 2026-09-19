# Shared radar / tactical intelligence model

Roadmap Section 31, Zero Hour / SAGE feature 16. Donor: `W3DRadar`.

`Code/Combat/tacticalintelligence.{h,cpp}`, consumed by the existing `RadarManager`
(`Code/Combat/radar.{h,cpp}`) and built on `WorldShroudManager` (Section 30, P27).

## "Existing Renegade and TT radar APIs feed the same model"

`RadarManager::Update` used to be the model: it walked every `PhysicalGameObj` inline,
applied `RadarMode` and stealth filtering, and drew a blip. That loop is now
`TacticalIntelligenceModel::Build_FPS_Contacts`, moved verbatim rather than rewritten --
same viewer/star exclusion, same dead-soldier exclusion, same `RadarMode` filter (including
the unguarded `viewer->Get_Player_Type()` dereference in the stealth check the original code
already had), same altitude-fade comparison, computed with the exact `RADAR_Z_RANGE` constant
`radar.cpp` still owns and passes in. `RadarManager::Update` now calls it and does exactly what
it did before with the result: `Add_Blip` and the intensity write-back. Nothing about what a
player's HUD radar looks like changed by this move -- it could not have, since the filtering
code itself did not change, only where it lives. That is what makes "FPS HUD radar... use[s]...
canonical intelligence state" true for the model's largest and most representative case (every
infantry and vehicle contact) without touching Section 35's later promise that ordinary FPS
behaviour is unchanged.

## Two functions, not one flag

Section 31 also says: *"Do not expose strategic radar-only/last-known information as live FPS
wallhacks."* `Build_FPS_Contacts` never includes `WorldShroudManager`'s header logic and can
never produce a `CONTACT_RADAR_ONLY` or `CONTACT_LAST_KNOWN` contact -- those values are only
ever assigned inside `Build_Strategic_Contacts`, which nothing FPS-facing calls. Keeping them as
two functions instead of one function with a "strategic" flag is what makes that a property of
the code rather than a runtime setting someone could flip on by accident.

`Build_Strategic_Contacts(viewing_team, out)` is the Commander/tactical query surface: it walks
every `PhysicalGameObj` (classified `CONTACT_INFANTRY`/`CONTACT_VEHICLE`/`CONTACT_OTHER`) and
every `BuildingGameObj` (`CONTACT_BUILDING`), queries `WorldShroudManager::Get_Shroud_State` for
each contact's position against `viewing_team`, and:

- `SHROUD_VISIBLE` -- kept with its natural type;
- `SHROUD_RADAR_DETECTED` -- retyped `CONTACT_RADAR_ONLY`;
- `SHROUD_LAST_KNOWN` -- retyped `CONTACT_LAST_KNOWN`;
- `SHROUD_UNKNOWN` / `SHROUD_EXPLORED` with nothing live there -- left out entirely.

Pending objectives (`CONTACT_OBJECTIVE`) and `RadarManager`'s existing generic markers
(`CONTACT_COMMANDER_MARKER`) are appended unconditionally, matching their existing always-shown
FPS behaviour rather than inventing a shroud rule for them the roadmap never asked for.
`CONTACT_RESOURCE` and `CONTACT_WAYPOINT` are named in the enum for completeness -- there is no
Tiberium-field `GameObj` and no waypoint/order renderer in this codebase yet (that is Section 32,
Phase 29) -- so nothing produces them this pass, the same "type exists, no producer yet"
discipline P26 used for its four unimplemented component contracts.

## Buildings needed a second classification path

A `BuildingGameObj` is not a `PhysicalGameObj` -- both derive independently from
`DamageableGameObj`/`CombatPhysObserverClass` rather than one from the other -- which is also
why the *original* `RadarManager` loop never drew a building blip: its walk started from
`As_PhysicalGameObj()`, which a building fails. `Build_Strategic_Contacts` walks the raw
`BaseGameObj` list instead and branches: `As_PhysicalGameObj()` first, and if that fails,
`As_ScriptableGameObj()` then `As_BuildingGameObj()` -- the two-step path is required because
`PhysicalGameObj`/`BuildingGameObj` each inherit a *second*, unrelated declaration of
`As_BuildingGameObj` from `CombatPhysObserverClass`, which makes calling it directly on either
type ambiguous; `BaseGameObj::As_ScriptableGameObj()` is the one link in the chain that resolves
without that ambiguity. A building has no radar-blip fields of its own (nothing ever blipped one
before), so its contact's shape/colour are synthesized: `BLIP_SHAPE_TYPE_STATIONARY` and the same
`PLAYERTYPE_*` -> `BLIP_COLOR_TYPE_*` mapping `PhysicalGameObj::Reset_Radar_Blip_Color_Type`
already uses, reproduced rather than duplicated as a second rule.

## Camera independence

Same construction as `WorldShroudManager`: neither function takes anything resembling a camera,
and coverage comes entirely from `WorldShroudManager::Get_Shroud_State`, which itself only
changes from registered sensor sources (Section 30). "Commander camera position is never radar
coverage" holds because there is no code path from a viewpoint into either function.

## What this phase does not build

No *gameplay* renderer reads `Build_Strategic_Contacts` yet -- the Commander map/sidebar
presentation is still Phase 36+. **Update, a later pass**: it does have a debug one now,
`DEBUG_LAYER_RADAR_CONTACTS` (`Code/Combat/worlddebugdisplay.cpp`'s `Draw_Radar_Contacts`, see
`docs/zerohour/DebugRenderSystem.md`) -- a coloured cross per contact, real proof the shroud-gated
query returns real positions, not the Commander map itself.

**Update, a later pass**: communications buildings now do call
`WorldShroudManager::RegisterStrategicSensorSource`, through Phase 34's `TeamPowerGridService`
integration into `BuildingGameObj`/`BaseControllerClass` (see
`docs/zerohour/TeamPowerGridService.md`) -- making the acceptance lines *"powered communications
buildings can expand configured strategic coverage"* and *"disabling/destroying those buildings
removes their coverage"* concretely true the moment a level places a building whose
`DescriptorName` resolves to a `communications.enabled` descriptor. `TacticalContactClass::GameObj`
is `nullptr` for building/objective/marker contacts -- nothing needs to mutate a building's radar
state today, so no back-reference was added for a write path that has no caller.

## Checks

`renegade -shaderselfcheck radar`, registered as ctest `shader_radar` and `fds_shader_radar`.
Device-less -- `RadarManager::Init()` is deliberately never called (it allocates a
`Render2DClass`, which wants a device), so the check exercises the model directly against an
empty `GameObjManager` list and `RadarManager`'s marker store. What it pins: an empty world
produces zero contacts from both functions without crashing; a registered marker comes back as
exactly one `CONTACT_COMMANDER_MARKER`, never shroud-gated, with no `GameObj` pointer; and
clearing markers empties the strategic list again.

**Not covered by any automated check**: everything that needs a real `PhysicalGameObj` or
`BuildingGameObj` to exist -- `-shaderselfcheck` cannot construct one without a device or a
combat scene. The shroud-gating branch (`SHROUD_VISIBLE`/`RADAR_DETECTED`/`LAST_KNOWN` inclusion,
`UNKNOWN`/`EXPLORED` exclusion) is proved for cells by `terrain_shroud` (P27) and is otherwise
unexercised here for want of a real contact to gate. The FPS-HUD refactor's *visual* equivalence
to the code it replaced is unverified for the same reason every HUD change in this project is:
it needs a display.
