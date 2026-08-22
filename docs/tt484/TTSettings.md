# TT 4.8.4 settings — native plan

## 1. Why this exists

TT ships most of its behaviour as *configuration*, not as unconditional code.
`tt.ini` and `hud.ini` are read once at startup and the values decide which
memory patches `tt.cpp` installs and how the runtime behaves thereafter.

Earlier passes in this phase treated "the TT change is gated on an ini option
that defaults to off" as a reason to decline the merge, on the reading that a
server toggle is not a correction to stock behaviour. **That reading is
withdrawn.** The user's direction is explicit:

> the entire point of implementing the tt code natively is to have all its
> features and fixes but without the engine patching and extra scripts2.dll

So the settings are part of the deliverable. Nothing is declined for being
configurable; a TT option becomes a first-class engine setting with the same
name, the same default and the same effect.

### Relationship to directive 0.4

Roadmap directive 0.4 forbids "duplicate stock-vs-TT implementations selected
by flags or wrappers". That still holds and is not in tension with this
document once the distinction is drawn:

- **Forbidden** — keeping the stock function *and* the TT function in the tree
  and picking one at runtime. That is what the DLL-hook architecture did.
- **Required here** — one merged implementation in the canonical owner whose
  behaviour is parameterised by a named, documented server setting.

There is exactly one implementation of each behaviour. `DisableCostMultiplier`
does not select between two code paths; it is an input to the single code path
that prices purchases.

## 2. Scope

| Source | Reader | Options | In scope |
| --- | --- | --- | --- |
| `tt.ini [General]` | `engine2.cpp:606-640` | 29 | yes |
| `tt.ini [General]` | `hud.cpp:ReadHUDIniBits`, called with `ttini` | 37 | yes |
| `hud.ini [General]` | `hud.cpp:ReadHUDIniBits2` | 31 (9 scalar + 22 colour) | yes |
| `hud.ini [General]` | `shaders/shaderhud.cpp:ReadHUDBits` | 176 | **no** — directive 0.6 |
| mod/gameplay-mode packs (`CTF`, `teamspeak`, `PointsDistribution`, …) | their own | 63 | **no** — out of the P04 scope reduction |

Note the trap: `ReadHUDIniBits` takes a parameter named `hudini` but
`engine2.cpp:646` calls it with **`ttini`**. Those 37 options live in `tt.ini`,
not `hud.ini`. A native reimplementation that goes by the parameter name will
silently read the wrong file.

`Get_Data_File("tt.ini")` is also hashed through `CheckLoad` for TT's
client/server consistency check; that is part of TT's anti-cheat channel and is
tracked separately, not here.

## 3. Native design

`TTSettingsClass` in `Code/wwlib` — the lowest library that everything above
links, which is what the consumers require: `wwphys` (vehicle flip kill),
`Combat` (unsquishable, weather, points, draw distance), `wwui` (list colours,
style colours), `Commando` (purchase terminal, HUD, message colours).

- static accessors only, no instance;
- `Load()` called once during engine init, after the file factory is up and
  before the first consumer runs;
- values are plain typed members with the TT default baked in, so a missing
  file leaves every option at TT's own default and the engine behaves exactly
  as TT does with no ini present;
- `StringClass`/`Vector3` rather than TT's `new char[260]` leaks.

## 4. Option inventory and status

Status values: **merged** the setting exists and is honoured; **wired** the
behaviour is merged but still unconditional and needs the setting; **open** not
yet implemented.

### 4.1 `tt.ini` — gameplay (`engine2.cpp`)

| Option | Type | Default | Behaviour it drives | Status |
| --- | --- | --- | --- | --- |
| `ContinueReloadOnVehicleExit` | bool | true | reload continues after leaving a vehicle | open |
| `DisableCostMultiplier` | bool | false | suppress the unpowered-base 2x purchase cost | open |
| `BuildTimeDelay` | float | 2.0 | vehicle build time floor | open |
| `VehicleOwnershipDisable` | bool | false | drop TT vehicle ownership enforcement | open |
| `VehicleBuildingDisable` | bool | false | alternate per-team production model; also gates the PT "building" message | open |
| `DisableVehicleFlipKill` | bool | false | a rolled-over vehicle does not self-destruct | **wired** |
| `Unsquishable` | bool | false | infantry cannot be run over | open |
| `UnsquishableArmor`…`4` | int | 0 | armour types exempt from squishing | open |
| `NeutralVechiclePointsFix` | bool | true | points for neutral-vehicle kills | open |
| `DrawDistance` | float | 300 | hibernation/draw distance | open |
| `ScriptsLastTeamTime` | int | -1 | last-team bookkeeping for scripts | open |
| `NukeWeatherDisable` | bool | false | suppress nuke weather effect | open |
| `IonWeatherDisable` | bool | false | suppress ion weather effect | open |
| `MapPrefix` | string | `C&C_` | map-name prefix used by map cycling | open |
| `NodHouseRed/Green/Blue` | int | 255/0/0 | Nod team colour | open |
| `GDIHouseRed/Green/Blue` | int | 255/204/0 | GDI team colour | open |
| `PrivateMessageRed/Green/Blue` | int | 0/0/255 | private chat colour | open |
| `PublicMessageRed/Green/Blue` | int | 255/255/255 | public chat colour | open |

### 4.2 `tt.ini` — HUD and purchase terminal (`ReadHUDIniBits`)

| Option | Type | Default | Behaviour it drives | Status |
| --- | --- | --- | --- | --- |
| `UseExtraPTPages` | bool | false | secret PT pages available, including in laddered games | open |
| `NewUnpurchaseableLogic` | bool | false | PT availability from the replicated `Can_Generate_*` flags | **wired** |
| `RefillLimit` | float | 0 | refill cooldown | open |
| `AlternateSelectEnabled` | bool | false | alternate sidebar selection art | open |
| `SidebarSoundsEnabled` | bool | false | sidebar purchase sounds; also gates the stock sound suppression | open |
| `SidebarRefillSound` / `SidebarInfantrySound` / `SidebarVehicleSound` | string | "" | those sounds | open |
| `GDIUpArrowTexture` … `NODBackgroundTexture2` (8) | string | "" | sidebar art per team | open |
| `GDIAlternateSelectTexture1`…`4`, `NODAlternateSelectTexture1`…`4` | string | "" | alternate selection art | open |
| `ModReg` | string | "" | registry subkey for mod settings | open |
| `VersionReg` | string | `Software\Westwood\Renegade` | registry path for the version string | open |
| `WOLUrlReg` | string | `WOLSettings\URL` | registry path for the WOL URL | open |
| `DisableKillMessages` | bool | false | suppress kill messages | open |

### 4.3 `hud.ini` — UI (`ReadHUDIniBits2`)

| Option | Type | Default | Behaviour it drives | Status |
| --- | --- | --- | --- | --- |
| `ScrollingRadarMap` | bool | false | scrolling rather than fixed radar | open |
| `ListColumnColorEnabled` + `…Red/Green/Blue` | bool/int | false, 255 | list-control column tint | open |
| `HidePlayerList` | bool | false | hide the in-game player list | open |
| `HideBottomText` | bool | false | hide the bottom HUD text | open |
| `LodBudgetDialogValue1` / `2` | int | 5000 / 10000 | LOD budget slider stops | open |
| 14 `StyleMgrClass` colours (`TitleColor`, `TextColor`, `BkColor`, …) | colour | stock values | dialog styling | open |
| 8 further colours (`DialogTextTitleColor`, `MenuHiliteColor`, `MerchandiseTextColor`, `MenuStaticGlowColor`, …) | colour | stock values | menu styling | open |

Note `lodbudgetval1 = max(lodbudgetval1, 65535)` in `hud.cpp:257` — `max`, not
`min`, so both LOD values are pinned to 65535 and the two options have no
effect whatsoever. Reproduce the option, not the bug; the native version
clamps with `min`.

## 5. Hook sites this reopens

`TTHookSites.tsv` rows moved off a terminal disposition by this decision:

| Intent | Rows | Was | Now |
| --- | --- | --- | --- |
| `enable secret PT pages` | 2 | n/a, ini-gated | open, needs `UseExtraPTPages` |
| `dead powerplant 2x cost disable` | 2 | deferred, ini-gated | open, needs `DisableCostMultiplier` |
| `purchase terminal "building" message change` | 1 | n/a, TT subsystem | open, needs `VehicleBuildingDisable` |
| `Do not load all .mix files at startup` | 4 | declined | open, re-examine against the OpenW3D file factory |
| `make vehicles not die when they flip over` | 2 | merged | merged-needs-setting |
| `new unpurchasable logic` | 10 | merged | merged-needs-setting |

The two `merged-needs-setting` clusters are the more urgent: both were merged
unconditionally, and TT gates both on an option that **defaults to off**. As
they stand the engine does something TT would not do out of the box.

Only two dispositions survive as terminal, and neither is a judgement call:
the 12 `DROP` rows null out stock debug routines and have no behaviour to port,
and the `radar fix` is provably a no-op (`NativeEventDispatch.md` 5.8). The 104
shader rows remain out of scope under directive 0.6.

## 6. Order

1. `TTSettingsClass` with the full inventory above and TT's defaults;
2. load it during engine init and gate the two `merged-needs-setting` clusters;
3. the four reopened hook clusters;
4. the remaining gameplay options (`Unsquishable`, weather, points, draw
   distance, `ContinueReloadOnVehicleExit`, `BuildTimeDelay`);
5. UI and colour options, which are inert until their consumers are touched
   anyway;
6. `VehicleBuildingDisable`, last — it is a whole alternate production model,
   not a switch, and it carries the PT "building" message with it.
