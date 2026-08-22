# Awaiting Answers

Only unresolved questions needing the user. Add a `Q-###` as soon as one appears.

---

## Active

- **Q-005** `docs/`, `tools/tt484/`, the four state files, `tt_4.8.4/`,
 `models/` and the roadmap are all untracked. Phase 1 output is now the authority
 for Phases 2-5, so it should be under version control, but committing the TT and
 Zero Hour donor trees is a separate call. Commit the analysis output and project
 state now, and if so, are the donor trees in or out?

---

## Resolved

- **Q-001** TT tree was named `tt_4.9.4/` but roadmap pins 4.8.4. User renamed it to
  `tt_4.8.4/`; `scripts/engine_common.h:18` confirms `TT_VERSION 4.8f`.
- **Q-002** ~93 untracked Commander/RTS files from the superseded pre-v1.1.0 roadmap,
  wired into no CMakeLists. User deleted them; P33-P39 start fresh from the v1.1.0
  spec. They existed in no commit and are not recoverable.
- **Q-003** Roadmap directive 0.2 pins OpenW3D `f6f9db43...`, absent from this clone.
  User directed using the present codebase, so the baseline is `b970f592` — a
  recorded, authorized deviation that all provenance must cite.
- **Q-004** Phase 0 runtime matrix not run. User accepted Phase 0 as build-only, so
  P00-BUILD closed and the runtime items stayed in `unstarted_features.md`. Premise
  later corrected: assets do exist, at `C:\Westwood\Renegade_full`, not `Run/` — so
  those items are unblocked, not blocked. Its shipped binaries are out of scope.
