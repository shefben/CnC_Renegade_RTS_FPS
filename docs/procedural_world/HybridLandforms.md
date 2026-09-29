# Hybrid rewrite I6 — runtime Phase 5, vector landforms

Owner of the plan's runtime Phase 5 (`Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md`
Section 8 Phase 5, Sections 2.3, 5, 6.8, 6.10, 10 and 16).

| Concern | Owner |
|---|---|
| Target counts, creation order, candidate process, plateau conversion, gate, checksum | `Code/wwphys/hybridlandforms.{h,cpp}` |
| The Section-10.3/10.4 field profiles and the Phase-7 `landform` height term | `Hybrid_Landform_Contribution` / `Hybrid_Landform_Height` |
| Centripetal Catmull-Rom sampling (added for this item, shared) | `Code/wwphys/hybridplanning.{h,cpp}` — `Hybrid_Sample_Centripetal_Catmull_Rom` |
| `LandformMask`, `RoughnessClass` and the landform flag bits | Phase 5 writes them into the Phase-4 `HybridSemanticMatrix`; Phase 4 leaves all three clear |
| `BASE_PROTECTED` / `NO_IMMUTABLE_BARRIER` exclusions | read from `HybridCoarseMatrices`; Phase 5 derives no reserve of its own |
| Section-6.1 bounded repair for a required feature | `Hybrid_Repair_Ring_Search` (`hybridmapidentity`), unchanged |
| Profile adapter, per-attempt arena and counters | `ProceduralWorldManager::Build_Hybrid_Landforms` |
| Gate self-check | `Check_Hybrid_Landforms` in `Code/Commando/terrainselfcheck.cpp`, ctest `terrain_hybridlandforms` + `fds_terrain_hybridlandforms` |

## What the phase produces

`HybridLandformSet` holds one `HybridLandform` per accepted feature in creation order (StableId is
1-based creation order), plus the Section-2.3 `ProceduralPlanningSpatialHash` over their AABBs in
coordinates relative to the world rectangle's minimum corner. Creation order is the plan's:

1. archetype principal features from the frozen Section-6.10 intents;
2. the remaining required canyon/ravine systems;
3. explicit cliff runs;
4. the remaining ridges;
5. hills;
6. the optional plateau conversion.

No heightfield is written — Phase 7 owns terrain — and no crossing window is created, because water
is not fixed until Phase 6. The provisional height Phase 6 and Phase 7 need is
`Hybrid_Landform_Height`, which sums hill/ridge/cliff fields and combines the members of one
canyon/ravine system by Section 10.3's rule (minimum contribution, then one `A`-wide smooth union)
before summing the per-system result.

## Resolutions the plan leaves open

These are implementation decisions taken from the plan's own evidence rather than invented
alternatives. Each is deterministic and recorded in `DECISIONS.md`.

**Ravine centerline length.** The Section-8 Phase-5 geometry table gives a *canyon* centerline
length (`0.35D..0.75D`) and never gives one for a ravine, although it gives a ravine its own floor
half-width, rim half-width and depth. Every shared row is the canyon row scaled by about two
thirds (ravine floor `1.0..2.0W` against canyon `1.5..3.0W`), so the ravine length range is the
canyon row scaled by `2/3`, and a Section-6.8 attached branch is that ravine range scaled by `2/3`
again. An independent ravine still has to pass Section 5's `max(12W,0.20D)` qualification; a branch
does not, because its system root carries the qualification.

**Cliff field shape.** Section 10.2 gives a cliff exactly two widths — edge width `0.5..1.5W` and
rock/talus transition width `1..3W` — so the field uses no other length scale: it rises from zero at
the talus toe to full height at the upper edge, holds full height across one talus width of shelf
and returns to zero across one more, with a longitudinal taper over the same distance at each end of
the run. That keeps the field compactly supported the way Section 10.4 requires of a landform field.
An unbounded one-sided step would lift terrain the cliff never reserved — including a base a third
of a map away — which no Phase-5 rejection rule would ever see.

**Generic canyon/ravine systems are ravines.** `terrain.valley_density` is the knob the ravine
target adds, so a density-created system is a ravine. A canyon exists only where an archetype
requires one (`PRINCIPAL_CANYON`).

**Basin and coast are not Phase-5 features.** Creation order step 1 names "archetype principal
ridge/canyon/ravine/basin/coast features", but `plateau_basin`'s basin is already a Phase-4 macro
zone and every coast/water polygon belongs to Phase 6, so Phase 5 creates a feature only for
`PRINCIPAL_CANYON`, `PRINCIPAL_RIDGE` (with its `SURFACE_SADDLE`) and `MAIN_RAVINE_GUIDE` plus its
Section-6.8 branches.

**Heavy-corridor dry width applies to generic barriers.** Rejection rule 5's `4W` dry/passable width
clause is enforced for generic immutable features only. An archetype principal canyon or ravine
exists in order to obstruct the axis, Phase 3 already recorded the crossing obligation for it, and
Phase 6 must produce the window; refusing it here would make `canyon_crossroads` and
`ravine_network` unbuildable.

**The candidate budget counts alternates, not misfits.** `retry.max_alternate_candidates_per_feature`
bounds the candidate cells tested. A cell where the feature simply does not fit inside the placeable
world — its polygon leaves the boundary or the planning matrix — is stepped over without spending a
slot; a cell that conflicts with other planned intent (`BASE_PROTECTED`, an `OPEN_MANEUVER`
no-immutable-barrier cell, a Tiberium reserve, a heavy corridor, a same-type overlap) spends one.
The keyed order and the set of acceptable positions are identical either way; only the retry budget
is spent on candidates that can physically work.

**Horizontal dimensions are clamped to the placeable interior.** Phase 5 already clamps its vertical
draws so provisional heights stay inside `terrain.min_elevation..max_elevation`. The horizontal
ranges need the same treatment: `0.75D` of canyon plus its rim does not fit a SMALL world once
`boundary_margin_cells` is taken off both sides, and a dimension is drawn once and never redrawn, so
without the clamp an arithmetic impossibility would fail otherwise-good seeds. The clamp can only
shrink a draw, never move it outside its range, and never takes a length below the Section-5
minimum; when even the minimum does not fit, the phase reports the named attempt failure.

**Reservation tests are taken at the feature's own sample points.** A cliff band or a ravine floor is
narrower than one coarse cell, so testing only cell centres would let a wall stand squarely inside a
reserve while every centre near it reads clear. Rule 5, the elevation clamp and the gate all sample
the influence outline plus the feature's spine, and a line feature additionally stamps the cell each
centerline sample stands in so a thin feature still appears in `LandformMask`.

## Layer encoding this phase owns

`LandformMask` is composable, one byte per coarse cell: `HILL`, `RIDGE`, `PLATEAU_TOP`, `CLIFF_FACE`,
`CANYON_FLOOR`, `CANYON_WALL`, `RAVINE_FLOOR`, `RAVINE_WALL`. `RoughnessClass` is one value per cell
and the roughest wins — `FLAT`, `ROLLING` (hill/ridge flank, plateau top, canyon or ravine floor),
`STEEP` (ridge core, canyon or ravine wall), `BROKEN` (explicit cliff face) — because the roughest
thing standing in a cell is what decoration, material and tactical passes must respect.

## Gate

`Hybrid_Build_Landforms` fails the attempt with a named failure and an empty set. The gate checks:

- the hill, ridge, cliff and canyon/ravine-system counts equal the Phase-5 targets;
- Section 5's minimum qualifications: cliff length `>= max(8W,0.12D)`, canyon/ravine system length
  `>= max(12W,0.20D)` and depth `>= 0.75*tier_step`;
- no immutable landform reaches a `BASE_PROTECTED` cell or an `OPEN_MANEUVER` no-immutable-barrier
  cell.

`out->Checksum` is a quantized digest of every accepted landform in StableId order, seeded with the
Phase-2 identity checksum.

## Known open question

**Q-018** (see `awaiting_answers.md`): at LARGE the plan's hill target
(`10 + RoundHalfUp(hill_density*6)`) with hill radius `0.08D..0.16D` cannot satisfy rejection rule
6's 25% same-type overlap cap together with the `BASE_PROTECTED` exclusion. An exhaustive search of
every fitting candidate cell refuses the last hill, so this is not a retry-budget shortfall. Phase 5
reports the named attempt failure the plan prescribes, and the self-check tolerates it at LARGE while
requiring success at MEDIUM, the size class the plan sets its own quality target on.
