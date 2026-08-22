# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P03: merge TT hooks/patches/overrides into canonical implementations

The native event layer is done (P03-A) and discharges Section 9's event-design
requirement, but not the per-site behaviour merges: 656 in-scope hook sites, 507
distinct donor symbols, each a reimplementation of the stock function it
displaced. Absorbs the backlog line "Acceptance: No required TT gameplay feature
depends on modifying executable memory or knowing a hard-coded function address",
which is not yet met. Next exact action: merge the `PacketManagerClass` cluster
(57 sites, 2 logical patches -- `TTParityMatrix.md` 4.5) into
`Code/wwnet/packetmgr.cpp`, then the rest of the netcode per
`NativeEventDispatch.md` 4.

## P04: native stock + TT script registry

The stock half is complete (P04-A, P04-B). Absorbs the three remaining backlog
lines: "Compile unchanged stock scripts only when TT does not supersede them",
"For stock scripts modified/replaced by TT, merge TT changes into the canonical
source" and "Compile TT-only new scripts as additional canonical scripts". The 13
script names the donor redefines need the donor implementation merged into the
canonical script, and the 2142 donor-only scripts cannot compile until their SDK
(`tt_4.8.4/scripts/engine_*.h`, which binds to the closed binary through
`REF_DEF2`) has a native destination -- that is P02/P03 work, not P04. Next exact
action: merge the 13 replacements listed in `TTParityMatrix.md` 3.1, taking the
donor side, and register each as `SCRIPT_SOURCE_STOCK_MERGED`.

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
