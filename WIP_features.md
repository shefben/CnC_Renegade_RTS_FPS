# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P03: merge TT hooks/patches/overrides into canonical implementations

The event layer is done (P03-A) and 139 of the 656 in-scope hook sites are
merged or dispositioned (P03-B netcode 125, P03-C the six `UDP fixes` plus
`SlidePrecision`, P03-D the vehicle flip, P03-E the six scroll-fix sites and the
2x cost message). All 135 addressed byte-patch rows are now triaged against the
stock image, so the remaining work is named rather than opaque. Absorbs the
backlog line "Acceptance: No required TT gameplay feature depends on modifying
executable memory or knowing a hard-coded function address", not yet met. Next
exact action: the 16 purchase-terminal sites (`new unpurchasable logic` 10,
`PT keypress fix` 2, `PT chatbox fix`, `enable secret PT pages` 2, `"building"
message change`) under directive 0.9, then `Commando/cnetwork.cpp` (32), per
`NativeEventDispatch.md` 5.9.

## P04: native stock + TT script registry

The stock half is complete (P04-A, P04-B). Absorbs the three remaining backlog
lines: "Compile unchanged stock scripts only when TT does not supersede them",
"For stock scripts modified/replaced by TT, merge TT changes into the canonical
source" and "Compile TT-only new scripts as additional canonical scripts". The
donor half is now scoped to the original TT library only (P04-E): 861 donor-only
scripts across 24 files, not 2142. The 13 script names the donor redefines are 1
of 13 merged (P04-C, `M00_Advanced_Guard_Tower`), and all 13 sit inside that
scope. The remainder is less blocked than recorded (P04-D): 406 of the SDK's 690
functions have portable source, and all 284 closed-binary bindings sit in
`engine_tt.h` alone. Next exact action: merge the remaining 12 replacements
listed in `TTParityMatrix.md` 3.1 -- the `M00_*` cluster in `jfwws.cpp` is the
largest group -- taking the donor side, and register each as
`SCRIPT_SOURCE_STOCK_MERGED`.

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
