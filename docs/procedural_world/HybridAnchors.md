# Hybrid rewrite I9 — runtime Phase 8, validate and freeze base/Tiberium anchors

Owner of the plan's runtime Phase 8 (`Procedural_World_Generator_Rewrite_Plan_FINAL_AUDITED.md`
Section 8 Phase 8, Section 0.5, Section 0.10).

| Concern | Owner |
|---|---|
| Base hard requirements, exit sectors, resource legality, provisional fairness, gate, checksum | `Code/wwphys/hybridanchors.{h,cpp}` |
| `PlanningDistance` over the frozen strategic graph | `Hybrid_Planning_Distance` in `Code/wwphys/hybridstrategicgraph.{h,cpp}` |
| The exit-sector convention Phase 9 reuses | `Hybrid_Exit_Sector_Direction` |
| The realized ground being validated | `WorldTerrainSystem`, read through a height sampler |
| Profile adapter and the per-attempt record | `ProceduralWorldManager::Build_Hybrid_Anchors` / `Get_Hybrid_Anchors` |
| Gate self-check | `Check_Hybrid_Anchors` in `Code/Commando/terrainselfcheck.cpp`, ctest `terrain_hybridanchors` + `fds_terrain_hybridanchors` |

## What the phase does, and what it deliberately does not

Phase 8 draws nothing and moves nothing. Phase 3 chose every role centre and Phase 4 reserved it
before geology and hydrology existed, so the only question here is whether the ground Phase 7
actually realized still satisfies the hard requirements. It does not: fail the attempt. Moving a
centre would move it into terrain nothing reserved.

What it freezes is what later phases treat as authoritative:

- each base's centre and `BASE_PROTECTED` radius, taken from the Phase-4 reservation itself rather
  than recomputed, plus the measured clear expansion area and the steepest slope over the
  starting-building proxy area;
- the sorted list of viable heavy exit-sector ordinals Phase 9 assigns base entrances from;
- each Tiberium anchor's centre, its `field_radius_max` protected reserve and the Phase-3 corridor
  that can reach that reserve's edge;
- the Section-0.10 provisional fairness numbers: home-field and base-to-front `PlanningDistance`
  times at `V_REF`, and the strategic base-to-base planning distance.

It applies the provisional fairness gates only. The base-to-base minimum and maximum time gates need
routed road distance, which does not exist until Phase 18, so Phase 19 owns them.

## Resolutions the plan leaves open

**The starting-building proxy area is the `R_BASE` core disc.** The plan asks for slope over "the
required starting-building proxy area" without naming it. Phase 4 already reserves `BASE_CORE` at
`R_BASE` for exactly the starting buildings, so that disc is the proxy area and the buildable-slope
test is measured over it.

**Expansion area is measured, using the existing owner's radius rule.** `BaseSelectionPlanner` turns
`fairness.min_base_expansion_area` into a radius with `sqrt(area/pi)` — Section 0.5's `R_EXPAND`,
already part of `R_BASE_PROTECTED` — so the question Phase 8 asks is how much of that same protected
disc is actually clear buildable land: placeable, dry, outside immutable walls and within the
buildable slope cap, sampled at `A/2`.

**Slope comes from the written heightfield, not the formula.** Phase 8 validates the ground that
exists, including whatever the ordinary-slope relaxation pass left behind, so the slope at a point is
the central-difference gradient of the sampled heightfield over one analysis spacing.

**"Outside BASE_PROTECTED" is measured on the disc, not on the rasterized flag.** A coarse cell is `S`
wide and is flagged for its whole area when its centre falls inside the protected disc, so a field
reserve that legally clears the disc by less than half a cell would otherwise be refused for a
rounding artifact. Phase 3 already separates a field centre from a base centre by
`R_BASE_PROTECTED + field_radius_max`; Phase 8 measures that same geometry.

**`V_REF` is used exactly as the existing owner uses it.** `BaseSelectionPlanner` divides world-unit
lengths by `WorldMetricsClass::Get_Reference_Vehicle_Speed()` with no unit conversion, and Section
0.10 says to use that value unchanged, so Phase 8 does the same rather than inventing a
world-units-per-metre step.

**A field's approach is a Phase-3 corridor, checked to the reserve edge.** The plan asks for "at least
one route-corridor approach direction from Phase 3 that can reach the reserve edge without immutable
obstruction". That is realized as: for each Phase-3 edge attached to the field, walk the straight run
from the centre toward the other endpoint out to the reserve radius and require it clear of water and
immutable walls. The first such edge is recorded as the anchor's approach.

## Failure behavior

Every failure is a named attempt failure and leaves nothing frozen: `missing_role`,
`base_not_placeable`, `base_hard_overlap`, `base_too_steep`, `base_no_expansion`, `base_no_exits`,
`resource_illegal`, `resource_no_approach`, `fairness_home`, `fairness_front`, `base_to_base_zero`,
and `invalid_input`.
