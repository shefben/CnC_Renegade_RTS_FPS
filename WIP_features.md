# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P02: reconcile TT public engine classes into canonical OpenW3D classes

The declaration delta is closed at 139 rows, all of them N/A, deferred behind
named later work, or reassigned to Phase 5/P20 -- matrix 5.8 has the disposition
of every one. Also absorbs the `unstarted_features.md` lines "Port TT
building/factory object types into `Code/Combat`" (2 of 4 done: construction yard
and superweapon; `AirFactoryGameObj` and `NavalFactoryGameObj` remain), "Port TT
object visibility/per-client state semantics", "Port TT extended network
behavior", "Port TT collision-group meanings and train/naval/player-building
behavior", and the two ABI/facade constraint lines. Next: port
`AirFactoryGameObj` and `NavalFactoryGameObj` as `VehicleFactoryGameObj`
subclasses -- the shared busy/purchaser/timer machinery is already there, so the
new work is the disabled flag (which belongs on `VehicleFactoryGameObj` itself,
not duplicated on both), air landing positions chosen per generation, and the
naval spawn zone with its construction zones.

---

P01 is complete (see `completed_features.md`). P03-P06 still gate everything after
them; the backlog is in `unstarted_features.md`.

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
