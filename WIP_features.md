# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P04: the 4.8.4 script library, natively

Done so far: P04-A..P04-Y in `completed_features.md` -- the stock half, the
registry, the per-client seam, the key-hook facility, the SSGM layer, and the
donor files `jfwpow.cpp`, `jfwws.cpp`, `jfwgun.cpp`, `jfwscr.cpp`,
`jfwweap.cpp`, `jfwhook.cpp`, `jfwveh.cpp`, `agtfix.cpp`, `obelfix.cpp`,
`jfwdef.cpp` and `jfwcust.cpp` (89/90). Registry is at 2011 built-in scripts,
no duplicate names; `renegade` and `leveledit` both link. What remains is 425
donor-only scripts across 10 files and the API names gating most of them --
`tools/tt484/readiness.py` ranks them and `docs/tt484/TTScriptApiGap.tsv`
carries the dispositions.

Next exact action: convert `tt_4.8.4/scripts/jfwpoke.cpp` (1407 lines, 56
registrations, now zero blockers since `Grant_Powerup` resolved to
`ScriptEngine::Give_PowerUp`) to `Code/Scripts/TT_Poke.cpp`. Then `dan.cpp`
(3 scripts, also clear) -- but note its crate script talks to players by
building SSGM console-command strings (`MESSAGE ...`, `PPAGE %d ...`) and this
tree has no such console commands, so those want
`ScriptEngine::Send_Message_Player` and a team/all equivalent directly rather
than a string parser; it also never picks its last crate type (`x < 9` over ten
entries) and indexes its weighting table one past the end.

After those two the remainder is the API gap rather than porting. In leverage
order: `Get_Player_Name_By_ID` (14 calls, 2 files), `Get_Damage_Warhead` (11,
3), `Hide_Preset_By_Name` (8, 1), `Is_Spy` (8, 2), `Get_INI`/`Release_INI` (8,
3), `Get_Cost` (4, 1), `Update_Network_Object` (5, 4), `GetExplosionObj` (4,
4), `Create_Zone` (2, 1). `GetExplosionObj` and `Get_Damage_Warhead` want the
same shape: an ambient recording what damage or explosion is being applied,
readable from inside `Damaged`. `Get_Mine_Limit` and `Set_Tech_Level` have no
counterpart here and need a decision about where the setting lives.
Before porting any further file, check `docs/tt484/TTScriptApiRenames.tsv`
first -- three of this session's "missing" names were already present under
other names, and `Get_Random_Int` is present under the *same* name with a
different range.

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
