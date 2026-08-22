# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P04: the 4.8.4 script library, natively

The stock half, the registry, the thirteen replacements, the per-client seam
and the portable API port are done (P04-A..P04-G in `completed_features.md`).
What is left is the rest of the in-scope donor-only library: 848 scripts across
24 files, absorbing the backlog lines "Compile unchanged stock scripts only
when TT does not supersede them" and "Compile TT-only new scripts as additional
canonical scripts". `tools/tt484/readiness.py` ranks the files by how many
calls the engine still cannot answer; `jfwws.cpp` and `jfwgun.cpp` are at zero.

Next exact action: convert `tt_4.8.4/scripts/jfwws.cpp` (742 lines, 43
registrations, 37 distinct classes) to `Code/Scripts/TT_World.cpp`, registering
each as `SCRIPT_SOURCE_TT`. Six of its registrations put a `JFW_*` class under
a second, stock name -- `M00_PCT_Pokable_DAK`, `M00_Disable_Transition`,
`M00_GrantPowerup_Created`, `M00_Play_Sound`, `Dr_Mobius_Script`,
`M00_BuildingStateSoundController` -- and those stock names are already merged,
so the `JFW_*` name has to become an alias registration of the merged class
(needs a `REGISTER_SCRIPT_ALIAS`), never a second copy: directive 0.4. Then
`jfwgun.cpp`, then port the 42 remaining portable SDK functions
(`docs/tt484/TTScriptApiGap.tsv`, `port-portable-source`) to unblock the rest.

## P04-H: SSGM's server-management layer

The five `gm*.cpp` files are 35 script registrations plus the server layer
around them -- a TCP log socket, console commands, moderation. `gmlog.cpp`
registers no scripts at all. The scripts belong to P04 above; this entry holds
the server-management layer, which is not a script registry and is not yet
decided. Next exact action: when P04's file conversion reaches `gm*.cpp`, split
the two and ask the user whether the server layer is wanted at all.

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
