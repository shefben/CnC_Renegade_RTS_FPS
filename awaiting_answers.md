# Awaiting Answers

Only unresolved questions needing the user. Add a `Q-###` as soon as one appears.

---

## Active

None.

---

## Resolved

- **Q-010** Team defaulting. User directed using TT's handling, so the removal
  stands: the stock swap-the-scores path that handed a win to the losing team
  when the leading team's last player disconnected is gone for good.

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
- **Q-005** Phase 1 output, project state and the donor trees were all untracked.
 User directed committing everything locally (no remote is under our control yet).
 Done in four commits on `main`: `ce4c3b75` analysis + state files, `cdd1ed2a` TT
 4.8.4 tree and the pinned `GeneralsGameCode` submodule, `0a191e58` the standalone
 `Code/` side projects, `41164d71` the ~900 MB `models/` art. Local agent/tool
 caches are gitignored instead.
