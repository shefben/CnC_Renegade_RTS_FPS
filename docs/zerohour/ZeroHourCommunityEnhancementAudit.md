# TheSuperHackers community enhancement audit

Roadmap Section 13.4. The pinned donor is not EA's 2003 code with a build
system bolted on; it carries eighteen months of community maintenance, and the
useful parts of that are part of what gets ported. This is the audit of it.

## Why the donor's own history is the audit

The donor's history begins at EA's source release and ends at the pin:

```text
1712 commits, authored 2025-02-27 through 2026-08-21
```

Every commit in it is community work, by construction. There is no need to
separate "EA code" from "community changes" by inspection, and no risk of
mistaking one for the other.

`tools/zerohour/community_audit.py` attributes each commit to the Zero Hour
systems whose **files it touched**, not to the words in its subject line, so a
pathfinding fix whose message never says "pathfinding" still lands on ZH-24.
Commits that touch only `Generals/` are dropped — that tree is not a donor and
its history is not ours to mine.

```text
1712 commits total
 125 touch only Generals/ and are dropped
1587 remain
 297 touch a file belonging to one of the 25 mandatory systems
 924 are bugfix / fix / perf / unify / refactor / feat / tweak
```

Full attribution is in `ZeroHourCommunityCommits.tsv`, one row per commit.

## Where the maintenance went

Commits per mandatory system, from the same attribution:

| System | Commits | System | Commits |
| --- | --- | --- | --- |
| ZH-24 AI / pathfinding / guard | 90 | ZH-16 Radar | 28 |
| ZH-19 Spatial partitioning | 84 | ZH-25 Telemetry | 28 |
| ZH-04 Heightmap / terrain | 78 | ZH-14 Shader manager | 23 |
| ZH-18 Modular draw components | 72 | ZH-03 Runtime roads | 20 |
| ZH-15 Shroud / fog-of-war | 55 | ZH-20 Debug rendering | 20 |
| ZH-08 Projected / cached shadows | 47 | ZH-10 Particle batching | 19 |
| ZH-01 Asset residency | 43 | ZH-17 Waypoint / status circles | 19 |
| ZH-02 Sectional bridges | 37 | ZH-06 Tree / foliage buffer | 17 |
| ZH-07 Water / river | 28 | ZH-09 Terrain tracks | 16 |

The tail — ZH-05 terrain textures (7), ZH-11 dynamic lights (6), ZH-23 weather
(6), ZH-13 debris (5), ZH-21 far terrain (4) — is small because those systems
are small, not because they were neglected.

**This distribution is itself a finding.** The three most-maintained systems
are pathfinding, spatial partition and terrain, which are exactly the three the
Commander work depends on most. Porting them at the pinned SHA rather than
from EA's release is worth 250-odd commits of other people's debugging.

## Accepted work, by the categories Section 13.4 asks for

Each named commit is in `ZeroHourCommunityCommits.tsv` with its full subject.
Accepting a category means the port takes the donor's **current** state of that
code, not EA's — it does not mean cherry-picking these commits one by one.

### Bug fixes worth having

- `dc5408780`, `55ccd6204` — two retail pathfinder crashes: a dangling linked
  list pointer in a `pathfindCell`, and a closed-list head with no `m_info`.
  Both are in code ZH-24 ports.
- `78221a001` — crash in `W3DTreeBuffer::updateVertexBuffer()` (ZH-06).
- `c11f37d4f` — out-of-bounds texture access in `W3DShroud::getShroudLevel()`
  (ZH-15).
- `6a19b8bcc`, `eccb41ef5` — ray casting in
  `BaseHeightMapRenderObjClass::Cast_Ray`, fixed twice (ZH-04). Mouse picking
  against terrain depends on this.
- `21cc640c9` — view ray casts for mouse picks and drawable occlusion. The
  Commander sidebar clicks on physical buildings; this is that path.
- `41bdc8a4a` — crash from an uninitialised debris shadow name (ZH-08/ZH-13).
- `0a65107d7` — `.wak` path handling in `WaterTracksRenderSystem` (ZH-07).
- `5871a889f` — uninitialised variable in `Pathfinder::classifyFence`.

### Performance improvements

The donor's optimisation work is unusually well quantified, which makes it easy
to judge:

- `2661b5c56` — conditional reverse insertion sort in the pathfinder, **50-66%
  off long paths**. ZH-24, and directly relevant to many AI units pathing at
  once.
- `bd376c43d` — radar pixel draw in
  `PartitionManager::refreshShroudForLocalPlayer`, **96%**.
- `25b173516` — `W3DRadar::renderObjectList` **80%**, `buildTerrainTexture`
  **25%**.
- `aa073667c` — min-height loop in `HeightMapRenderObjClass::updateCenter`,
  **93%**.
- `fa4aabfe8` — `ParticleSystemManager::findParticleSystem`, **80%**.
- `f06f18386` — waypoint and polygon-trigger label lookup, **90%**.
- `d37051f60` — redundant path-exists check removed from
  `Pathfinder::checkForAdjust`.
- `c47b01a6f`, `fe72137f3` — terrain tiles updated only when the draw origin
  really moved; no full terrain rebuild per frame.
- `de0fc8f38`, `e9895768f` — data locality and repeated-accessor work in the
  heightmap vertex buffers.
- `d95a9f8c9` — `W3DSmudgeManager::render` early-out when there are no smudges.

One negative result is worth as much: `bd4e909ad` **reverts** an earlier
`m_vertexBufferTiles` optimisation because it did not work properly. Taking the
donor's current state gets the revert too; reconstructing from EA's release and
"applying the optimisations" would reintroduce the bug.

### Memory safety

- `8727e1eb5` — `sprintf` replaced with `snprintf` throughout.
- `038f51f1b` — `strcpy` replaced with `strlcpy`.
- `0bb57b2c1` — `static_assert`s placed near the remaining `strcpy` calls so
  the sizes stay provable.
- `20f42549c` — a batch of memory leaks.
- `53845b715`, `00d67999a` — scorch buffers released before reallocation.
- The WWAudio handle wrappers (`listenerhandle`, `sound2dhandle`,
  `sound3dhandle`, `soundstreamhandle`, ZH-32) exist for this reason: they
  replace raw Miles handles with owning types. OpenW3D's `Code/WWAudio` has the
  same raw-handle shape and the same exposure.

### Stability

`dbghelpguard`, `dbghelploader` and the minidump writer (ZH-29) are the
donor-only WWLib files that turn a crash into a report. OpenW3D has no
equivalent. `75d59379e` also simplifies unconditional `DEBUG_ASSERTCRASH` uses
to `DEBUG_CRASH`, which matters for a dedicated server that must not stop on a
modal box — the same lesson Phase 6 learned the hard way.

### Cross-platform work

A full MinGW-w64 i686 port (`#2067`: `c2d156048`, `b873b9e24`, `e4c78ab36`,
`efca138c5`, `7c6cd3514`, `be30428a1`, `f244a23c2`) plus Linux LSP support
(`f79f6be4a`). Classified `PORT_PORTABILITY_IDEA`: OpenW3D targets MSVC x86 and
adopting MinGW is not on this roadmap, but the compiler guards, the forward
declarations and the inline-assembly handling are exactly the changes anyone
would need later, and the WWLib shims (`mingw.h`, `visualc.h`, `strtok_r`) come
with them as ZH-35.

### Renderer correctness

- `920c4e639`, `124daa98a` — smudge and police-car light time steps decoupled
  from the render update. The general lesson (simulation time must not be
  driven by frame rate) applies to anything OpenW3D ports from these files.
- `a2b5d7210` — scorch texture mip quality follows the texture quality setting.
- `ccd876661`, `c9899fc55` — terrain draw area at low camera pitch, and a
  settable terrain draw size. The Commander's strategic camera looks along the
  ground at exactly the angles this fixes.
- `86eccd74e` — ghosted and shrouded objects stay hidden when taking a ghost
  snapshot or changing the local player. This is the shroud honouring the
  intelligence firewall, which is a hard requirement here, not a nicety.

### Useful modern refactors

91 commits are `unify`, moving Zero Hour code into shared `Core` — for example
`330ad2252`, which moved `BaseHeightMap`, `FlatHeightMap`, `W3DPropDraw`,
`W3DTreeDraw`, `W3DPropBuffer`, `W3DSnow` and `W3DTerrainBackground` into
`Core`, and `1e2154225`, which moved `W3DDevice/Common`. This is why the
capability map finds most terrain and render systems in `Core` rather than
`GeneralsMD`, and it is why the map is generated rather than written down.

Two structural refactors are worth reading before porting the systems they
touch: `83ca91bc9` extracts scorch rendering out of `BaseHeightMap` into a new
`W3DScorch` (a cleaner seam than EA's), and `4990ca933` merges most of
`Drawable` and `Locomotor` — relevant to ZH-18, and a reminder that the SAGE
`Drawable` ownership model is a **rejected** subpart even where the code around
it is taken.

### Profiling and debugging additions

Tracy integration (`Core/Libraries/Source/profile`, ZH-26), the structured
debug library and its out-of-process window (ZH-27), and `CRCDiff` (ZH-36) for
finding the frame where two runs diverged. OpenW3D has none of the three.

## What is not accepted

- **Balance and content changes** — `BALANCE_ONLY_REJECT`,
  `CONTENT_ONLY_REJECT`. The donor's `tweak(...)` commits are frequently
  gameplay balance for Zero Hour factions, which OpenW3D does not have.
- **Generals challenge and campaign shell work** — `CAMPAIGN_CHALLENGE_REJECT`.
- **GameSpy, WOL and retail patcher maintenance** —
  `RETAIL_INFRASTRUCTURE_REJECT`, per `ZeroHourSurfaceClassification.tsv`.
- **Replay-system and SAGE-networking fixes** — the same code these fix is
  rejected architecture here; OpenW3D keeps its own networking.
- **WorldBuilder and GUIEdit improvements** — tools for formats OpenW3D does
  not use.

## One canonical implementation

Section 13.4's closing rule, restated because it is easy to violate by
accident: a community change becomes part of the same canonical OpenW3D
implementation. There is no "EA behaviour" mode and no "TheSuperHackers
behaviour" mode, no flag selecting between them, and no wrapper preserving both.
This is the same directive that governed the TT merge in Phases 1 through 6,
and it is not weakened by the donor changing.
