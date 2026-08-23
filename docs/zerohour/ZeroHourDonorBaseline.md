# The Zero Hour donor, frozen

Roadmap Section 13.1. Everything Phase 8 and later adapt comes from one
revision of one repository in one configuration. This file is that pin.

## The revisions

| What | Revision | Date | Note |
| --- | --- | --- | --- |
| TheSuperHackers `GeneralsGameCode` main | `9fee97a1792efc50edfe27300581970b4c022951` | 2026-08-21 | The implementation donor. Describes as `weekly-2026-08-21-3-g9fee97a17`; subject `refactor(gamestate): Decouple result handling from game state (#3000)`. |
| EA `electronicarts/CnC_Generals_Zero_Hour` main | `0a05454d8574207440a5fb15241b98ad0b435590` | queried 2026-08-23 | Archival validation reference only, per Section 13.5. Not checked out here, and not a source of implementation. |
| OpenW3D main | `b5fecf17ec92c99b3b73e2e9ecad1bee6982a21e` | 2026-08-23 | The destination at freeze time — the commit tagged `tt-native-complete`, where TT 4.8.4 is native and the Phase 6 hard gate passes. |

The donor is a git submodule at `GeneralsGameCode/`, so the pin is enforced by
the tree rather than by this document: `git submodule status` printing anything
other than the SHA above means the pin has moved and Phase 7's conclusions need
rechecking.

## The configuration

The implementation donor is **`Core` + `GeneralsMD`**, and that is not a
convention we adopted — it is the donor's own build condition. Its root
`CMakeLists.txt` reads:

```cmake
add_subdirectory(Core)

# Add main build targets
if(RTS_BUILD_ZEROHOUR)
    add_subdirectory(GeneralsMD)
endif()

if(RTS_BUILD_GENERALS)
    add_subdirectory(Generals)
endif()
```

`Core` is unconditional; `GeneralsMD` is the Zero Hour target; `Generals/` is a
separate target behind a separate flag. Building Zero Hour never compiles a
line of `Generals/`.

## What is excluded, and why it stays excluded

**`Generals/` is not a donor stage, a predecessor implementation, or a
fallback.** Roadmap Section 13.1 forbids adding it to the donor inventory and
Section 13.7 forbids staging through it. There is no plan anywhere in this
project to build a predecessor-game version of a system and then upgrade it;
each of the 25 mandatory systems is adapted once, from the Zero Hour
configuration, in its current form.

This is enforced rather than promised. `tools/zerohour/donor_surface.py` walks
the donor's CMake graph with `RTS_BUILD_GENERALS` treated as false, and exits
with an error if a `Generals/` path ever reaches the inventory.
`tools/zerohour/community_audit.py` drops commits that touch only `Generals/`
— 125 of the donor's 1712 — so that tree's history does not leak into ours
either.

`Dependencies/`, `GeneralsReplays/`, `resources/`, `scripts/` and `triplets/`
are donor build infrastructure. They are inventoried where the target graph
reaches them and are not port candidates.

## When EA's tree may be opened

Section 13.5, and only Section 13.5: when a community change has materially
altered semantics and the intended original behaviour is unclear, EA's
`GeneralsMD` may be read to understand what shipped in 2003. It is not a
prerequisite for a port, it is not a second implementation to reconcile
against, and consulting it does not put `Generals/` back in the workflow. Any
port that actually consulted it records the fact in its port-matrix row; a
blank in that column means it was not needed.

## Why the donor's own history is the audit

The donor's history begins at EA's source release and runs to the pin:

```text
1712 commits, authored 2025-02-27 through 2026-08-21
```

Everything in it is community work. That is what makes Section 13.4's audit
tractable — there is no need to separate "EA code" from "TheSuperHackers
changes" by inspection, because the repository already does it by construction.
See `ZeroHourCommunityEnhancementAudit.md`.

## Generated companions

Four TSVs in this directory are generated from the pinned donor, not written by
hand. Regenerate them if the pin moves; they will disagree loudly if it has.

| File | Tool | What it holds |
| --- | --- | --- |
| `ZeroHourDonorSurface.tsv` | `tools/zerohour/donor_surface.py` | The CMake target graph the Zero Hour configuration reaches: 51 directories, 71 targets. |
| `ZeroHourCapabilitySources.tsv` | `tools/zerohour/capability_map.py` | Each of the 25 mandatory systems resolved to real donor files. |
| `ZeroHourCommunityCommits.tsv` | `tools/zerohour/community_audit.py` | Every donor commit, attributed to the systems whose files it touched. |
| `ZeroHourSharedLineage.tsv` | `tools/zerohour/shared_lineage.py` | Donor library files that already exist in this tree under the same name. |
