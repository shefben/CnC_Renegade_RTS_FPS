# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P04: the 4.8.4 script library, natively

The stock half, the registry, the thirteen replacements, the per-client seam,
the portable API port, `jfwpow.cpp`, `jfwws.cpp`, the SSGM layer, `jfwgun.cpp`
and `jfwscr.cpp` are done (P04-A..P04-N in `completed_features.md`). What is
left is the rest of the in-scope donor-only library: 702 scripts across 19
files, absorbing the backlog lines "Compile unchanged stock scripts only when
TT does not supersede them" and "Compile TT-only new scripts as additional
canonical scripts". `tools/tt484/readiness.py` ranks the files by how many
calls the engine still cannot answer; nine portable SDK names remain
(`docs/tt484/TTScriptApiGap.tsv`, `port-portable-source`), all of them purchase
terminal, INI or `Attach_Script_Once_V`.

Next exact action: convert `tt_4.8.4/scripts/jfwweap.cpp` (1509 lines, 24
registrations) to `Code/Scripts/TT_Defenses.cpp` -- turret, obelisk, both
Advanced Guard Towers, guard duty, hunt, and the weapon-swap scripts -- leaving
out `JFW_Vehicle_Weapon_Switcher` and `JFW_Char_Weapon_Switcher`, which want
the key hook. Then build the key hook: a client-to-server event of the same
shape as `cCsDamageEvent` carrying a logically-named key press, a server-side
registry of who asked for which key, and a script-side base to replace
`JFW_Key_Hook_Base`. It gates twenty registrations -- fourteen in
`jfwhook.cpp`, the two above, and `SSGM_Log_Key`, `SSGM_C4_Key`,
`SSGM_Bind_Key` and `SSGM_BL_Key` from `gmsoldier.cpp`, which `TT_SSGM.cpp`
left out. There is no `jfwkey.cpp`; an earlier note in this file named one.

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
