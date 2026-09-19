# Required documentation crosswalk

Roadmap Section 50, Phase 45.

## Why a crosswalk instead of matching paths exactly

Section 50 names a `docs/commander/` directory (8 files) and a handful of `docs/zerohour/`
filenames that differ from the ones actually in use. Every phase from Section 42 onward
put its design doc in `docs/zerohour/`, one file per system, named after the class the
system is built around (`CommanderSidebar.md`, `CommanderPlacement.md`,
`TeamPowerGridService.md`, ...) — the same convention `docs/zerohour/README`-adjacent
files already established for Sections 14-38. That convention has been applied
consistently across roughly fifty files by the time Phase 45 runs. Re-splitting it into
a second `docs/commander/` directory now would duplicate every one of those files' content
under a second path with no functional benefit, and two copies of the same design doc is
exactly the kind of second independent truth this project's own rules argue against
elsewhere (`TeamPowerGridService.md`'s replication section, `RTSPersistence.md`'s "one
subsystem" section).

This file is the resolution: every path Section 50 requires, mapped to where its content
actually lives. Section 50's intent — this project's documentation existing and being
findable — is satisfied by the mapping; the literal directory layout is not.

## Section 50 required paths

| Required path | Actual location | Note |
| --- | --- | --- |
| `docs/integration/OpenW3DBaseline.md` | same path | matches exactly |
| `docs/integration/BaselineKnownIssues.md` | same path | matches exactly |
| `docs/integration/THIRD_PARTY_PROVENANCE.md` | same path | added this phase |
| `docs/tt484/TTParityMatrix.md` | same path | matches exactly |
| `docs/tt484/NativeScriptArchitecture.md` | `docs/tt484/NativeScriptRegistry.md` + `docs/tt484/NativeEventDispatch.md` | split into registry and dispatch when each was written; together they are the native script architecture |
| `docs/zerohour/ZeroHourDonorBaseline.md` | same path | matches exactly |
| `docs/zerohour/ZeroHourCapabilityMatrix.md` | same path | matches exactly |
| `docs/zerohour/OpenW3DPortMatrix.md` | same path | matches exactly |
| `docs/zerohour/WorldTerrainSystem.md` | same path | matches exactly |
| `docs/zerohour/RendererDonorIntegration.md` | `docs/zerohour/ShaderManager.md` | the one-canonical-shader-manager-architecture record Section 52 item 32 asks for |
| `docs/zerohour/SurfaceMarkSystem.md` | `docs/zerohour/WorldSurfaceMarkManager.md` | named after the class, per this project's convention |
| `docs/zerohour/WeatherEnvironmentRenderer.md` | same path | matches exactly |
| `docs/zerohour/ZeroHourAIAdaptation.md` | `docs/zerohour/GuardStateController.md` + `docs/zerohour/TacticalIntelligenceModel.md` | guard/retaliate/path-replan adaptation, and the Commander intelligence-firewall adaptation, are two different AI adaptations and were given two docs |
| `docs/zerohour/RTSTelemetry.md` | `docs/zerohour/RTSTelemetryService.md` | named after the class |
| `docs/commander/CommanderMode.md` | `docs/zerohour/CommanderModeController.md` | named after the class |
| `docs/commander/FPSFirstGameplayInvariants.md` | `docs/zerohour/TacticalIntelligenceModel.md` | the intelligence-firewall/LAST_KNOWN invariants live here, alongside the AI adaptation they constrain |
| `docs/commander/CommanderUI.md` | `docs/zerohour/CommanderSidebar.md` | covers Section 50's required `BUILDINGS`/`GROUND_VEHICLES`/`AIR_VEHICLES`/`INFANTRY`/context-switch/power-bar/facility-binding/queue-presentation list |
| `docs/commander/BuildingDescriptorSchema.md` | `docs/zerohour/BuildingDescriptors.md` | matches by content |
| `docs/commander/BuildingAnchorRoles.md` | `docs/zerohour/BuildingDescriptors.md` (schema) + `docs/zerohour/BuildingConstruction.md` (`Resolve_Anchor_World_Transform` at completion) | anchor roles are a descriptor-schema concept with one runtime consumer; split the same way the code is |
| `docs/commander/BuildingPlacement.md` | `docs/zerohour/CommanderPlacement.md` | matches by content |
| `docs/commander/BuildingConstruction.md` | `docs/zerohour/BuildingConstruction.md` | matches by content, different directory |
| `docs/commander/PowerGrid.md` | `docs/zerohour/TeamPowerGridService.md` | covers Section 50's required schema/formula/shedding/override/canonical-integration/reload/save-load list |
| `docs/commander/StrategicBuildingCapabilities.md` | `docs/zerohour/BuildingDescriptors.md` (Communications/Silo capability blocks) + `docs/zerohour/TeamPowerGridService.md` (Communications coverage, Silo wallet bonus) | capability data and capability behaviour are documented next to the systems that own each |
| `docs/commander/AIProductionAndOrders.md` | `docs/zerohour/CommanderProduction.md` | matches by content, different directory |

## What this does not claim

This is a location index, not a completeness audit. A row above says where the content
for that topic lives, not that the topic is finished — `CommanderProduction.md` and
`BuildingConstruction.md` both still carry "What this phase does not build" sections
naming real gaps, and `gameplay-gaps.md` tracks the player-facing consequences. Section
52's 110-item final-acceptance checklist is a separate, later gate; most of its items
depend on phases (P08-P40 mid-flight, P46-P65 unstarted) that are not done, so it is not
attempted here. This file only answers "where is the doc Section 50 asked for."
