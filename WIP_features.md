# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P03: merge TT hooks/patches/overrides into canonical implementations

The event layer is done (P03-A) and `TTHookSites.tsv` now carries a
`disposition` column giving the exact standing: **187 merged, 456 open**, 13
n/a, 104 out of scope under directive 0.6 (P03-A/B/C/D/E/F/G/H). Absorbs the
backlog line "Acceptance: No required TT gameplay feature depends on modifying
executable memory or knowing a hard-coded function address", not yet met. The
open remainder is 431 `exact-def` rows -- each a TT reimplementation to diff
against its canonical function -- plus ~25 discrete byte patches. Largest
groups: `cNetwork` 46, `SoldierGameObj` 16, `VehicleGameObj` 14,
`BeaconGameObj` 14, `WeaponBagClass` 13, `ModPackageMgrClass` 13, `cGameData`
12, `DialogMgrClass` 11, `SmartGameObj` 11, `DialogBaseClass` 10. Next exact
action: finish the `cNetwork` diff started this session
(`scratchpad/fndiff.py` extracts and normalises both sides; the finding so far
is that OpenW3D `cnetwork.cpp` has already moved past stock, so most rows are
"canonical already supersedes" and the TT-forward items are few -- named so far,
`isPlayerNameAllowed` in `Application_Acceptance_Handler`), then
`Combat/soldier.cpp` (30) and `Combat/combat.cpp` (20).

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

`TTSettingsClass` exists and three clusters are wired through it (P03-G). The
remaining ~90 options are declared with their defaults but not yet consumed.
Sequenced in `docs/tt484/TTSettings.md` 6: the four reopened hook clusters
(`UseExtraPTPages`, the `.mix` startup scan, the PT "building" message,
`enable secret PT pages`), then the gameplay options (`Unsquishable` and its
four armour exemptions, the two weather disables, `NeutralVechiclePointsFix`,
`DrawDistance`, `ContinueReloadOnVehicleExit`, `BuildTimeDelay`,
`VehicleOwnershipDisable`, `MapPrefix`, the four colour triples), then the UI
and styling colours, and `VehicleBuildingDisable` last because it is a whole
alternate production model rather than a switch. Next exact action:
`UseExtraPTPages` in `dlgcncpurchasemainmenu.cpp`, which closes the two
`enable secret PT pages` rows.

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
