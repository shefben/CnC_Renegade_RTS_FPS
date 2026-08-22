# Baseline Known Issues

Roadmap Phase 0 deliverable (roadmap v1.1.0, Section 6).
Recorded: 2026-08-21 against OpenW3D `b970f592282f89c7a4a2f86d54c42c1dafe7c388`.

Issues observed in the **stock, pre-TT** baseline. Anything listed here already
existed before TT 4.8.4 integration and must not be misattributed to TT work in
Phases 1-6, nor to Zero Hour donor work later.

---

## Scope of this record

This is a **static/build-time** known-issues record only. The runtime half of Phase 0
did not run because no game assets are installed (see BKI-006), so no gameplay,
rendering, networking, or save/load defects are captured here. This document must be
revisited once a Renegade install is available.

---

## BKI-001: `wwlib/vector.h:313` truncates a pointer-derived index to `int`

- Severity: latent 64-bit correctness; benign in practice
- Warning: C4244, 764 occurrences across the Release build
- Location: `Code/wwlib/vector.h:313`, in `int VectorClass<T>::ID(const T *)`

```cpp
return(((uintptr_t)ptr - (uintptr_t)&(*this)[0]) / sizeof(T));
```

A `uintptr_t` quotient is returned through an `int` return type. On the x64 build
this truncates for any index above `INT_MAX`, which would require a vector of more
than ~2.1 billion elements, so it is not reachable in practice today.

It matters for the roadmap because `VectorClass` is shared low-level container
infrastructure. Roadmap Section 0B.3 lists "shared renderer/physics/container/
resource-manager infrastructure changes" as an escalation trigger, so any phase
that touches this signature must validate more broadly than the default targeted
mode. Do not silently widen the return type as a drive-by change during TT
integration; it is an ABI-visible signature on a template used throughout the
engine.

## BKI-002: `wwlib/wchar/unichar.h` passes `size_t` lengths to Win32 `int` parameters

- Severity: latent truncation; benign in practice
- Warning: C4267, 1512 occurrences across the Release build
- Locations: `Code/wwlib/wchar/unichar.h:51` (`u_mbtows`), `:62` (`u_wstomb`)

```cpp
int retval = MultiByteToWideChar (CP_UTF8, 0, src, -1, dst, len);
int retval = WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, len, nullptr, nullptr);
```

`len` is `size_t`; the Win32 `cchWideChar`/`cbMultiByte` parameters are `int`.
Truncation requires a >2 GB string, so it is not reachable in practice.

Together, BKI-001 and BKI-002 account for **2276 of the 2554 own-source warnings
(89%)**. Both are header-resident, so the count reflects one defect each multiplied
across every translation unit — not 2276 distinct problems. Any future decision to
raise the warning level or enable warnings-as-errors should fix these two headers
first, as that alone would clear the overwhelming majority of build noise.

## BKI-003: Build warning profile (own source)

Release build, `/W3`, 2554 warnings in `Code/`:

| Code | Count | Meaning |
| --- | --- | --- |
| C4267 | ~1515 | `size_t` to `int`, possible loss of data |
| C4244 | ~1039 | narrowing conversion, possible loss of data |
| C4838 | 5 | narrowing conversion in array initialization |
| C4828 | 1 | illegal character in source file |

Files beyond the two headers above are individually minor: `dlgmpwolgamelist.cpp`
(22), `MissionX0.cpp` (17), `Test_RAD.cpp` (15), `Mission09.cpp` (15),
`PerformanceConfigDialog.cpp` (12), `Mission01.cpp` (10).

**Zero errors.** The baseline compiles clean at `/W3` in both Release and Debug.

## BKI-004: 34 pointer-size cast warnings in the vendored `crunch` dependency

- Severity: none for OpenW3D
- Warning: C4312, all 34 in `crunch-src/crnlib/crn_stb_image.cpp`

All C4312 (`conversion from 'int' to 'unsigned char *' of greater size`) warnings
come from third-party `crnlib`/`stb_image` code fetched into the build tree for
LevelEdit, not from OpenW3D source. Of the 2594 total build warnings, only 40 are
outside `Code/`. Do not spend TT- or Zero Hour-phase effort on these; if they ever
need addressing it is a dependency-update task.

## BKI-005: The automated test surface is two tests

The stock repository ships exactly two CTest targets:

- `wwaudio_lifetime_tests`
- `wwlib_mempool_tests`

Both pass in 0.44 s total. This is the **entire** automated regression net available
while TT integration replaces stock behavior in Phases 2-5.

This is a project risk rather than a defect. Roadmap Section 0B.6 says not to default
to test-first development, and that stands — but it also means Phases 2-5 have almost
no existing safety net to lean on. Where TT merges touch serialization, networking,
or memory ownership, Section 0B.6's own exceptions apply and a targeted test is
justified. Phase 6 is a hard gate that cannot rely on these two tests alone.

## BKI-006: Runtime baseline not yet captured

- Severity: Phase 0 runtime scope outstanding
- Tracked as P00-RUNTIME in `unstarted_features.md` (see resolved Q-004)

> **Corrected 2026-08-21.** This entry originally read "game assets absent" on the
> assumption that assets belonged in `Run/`. That assumption was wrong. The working
> game directory is **`C:\Westwood\Renegade_full`**, which holds a complete Renegade
> install (`Data/` with the `C&C_*.mix` multiplayer maps, `Always2.dat`, fonts,
> config). Assets were available all along; the runtime matrix simply has not been
> run yet. P00-RUNTIME is **not blocked**.
>
> Per user instruction the **binaries** in that directory (`Game.exe`, `TTCore.dll`,
> and the other shipped DLLs) are out of scope for inspection — this project replaces
> them. Treat the directory as an asset/data source only.

The Phase 0 runtime matrix (stock content, campaign map, multiplayer map, player
spawn, infantry movement, vehicle entry/exit, purchase terminal, building
damage/destruction, script creation, map transition, save/load, listen/network play,
dedicated-server startup, baseline screenshots, performance counters) has not been
executed.

Consequence: there is currently **no behavioral reference** for "unchanged stock
Renegade behavior". Roadmap directive 0.9 requires preserving canonical infantry
movement, aiming, weapon handling, vehicle handling, repair, Purchase Terminal
interaction, and MCT/building combat unless the roadmap explicitly changes them —
and Phases 2-5 will be modifying exactly those paths. Without a captured baseline,
"we did not regress it" cannot be evidenced, only asserted.

## BKI-007: `scripts.dll` is a live build target

- Severity: not a defect; a recorded starting condition for Phase 4

The stock build produces `scripts.dll` (2.9 MB). Roadmap directive 0.5 requires the
shipping game to not require `scripts.dll`, `scripts2.dll`, `tt.dll`, `shaders.dll`,
or any TT engine-patch DLL for built-in functionality. Phase 4 must retire this
target's role by moving its 1747 scripts into the native registry — not merely stop
copying the file.

## BKI-008: Environment — build requires substantial free disk space

- Severity: environmental

The first Release build attempt failed at 894/1953 with
`fatal error C1085: Cannot write compiler generated file: ... No space left on
device` when drive `F:` reached 0 bytes free. A full Release + Debug build tree for
this project needs several GB. This is recorded so a future session recognizes
C1085 as an environment fault rather than chasing it as a source defect.

---

## Summary

| ID | Issue | Blocks Phase 0 closure |
| --- | --- | --- |
| BKI-001 | `VectorClass::ID` truncates index to `int` | no |
| BKI-002 | `unichar.h` passes `size_t` to Win32 `int` params | no |
| BKI-003 | 2554 own-source `/W3` warnings, 0 errors | no |
| BKI-004 | 34 C4312 warnings in vendored `crunch` | no |
| BKI-005 | Only two automated tests exist | no |
| BKI-006 | No runtime baseline; assets absent | **yes** |
| BKI-007 | `scripts.dll` still a build target | no |
| BKI-008 | Build needs several GB free disk | no |

The baseline builds and tests clean. The single outstanding item for Phase 0 closure
is BKI-006 / Q-004.
