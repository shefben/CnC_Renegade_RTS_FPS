# V2 Baseline (Phase 0)

Roadmap `openw3d_commander_procedural_runtime_world_roadmap_v2.2.0_claude.md`, Phase 0 --
"Verify the v1.1.0 prerequisite and establish the v2 baseline." Required outputs per Section
34's Phase 0 work package: this file, a Phase 0 completed-state entry, and the actual
canonical-owner path list below (not the roadmap's illustrative names).

## Repository identity

- Commit SHA: `e44783fd41f16140116d96379d298c52e1e5b517`
- Branch: `main`
- Build configuration: CMake `build/win32`, Debug, Ninja generator, invoked through
  `b.bat <target>` (`vcvarsall.bat x86` then `cmake --build build/win32 --config Debug --target
  <target>`).

## V1 final-phase status (gate check)

`unstarted_features.md` (delete-only; a remaining line means genuinely unstarted work) currently
holds bullet items under exactly one heading in the whole 66-phase (`P00`-`P65`) v1.1.0 roadmap:
`P00`'s own checklist, which is the original stock-Renegade-content manual runtime verification
(`Q-004`, resolved: accepted as build-only, the listed runtime items are non-blocking manual
validation, not code work). Every other phase heading from `P01` through `P65` --
including `P46`-`P62` (Section 52A visual-fidelity program) and `P63`-`P65` (Section 52B,
explicitly named "procedural-world prerequisites" -- the direct v1-to-v2 bridge) -- carries no
remaining bullet items and is recorded complete in `completed_features.md`. `P46`-`P65` were
individually confirmed complete this pass by reading their `completed_features.md` entries
directly (`P56`: tonemapping/eye adaptation, `P57`: bloom, `P58`: wind field, `P59`: wetness,
`P60`: surface marks, `P61`: progressive damage, `P62`: weapon-fire camera response, `P63`: world
metrics service, `P64`: vertical world structure, `P65`: traversability/connectivity graph).

Conclusion: v1.1.0's final phase is complete. The only outstanding v1 item anywhere is P00's
manual runtime checklist, already on record (`Q-004`/`Q-006`/`Q-012`) as not blocking any other
work. V2 Phase 0's own "do not proceed if v1 final state is not complete" condition does not
apply.

## Canonical v1 owners located (actual repository paths, not the roadmap's illustrative names)

| Roadmap-named system | Canonical owner found |
| --- | --- |
| WorldTerrainSystem | `Code/wwphys/worldterrainsystem.h/.cpp` |
| WorldSpatialIndex | `Code/Combat/worldspatialindex.h/.cpp` |
| Runtime road system | `Code/wwphys/roadsystem.h/.cpp` |
| Sectional bridge system | `Code/wwphys/bridgesystem.h/.cpp`, `Code/wwphys/bridgesection.h/.cpp` |
| Water system | `Code/wwphys/watersystem.h/.cpp` |
| Foliage system | `Code/wwphys/foliagesystem.h/.cpp`, `Code/wwphys/foliagetype.h/.cpp` |
| Shroud/radar tactical intelligence | `Code/Combat/radar.h/.cpp`, `Code/Combat/tacticalintelligence.h/.cpp` |
| Commander camera/input | `Code/Combat/ccamera.h/.cpp`, `Code/Combat/commandermodecontroller.h/.cpp`, `Code/Combat/input.h/.cpp` |
| Commander sidebar/production | `Code/Combat/commandersidebar.h/.cpp`, `Code/Combat/commandersidebarrenderer.h/.cpp`, `Code/Combat/commanderproduction.h/.cpp` |
| Building semantic descriptors | `Code/Combat/buildingdescriptor.h/.cpp`, `Code/Combat/buildingdescriptormanager.h/.cpp` |
| Finite team power grid | `Code/Combat/teampowergridservice.h/.cpp` |
| Commander construction | `Code/Combat/buildingconstruction.h/.cpp`, `Code/Combat/runtimebuildingfactory.h/.cpp` |
| Commander AI production/orders | `Code/Combat/commanderproduction.h/.cpp`, `Code/Commando/commanderorderevent.h/.cpp`, `Code/Combat/commanderguardai.h/.cpp`, `Code/Combat/commanderwaypoint.h/.cpp` |
| Save/load integration | `Code/Combat/combatsaveload.h/.cpp`, `Code/Combat/savegame.h/.cpp`, `Code/Commando/commandosaveload.h/.cpp` |
| Network integration | `Code/Commando/cnetwork.h/.cpp` |
| Dedicated-server target | `renegadeserver` (`Code/Commando/CMakeLists.txt:537`) |
| Client target | `renegade` (`Code/Commando/CMakeLists.txt:523`) |

Every one of these is a real, already-implemented owner -- none needed to be created for this
phase, and per the roadmap's own "do not guess rule" none is guessed: all were located by
grep/glob against the actual tree, not assumed from the roadmap's illustrative names.

## Generator feature flag / config entry point

None exists yet. A repository-wide search for `ProceduralWorld`, `procedural_world`,
`WorldSeed`, `WorldGenerationPipeline` (the roadmap's own illustrative names) returns zero
matches anywhere in `Code/` or `data/`. This is expected -- Phase 1 is where that entry point
gets created -- and is recorded here so Phase 1 does not have to re-discover it.

## Known baseline issues relevant to v2

Recorded here rather than re-litigated, from the gameplay-gap audit pass immediately preceding
this one (`docs/GameplayGapResolutionReport.md`, full detail):

- **8 of the roadmap's 12 named building categories have no `data/building_descriptors/*.json`
  at all** (Barracks/Hand of Nod, Refinery, Repair Bay, War/Vehicle Factory, Airstrip,
  Construction Yard, defense structures). Section 16A's `ProceduralBaseLayoutService` needs a
  real descriptor per initial-base structure a profile wants present; this is a content
  dependency for Phase 11/16A, not a v2 code blocker, but it means Phase 11 cannot place a full
  base until this content exists.
- **No build-prerequisite schema field exists** on `BuildingModelDescriptor` at all (not merely
  unauthored). Not a v2 blocker directly, but Section 16A/18C buildability logic should be aware
  the v1 descriptor schema does not yet carry a prerequisite concept if v2 base-layout ever needs
  to reason about one.
- **`CommanderProductionService::On_Facility_Destroyed`** (queue refund on facility death) and
  **`Cancel_Item`** (single-item cancel) both exist, are correct, and have no real caller. Not a
  v2 blocker; relevant if Phase 24/25 AI Commander economy/production planning ever assumes
  destroyed-facility refund behavior actually fires today -- it does not yet.
- **Commander-role disconnect recovery does not exist** -- nothing clears
  `CommanderProductionService::Get_Team_Commander` when the holder disconnects. Directly relevant
  to Section 25A (Commander governance numeric rules, v2 Phase 23) and to Section 2.2's procedural
  skirmish mode (an AI Commander taking over a vacated human slot needs a real vacancy signal,
  which today never fires).
- **No per-unit population weight** exists (flat `+1` per produced unit). Relevant to Phase 24
  AI Commander economy planning if it needs to reason about population cost per unit type.
- **No air unit roster exists** for either faction. Relevant to Phase 24/25 AI Commander
  production/combat planning, which will need a real air roster before it can plan any air
  production for either side.

None of these block Phase 0 or Phase 1. They are recorded so later phases (11, 16A-dependent
work, 23, 24, 25) do not rediscover them mid-phase.

## Targeted validation

- `renegade` (client): built via `b.bat renegade` -- already up to date at this commit, `ninja:
  no work to do`, confirming the last real build was clean.
- `renegadeserver` (dedicated server): built via `b.bat renegadeserver` -- same result, already
  up to date, clean.
- Per Section 0B/standing project rules, no broader regression was run; neither the game client
  nor server was launched.

## Gate

Satisfied: every v1 system v2 depends on has an identified canonical source owner (table above),
and both baseline targets v2 will extend (`renegade`, `renegadeserver`) build clean at this
commit. Phase 0 is complete. No v1 architecture was modified during this phase.
