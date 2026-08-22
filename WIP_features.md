# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P02: merge TT members into canonical OpenW3D classes

Absorbs the `unstarted_features.md` line "Merge required TT members, virtuals,
accessors, definitions, and semantics into canonical OpenW3D classes." Scoped in
`docs/tt484/TTMethodSources.tsv`; matrix 5.7 records the parser correction to the
5.6 survey. `SoldierGameObj`, `VehicleGameObj`, `BuildingGameObj` and
`NetworkObjectClass` are merged, every flag wired to a real enforcement site, and
the delta stands at 159. Two scope corrections: `HUDSurfaceClass`,
`ScriptedDialogClass` and `ExtendedNetworkObject` do not exist in OpenW3D at all,
so their members are Phase 5 new-class work and not P02 (matrix 5.5); and
`PhysicsSceneClass`'s 6 remaining methods are shadow/polygon-budget control and
belong with P20. Next: the `SoldierGameObj` write-from-declaration clusters, in
this order — skeleton resize (`Get`/`Set_Skeleton_Height`/`_Width`,
`Trigger_Smooth_Skeleton_*_Resize`), collision-mode lock (`Lock_Collision_Mode`,
`Get_Locked_Collision_Mode`, and the deferred `Disable_Ghost_Collision` with it),
then the anim/hold-style overrides.

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
