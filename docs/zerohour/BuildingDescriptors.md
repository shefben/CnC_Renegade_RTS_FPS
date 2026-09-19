# Building model semantic JSON + live building definitions

Roadmap Section 39, Phase 33.

`Code/Combat/buildingdescriptor.{h,cpp}` (the resolved descriptor),
`Code/Combat/buildingdescriptormanager.{h,cpp}` (the registry and the reload transaction),
`Code/wwlib/nlohmann/json.hpp` (vendored JSON library, see Q-013),
`data/building_descriptors/*.json` (example content), console commands
`reload_building_descriptors`/`reload_building_descriptor`/`building_json_status`.

## Scope: the load-bearing half, built for real

Section 39's full seven-step load pipeline eagerly enumerates a W3D model's *complete*
hierarchy/subobject graph, captures every node's transform, bounds and render/collision type, and
validates every named node exists, before producing a descriptor. That is a large, separate
integration with the asset-loading pipeline, and this pass did not have room to build it well
alongside everything else in this same session. What *is* built for real, because Phase 34 (the
power grid, next) and Phase 38 (procedural construction) directly depend on it existing and being
correct, is:

- a validated `BuildingModelDescriptor` with queryable power, capability and construction-visual
  values;
- the reload transaction, exactly as specified: valid replaces atomically, invalid preserves the
  last known-good descriptor and reports the error;
- semantic anchors resolved *by bone name* against the real model, on demand, rather than eagerly
  captured -- see below.

## JSON library: vendored, not vcpkg

The roadmap asks for `nlohmann-json` "through the existing vcpkg/CMake dependency system." This
machine has no `VCPKG_ROOT` and no vcpkg installation, and the actual `build/win32` tree `b.bat`
builds has no vcpkg toolchain wired into it at all -- `vcpkg.json`'s dependencies are for a preset
this build does not use. Bootstrapping vcpkg from the network mid-pass was judged out of scope.
Recorded as **Q-013** in `awaiting_answers.md`: the standard alternative, a single vendored header
pinned to a tagged release (`v3.11.3`), at `Code/wwlib/nlohmann/json.hpp`. It resolves via
`#include "nlohmann/json.hpp"` because `wwcommon`'s `INTERFACE` include directories already expose
`wwlib` to every target -- no new CMake include-path wiring was needed. This is not a hand-rolled
parser; it is the same library the roadmap asked for, distributed the other standard way.

## The descriptor

`BuildingModelDescriptor` holds `DefinitionName`/`ModelName`/`SchemaVersion`, a `Footprint`
(width/depth), the required `power` block (`requires_power`/`generation`/`consumption`/
`priority`), the two named capabilities (`communications`, `tiberium_silo`), the
`construction_visual` tuning block Phase 38 will read, and a list of `(role, bone_name)` anchor
entries. Every field name in the JSON schema matches the roadmap's own examples verbatim.

### Anchors resolve late, by bone name

`Resolve_Anchor_World_Transform(building_model, building_world_transform, role, &out)` implements
Section 39's own formula, `worldTransform = buildingWorldTransform * nodeModelTransform`, by
looking up the role's declared bone name and asking the real, instantiated model's `HTree` for
that bone's transform through `RenderObjClass::Get_Bone_Index`/`Get_Bone_Transform` -- the exact
mechanism `CompositeAttachmentClass` (Section 29) already established for "the bone name is the
transform, not a second implementation of what `HTree` already does." `Get_Bone_Index` returning
`0` for both "root" and "no such bone" is read the same way P26 and Section 31's tactical model
already read it: a role cannot resolve to a component's root. This is what makes "never replace
model-node semantics with hard-coded mapper world coordinates" true without needing the eager
hierarchy-enumeration step: there is nothing to keep in sync, because nothing is cached.

### The one role-compatibility rule this scope checks

Section 39 asks for "incompatible capability/building role" to be rejected but does not enumerate
every incompatible pair, and this scope has no general role-conflict model. The one concrete case
checked: a generator (`requires_power == false`) cannot also declare `communications.enabled`,
because no donor definition is both.

## The reload transaction

`BuildingDescriptorManager::Reload_Descriptor` re-reads the file a descriptor was loaded from,
parses and validates into a *temporary* descriptor, and only on success replaces the live one with
one assignment (`_Records[index].Descriptor = candidate`). A read failure or a validation failure
increments `Get_Reload_Failure_Count`, sets `Get_Last_Error`, and returns without touching the live
descriptor at all -- checked end to end against a real scratch file by `shader_buildingjson`, not
only asserted. `Reload_All_Descriptors` reloads every file-backed descriptor independently; one
definition's failure never touches another's. A descriptor loaded from text
(`Load_Descriptor_From_Text`, what the self-check and any future in-memory caller use) has no file
to reload from and `Reload_Descriptor` refuses it rather than guessing a path.

## Console commands

`reload_building_descriptors`, `reload_building_descriptor <definition>` -- the roadmap's own
named commands, verbatim -- plus `building_json_status` for visibility. All server-console-class
commands; nothing here grants a client authority, since nothing here has network code at all yet
(see "What this phase does not build").

## Example content

Four real, valid descriptors under `data/building_descriptors/`: `gdi_power_plant.json` (a
generator), `gdi_communications_center.json` and `gdi_advanced_communications_center.json` (one
implementation, larger JSON values for the Advanced variant -- the roadmap's own instruction), and
`gdi_tiberium_silo.json`. These are the four categories Phase 34 needs first (a generator, a
comms-capable consumer at two tiers, a Silo). The full twelve-building GDI/Nod coverage list
Section 39 names is not written this pass -- more content, not more code, and named here rather
than silently dropped.

## What this phase does not build

- **Eager hierarchy enumeration/anchor validation at load time.** See "Scope" above -- anchors
  are validated lazily, when actually resolved against a real model.
- **Content-hash caching.** `Load_Descriptor_From_File` always re-reads and re-parses; there is no
  cache keyed on model/JSON content hash yet. Descriptor counts in this project are small (dozens
  at most), so this is a real cost with no measured impact yet, not a correctness gap.
- **`dump-building-model`/`generate-building-json-skeleton` tooling.** `validate-building-json`'s
  and `validate-all-building-descriptors`' functionality exists as `Load_Descriptor_From_Text`/
  `_From_File`'s own validation and `shader_buildingjson`'s coverage, reached through the
  self-check and console commands this project already uses in place of separate CLI tool
  binaries -- consistent with every other phase this session, not a missed item.
- **Anything that loads a descriptor at engine startup.** No such call site exists; a descriptor
  is only ever loaded from a `reload_building_descriptor*` console command or a self-check.
  Building placement/construction (Phases 37-38) are the first intended callers of
  `Resolve_Anchor_World_Transform`.

**Update, a later pass**: `BuildingGameObj` *does* now know its own descriptor key. See
`docs/zerohour/TeamPowerGridService.md`'s "Wired into `BaseControllerClass`, and into every
`BuildingGameObj`" -- `BuildingGameObjDef::DescriptorName` (an editable, saved string, default
empty) is the field this section originally named as missing, and `Initialize_Building` now
registers every building with `TeamPowerGridService` using it. No preset in this tree's own
content sets it yet -- that is authoring the twelve-building coverage list above, not code -- but
the moment one does, `Get_Descriptor(DescriptorName)` starts resolving for a real, live building
with no further code change on either side.
- **Network replication of a reload.** "Clients never gain gameplay authority by editing local
  JSON" holds today by construction -- there is no client-reachable path to any of this code at
  all, since nothing calls it from gameplay yet.

## Three more blocks, added by Phases 36-38

The schema Section 39 specifies covers power, capabilities, construction visuals and anchors. Three
things the later Commander phases need had no home in it, and each is JSON rather than C++ for the
same reason the power block is -- Section 39's own *"do not hard-code Power Plant output or building
draw in C++"* applies just as much to a building's price and its slope limit as to its wattage.

```json
{
  "build": {
    "buildable": true,
    "cost": 1500,
    "build_time_seconds": 20.0,
    "tech_level": 0,
    "team": 1,
    "display_name": "Power Plant",
    "icon_texture": "ow_pt_powerplant.tga"
  },
  "production": {
    "kind": "ground_vehicle",
    "queue_capacity": 5,
    "requires_power": true
  },
  "placement_rules": {
    "max_slope_degrees": 15.0,
    "exit_clearance": 8.0,
    "requires_foundation_support": true
  }
}
```

- **`build`** is Phase 36's card: what the Commander sidebar shows and what a purchase costs.
 `team` is a `PLAYERTYPE_*` value, or `-1` (the default) for "any faction may build this".
 `build_time_seconds` wins over `construction_visual.duration_seconds` when both are set, resolved
 once by `Get_Build_Time_Seconds()` so no caller has to remember which block wins. A descriptor with
 no `build` block, or with `buildable` false, is a perfectly valid building -- it simply is not one
 the Commander may place, which is most of them.
- **`production`** is what Phase 36 binds the contextual sidebar to and what Phase 39 queues
 against. One enumerated `kind` (`none`/`infantry`/`ground_vehicle`/`air_vehicle`) rather than three
 booleans, because *"Airstrip configured as ground factory"* is a real case in the roadmap's own
 list and it is the descriptor that decides, not the model name. An unknown `kind` string is a
 validation failure, not a silent `none`.
- **`placement_rules`** is Phase 37's per-definition half of the placement query -- the slope a
 foundation tolerates and how much clear ground a production exit needs in front of it. The
 world-side half (overlap, bounds, support) comes from `WorldSpatialIndex` and `WorldTerrainSystem`
 and is not in JSON.

All three are validated with the rest: a negative cost, a non-positive queue capacity, a slope
outside 0-90 degrees or a negative clearance each refuse the descriptor and leave any previously
loaded one untouched, exactly like every other field.

## Checks

`renegade -shaderselfcheck buildingjson`, registered as ctest `shader_buildingjson` and
`fds_shader_buildingjson`. Exercises real file I/O (a scratch JSON file written, reloaded, and
rewritten by the check itself, then deleted) alongside device-less in-memory parsing. Pins: valid
resolution and power/capability/anchor queryability for a generator and a communications building;
rejection of malformed JSON, negative generation, and the generator+communications incompatibility,
each with a non-empty error and no descriptor registered; the reload transaction succeeding and
publishing a new value; the reload transaction failing on an invalid rewrite and leaving the prior
value in place, counted as a failure; a text-loaded descriptor refusing to reload; and
`Unload_All`/`Shutdown` clearing the registry.

**Not covered by any automated check**: `Resolve_Anchor_World_Transform` against a real
instantiated model (needs a device/asset manager with real `.w3d` content, the same gap every
model-dependent check in this project has), and the three console commands, which need a person at
a prompt.
