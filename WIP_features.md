# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P04: the 4.8.4 script library, natively

The stock half, the registry, the thirteen replacements, the per-client seam,
the portable API port, `jfwpow.cpp`, `jfwws.cpp` and the whole SSGM layer are
done (P04-A..P04-K in `completed_features.md`). What is left is the rest of the
in-scope donor-only library: 787 scripts across 21 files, absorbing the backlog
lines "Compile unchanged stock scripts only when TT does not supersede them" and
"Compile TT-only new scripts as additional canonical scripts".
`tools/tt484/readiness.py` ranks the files by how many calls the engine still
cannot answer.

Next exact action: convert `tt_4.8.4/scripts/jfwgun.cpp` (5015 lines, 60
registrations) to `Code/Scripts/TT_Weapons.cpp`, registering each as
`SCRIPT_SOURCE_TT` and checking each name against the merged catalog first --
`jfwws.cpp` had seven that were already ours and had to become aliases rather
than copies. Then `jfwkey.cpp`, which also carries the four `SSGM_*_Key`
scripts left out of `TT_SSGM.cpp`; then port the 34 remaining portable SDK
functions (`docs/tt484/TTScriptApiGap.tsv`, `port-portable-source`) to unblock
the rest.

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
