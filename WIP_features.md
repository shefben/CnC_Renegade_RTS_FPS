# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P04: the 4.8.4 script library, natively

The stock half, the registry, the thirteen replacements, the per-client seam,
the portable API port, the key-hook facility, and `jfwpow.cpp`, `jfwws.cpp`,
the SSGM layer, `jfwgun.cpp`, `jfwscr.cpp`, `jfwweap.cpp` and `jfwhook.cpp` are
done (P04-A..P04-R in `completed_features.md`). What is left is the rest of the
in-scope donor-only library: 663 scripts across 15 files, absorbing the backlog
lines "Compile unchanged stock scripts only when TT does not supersede them" and
"Compile TT-only new scripts as additional canonical scripts".
`tools/tt484/readiness.py` ranks the files by how many calls the engine still
cannot answer; six portable SDK names remain
(`docs/tt484/TTScriptApiGap.tsv`, `port-portable-source`).

Next exact action: convert `tt_4.8.4/scripts/jfwveh.cpp` (1428 lines, 31
registrations, zero blockers) to `Code/Scripts/TT_Vehicles.cpp`. After that,
`agtfix.cpp` (4) and `obelfix.cpp` (5) -- but those register **stock** names
(`M00_Advanced_Guard_Tower`, `GDI_AGT`, `Nod_Obelisk_CnC`, `Obelisk_Weapon_CnC`
and so on), so under directive 0.4 they are merges into the canonical scripts
in `Code/Scripts/Toolkit.cpp` with 4.8.4's second names registered as aliases,
not new registrations. The next cheapest API unblock is `GetExplosionObj`,
which is the only thing holding `gmbuilding.cpp` and `gmvehicle.cpp`: it is the
object an explosion is currently being applied for, and needs a seam rather
than a port.

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
