# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P02: merge TT members into canonical OpenW3D classes

Absorbs the `unstarted_features.md` line "Merge required TT members, virtuals,
accessors, definitions, and semantics into canonical OpenW3D classes." Scoped
first (`docs/tt484/TTMethodSources.tsv`, matrix 5.6): of 227 TT-only declarations
only 100 are mergeable TT code, 57 already exist on another OpenW3D owner, and 58
must be written. `SoldierGameObj`, `VehicleGameObj` and `BuildingGameObj` are
merged with every flag wired to a real enforcement site, not left as inert state;
`combat` builds clean and the delta stands at 176. Next: merge the 3
`NetworkObjectClass` and 2 `RenderObjClass` inline additions, then the 8
`HUDSurfaceClass` and 6 `ScriptedDialogClass` ones (filter `TTMethodSources.tsv`
on `source_kind=inline-header`).

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
