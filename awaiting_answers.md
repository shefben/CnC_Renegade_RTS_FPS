# Awaiting Answers

Only unresolved questions needing the user. Add a `Q-###` as soon as one appears.

---

## Active

- **Q-006** P08's acceptance -- that repeated map load and unload keeps what was
 retained and leaks nothing -- is proved as arithmetic by `asset_residency` but not
 against real game data, and it cannot be automated here: the client takes no map
 argument, so reaching a second level means driving the menus. Needs one manual run:
 `renegade --gamedir "C:\Westwood\Renegade_full"`, start a level, change to another,
 and report the `WWDEBUG` `Log_Report` lines. Expected: permanent texture count
 unchanged across the change, live prototype count dropping back. Not blocking any
 other work.


---

## Resolved

- **Q-007 (resolved by decision, reversible)** What a player is told before a
  picture of their game window leaves their machine. P05 held `ssurl`/`sshot`
  back as a policy question; it is answered and implemented. The capture is the
  game window and there is no path that captures anything else; the player is
  always told, naming the destination host, before the upload starts; the URL is
  held in memory only, so the feature is off after every restart until an operator
  turns it on; and every request is answered, refusals and failures included. No
  client-side opt-out was added, on the grounds that a silent refusal removes the
  only reason the feature exists while leaving the appearance of it. Say the word
  and any of the four moves. See `docs/tt484/RemoteScreenshots.md`.

- **Q-011 (resolved by decision, reversible)** Where the tech level and the
 mine limit live. Taken as recommended: both are `SSGMSettingsClass`
 settings read from `ssgm.ini`, which is where 4.8.4 kept them. The tech
 level splits in two -- the starting value is the setting, the running
 value is `PurchaseAvailabilityClass::TechLevel`, because a script moves
 it during a match and every client's menu has to follow. Say the word and
 it moves.

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
