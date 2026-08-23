# The OpenW3D port matrix

Roadmap Section 13.6. Forty accepted Zero Hour capabilities, each recording the
eighteen things Section 13.6 asks for. The data is in
`OpenW3DPortMatrix.tsv` — eighteen columns is not a readable markdown table,
and splitting it into three tables would let the columns drift apart. This file
says how to read it and what it adds up to.

The donor pin every `DonorSources` cell is relative to is
`9fee97a1792efc50edfe27300581970b4c022951`; see `ZeroHourDonorBaseline.md`.

## The columns

| Column | What it holds |
| --- | --- |
| `ID` | `ZH-01` … `ZH-40`. The first 25 are roadmap Section 3's mandatory systems in order; `ZH-26` and up came out of the Section 3.2 audit. |
| `Capability` | The system's name. |
| `DonorSources` | Files and symbols at the pinned SHA. |
| `Tree` | `Core`, `GeneralsMD`, or both. |
| `Classification` | The Section 13.2 vocabulary. |
| `EAValidation` | Blank unless EA's tree is actually needed, and blank is information — Section 13.5 makes it optional. |
| `OpenW3DEquivalent` | What this tree has today. `none` where there is nothing. |
| `TargetModule` | Where the port lands. |
| `TTDependency` | Whether native TT 4.8.4 is involved. |
| `RendererDependency` … `SaveLoadDependency` | Five dependency columns: renderer, physics, AI, network, save/load. |
| `ClientServer` | Client, server, both, or tool. |
| `MigrationClass` | The Section 13.6 vocabulary. |
| `RejectedSubparts` | What is deliberately not taken, **with the reason**, or `none`. |
| `Tests` | What would demonstrate the port works. Not a promise of a specific harness; a statement of what the port has to be able to show. |

`tools/zerohour/check_port_matrix.py` enforces all of it: eighteen fields per
row, no required field blank, migration class and classification inside their
vocabularies, a reason attached to every named rejected subpart, at least one
test per row, and exactly one row per capability the capability matrix names.
It caught thirteen rows missing a column on its first run.

## What the forty add up to

```text
DIRECT_ADAPT           18
ADAPTED_BACKPORT       12
ALGORITHM_ONLY          5
ARCHITECTURE_ONLY       3
ALREADY_BETTER_EXTEND   1
PERFORMANCE_IDEA_ONLY   1
```

Thirty of forty are `DIRECT_ADAPT` or `ADAPTED_BACKPORT`, which is unusually
high for a cross-engine port and has one cause: **87% of the donor's library
files already exist in this tree under the same name** (see
`ZeroHourCapabilityMatrix.md`). Adapting `W3DTreeBuffer` means writing against
a `ww3d2` that is the same `ww3d2`, not translating between two renderers.

Only three capabilities need EA's tree consulted at all, and each says why:
ZH-04 if the community's `Cast_Ray` fixes turn out to conflict, ZH-18 before
adopting the `Drawable`/`Locomotor` merge, and ZH-24 when a pathfinder change
alters retail behaviour. The other thirty-seven are blank, meaning the current
community implementation is the behaviour we want.

## The dependency structure, read down the columns

Reading the five dependency columns together says more than any single row.

**Save/load is the widest dependency.** Nineteen of forty capabilities touch
persistence — every terrain and placement system stores level data, and shroud,
radar, AI state, attachments and telemetry store per-player state. OpenW3D and
the donor share `wwsaveload` file-for-file (33 of 33 names), so the persistence
contract is common ancestry rather than a translation layer. That is why
`ADAPTED_BACKPORT` is viable this often.

**Only ZH-24 depends on TT.** Commander-produced units are ordinary Renegade AI
objects, so the 2477 native scripts still attach to them. Nothing else in the
matrix touches TT at all, which is the expected result after Phases 1 through 6
merged TT into the canonical owners: there is no TT layer left to depend on.

**Seventeen rows carry a network or inter-process dependency; three say
server-authoritative in as many words.** ZH-15 (shroud) has the roadmap's
intelligence firewall written into its `RejectedSubparts` cell: client-side
shroud authority is rejected because strategic camera movement must not be a
sensor. ZH-24 (AI orders) and ZH-25 (telemetry) are the other two. The rest are
weaker couplings — replicated state, client prediction, or a tool talking to a
running game.

**Eighteen capabilities are client only, and seven of those have no AI,
network or save/load dependency at all**: ZH-08 shadows, ZH-11 dynamic lights,
ZH-14 the shader manager, ZH-21 far terrain, ZH-30 texture filtering, ZH-31 the
sorted draw list and ZH-32 the audio handles. Those seven can proceed
independently of the Commander work and are the natural first batch. The other
eleven client-only rows depend on level data being stored, which is a lighter
dependency but not nothing.

## Where the rejections are

Seventeen rows name a rejected subpart. They fall into three groups:

- **SAGE ownership** — ZH-18 rejects `Thing`/`Drawable` ownership, ZH-01
  rejects prototype/Thing binding, ZH-38 rejects the SAGE INI particle format.
  OpenW3D has definitions and its own game loop, and directive 0.4 forbids
  keeping both.
- **Preserving Renegade** — ZH-04 rejects replacing authored W3D world geometry
  outright. The heightmap has to coexist with hand-built interiors, which is
  the single hardest constraint in the whole matrix and is stated in the row
  rather than discovered later.
- **Already better here** — ZH-16 rejects replacing `RadarManager` and ZH-19
  rejects `PartitionManager` as a container, in both cases because OpenW3D's
  existing implementation suits a world with interiors better than the donor's
  does.

The remaining rejections are content, faction behaviour and hardware fallbacks
OpenW3D no longer targets.

## What this matrix is not

It is not an implementation order. Phase 8 and later decide that, and the
roadmap's own dependency order governs. It is not a promise that all forty get
built. And it is not a place where a capability may quietly disappear: the
checker fails if a capability named in the capability matrix loses its row, so
dropping one is a decision somebody has to write down.
