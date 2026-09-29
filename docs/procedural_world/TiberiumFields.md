# Tiberium Fields (V2 Phase 12)

Roadmap Section 34 Phase 12 -- "Tiberium fields, crystals, harvest areas, and Harvester routes."
Section 15, Section 17A.6. See `StrategicPlanning.md` for the `RESOURCE_CANDIDATE` nodes this
phase resolves, `RoadGraph.md` for the real routes it measures, and `BaseSelection.md` for the
selected sites an initial field is initial *for*.

## Resolve, not discover

`TiberiumFieldGenerator` (`Code/Combat/tiberiumfieldgenerator.h/.cpp`) never re-derives terrain,
roads or bases. `StrategicPlanner` (Phase 7) already placed and range-gated every
`RESOURCE_CANDIDATE`; `RoadGraphGenerator` (Phase 8) already connected every one of them into its
own minimum spanning tree; `BaseSelectionPlanner` (Phase 11) already chose the one GDI/Nod pair
this phase's initial fields belong to. `RoadGraphGenerator::Find_Anchor_Path` -- promoted out of
`BaseSelectionPlanner`'s own file-local breadth-first walk once this phase needed the identical
question answered a second time -- is the one owner both generators now measure a real road
route through, rather than two independent copies of the same walk quietly drifting apart.

With a map plan (D-003) the resource candidates the planner offers already sit on the plan's
`FIELD_CENTER` and `FIELD_FLANK` discs, so the fields resolve there without this generator
reading the plan. See `MapPlan.md`.

## Schema 5: the profile fields Section 17A.6 requires

Section 17A.6 requires field/profile data to "define or derive" a field radius range, initial
yield range, crystal density, harvest anchor count, Harvester load amount, and regeneration/
growth rate. No prior schema reserved these the way Phase 1 reserved `Fairness` for Phase 15 --
`ProceduralWorldResourceSettings` (`proceduralworldprofile.h`) is new this phase, bumping
`CURRENT_SCHEMA_VERSION`/`CURRENT_GENERATOR_VERSION` to 5. A schema-1..4 profile still loads; the
new `resource` block's eight fields take their documented defaults. `Format_Canonical_Profile_String`
(`worldgenerationmanifest.cpp`) was extended in the same pass -- two profiles differing only in
this block now hash differently, as Section 3B.6 requires.

## Field footprint: a real, bounded shrink, not a fixed circle

`Resolve_Footprint_Radius` tries `FieldRadiusMax` first, then steps down toward `FieldRadiusMin`
over a bounded number of attempts (step 3's own "generate field footprints", step 10's own
"adjust field edge"). `Is_Footprint_Legal` is step 4's own rejection list, made concrete: the
whole disc must be on terrain and within the shared buildable-slope cap
(`WorldTerrainSystem::Query_Placement_Surface`, the same whole-disc measurement a building
placement query would use, not one sample at the centre); it must clear both selected bases' own
`BaseSelectionPlanner::ProtectedRadius` reserved regions (base spawn); and, ring-sampled around
the rim, it must clear water (`WaterSystem::Is_Point_In_Water`) and the road mask
(`TerrainTextureSystem::Peek_Mask(TERRAIN_MASK_ROAD)`, the same mask `RoadSystem` itself already
stamps). Cliff band and excessive slope beyond the whole-disc check need no separate test: every
`RESOURCE_CANDIDATE`'s own anchor point already survived `StrategicPlanner`'s own
`Buildable && !water` eligibility gate in Phase 7, so the only new risk a footprint introduces is
its own edge reaching past that guarantee.

## Step 5: the Tiberium terrain mask, its first writer

`TERRAIN_MASK_TIBERIUM` (`terraintexturesystem.h`) has existed since whichever phase first wrote
`terraintexturesystem.h`'s own mask enum, named in Section 18's own material-selection input list,
and until this phase nothing ever wrote to it -- `grep`ping the tree for it found zero callers.
Every resolved field stamps it (`TerrainMaskClass::Stamp_Disc`), the same primitive `RoadSystem`
and `WaterSystem` already use for their own masks, so Phase 14's own biome/material blending has
real Tiberium-influence data to read whenever it arrives.

## Step 6: crystal scatter, placement without a model

`Scatter_Point` draws a deterministic, uniform-over-area point inside a field's own disc from two
independent `TerrainMaskClass::Hash_Value` draws (angle, and the square root of a second draw for
radius -- a uniform radius draw alone clusters points at the centre, which a square root
corrects). `TiberiumCrystalPlacement` records the position and which field it belongs to; it
resolves no live W3D crystal model, the same "missing art never blocks engine implementation"
rule this project's own instructions state. A real `ow_tiberium_crystal` family of assets is the
open request this leaves; see "What is NOT resolved" below.

## Steps 7-9: Harvester routes, and step "both teams have viable early field"

Every field's own route is `RoadGraphGenerator::Find_Anchor_Path` from an owning base's node to
the field's own `RESOURCE_CANDIDATE` node -- real road distance, converted to an expected one-way
travel time and an expected income-per-minute (one `HarvesterLoadAmount` per round trip) using the
same fairness-estimate reference vehicle speed `BaseSelectionPlanner` already documents (no
canonical v1 travel-speed data exists to measure one from). `ResolvedTiberiumField::
CountsAsEarlyField` is true only for a non-contested field (a real owning faction) with a resolved
route -- the literal reading of Phase 12's own targeted-validation line, and what
`Check_Both_Factions_Have_Early_Field` asserts on a real chain.

**Turning turning/clearance into vehicle-envelope validation (step 9)** is not attempted --
`WorldTraversabilityGraph`'s own swept-envelope reasoning (Phase 65) needs a loaded wheeled
definition, the same honest degradation `BaseSelectionPlanner`'s own vehicle-route enrichment
already accepts; this generator's own route proof is the road-graph tree's real distance, not a
swept-turn simulation.

## Step 10: real repair for one operation, an alternate candidate for the other

`RESOURCE_REPAIR_ADJUST_FIELD_EDGE` is the radius shrink above, logged whenever a field actually
needed one. For an *initial* field specifically -- the one outcome Phase 12's own gate treats as
a real failure -- `RESOURCE_REPAIR_ALTERNATE_FIELD_CANDIDATE` retries the next-nearest
`RESOURCE_CANDIDATE` when the nearest one is illegal at every radius, bounded by the same
`Retry.MaxLocalRetriesPerStage` ceiling every other bounded local-repair loop in this chain
answers to. `RESOURCE_REPAIR_REPAIR_ROAD_LOCALLY` is named for diagnostic completeness and never
attempted -- the same scope boundary `BaseSelectionPlanner`'s own refused road-edit repair
already documents (no safe owner at this layer for mutating a road that may already carry a
committed bridge).

## What is NOT resolved this phase

**Live economy (steps 7's own "canonical harvestable field data" beyond this).** `HarvesterClass`
(`harvester.h`) already has the whole runtime contract a real Harvester needs --
`Set_Tiberium_Region`, `Set_Dock_Location`, `Set_Dock_Entrance` -- and already drives credits
through the canonical economy once a level script gives it those three. No caller in this
codebase spawns a Harvester for a generated world yet; that gap belongs to whichever later phase
actually instantiates starting units (Section 16A.8's own match-bootstrap contract, the same one
`BaseSelectionPlanner`'s own header already declined to close). This generator's job stops at
producing the real field data that call is missing.

**Growth/regeneration (steps 11-12).** `Resource.GrowthEnabled`/`GrowthRatePerSecond` are parsed
and validated; nothing simulates time passing yet, so a profile that enables growth today gets a
field that never grows, honestly, rather than one that silently ignores the setting elsewhere.

**Debug overlay/route visualization (step 13)** is not attempted -- a rendering concern with no
device in this generation path, the same reasoning every other headless generator in this chain
already gives for its own debug-draw step.

## What is proven, and what is not

`terrain_tiberiumfields`/`fds_terrain_tiberiumfields` (`-terrainselfcheck tiberiumfields`) chain
the same terrain/hydrology/strategic-plan/road-graph/bridge-plan/base-selection setup
`terrain_baseselection` already proves sufficient, then assert: refusal reports a reason and
leaves no state; a real chain resolves at least one field; every resolved field's footprint is
legal, has at least one crystal, and is neutral only when not counted as an early field; **both
factions actually have a viable early field on this terrain** (the phase's own real gate, not
merely structurally checked); every owned field has a resolved route; every repair log entry
names a recognised operation; and the same inputs reproduce the same checksum. Not proven: a
scenario where the `RESOURCE_REPAIR_ALTERNATE_FIELD_CANDIDATE` path is actually exercised (this
terrain's own nearest candidate per team already resolves cleanly), and everything named under
"What is NOT resolved" above.

## Base-to-front fairness (D-001)

`Check_Front_Travel_Time_Within_Threshold` measures Section 17A.1's
`MaxBaseToFrontTravelTimeDifferenceSeconds`. The front is the contested fields both selected bases
can reach by road; each base's front time is its road time to the nearest of them at the reference
vehicle speed. An unfair front joins the world's retry trigger. A world with no such field is not
checked, and the manager logs that it was not.
