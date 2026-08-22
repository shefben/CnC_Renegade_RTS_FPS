# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P03: merge TT hooks/patches/overrides into canonical implementations

The event layer is done (P03-A) and `TTHookSites.tsv`'s `disposition` column
gives the exact standing: **488 merged, 114 open**, 54 n/a, 104 out of scope
under directive 0.6 (P03-A through P03-O). Absorbs the backlog line
"Acceptance: No required TT gameplay feature depends on modifying executable
memory or knowing a hard-coded function address", not yet met. What is left is
a long tail: no cluster is bigger than two rows. The named ones with real work
in them are `better scope code` (2, a per-weapon scope texture and zoom range
from `scopes.cfg`), the beam-manager shutdown crash (2), `multihudclass
rendering improvements` (2), `send stealth state over the network` (2),
`Fix printf issue` (2), `enable secret PT pages` (2, and its `UseExtraPTPages`
option), `hook for VehicleBuildingDisable stuff` (2), plus singles for the
`wall lag fix`, `join after server is gameovered fix`, `emoticons fix`,
`powerup sound patch`, `MeshModelClass destroy patch`,
`Fix memory leak in cMiscUtil::File_Exists`, `patch Get_OS_Info`, the WOL sort
order and favourite-server rows, and the console/registry string changes. The
rest are `hook X` rows that will resolve to canonical the same way the last
sixteen clusters did.

Next exact action: `better scope code` -- TT's `WeaponClass::Select`/`Deselect`
(`tt_4.8.4/tt/WeaponClass.cpp:49`) swap the sniper HUD overlay and the camera's
min/max zoom per weapon from a `scopes.cfg` list parsed in
`tt_4.8.4/tt/hud.cpp:81`. Canonical has `CameraProfileClass::Set_Zoom` and the
sniper HUD in `Code/Combat/hud.cpp`; the work is a scope table plus the texture
and zoom application on select. After that the beam-manager shutdown crash.

Two rows are deliberately left open rather than guessed at, each with the
reason in `mapping_method`: `C4GameObj::Export_Rare` / "C4 getting stuck to
repair bay arc fix" (delta not identified), and
`CombatManager::postSceneInit` / "disable collision group for ladder fix" (TT
drops the SOLDIER_GHOST-vs-DEFAULT collision pair, which changes how every
ghosted soldier meets doors, elevators and crates, not only ladders -- needs
in-game evidence).

Method that is working, for the next session: TT reimplemented most of what it
hooks purely so it had something to hook, so the default answer for an
`X hooks` cluster is "canonical already supersedes" and the job is to find the
few rows where TT went somewhere OpenW3D had not. `fndiff.py CLASS donor.cpp --
canon.cpp` extracts and normalises both sides; piping its output through
`awk '/^######## /{fn=$2} /^\+/ && !/^\+\+\+/{print fn": "$0}'` shows only the
donor-added lines, which is the fastest way to spot TT-forward content in a
large class. `disp.py` settles a whole cluster with the reasoning in
`mapping_method`. The named clusters (`... fix`, `... fixes`, `... patch`) are
where the real gameplay work is and are worth doing first.

## P04: native stock + TT script registry

The stock half is complete (P04-A, P04-B). Absorbs the three remaining backlog
lines: "Compile unchanged stock scripts only when TT does not supersede them",
"For stock scripts modified/replaced by TT, merge TT changes into the canonical
source" and "Compile TT-only new scripts as additional canonical scripts". The
donor half is now scoped to the original TT library only (P04-E): 861 donor-only
scripts across 24 files, not 2142. The 13 script names the donor redefines are 1
of 13 merged (P04-C, `M00_Advanced_Guard_Tower`), and all 13 sit inside that
scope. The remainder is less blocked than recorded (P04-D): 406 of the SDK's 690
functions have portable source, and all 284 closed-binary bindings sit in
`engine_tt.h` alone. Next exact action: merge the remaining 12 replacements
listed in `TTParityMatrix.md` 3.1 -- the `M00_*` cluster in `jfwws.cpp` is the
largest group -- taking the donor side, and register each as
`SCRIPT_SOURCE_STOCK_MERGED`.

## P03-INI: the rest of the TT option surface

`TTSettingsClass` exists and the gameplay half is now wired through it (P03-G,
P03-INI-B). Honoured so far: `DisableCostMultiplier`, `DisableVehicleFlipKill`,
`NukeWeatherDisable`, `IonWeatherDisable`, `Unsquishable` and its four armour
exemptions, `DrawDistance`, `BuildTimeDelay`, `VehicleOwnershipDisable`,
`ContinueReloadOnVehicleExit`. `NeutralVehiclePointsFix` is closed as N/A -- the
flag is never read in TT 4.8.4 and its mechanism is the scripts team override.

Left, sequenced by `docs/tt484/TTSettings.md` 6: `UseExtraPTPages` and the PT
"building" message; `MapPrefix`; the four colour triples (`NodHouseColor`,
`GDIHouseColor`, `PrivateMessageColor`, `PublicMessageColor`); the terminal and
sidebar group (`RefillLimit`, `NewUnpurchasableLogic`, `AlternateSelectEnabled`,
`SidebarSoundsEnabled`, `DisableKillMessages` and their texture/sound names);
the registry keys (`ModRegistryKey`, `VersionRegistryKey`, `WOLUrlRegistryKey`);
the HUD switches (`ScrollingRadarMap`, `HidePlayerList`, `HideBottomText`, the
two LOD budget values, `ListColumnColor`); the ~23 dialog and menu colours,
which are inert until their consumers are touched anyway; and
`VehicleBuildingDisable` last, because it is a whole alternate production model
rather than a switch. `ScriptsLastTeamTime` stays unconsumed with
`NeutralVehiclePointsFix`.

Next exact action: `UseExtraPTPages` in `dlgcncpurchasemainmenu.cpp`, which
closes the two `enable secret PT pages` rows.

---

P01 and P02 are complete (see `completed_features.md`). The backlog is in
`unstarted_features.md`.

---

## Reference: pinned baselines (not a lifecycle entry)

OpenW3D `b970f592282f89c7a4a2f86d54c42c1dafe7c388` on `main` (user-authorized
deviation from directive 0.2). TT donor `tt_4.8.4/`. Zero Hour donor submodule
`9fee97a1792efc50edfe27300581970b4c022951`, `Core` + `GeneralsMD` only. Canonical
build `build/win32` (x86); `build/win` (x64) secondary. Game data at
`C:\Westwood\Renegade_full` — assets only, its binaries are out of scope.
Builds must run through `vcvarsall.bat x86`; this machine's system-wide `INCLUDE`
points at Visual Studio 6 and a plain shell compiles against the VC98 CRT, whose
errors look like broken source rather than a bad environment.
