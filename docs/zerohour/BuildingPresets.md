# Building presets: resolving Q-017 from real installed data

Roadmap Section 44 / **Q-017**. Records how each Commander-buildable descriptor's
`game_object_preset` was chosen, so this does not have to be rediscovered.

## How this was resolved

Guessing a preset name and shipping a descriptor that loads but refuses at placement was
explicitly ruled out. Instead, `C:\Westwood\Renegade_full\Data\Objects.DDB` -- the
installed retail game's real definition database, mounted through `Always.dat` the same
way real game startup mounts it -- was loaded and every `CLASSID_BUILDINGS` definition in
it enumerated: name, id, class id, and every editable parameter. The tool that did this
is not a one-off script; it is a permanent LevelEdit command-line mode
(`leveledit.exe -dumpbuildingdefs=<out> -datadir=<installed Data dir>`,
`Code/Tools/LevelEdit/BuildingDefinitionDump.{h,cpp}`), kept because the same question
will come up again for every other building type the roadmap eventually wants
Commander-buildable.

## Resolved presets

| Descriptor | `game_object_preset` | Real class | Team |
| --- | --- | --- | --- |
| `gdi_power_plant` | `mp_GDI_Power_Plant` | `PowerPlantGameObjDef` (`CLASSID_GAME_OBJECT_DEF_POWERPLANT`) | GDI |
| `nod_power_plant` | `mp_Nod_Power_Plant` | `PowerPlantGameObjDef` | Nod |
| `gdi_communications_center` | `mp_GDI_Com_Center` | `ComCenterGameObjDef` (`CLASSID_GAME_OBJECT_DEF_COMCENTER`) | GDI |
| `nod_communications_center` | `mp_Nod_Com_Center` | `ComCenterGameObjDef` | Nod |
| `gdi_advanced_communications_center` | `ow_gdi_adv_comm_center` | `ComCenterGameObjDef`, project-owned | GDI |
| `nod_advanced_communications_center` | `ow_nod_adv_comm_center` | `ComCenterGameObjDef`, project-owned | Nod |
| `gdi_tiberium_silo` | `ow_gdi_tiberium_silo` | `BuildingGameObjDef` (base), project-owned | GDI |
| `nod_tiberium_silo` | `ow_nod_tiberium_silo` | `BuildingGameObjDef` (base), project-owned | Nod |

GDI and Nod do not share a preset for anything -- confirmed by the dump, not assumed.
`mp_GDI_Power_Plant` and `mp_Nod_Power_Plant` are two separate definitions, both class
`CLASSID_GAME_OBJECT_DEF_POWERPLANT`, same as every other faction pair the dump showed
(`mp_GDI_Barracks`/`mp_Hand_of_Nod` both `SOLDIER_FACTORY`, `mp_GDI_Repair_Bay`/
`mp_Nod_Repair_Bay` both `REPAIR_BAY`, and so on).

## Rejected alternatives

The dump surfaced many other `CLASSID_BUILDINGS` definitions that name-match a target
building semantically but were not used:

- **`SP_Power_Plant_GDI`, `SP_Power_Plant_Nod`, `Tut_Power_Plant_GDI`** and similar
  `SP_*`/`Tut_*`/`Test_*`-prefixed definitions -- single-player-mission and tutorial
  placements, all generic `CLASSID_GAME_OBJECT_DEF_BUILDING` (class `0x0000D001`, the base
  class with none of `PowerPlantGameObjDef`'s specialisation), scattered across specific
  missions' own geometry and balance. Not the real, specialised multiplayer economy
  building, and picking the first name match here would have been exactly the mistake
  Q-017 warned against.
- **`SP_Comm_Center_Nod`** -- same reasoning: single-player, generic base class, not
  `ComCenterGameObjDef`.
- **`SP_Silo_Nod`** -- the only definition anywhere in the retail data with "Silo" in its
  name. A single-player mission's set-dressing prop, generic base class, no gameplay
  behaviour of its own. It is not this project's Tiberium Silo (a multiplayer team
  wallet-capacity mechanic that does not exist in stock Renegade at all) and reusing it
  would have tied our economy building to a campaign prop's geometry and bounds for no
  reason.
- **No `mp_*` Advanced Communications Center of any kind exists.** Confirmed by exhaustive
  search of the full dump, not inferred from a naming convention. Section 44's "tactical
  coverage" tier is a new concept this project is adding; stock Renegade has ordinary and
  advanced guard towers (`mp_GDI_Advanced_Guard_Tower`) but nothing analogous for the
  Communications Center.

## Project-owned presets: why these classes, not a generic one

Q-017 required a deliberate class choice for anything with no stock equivalent, not a
bare `BuildingGameObjDef` merely so placement succeeds:

- **Tiberium Silo -> plain `BuildingGameObjDef`.** Its entire gameplay contribution is
  the team wallet-capacity bonus, already fully data-driven through
  `BuildingModelDescriptor::TiberiumSilo` and `TeamPowerGridService` -- nothing about it
  needs a specialised C++ class. This is not a shortcut: real stock multiplayer defense
  structures (`mp_GDI_Advanced_Guard_Tower`, `mp_Nod_Turret`, `mp_Nod_Obelisk`,
  `mp_Nod_Sam_Site`) are the *same* base class for the *same* reason -- their gameplay
  also lives in other systems (weapon/targeting components), not in a `BuildingGameObjDef`
  subclass.
- **Advanced Communications Center -> `ComCenterGameObjDef`**, the identical class stock
  `mp_GDI_Com_Center`/`mp_Nod_Com_Center` use. It is architecturally the advanced tier of
  the same building type. `ComCenterGameObjDef` today implements stock Renegade's
  harvester-unload bookkeeping (`FundsGathered`, `HarvesterDefID`), which a
  Commander-built instance simply never exercises -- harmless, and the correct class to
  receive Section 44's still-unbuilt "tactical coverage" behaviour once that is written,
  rather than inventing a second class that would need migrating onto later.

Constructed and registered in code
(`Code/Combat/projectbuildingdefs.{h,cpp}`, `ProjectBuildingDefs::Register`), the same
`DefinitionMgrClass::Register_Definition` call LevelEdit's own preset editor uses for a
brand-new preset -- never touching `always.dat`. Re-registers every level load (not just
the first), because `SaveGameManager::Load_Definitions` in `combat.cpp` is itself called
per level load, preceded by `DefinitionMgrClass::Free_Definitions`, which would otherwise
silently drop these four presets on the second map.

## The `DescriptorName` back-link

`BuildingGameObjDef::Get_Descriptor_Name` (Section 40) is what
`BuildingGameObj::Initialize_Building` reads to register a building with
`TeamPowerGridService` -- and no stock or project-owned preset's `DescriptorName` field is
ever authored, since Section 40 postdates the retail game by two decades and the
project-owned presets are constructed with nothing but a name and an id. Without wiring
this, every real building -- stock preset or project-owned -- would register as a neutral,
non-generating, non-consuming member no matter what its descriptor says.

`BuildingDescriptorManager::Resolve_Game_Object_Presets` closes this: after descriptors
and definitions are both loaded, it walks every loaded descriptor that names a
`game_object_preset`, resolves it through `DefinitionMgrClass::Find_Named_Definition`,
confirms `SuperClassID_From_ClassID(...) == CLASSID_BUILDINGS` (refusing loudly otherwise,
via `WWDEBUG_SAY`), and calls the new `BuildingGameObjDef::Set_Descriptor_Name` setter.
Called from `combat.cpp` every level load, right after `ProjectBuildingDefs::Register`,
because the definition objects themselves are fresh each time.

No preset name is hard-coded into this or any construction-path code: the mapping is
entirely descriptor JSON -> `game_object_preset` -> resolved definition, in that
direction, exactly the direction `RuntimeBuildingFactory::Create_Constructing_Building`
already used.

## The descriptor directory was never actually loaded at real startup

A separate, pre-existing gap this work also closed: nothing in production code ever
called `BuildingDescriptorManager::Load_Descriptor_From_File` for
`data/building_descriptors/*.json` at real game startup -- only the `-shaderselfcheck`
harness and LevelEdit's Q-015 validate command ever populated the registry. Every
descriptor in this project would have loaded successfully and then never been found by
`BuildingDescriptorManager::Get_Descriptor` in an actual running game. Fixed by
`BuildingDescriptorManager::Load_Directory` (the same Win32 directory walk
`Validate_Directory` already used, `Load_Descriptor_From_File` instead of `Validate_File`),
called once from `combat.cpp` guarded by a function-local static, since (unlike
`DefinitionMgrClass`) `BuildingDescriptorManager`'s own registry is not wiped by
`Free_Definitions` and does not need reloading every level.

## Validation now checks preset resolution, when there is anything loaded to check against

`BuildingDescriptorManager::Validate_Descriptor` (Q-015) now also resolves
`game_object_preset` through `DefinitionMgrClass` and confirms the superclass, failing
loudly (per Q-017's explicit instruction) when a named preset does not resolve or
resolves to something that is not a building -- but only when at least one
`CLASSID_BUILDINGS` definition is registered at all. An editor session or most
self-check runs have no `Objects.DDB` in reach and cannot distinguish "wrong name" from
"nothing to check against"; the same missing-data rule that already governs every other
check in that function applies here too.
