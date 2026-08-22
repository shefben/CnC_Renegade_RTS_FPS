# OpenW3D Baseline

Roadmap Phase 0 deliverable (roadmap v1.1.0, Section 6).
Recorded: 2026-08-21.

This document freezes the state of OpenW3D **before any TT 4.8.4 code is imported**.
Every later phase compares against this record.

---

## 1. Frozen source baseline

| Item | Value |
| --- | --- |
| Repository | `https://github.com/w3dhub/OpenW3D.git` |
| Branch | `main` |
| Baseline SHA | `b970f592282f89c7a4a2f86d54c42c1dafe7c388` |
| Commit subject | `fix: Use OpenW3D config file in WWConfig` |
| Working tree | clean of implementation changes; only project-state and tooling files untracked |

### Deviation from roadmap directive 0.2

Directive 0.2 pins the audited baseline to
`f6f9db43149d01367c8e0f9d0c30b0a5d7a4552d`. That object is **not present** in this
clone (`git cat-file -t` reports `could not get object info`) and `main` tracks no
upstream. The user directed the project to ignore the audited SHA and use the
codebase currently present, so no `git fetch`, rebase, or upstream reconciliation
was performed.

This is a recorded, user-authorized deviation. All TT and Zero Hour provenance
records must cite `b970f592` as the OpenW3D baseline. See resolved Q-003 in
`awaiting_answers.md`.

### Donor baselines pinned alongside

| Donor | Pin |
| --- | --- |
| TT Scripts | `tt_4.8.4/` — Tiberian Technologies Scripts 4.8 Update 4. `scripts/engine_common.h:18` declares `TT_VERSION ((float)4.8f)`. |
| Zero Hour | `GeneralsGameCode` submodule `9fee97a1792efc50edfe27300581970b4c022951` (`weekly-2026-08-21-3-g9fee97a17`), used as `Core` + `GeneralsMD` only. `Generals/` is present in that submodule and is **not** a donor. |

---

## 2. Recorded CMake configuration

| Setting | Value |
| --- | --- |
| Generator | `Ninja Multi-Config` |
| **Canonical architecture** | **x86 (32-bit)** |
| Canonical binary dir | `build/win32` |
| Secondary (x64) binary dir | `build/win` |
| CMake | 4.0.2 |
| Ninja | 1.11.1 |
| Compiler (x86) | MSVC 19.44 — `VC/Tools/MSVC/14.44.35207/bin/HostX64/x86/cl.exe` |
| Compiler (x64, secondary) | MSVC 19.44 — `.../bin/Hostx64/x64/cl.exe` |
| Language standard | C++20 (`-std:c++20`) |
| Warning level | `/W3` (`W3D_COMPILER_FLAGS`) |
| Release flags | `/O2 /Ob2 /DNDEBUG -MD -Z7` |
| Debug info format | Embedded (`CMAKE_MSVC_DEBUG_INFORMATION_FORMAT`) |
| Runtime library | `MultiThreadedDLL` / `MultiThreadedDebugDLL` |
| vcpkg | not used; `VCPKG_ROOT` unset. The `win` preset does not require it. |

### Feature options

| Option | Value | Note |
| --- | --- | --- |
| `W3D_CLIENT` | ON | |
| `W3D_FDS` | ON | headless/dedicated server |
| `W3D_TOOLS` | ON | |
| `W3D_BUILD_QT_TOOLS` | OFF | |
| `W3D_BUILD_OPTION_SDL3` | OFF | Windows default; `OPENW3D_WIN32=1` is defined instead |
| `W3D_BUILD_OPTION_WEBBROWSER` | ON | `WEBBROWSER_ENABLED=1` |
| `W3D_BUILD_OPTION_FFMPEG` | OFF | Windows default |
| `W3D_BUILD_OPTION_OPENAL` | OFF | depends on FFMPEG |
| `W3D_BUILD_OPTION_BINK` | ON | video decoding; `W3D_HAS_BINK` |
| `W3D_BUILD_OPTION_MILES` | ON | audio playback |
| `W3D_BUILD_OPTION_FREETYPE` | OFF | Windows default |
| `W3D_BUILD_OPTION_ICU` | OFF | Windows default |

### Preprocessor definitions in effect

`DIRECTINPUT_VERSION=0x800`, `GSI_WINSOCK2`, `NOMINMAX`, `OPENW3D_WIN32=1`,
`PARAM_EDITING_ON`, `WEBBROWSER_ENABLED=1`, `WIN32_LEAN_AND_MEAN`,
`_CRT_NONSTDC_NO_WARNINGS`, `_CRT_SECURE_NO_WARNINGS`, `_WIN32_WINNT=0x0601`,
`_WINSOCK_DEPRECATED_NO_WARNINGS`.

### FetchContent dependencies

Resolved into `build/win/_deps`: `gamespy`, `dx9`. LevelEdit additionally builds a
vendored `crunch`/`crnlib`.

---

## 3. Build results

### 3.0 Architecture

The project targets **32-bit binaries**. 64-bit builds are acceptable but require
stubs for some non-engine DLLs, so **x86 is the canonical baseline** and the x64
tree is retained only as a secondary configuration.

### x86 Release (canonical) — PASS

```text
cmake --build build/win32 --config Release
-> exit 0, 1953/1953 targets, 0 compiler errors
```

PE machine type verified as `0x014c` (x86) for `renegade.exe`,
`renegadeserver.exe`, and `leveledit.exe`.

Binaries in `build/win32/Release`: `renegade.exe` 4.0 MB,
`renegadeserver.exe` 4.0 MB, `leveledit.exe` 4.8 MB, `w3dview.exe` 1.8 MB,
`wwconfig.exe` 1.1 MB, `scripts.dll` 2.3 MB, `wdump.exe` 124 KB,
`mixviewer.exe` 108 KB, `W3DShellExt.dll` 108 KB, `launcher.exe` 44 KB,
`makemix.exe` 32 KB, `renrem.exe` 24 KB, `bandtest.dll` 24 KB,
`mss32.dll` 16 KB, `binkw32.dll` 12 KB, plus both test binaries.

### x86 Debug client — PASS

```text
cmake --build build/win32 --config Debug --target renegade
-> exit 0, 939 targets, 0 compiler errors
```

### x86 Tests — PASS

```text
ctest --test-dir build/win32 -C Release
1/2 wwaudio_lifetime_tests ... Passed  0.30 sec
2/2 wwlib_mempool_tests ..... Passed  0.01 sec
100% tests passed, 0 tests failed out of 2
```

---

### x64 Release (secondary) — PASS

```text
cmake --build build/win --config Release
-> exit 0, 1068/1068 targets, 0 compiler errors
```

Binaries produced in `build/win/Release`:

| Category | Binary | Size |
| --- | --- | --- |
| Client | `renegade.exe` | 4.5 MB |
| Dedicated server | `renegadeserver.exe` | 4.5 MB |
| Tools | `leveledit.exe` | 5.5 MB |
| Tools | `w3dview.exe` | 2.0 MB |
| Tools | `wwconfig.exe` | 1.2 MB |
| Tools | `wdump.exe` | 152 KB |
| Tools | `mixviewer.exe` | 124 KB |
| Tools | `makemix.exe` | 36 KB |
| Tools | `renrem.exe` | 28 KB |
| Tools | `W3DShellExt.dll` | 140 KB |
| Other | `launcher.exe` | 48 KB |
| Other | `scripts.dll` | 2.9 MB |
| Other | `bandtest.dll` | 28 KB |
| Stub | `binkw32.dll` | 12 KB |
| Stub | `mss32.dll` | 16 KB |
| Tests | `wwaudio_lifetime_tests.exe` | 924 KB |
| Tests | `wwlib_mempool_tests.exe` | 20 KB |

> **Phase 4 note.** A `scripts.dll` target exists in the stock build. Roadmap
> directive 0.5 requires the shipping game to stop depending on it. Phase 4 must
> retire this target's role, not merely stop shipping the file.

### x64 Debug client (secondary) — PASS

```text
cmake --build build/win --config Debug --target renegade
-> 422 targets, 0 compiler errors, build/win/Debug/renegade.exe produced (15 MB)
```

### x64 Tests (secondary) — PASS

```text
ctest --test-dir build/win -C Release
1/2 wwaudio_lifetime_tests ... Passed  0.41 sec
2/2 wwlib_mempool_tests ..... Passed  0.01 sec
100% tests passed, 0 tests failed out of 2
```

The stock repository ships exactly two CTest targets. This is the entire automated
test surface available as a regression net for Phases 1-5.

### First build attempt (recorded for completeness)

The first Release attempt failed at 894/1953 with
`fatal error C1085: Cannot write compiler generated file: ... No space left on
device`. Drive `F:` was at 0 bytes free. This was an environment fault, not a
source defect. After space was reclaimed the build completed cleanly with zero
errors.

---

## 4. Script catalog

See `docs/integration/BaselineScriptCatalog.txt`.

- **1747 unique stock script names, zero duplicates.**
- All registration flows through `DECLARE_SCRIPT(x, d)`
  (`Code/Scripts/scripts.h:196`), which expands to `REGISTER_SCRIPT(x, d)`
  (`Code/Scripts/ScriptRegistrant.h:63`) and stringifies the class name via `#x`.
  The registered script name is therefore identical to the C++ class name.
- 4 commented-out `/*DECLARE_SCRIPT` occurrences in `Mission01/03/04.cpp` were
  excluded. One of them (`M11_MutantCrypt_Spawner03_Guy_JDG` in `Mission04.cpp`)
  would otherwise have looked like a duplicate of the live declaration in
  `Mission11.cpp`.
- Largest contributors: `Mission01.cpp` (291), `Mission11.cpp` (171),
  `Mission04.cpp` (138), `Mission07.cpp` (122), `mission08.cpp` (114).

This catalog is the input to two later gates:

- **Phase 1** must diff it against the TT 4.8.4 script set to classify each TT
  script as a replacement of a stock script, a TT-only addition, or a name
  collision.
- **Phase 4** must reproduce every name in it from the native registry, with no
  duplicate names and no stock/TT variant pairs.

---

## 5. Runtime baseline — NOT CAPTURED

Roadmap Section 6 additionally requires running stock Renegade content and testing
player spawn, infantry movement, vehicle entry/exit, purchase terminal, building
damage/destruction, script creation, map transition, save/load, listen/network play,
and dedicated-server startup, plus baseline screenshots and performance counters.

**None of this was performed yet.**

> **Corrected 2026-08-21.** This section originally said no game assets were
> installed, based on `Run/` holding only `place_steam_build_here.txt`. That was the
> wrong location. The working game directory is **`C:\Westwood\Renegade_full`**,
> a complete Renegade install (`Data/` holds the `C&C_*.mix` multiplayer maps,
> `Always2.dat`, fonts, config). Assets are available; the matrix simply has not
> been run.
>
> Per user instruction the **binaries** shipped in that directory (`Game.exe`,
> `TTCore.dll`, and the other shipped DLLs) are out of scope for inspection — this
> project replaces them. Treat it as an asset/data source only.

Phase 0's stated acceptance is that the selected SHA "builds **and runs**". The
build half is satisfied and evidenced above. The runtime half is outstanding and
tracked as **P00-RUNTIME** in `unstarted_features.md` (see resolved Q-004). It must not be silently inherited as satisfied by the Phase 6
hard gate.

---

## 6. Baseline status

| Phase 0 requirement | Status |
| --- | --- |
| Record exact SHA | DONE (`b970f592`, with recorded deviation) |
| Build Release client | PASS |
| Build Debug client | PASS |
| Build Release `renegadeserver` | PASS |
| Build with `W3D_TOOLS=ON` | PASS |
| Record CMake cache/options | DONE (Section 2) |
| Dump stock script names | DONE (1747 scripts) |
| Run stock content / map load / gameplay matrix | BLOCKED — no assets (Q-004) |
| Baseline screenshots and performance counters | BLOCKED — no assets (Q-004) |

**Phase 0 was split on 2026-08-21 by user decision (resolved Q-004):**

- **P00-BUILD — COMPLETE.** The compile-and-test baseline is established and green.
  Evidence recorded in `completed_features.md`.
- **P00-RUNTIME — UNSTARTED.** The runtime matrix is tracked as its own backlog item
  in `unstarted_features.md`, to be executed once `Run/` holds a real Renegade
  install.

The runtime scope was deliberately **not** folded into the closed entry. The
Phase 6 hard gate must not treat P00-RUNTIME as satisfied.
