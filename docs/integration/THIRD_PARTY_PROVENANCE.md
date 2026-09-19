# Third-party provenance

Roadmap Section 50 required deliverable, Phase 45.

This file consolidates the source pins that already exist, scattered across the phase
documents that recorded them at the moment each one was frozen. It is a pointer/summary,
not a second independent record — if this table and the document it cites ever disagree,
the cited document is correct and this file needs updating, not the other way round.

## Repositories consulted or imported

| Source | Role | Pin | Recorded in |
| --- | --- | --- | --- |
| `w3dhub/OpenW3D` | Destination codebase (this repository) | `b970f592282f89c7a4a2f86d54c42c1dafe7c388` (`main`), a user-authorized deviation from the roadmap-audited `f6f9db4`, which is not reachable from this clone | `docs/integration/OpenW3DBaseline.md`, resolved Q-003 |
| Tiberian Technologies Scripts 4.8 Update 4 | Implementation donor, merged native (Phases 1-6) | `tt_4.8.4/`, `TT_VERSION ((float)4.8f)` per `scripts/engine_common.h:18` | `docs/integration/OpenW3DBaseline.md`, `docs/tt484/TTParityMatrix.md` |
| TheSuperHackers `GeneralsGameCode` (`Core` + `GeneralsMD`) | Zero Hour / SAGE implementation donor (Phases 7+) | `9fee97a1792efc50edfe27300581970b4c022951`, described as `weekly-2026-08-21-3-g9fee97a17` | `docs/integration/OpenW3DBaseline.md`, `docs/zerohour/ZeroHourDonorBaseline.md` |
| EA `electronicarts/CnC_Generals_Zero_Hour` | Archival validation reference **only** (Section 13.5) — never a source of implementation, `Generals/` inside it is never a donor stage | `0a05454d8574207440a5fb15241b98ad0b435590`, queried not checked out | `docs/zerohour/ZeroHourDonorBaseline.md` |

## What is explicitly not a provenance source

- `Generals/` inside the `GeneralsGameCode` submodule — CLAUDE.md forbids treating it as
  a donor stage or predecessor implementation source, and `ZeroHourDonorBaseline.md`
  records that the submodule's `Generals/` directory is present but unused.
- Any historical TT DLL-hook/proxy binary — TT is imported as source and merged into
  native OpenW3D classes (Phase 1-6), never kept as a runtime-patched binary.

## Verifying a pin

`git submodule status` for the Zero Hour donor; the OpenW3D baseline SHA is this
repository's own history, so `git log --oneline -1 b970f592` (or the current tip, since
that commit is this project's starting point rather than a moving target) confirms it.
There is no submodule for the TT source — it was imported and merged, not vendored — so
its pin is the version string cited above, not a fetchable object.

## Maintenance

Add a row here whenever a new external source is consulted or imported, and update the
"Recorded in" column rather than duplicating the fact. Do not record a source's rationale
here twice — one sentence and a pointer to the phase document that made the call.
