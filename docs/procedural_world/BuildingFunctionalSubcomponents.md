# Advanced Building Functional Subcomponents (V2 Phase 20)

Section 21's own purpose line: "Add optional damageable functionality to meaningful W3D building
parts without replacing canonical whole-building health/power behavior." Phase 20 is the last
roadmap phase; it does not touch terrain generation at all, but it is numbered and tracked
alongside Phases 16-19 in `WIP_features.md`, so this doc lives beside theirs.

| Owner | File | Steps |
|---|---|---|
| `BuildingSubcomponentRoleType`, `BuildingSubcomponentDefinition`, `BuildingModelDescriptor::Subcomponents` | `Code/Combat/buildingdescriptor.h/.cpp` | 1-2 |
| JSON parse + validation | `Code/Combat/buildingdescriptormanager.cpp` | 1 |
| `BuildingGameObj::Get_Effective_*`, `Get_Subcomponent_Status`, the `Update_State` hook | `Code/Combat/building.h/.cpp` | 3, 6, 7 |
| `TeamPowerGridService::RecalculateTeam` (generation/vision/radar reads) | `Code/Combat/teampowergridservice.cpp` | 8 |
| `building_subcomponents` console command | `Code/Commando/consolefunction.cpp` | 10 |
| Real descriptor data | `data/building_descriptors/{gdi,nod}_power_plant.json`, `gdi_communications_center.json` | — |

Self-check: `shader_subcomponents` and `fds_shader_subcomponents`. `ctest -N` is now 179, was 177.

---

## The gate: no second health/state system

Section 21's phase gate is explicit: *"Functional damage exists without a second building
health/state system and remains optional per descriptor."* Every design decision in this phase
follows from taking that literally rather than as a slogan.

A `BuildingSubcomponentDefinition` carries no health of its own. It carries two numbers --
`FullFunctionHealthFraction` and `DisabledHealthFraction` -- which are two points on the
*building's own* normalized health, the same `DefenseObject.Get_Health()/Get_Health_Max()`
`DamageVisual`'s table already reads. The function a subcomponent contributes is a straight
linear interpolation between those two points, clamped 0..1 outside them. There is nothing to
save, replicate or repair separately: whatever already keeps a building's health correct across
damage, repair, save and network already keeps every subcomponent's function correct, because
the subcomponent's function *is* a pure read of that one number.

A role absent from a descriptor's `subcomponents` list is not tracked at all --
`Get_Subcomponent_Function_Fraction` returns `-1.0f`, and every caller reads that as "leave my
value alone." This is what "remains optional per descriptor" means operationally: every
descriptor before this phase, and the overwhelming majority after it, declares no
subcomponents and is provably unaffected -- `shader_subcomponents` asserts this directly against
a descriptor built from the same JSON every other descriptor in this tree already uses.

## Steps 1-2: the schema, and how a role resolves to real geometry

Ten roles, exactly Section 21.1's own list, spelled as `BuildingSubcomponentRoleType` so an
authoring typo in JSON is a load-time refusal (`Subcomponent_Role_From_Name`) rather than a role
that silently tracks nothing. Each declared entry carries an optional `anchor_bone` -- a W3D
bone/pivot name, resolved the same way `BuildingAnchorEntry` already is (`Get_Bone_Index` against
every currently-loaded runtime part, first match wins), but *not* through the `Anchors` table's
role indirection: a subcomponent's `anchor_bone` is the bone name itself, not a second role name
to look up. Get_Bone_Index returning 0 (root or "no such bone") is a normal, non-fatal miss --
the anchor is absent from the status report, the numeric effect is unaffected either way.

### Anchor bones were verified, not guessed

Every existing anchor in this tree before this phase (`mct_anchor: "MCT"`,
`purchase_terminal_anchor: "PT01"` on every Comm Center descriptor) carries its own comment
admitting nobody has opened the model to check. This phase does not repeat that. No W3D
inspector tool exists in this environment (the same gap `BuildingComponentAssembly.md`'s door
work already recorded), so bone/pivot names were recovered the same way that pass recovered
`mgpwr_doors.w3d`'s door bone: a direct scan of the real file's printable ASCII runs.

- `data/building_models/gdi/mgpwr/mgpwr_int.w3d` contains the literal string `MGPWR_MCT`
  (material `mct_gdi`) -- the GDI Power Plant's real, confirmed MCT node.
- `data/building_models/nod/mnpwr/mnpwr_int.w3d` contains `MNPWR_MCT` the same way.
- `data/building_models/gdi/gdi_comm/G_Comcntr.w3d` contains `COM_ANTEN000` through
  `COM_ANTEN005` (a six-piece antenna array) and `COM_EXT-ANT`/`COM_EXT-FRM`/`COM_EXT-JAM`/
  `COM_EXT-LOGO`/`COM_EXT-STRC`/`COM_EXT-TWR` (an exterior structure group; `TWR` reads as
  "tower"). The same scan found **no** `MCT` or `PT01` substring anywhere in that file --
  independent confirmation that this file's own pre-existing `mct_anchor`/`purchase_terminal_anchor`
  entries really are the unverified guesses their comment already said they were. They are left
  alone (out of this phase's scope; not broken by it either -- `Resolve_Runtime_Anchor` already
  degrades a miss to "no anchor," never a crash).

### Why the Power Plant's anchor is its MCT, not one of its four aggregates

The real Power Plant assembles from an exterior, interior, four aggregates and a door part
(`BuildingComponentAssembly.md`). Every aggregate but the interior is scanned into this doc's own
working notes and none of the four is documented anywhere, before or after this pass, as "the
generator" specifically -- they carry cracks, damage-state meshes, exterior panel banks and
piston/arc effects, any of which could plausibly be called part of "the generator" with equal
justification and equal lack of evidence. Picking one would have been exactly the guess Q-017's
own rule forbids, aimed at geometry instead of a preset name. The MCT is different: it is the one
part of this asset whose identity is textually unambiguous (`MGPWR_MCT`/`MNPWR_MCT`, confirmed by
name, not inferred from shape), and for a Power Plant specifically its whole function *is* power
generation -- there is no separate "generator" concept to distinguish from "the building's control
node" the way there might be for a building with several unrelated capabilities. So the plant's
own MCT doubles as the anchor for `power_generator_module`.

## Step 3: capability state owned by the canonical building object

There is no new state to own, by the gate above -- but there is a new *query surface*:
`BuildingGameObj::Get_Effective_Power_Generation`/`Get_Effective_Vision_Radius`/
`Get_Effective_Radar_Radius`, each `descriptor value * Get_Subcomponent_Function_Fraction(role,
health)`, and `Get_Subcomponent_Count`/`Get_Subcomponent_Status` for the debug report. All five
are looked up by `Get_Definition().Get_Descriptor_Name()` rather than
`RuntimeDescriptorName`/`Peek_Runtime_Descriptor` -- that pair is inert for a map-authored
building by design (see `building.h`'s own comment on the runtime-constructed state block), but
`DescriptorName` is the definition's own back-link and resolves for *any* instance of a described
definition, map-placed or Commander-built alike, the same resolution
`TeamPowerGridService::Peek_Descriptor` already performs. Health-driven degradation is not a
runtime-construction-only concept; only the anchor/bone *status* is, because only a runtime
building's parts live in `RuntimeParts` -- a map-authored building's numeric generation/radius
still degrades correctly, it simply cannot report which bone represents it.

## Step 4-5: which of the six effect groups are actually wired, and why the rest are not

Section 21's own phase description names six capability-effect groups. This tree has real,
buildable descriptors for exactly two of the building categories any of them could attach to:

| Effect group | Building category | Real descriptor exists? | Wired this phase? |
|---|---|---|---|
| power_generator_module -> generation reduction | Power Plant | Yes (GDI + Nod) | **Yes** |
| communications_array / radar_dish -> comms/radar reduction | Communications Center | Yes, GDI only has real geometry | **Yes, GDI only** |
| refinery_processor -> throughput modifier | Refinery | No | No |
| factory_production_module -> production-time modifier | War/Vehicle Factory, Barracks, Airstrip | No | No |
| repair_module -> repair-rate modifier | Repair Bay | No | No |
| defense_targeting/weapon_module -> defense modifier | any defense structure | No | No |

The four not wired are not a bone-naming gap -- they are the same gap `WIP_features.md`'s own P33
entry already tracks under a different heading: *"the remaining eight of the roadmap's twelve
named categories... have real assets... but no `data/building_descriptors/*.json` at all yet."*
Authoring those four descriptors is P33's backlog item, not Phase 20's; inventing a subcomponent
effect for a building category with no descriptor would mean the effect modifies nothing any
caller can ever construct. Step 5's own instruction -- *"do not implement an effect when the model
lacks a coherent semantic part"* -- reads the same way whether what is missing is a bone or the
whole buildable definition.

Within the two wired groups, three of the four Communications-capable descriptors are also
skipped, and for the same reason: Nod Communications Center and both factions' Advanced
Communications Center all point at `ow_*` model names that do not exist anywhere in this tree
(confirmed -- `find` turns up nothing) and never have (`BuildingComponentAssembly.md`'s own table
says the Nod Comm Center's ~25 real source files were never curated into a usable model, and the
Advanced Comm Center is a project-owned definition with no source asset at all). There is no real
geometry to verify a bone name against for any of the three, so none of them declares a
`subcomponents` block. Their numeric `Communications` capability is untouched and unaffected --
this is a "no anchor to report" gap, not a missing effect, and `Get_Effective_Vision_Radius` on an
instance of one of these descriptors would still return the right number if any of them ever
became constructible; there is simply no subcomponent to degrade with health, so it returns the
descriptor's raw radius unmodified via the `-1.0f` not-tracked path, exactly like every other
undeclared role.

`tiberium_storage` (the tenth role in Section 21.1's list) is not implemented for a third reason:
21.2 gives no worked example of what its effect should be, and Tiberium Silo is itself a
project-owned single-mesh placeholder with no real geometry either. Inventing an effect with
neither a spec nor an asset would be two guesses stacked on top of each other.

## Steps 6-8: damage/repair integration and recompute

Because subcomponent function is a pure read of the canonical health, "integrate with the damage
system" and "integrate with repair" reduce to one question: does anything read the new numbers
again when health changes? `BuildingGameObj::Update_State` -- the single choke point both
`Apply_Damage` and the repair-gun path already call through -- now also asks whether this
building's descriptor declares any of the three grid-facing roles
(`power_generator_module`/`communications_array`/`radar_dish`), and if so calls
`TeamPowerGridService::RecalculateTeam` for its team, exactly the way
`On_Runtime_Destroyed_State_Entered` already does for full destruction. Gated on actually
declaring one of those three roles so the overwhelming majority of buildings -- every descriptor
before this phase and everything after it that does not opt in -- take no extra recalculation on
every hit, honoring `TeamPowerGridService.h`'s own "call on relevant state/config changes, never
every frame."

**A real recursion this pass found and fixed, not merely disclosed:** `RecalculateTeam` drives
`Enable_Power` on every member, and `Enable_Power` calls `Update_State` -- which, after this
phase's own hook, can ask to recalculate the same team a second time from inside the first call.
A naive nested call would stack-overflow. `TeamPowerGridService` now guards `RecalculateTeam`
against re-entering the same team; the nested call is a safe no-op rather than a queued deferral,
because the in-progress outer pass already reads every member's *current* effective
generation/radius, health included, by the time it reaches that member.

`RecalculateTeam` itself now reads `member.Building->Get_Effective_Power_Generation()` /
`Get_Effective_Vision_Radius()` / `Get_Effective_Radar_Radius()` in place of the descriptor's raw
fields whenever a real `BuildingGameObj` is registered, and falls back to the raw descriptor value
for a `nullptr` member -- the self check's own synthetic members, unaffected by this phase, still
pass exactly as they did before it.

## Step 9: save/network

Satisfied by construction, not by new code. Subcomponent function is a pure derivation of
`DefenseObject.Get_Health()`/`Get_Health_Max()`, which already saves and replicates through the
existing building save/export path. Adding a parallel saved/networked field would be exactly the
second independent truth `TeamPowerGridService.h`'s own Section 46 note already forbids for the
power grid ("do not serialize a second independent powered/unpowered truth"), and the same
reasoning applies here without modification.

## Step 10: debug overlay / status report

`building_subcomponents` (client and server console). No per-building selection exists yet
(`WIP_features.md`'s own P36 entry: "no building-selection input... yet"), so it walks every
`BuildingGameObj` in the world (the same `GameObjManager` -> `ScriptableGameObj` ->
`As_BuildingGameObj` cast `worlddebugdisplay.cpp`'s building-footprints layer already uses) and
prints role, current function fraction and anchor-resolution status for every one that declares at
least one subcomponent -- in practice, at most three buildings per side today.

---

## What is proven, and what is not

**Proven by `shader_subcomponents`**, pure data and arithmetic, no scene, no device: the schema
parses and round-trips; an unknown role, a duplicate role, and thresholds that are out of range or
not strictly ordered are all refused at load with the load left unchanged; a declared role with no
`anchor_bone` is a legitimate state, not an error; `Get_Subcomponent_Function_Fraction` is exact at
both thresholds, at the linear midpoint, at a non-zero floor, and clamps rather than extrapolates
outside 0..1 health; an undeclared role returns the `-1.0f` sentinel, never zero and never a
crash; a descriptor with no `subcomponents` block at all -- every descriptor before this phase --
tracks nothing, proving "remains optional" rather than merely asserting it; the role name table
round-trips both directions and refuses an unknown name.

**Not proven here, for the same reason every runtime-building construction path in this harness
is not:** `-shaderselfcheck` has no `PhysicsSceneClass`/`GameObjManager` scene
(`Run_Runtime_Door_Service`'s own note says so explicitly, and no `Run_*` in this file has ever
constructed a real `BuildingGameObj`). So `Get_Effective_Power_Generation` actually reading a
real, damaged building's health, `TeamPowerGridService::RecalculateTeam` actually seeing that
number change, `WorldShroudManager`'s registered sensor radius actually shrinking, and
`building_subcomponents`' own anchor-resolution check actually finding `MGPWR_MCT` on a real
loaded model are none of them exercised by automation. This is the same category of gap the whole
Q-012 backlog already tracks for every other building system in this tree, not a new one.

## Next exact action

A manual pass, folded into the same live Procedural Skirmish / skirmish-map session Q-012 already
asks for: construct a Power Plant, damage it below 75% health (its `power_generator_module`
threshold), confirm the team's generation total in the power bar/`worldop`-adjacent status drops
proportionally, repair it back above 75%, confirm generation is restored; do the same for a
Communications Center's vision/radar radius and confirm the shroud actually shrinks and recovers;
run `building_subcomponents` at the console throughout and confirm the printed function fractions
track what damage/repair actually did, and that `anchor MGPWR_MCT`/`COM_ANTEN000`/`COM_EXT-TWR`
report `resolved` against the real loaded models rather than `not resolved`.
