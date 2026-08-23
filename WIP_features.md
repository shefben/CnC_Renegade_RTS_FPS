# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P04: the 4.8.4 script library, natively

The stock half, the registry, the thirteen replacements, the per-client seam,
the portable API port, the key-hook facility, and `jfwpow.cpp`, `jfwws.cpp`,
the SSGM layer, `jfwgun.cpp`, `jfwscr.cpp`, `jfwweap.cpp`, `jfwhook.cpp`,
`jfwveh.cpp`, `agtfix.cpp` and `obelfix.cpp` are done (P04-A..P04-U in
`completed_features.md`), absorbing the backlog lines "Compile unchanged stock
scripts only when TT does not supersede them" and "Compile TT-only new scripts
as additional canonical scripts". What is left is 561 donor-only scripts across
11 files, and the 38 API names still gating most of them.
`tools/tt484/readiness.py` now says which donor files are already ported and
which are held up by what; `docs/tt484/TTScriptApiGap.tsv` carries the
dispositions.

Next exact action: convert `tt_4.8.4/scripts/jfwdef.cpp` (4183 lines, 48
registrations, now zero blockers) to `Code/Scripts/TT_Defences.cpp`. It is one
script written out 48 times over a trait matrix -- target filter (all /
no-aircraft / aircraft-only / no-VTOL / VTOL-only / no-VTOL-no-stealth, the
filtered ones carrying eight `PresetN` exceptions), animated (Animation,
LastFrame, PopupTime), sound, secondary weapon, swap (SwapMessage), plus
`JFW_User_Controllable_Base_Defence` -- so it wants one base class and a table,
the way the six jetpacks became one. `M00_Base_Defense` is already canonical in
`Code/Scripts/Toolkit.cpp:2150` as `DECLARE_SCRIPT_MERGED`, so under directive
0.4 the plain `JFW_Base_Defence` is an alias merged into it, not a 49th
registration.

After that the remaining work is almost entirely the API gap, not porting: 38
names still block 11 files. In leverage order -- `Grant_Powerup` (24 calls, 3
files), `Get_Player_Name_By_ID` (14, 2), `Get_Damage_Warhead` (11, 3),
`Hide_Preset_By_Name` (8, 1), `Is_Spy` (8, 2), `Update_Network_Object` (5, 4),
`GetExplosionObj` (4, 4), `Create_Vehicle` (5, 3), `Get_INI`/`Release_INI` (8,
3). `GetExplosionObj` and `Get_Damage_Warhead` both want the same shape: an
ambient recording what damage or explosion is currently being applied, readable
from inside `Damaged`. `Get_Mine_Limit` and `Set_Tech_Level` have no OpenW3D
counterpart at all and need a decision about where the setting lives.

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
