# Gameplay Gap Resolution Report

Full audit pass over `gameplay-gaps.md` against current source, requested by
`GAMEPLAY_GAPS_AGENT_TASK.md`. Every entry below is checked against the repository as of this
pass (post component-assembly/door work, commits `95f1d2e2`/`e44783fd`), not against the
tracker's own wording. Several entries the tracker called still-broken are implemented and
working; one entry the tracker called working (production refund on facility destruction) is
confirmed broken in a live game despite the underlying arithmetic being correct. Both directions
are recorded with the exact file/line evidence found.

## Summary table

| Gap | Status | Classification | Required action | Blocking? |
| --- | --- | --- | --- | --- |
| No air units can ever be ordered | STILL TRUE | CODE + CONTENT | Author Air Pad descriptor + real preset roster + roster-rendering code | No |
| Building prices/build times/power unset | PARTIALLY TRUE (8/8 existing authored; 8/12 categories missing entirely) | CONTENT | Author remaining 8 categories | No |
| Building cards have no icons | STILL TRUE | CONTENT/packaging | Measure card size, convert+package icons, set `icon_texture` | No |
| Faction rosters not defined | OBSOLETE as its own item | -- | Folded into building-prices entry | -- |
| Nothing appears when building placed | OBSOLETE | -- | Deleted; see new MCT/PT entry for the real remaining piece | -- |
| Runtime buildings have no working MCT/PT *(new)* | STILL TRUE | CODE | Interactive MCT proxy for runtime buildings; PT design pass | No |
| Placement has no cursor | OBSOLETE | -- | Deleted | -- |
| No build prerequisites | STILL TRUE | CODE (schema gap) | Add prerequisite block to descriptor schema + placement check | No |
| Buildings cannot be sold/repaired | STILL TRUE | CODE | New building-management UI + sell/repair/power events | No |
| No unit is ever produced | OBSOLETE | -- | Deleted | -- |
| Produced units have nowhere to walk out | PARTIALLY TRUE | CODE (schema) + CONTENT | Add kind-specific anchor roles + fallback transform fields | No |
| Orders recorded but not obeyed | PARTIALLY TRUE (PATROL only) | CODE | Two-point patrol wire format + loop state | No |
| No unit selection by pointing | PARTIALLY TRUE (3 pieces) | CODE | Shift+click toggle, control groups, building hit-test | No |
| Population caps placeholder | PARTIALLY TRUE | DESIGN-BALANCE + CODE | Per-unit weight table, sum instead of count | No |
| Wallet has no ceiling | OBSOLETE | -- | Deleted | -- |
| Refund policy full-price | PARTIALLY TRUE, real bug found | CODE | Wire `Cancel_Item`/`On_Facility_Destroyed` to real call sites | No |
| Power only matters with descriptor | PARTIALLY TRUE | CODE | Add alias-resolution tier between explicit name and legacy fallback | No |
| No audio/warning on power loss | STILL TRUE | CODE + CONTENT | Edge-triggered HUD+sound announcement, team-wide | No |
| Anyone can become Commander | PARTIALLY TRUE (real bug: disconnect) | CODE | Clear commander on disconnect; gate console command; add resign | No |
| Commander Mode has no dedicated input | OBSOLETE (pending one file check) | -- | Confirm B appears in rebind screen | No |

---

## Commander sidebar

### No air units can ever be ordered

Status:
- STILL TRUE

Classification:
- CODE + CONTENT-AUTHORING

Current project state:
- `Code/Combat/commandersidebar.cpp:337-345` -- `SIDEBAR_MODE_AIR_VEHICLES` is real: it binds to
  an Air Pad, pages, and its `case` is intentionally empty with a comment explaining why (no
  `PurchaseSettingsDefClass::TYPE_AIRCRAFT` in stock Renegade). `BuildingProductionKindType`
  already has `BUILDING_PRODUCTION_AIR_VEHICLE` (`buildingdescriptor.h:143`). No descriptor sets
  `production.kind` to it -- zero air-producing facility exists in content.

Player-visible problem:
- Build an Air Pad, open the `AIR_VEHICLES` tab, see zero cards forever.

Final intended behavior:
- A data-driven, faction-aware air catalog owned by the Commander sidebar itself (not a stock
  `PurchaseSettingsDefClass::TYPE_AIRCRAFT` that does not exist), with real GDI/Nod flying
  `VehicleGameObjDef` presets behind it, bound to a dedicated Air Pad facility.

Implementation:
- Enumerate real flying vehicle presets from `Objects.DDB` using the same LevelEdit CLI mode
  Q-017 already built (`docs/zerohour/BuildingPresets.md`) -- do not guess names.
- Author an Air Pad `data/building_descriptors/*.json` with `production.kind: "air_vehicle"`.
- Add a small project-owned air-roster table (JSON, same convention as everything else in
  `data/`) naming entry/cost/tech level/icon per faction.
- Extend `commandersidebar.cpp`'s `SIDEBAR_MODE_AIR_VEHICLES` case with an
  `Add_Air_Catalog_Cards(available_credits)` reading that table, matching
  `Add_Purchase_Page_Cards`'s existing card shape.

Compatibility constraints:
- Stock flying-map aircraft (if any use the existing `PurchaseSettingsDefClass` machinery some
  other way) untouched; this is additive and Commander-sidebar-scoped only.

Network/save-load implications:
- None new -- reuses the existing `cCommanderProductionEvent` purchase path once the roster
  table resolves a `DefinitionId`/cost pair.

Acceptance:
- Device-less test parsing/validating the roster table.
- Manual: build Air Pad, open `AIR_VEHICLES`, purchase, confirm spawn.

Tracker action:
- REWRITE -- keep the gap, replace "no content exists at all" framing with the concrete next
  steps (preset enumeration + Air Pad descriptor + roster table), since the mode/paging/refusal
  code itself is already correct and does not need to change.

Dependencies/blockers:
- None. Needs one `Objects.DDB` enumeration pass, which is investigation, not a guess.

---

### Building prices, build times and power figures are unset

Status:
- PARTIALLY TRUE / REFRAMED

Classification:
- CONTENT-AUTHORING

Current project state:
- Read every file in `data/building_descriptors/` this pass. All 8 existing descriptors
  (GDI/Nod x Power Plant, Communications Center, Advanced Communications Center, Tiberium Silo)
  have real authored `cost`, `tech_level`, `team` and `power.generation`/`power.consumption`
  values -- e.g. `gdi_power_plant.json`: cost 1000, tech 1, team 1 (GDI), power generation 500;
  `gdi_advanced_communications_center.json`: cost 1800, tech 2, power consumption 150.
  `build_time_seconds` is unset on all 8, which is the documented, intentional fallback to
  `ConstructionVisual.DurationSeconds` (15s) per `BuildingModelDescriptor::Get_Build_Time_Seconds`
  (`buildingdescriptor.h:125`), not a missing value. The real gap: only 8 of the roadmap's 12
  named categories have a descriptor file at all -- Barracks/Hand of Nod, Refinery, Repair Bay,
  War/Vehicle Factory, Airstrip, Construction Yard and faction defense structures have none
  (confirmed: `data/building_descriptors/` contains exactly 8 files), matching
  `WIP_features.md`'s own already-accurate note at its P33 entry.

Player-visible problem:
- The 8 existing buildings already show a real, judgeable price/power figure. The other 8+
  categories simply do not appear as buildable yet -- not "buildable with blank numbers."

Final intended behavior:
- Descriptors authored for the remaining categories with a full playable first-pass balance
  table (see Decision Log below for the recommended starting numbers).

Implementation:
- New `data/building_descriptors/*.json` per missing category. No new code --
  `BuildingDescriptorManager`'s parser/validator (`Parse_And_Validate`, `Validate_Directory`)
  already handles every field these need.

Compatibility constraints:
- None -- additive content only.

Network/save-load implications:
- None -- static data, read at startup and by `reload_building_descriptors`.

Acceptance:
- `BuildingDescriptorManager` validation passes for each new file (LevelEdit's existing
  **Report -> Validate Building Descriptors...**, Q-015).
- Manual: each new building's sidebar card shows the authored price/power.

Tracker action:
- REWRITE -- the "entirely unauthored" framing is false for the 8 that exist. Replace with the
  precise 8-of-12-categories-missing framing. Absorbs the separate "Faction rosters are not
  defined" entry (see below).

Dependencies/blockers:
- None. Content-only, once the recommended balance table (Decision Log) is accepted or adjusted.

---

### Building cards will draw without pictures until the icons are packaged

Status:
- STILL TRUE

Classification:
- CONTENT-AUTHORING / asset packaging

Current project state:
- Confirmed this pass: no file under `data/` matches `*.tga` or an icon-naming pattern for any
  building. The 33 icons remain only at `models/building images/purchase_images_2/tga/` (and
  `purchase_images_1/`), never on `WW3DAssetManager`'s `DATA_SUBDIRECTORY` search path. No
  descriptor sets `build.icon_texture` (the field exists,
  `BuildingBuildDefinition::IconTexture`, `buildingdescriptor.h:129`, but every one of the 8
  files leaves it unset).

Player-visible problem:
- Every existing building card renders as a name-plus-cost frame with no picture.

Final intended behavior:
- A runtime-sized packaged copy of each faction's icon set, on the real asset search path,
  referenced by `build.icon_texture` per descriptor; the source TGAs stay as authoring masters.

Implementation:
- Measure the sidebar's actual card draw resolution in `commandersidebarrenderer.cpp` before
  choosing a target size (not measured this pass -- named as the concrete next step, not a
  guessed number).
- Convert/downsize the 33 source TGAs (currently 3-6MB uncompressed each) to that size.
- Place runtime copies under a loose-file `DATA_SUBDIRECTORY` location, following the exact
  precedent this session's `data/building_models/` already established for building meshes, or
  a mix file if that becomes the project's packaging convention.
- Set `icon_texture` per descriptor.

Compatibility constraints:
- None -- new asset only.

Network/save-load implications:
- None.

Acceptance:
- Manual: sidebar card renders the icon in Commander Mode. No new automated check needed
  (visual-only, no device-less arithmetic to prove).

Tracker action:
- KEEP, rewritten to cite the exact confirmed-empty search path and the one missing measurement
  step.

Dependencies/blockers:
- None blocking.

---

### Faction rosters are not defined

Status:
- OBSOLETE as a separate entry

Classification:
- CONTENT-AUTHORING (same content as the building-prices entry)

Current project state:
- All 8 existing descriptors already set `build.team` explicitly and correctly -- GDI ones at 1
  (`PLAYERTYPE_GDI`), Nod ones at 0 (`PLAYERTYPE_NOD`), confirmed by reading every file this
  pass. None defaults to "any faction," contradicting the gap's literal claim. The only real gap
  is the same missing-8-categories problem the building-prices entry already covers -- there is
  no separate mechanism gap here.

Tracker action:
- DELETE as a standalone entry; fold into the rewritten "Building prices, build times and power
  figures" entry above.

---

## Base building

### Nothing physically appears when a building is placed

Status:
- OBSOLETE

Classification:
- was CODE, now resolved

Current project state:
- Q-016 (resolved, `awaiting_answers.md`), `completed_features.md` P38/addendum-2/addendum-3.
  `BuildingGameObj::Init_Runtime_Building` (`building.cpp:2560`) creates a real canonical
  `BuildingGameObj`. `Build_Runtime_Components`/`Build_Runtime_Assembly` attach real
  W3D-derived geometry -- a single mesh, or for the two Power Plants a real multi-part
  component assembly (`docs/zerohour/BuildingComponentAssembly.md`).
  `TeamPowerGridService::RegisterBuilding` (`building.cpp:1037`) registers power
  unconditionally for every building, runtime included. `BuildingConstructionService` drives a
  real timed, replicated construction visual before `Init_Runtime_Building` runs. This session
  also gave component-assembly buildings a real, working door (solid closed, animated
  open/close, non-solid open -- `Code/Combat/runtimedoorservice.h/.cpp`,
  `docs/zerohour/BuildingComponentAssembly.md`).

Player-visible problem:
- None remaining under this heading -- a Commander-built building genuinely appears, stands,
  collides and is destroyable.

Tracker action:
- DELETE outright. The two genuinely-remaining, narrower player-visible pieces are split into
  their own entries: the new "Runtime buildings have no working MCT/PT" below, and the
  already-existing "Buildings cannot be sold or repaired" entry (power on/off exists
  authoritatively but has no button, same as before this pass).

---

### Runtime-constructed buildings have no working MCT or Purchase Terminal *(new entry, split from the deleted one above)*

Status:
- STILL TRUE

Classification:
- CODE

Current project state:
- `mct_anchor`/`purchase_terminal_anchor` role names are authored in 4 of the 8 descriptors
  (both factions' Communications Center and Advanced Communications Center -- confirmed this
  pass) but nothing in the codebase consumes them. `BuildingAggregateClass::Is_MCT()`
  (`buildingaggregate.h`) is what the repair gun's MCT check actually gates on
  (established earlier this session tracing `c4.cpp`); every runtime-constructed part, assembly
  or single-mesh, is a plain `StaticPhysClass`, never a real `BuildingAggregateClass`, so
  `Is_MCT()` can never fire for a Commander-built building. No `PurchaseTerminal`/`MCTGameObj`-
  equivalent interaction class exists anywhere in the tree, for any building, stock or runtime
  (confirmed by this pass's search) -- stock Renegade's own PT is a level-authored script zone,
  architecturally unrelated to a building's model.

Player-visible problem:
- A player cannot use the repair gun on a Commander-built building's MCT (this is
  architecturally impossible today, independent of the also-missing sidebar repair button --
  see the sell/repair entry below), and cannot open a purchase terminal inside one at all.

Final intended behavior:
- At minimum, `mct_anchor` should resolve to something the repair gun's existing MCT check can
  actually see. Purchase terminal interaction is a larger, currently entirely-unimplemented
  mechanism for every building in this codebase and needs its own design pass, not a quick fix
  bolted onto this one.

Implementation:
- Likely a lightweight runtime-only aggregate wrapper exposing `Is_MCT() == true` at the
  `mct_anchor` transform without needing the animation-frame data a real
  `BuildingAggregateClass` requires -- the same "real class needs authored animation data this
  project does not have, so build a project-owned static-geometry-plus-behavior wrapper instead
  of guessing the missing data" pattern this session's own `RuntimeDoorService` already
  established for doors. Named here as the shape of the fix, not implemented -- it needs its
  own scoping pass separate from this audit.
- PT: no implementation recommendation given here; flagged as needing a design decision on
  whether Commander-built buildings get an in-world PT interaction at all, or route purchases
  through the sidebar exclusively (which already exists and already works) -- these may turn
  out to be the same mechanism seen two ways, which is exactly the kind of question this pass
  should surface rather than guess an answer to.

Compatibility constraints:
- Map-authored buildings' existing MCT/PT behavior is completely untouched.

Network/save-load implications:
- A new interactive-proxy class would mirror `BuildingAggregateClass`'s own persistence/
  replication, which already exists and already works.

Acceptance:
- Device-less test of the new anchor-to-interactive-proxy resolution.
- Manual: fire the repair gun at a Commander-built Comm Center's MCT anchor location, confirm
  it registers as MCT.

Tracker action:
- NEW entry, split from the deleted "nothing appears when placed" entry above.

Dependencies/blockers:
- The PT half genuinely needs a design decision (in-world interaction vs. sidebar-only) before
  implementation -- not resolvable from repository evidence alone. Not written as a blocking
  `Q-###` because nothing is currently waiting on it; recorded here so it is not lost.

---

### Placement has no cursor

Status:
- OBSOLETE

Classification:
- was CODE, now resolved

Current project state:
- Confirmed this pass by reading `Code/Commando/commanderuiinput.cpp` in full.
  `Handle_Left_Click` (line 277) resolves a click against the command-palette panel and sidebar
  first, then -- with a ghost active (`CommanderPlacementValidator::Is_Ghost_Active()`) -- calls
  `Pick_World_Point`, `CommanderPlacementValidator::Move_Ghost`, and fires a real
  `cCommanderPlacementEvent` at the committed point (lines 347-363).
  `Handle_Right_Click` cancels the ghost (lines 433-436). `commanderplacement.cpp`'s
  `Begin_Ghost`/`Move_Ghost`/`Set_Ghost_Rotation`/`Get_Ghost_Result` already compute a live
  red/green verdict on every move. This is exactly the mouse-to-world step the gap said was
  missing.

Player-visible problem:
- None -- a Commander already places buildings by pointing, clicking to commit, right-clicking
  to cancel.

Tracker action:
- DELETE. The live-exercise need this has never actually been run under (`awaiting_answers.md`
  Q-012's sixth pass already says so explicitly) is manual-validation, not a gameplay gap, and
  already lives in that file per this tracker's own stated scope ("engineering gaps... belong in
  `WIP_features.md`, not here").

---

### No build prerequisites

Status:
- STILL TRUE

Classification:
- CODE (schema gap, not merely unauthored content)

Current project state:
- Read `buildingdescriptor.h` in full this pass. `BuildingModelDescriptor` has no prerequisite
  field or struct of any kind. `tech_level` (`BuildingBuildDefinition::TechLevel`) is the only
  gate, and it is a single match-wide number a script moves, exactly as the tracker already
  says.

Player-visible problem:
- Nothing stops a Commander with enough credits from building any structure in any order, once
  the missing categories above are authored.

Final intended behavior:
- A descriptor/rules-data-driven prerequisite list, separate from `tech_level`, checked at
  placement time. Destroying a prerequisite must not auto-destroy downstream buildings -- a pure
  gate on new construction. Power stays a separate operational dependency, owned by
  `TeamPowerGridService` exactly as today.

Implementation:
- Add a `BuildingPrerequisiteDefinition { DynamicVectorClass<StringClass> RequiredDescriptorNames; }`
  (or equivalent) to `buildingdescriptor.h`.
- Parse a `"prerequisites"` JSON array in `buildingdescriptormanager.cpp`'s
  `Parse_And_Validate`.
- Check "at least one active, completed building of each required descriptor exists for this
  team" inside `CommanderPlacementValidator::Validate` (`commanderplacement.cpp`), alongside its
  existing checks.

Compatibility constraints:
- An empty/absent prerequisites list must mean "buildable anytime, unchanged" -- the same
  degrade-safely convention every other optional descriptor block in this codebase already
  follows.

Network/save-load implications:
- None beyond the existing placement-validation replication (client preview, server
  re-validates).

Acceptance:
- Device-less test of the new prerequisite check against a fake team-building list.
- Manual: attempt to build a gated structure without its prerequisite present, confirm refusal.

Tracker action:
- KEEP, rewritten with the concrete schema/implementation shape above instead of only naming a
  decision that needs making.

Dependencies/blockers:
- None blocking. The initial prerequisite graph (task's own suggested starting shape, GDI/Nod
  Power Plant -> Refinery/infantry -> Silo/War Factory/Comms -> Repair Pad/advanced
  defenses/Air Pad/advanced tech) should be reconciled against whichever missing categories get
  authored first -- a sequencing note, not a blocker.

---

### Buildings cannot be sold or repaired from the sidebar

Status:
- STILL TRUE

Classification:
- CODE

Current project state:
- Confirmed this pass: no `"Sell"`/`"Repair"` string or method anywhere in
  `commandersidebar.h`/`.cpp`. No building-selection-then-manage UI exists --
  `CommanderSidebarHitType` only has mode-tab/page-arrow/card/panel hit kinds; there is no
  "clicked a building in the world" kind at all. Manual power on/off and priority override calls
  do exist authoritatively (`TeamPowerGridService`, matching the tracker's own accurate note)
  but have no button either.

Player-visible problem:
- No way to sell, pay-repair, or toggle power/priority from the sidebar, at all.

Final intended behavior:
- A building-management surface reached by clicking a friendly building in the world, exposing
  Repair, Sell, Power On/Off, Power Priority.

Implementation:
- Extend `Handle_Left_Click`'s world-click fallthrough to hit-test buildings (not just units),
  reusing the same `WorldSpatialIndex` query pattern `commanderproductionevent.cpp` already uses
  for exit-clearance, then open a small building-management overlay.
- Repair: recommend supporting the automatic paid-repair toggle first (needs no new AI-dispatch
  system this codebase does not have), leaving Engineer/Technician AI-dispatch-to-MCT as a later
  addition; never remove the existing repair-gun-at-MCT gameplay.
- Sell: recommend the task's own suggested first-pass formula,
  `refund = build_cost * 0.50 * current_health_fraction`, with a short (recommend 3 second)
  cancellable countdown, refused while `BuildingConstructionService::Is_Under_Construction` is
  true (that already refunds fully through `Cancel_Construction`) and refused for a short window
  after recent damage (recommend 5 seconds).

Compatibility constraints:
- None -- new sidebar surface only; existing manual power on/off/priority calls are reused, not
  replaced.

Network/save-load implications:
- New server-authoritative events (a sell event, a paid-repair toggle event) following the exact
  existing `cCommanderProductionEvent`/`cCommanderPlacementEvent` pattern -- sender's
  team/ownership re-validated server-side, never trusted from the packet.

Acceptance:
- Device-less test of the sell-refund formula and repair-toggle state machine.
- Manual: sell a building mid-health, confirm the exact refund; toggle paid repair, watch health
  rise.

Tracker action:
- KEEP, rewritten with the concrete recommended design above (the task explicitly asked for a
  playable first-pass value here, not another placeholder).

Dependencies/blockers:
- None blocking.

---

## Production and units

### No unit is ever actually produced

Status:
- OBSOLETE

Classification:
- was CODE, now resolved

Current project state:
- Confirmed this pass: `Drain_Completed_Commander_Production` (`Code/Combat/combat.cpp:1083`,
  called once a server frame at `combat.cpp:1246`) calls `physical_def->Create()`,
  `unit->Set_Transform(Matrix3D(spawn_pos))`, `Add_Observer(new CommanderPopulationObserverClass(...))`
  and `Start_Observers()` for every completed queue item. A real `SoldierGameObj`/`VehicleGameObj`
  is created via `PhysicalGameObjDef::Create()`, not nothing.

Player-visible problem:
- None remaining -- a completed queue item genuinely produces a real unit standing in the world.

Tracker action:
- DELETE.

---

### Produced units would have nowhere to walk out of

Status:
- PARTIALLY TRUE / REFRAMED

Classification:
- CODE (schema gap) + CONTENT-AUTHORING

Current project state:
- The anchor mechanism itself is real and already working for one role. `BuildingGameObj::
  Get_Commander_Exit_Point` (`building.cpp:2024`) tries a named `exit_anchor` bone first (via
  `Resolve_Runtime_Anchor` -> `BuildingModelDescriptor::Resolve_Anchor_World_Transform`, which
  already tries bone-name resolution against every loaded part) and falls back to a derived
  direction (`RuntimeTransform.Get_X_Vector() * standoff`) when no bone/anchor is authored --
  this is the exact point `Drain_Completed_Commander_Production` spawns units at, and it already
  degrades safely with zero art. Confirmed genuinely missing: (a) the specific role names both
  the tracker and this task ask about -- `infantry_spawn`/`infantry_staging`/`vehicle_exit`/
  `vehicle_staging`/`aircraft_spawn`/`aircraft_staging` -- do not exist anywhere in code or
  content (a full-tree search this pass found only `exit_anchor`, `mct_anchor`,
  `purchase_terminal_anchor` in actual use), so production currently always exits through one
  generic point regardless of unit kind; (b) `BuildingAnchorEntry` (`buildingdescriptor.h:213`)
  has no `fallback_local_position`/`fallback_local_yaw` fields at all -- the only fallback that
  exists is `Get_Commander_Exit_Point`'s own hard-coded C++ direction, specific to that one call
  site, not a generic descriptor-declared fallback other anchors (like `mct_anchor`) could reuse.

Player-visible problem:
- Units already walk out of a real point today (this half of the gap is stale). But every unit
  kind shares the same exit regardless of what would make sense per kind, and no staging/rally
  point exists at all.

Final intended behavior:
- Per the task's own schema sketch: add `fallback_local_position`/`fallback_local_yaw` to
  `BuildingAnchorEntry`, add the six named roles, and make spawn-point resolution pick the role
  by `BuildingProductionKindType` instead of always using `exit_anchor`.

Implementation:
- Extend `BuildingAnchorEntry` with the two optional fallback fields, parsed in
  `buildingdescriptormanager.cpp`, defaulting to each call site's current hard-coded fallback
  when absent so nothing regresses.
- Add a kind-to-role-name lookup in `building.cpp` (e.g. a small
  `Get_Commander_Spawn_Point(BuildingProductionKindType kind)` that resolves `infantry_spawn`/
  `vehicle_exit`/`aircraft_spawn` by kind, falling back to `exit_anchor` when the kind-specific
  role is absent -- preserving every current descriptor's behavior unchanged).
- Author the six roles into descriptors as content follows.

Compatibility constraints:
- `exit_anchor` keeps working exactly as today for any descriptor that does not adopt the new
  kind-specific roles -- pure extension.

Network/save-load implications:
- None new.

Acceptance:
- Device-less test of the fallback-vs-bone resolution order (extends the existing anchor tests
  rather than duplicating them).
- Manual: produce one of each unit kind from a descriptor with the new roles authored, confirm
  each exits its own point.

Tracker action:
- REWRITE with the precise remaining scope above. Delete the "no building model has these bones
  named" framing -- the fallback path already covers that case for `exit_anchor`, and the real
  gap is the missing kind-specific roles and the missing generic fallback-transform schema
  field.

Dependencies/blockers:
- None blocking.

---

### Commander orders are recorded but not obeyed

Status:
- OBSOLETE for every order type except PATROL / PARTIALLY TRUE

Classification:
- was CODE, now resolved except one order type

Current project state:
- Read `Dispatch_Order` (`Code/Commando/commanderorderevent.cpp:47-150`) in full this pass.
  `ORDER_MOVE`/`ATTACK_MOVE`/`ATTACK_TARGET`/`FOLLOW`/`GUARD`/`REPAIR_TARGET`/
  `RETREAT_TO_REPAIR`/`HOLD_POSITION` all issue real `ScriptEngine::Action_Goto`/`Action_Attack`/
  `Action_Follow_Input`/`Action_Reset` calls at priority 200 (above every scripted command in
  this tree), and `CommanderGuardAI` (`commanderguardai.cpp`) is a real per-frame state
  controller for `ORDER_GUARD`. `ORDER_PATROL` is explicitly documented in the source itself
  (lines 118-125) as walking to the single point the wire format carries --
  `cCommanderOrderEvent::Init` (`commanderorderevent.h:45`) takes exactly one `target_point`,
  not a path.

Player-visible problem:
- Every order except PATROL is obeyed correctly today. PATROL currently behaves identically to
  MOVE -- walks to one point and stops, no back-and-forth.

Final intended behavior:
- Real two-point back-and-forth patrol.

Implementation:
- Extend `cCommanderOrderEvent::Init`/`Export_Creation`/`Import_Creation` with a second optional
  `Vector3` (`SecondPoint`).
- Add a small per-unit patrol loop state (shaped like `CommanderGuardAI`'s own per-unit state),
  ticked in the same place `CommanderGuardAI` already is (`combat.cpp`'s Commander/RTS update
  block), alternating `ScriptEngine::Action_Goto` calls between the two points on arrival.
- Cancel/replace cleanly on any new order, mirroring `CommanderGuardAI::Stop_Guarding`'s existing
  precedent.
- Multi-waypoint patrol explicitly deferred -- the task allows this.

Compatibility constraints:
- None.

Network/save-load implications:
- Wire format grows by one `Vector3` on this project-owned event -- no compatibility burden per
  `PROJECT_RULES.md` section 4 (no network compatibility owed to any other client). Patrol loop
  state is server-authoritative like GUARD; no new client authority.

Acceptance:
- Device-less test of the two-point alternation state machine, mirroring the existing Guard AI
  test pattern.
- Manual: order a unit to patrol between two clicked points, watch it loop.

Tracker action:
- REWRITE -- narrow to PATROL only. Delete the "orders are recorded but not obeyed" framing for
  the other seven order types, which is obsolete.

Dependencies/blockers:
- None blocking.

---

### No unit selection by pointing

Status:
- PARTIALLY TRUE / substantially OBSOLETE

Classification:
- was CODE, now mostly resolved

Current project state:
- Read `commanderselection.h`/`.cpp` and `Code/Commando/commanderuiinput.cpp` in full this pass.
  Real, working: left-click select, drag-box select with a real screen-space rectangle and
  significance threshold (`CommanderSelection::Is_Drag_Significant`), `Select_In_Box` projecting
  every `SmartGameObj` on the clicking player's team through the camera and testing screen
  bounds (`commanderuiinput.cpp` ~180-224), a real selection-ring renderer
  (`commanderselectionrenderer.cpp`, alongside every other real Commander renderer this pass
  found -- `commanderghostrenderer.cpp`, `commanderwaypointrenderer.cpp`), right-click issuing a
  MOVE order to the selection (`Handle_Right_Click`, lines 443-465), and a 9-button command
  palette (`commandercommandpalette.cpp`/`commandercommandpaletterenderer.cpp`) for the other
  order types. This is nearly everything the gap and the task's own suggested control scheme
  ask for. Confirmed genuinely still missing: (a) Shift+click additive/toggle selection --
  explicitly, deliberately not implemented (`commanderuiinput.cpp:411-414`'s own comment: "A
  fresh box replaces the selection rather than adding to it... the input gate this file installs
  suppresses every modifier that is not on its allow-list"); (b) control groups (Ctrl+1-9
  assign, 1-9 recall) -- zero matches anywhere in the tree for any group-assignment mechanism;
  (c) building selection through the same pointer (only units are hit-tested in the world; a
  building click currently falls through to "begin a selection box" with nothing under it --
  same gap the sell/repair entry above covers from the other side).

Player-visible problem:
- Click-select, drag-box and right-click-move already all work. A player cannot shift-click to
  build a mixed selection incrementally, cannot save/recall control groups, and cannot click a
  building to select it.

Final intended behavior:
- Per the task's own suggested control scheme: add Shift+click additive/toggle and Ctrl+1..9/
  1..9 control groups. Building selection folds into the sell/repair entry's work.

Implementation:
- Shift+click: `CommanderSelection` already has `Contains`/`Add` and just needs a
  toggle-remove; `commanderuiinput.cpp`'s input-gate allow-list needs Shift added, and
  `Handle_Left_Click`/`Handle_Left_Release` taught to skip the `Clear()` call when Shift is held.
- Control groups: a small new `CommanderSelection::Assign_Group(int slot)`/
  `Recall_Group(int slot)` pair backed by up to 9 saved id lists -- client-local only (a pure UI
  convenience over an already-server-validated selection, no new server authority needed).

Compatibility constraints:
- None.

Network/save-load implications:
- None new -- groups are client-local memory of ids; orders still go through the existing
  validated `cCommanderOrderEvent` path.

Acceptance:
- Device-less test of `CommanderSelection`'s toggle/group behavior, extending its existing test
  pattern.
- Manual: shift-click to add/remove a unit; assign/recall a control group.

Tracker action:
- REWRITE, narrowed to the three specific missing pieces above. Delete the "no click-select, no
  drag box... no on-screen indication" framing, which is obsolete.

Dependencies/blockers:
- None blocking.

---

### Population caps are placeholder numbers

Status:
- PARTIALLY TRUE / REFRAMED

Classification:
- DESIGN-BALANCE + CODE (missing per-unit weight field)

Current project state:
- `CommanderProductionService::Get_Population_Cap`/`Set_Population_Cap`
  (`commanderproduction.h:188-189`) already cap per `BuildingProductionKindType` -- a real,
  data-adjustable mechanism. But every unit increments its kind's population by a flat `+1`
  (`combat.cpp:1130-1131`), so a Mammoth Tank and a light vehicle cost the same population within
  the ground-vehicle kind -- there is no per-unit weight field anywhere in this codebase.

Player-visible problem:
- Caps exist and are enforced by kind, but population cost does not track a unit's actual
  combat weight within its kind, so the numbers do not mean what a player would expect.

Final intended behavior:
- Per the task's recommended starting weights (Infantry 1, Light vehicle 2, APC/artillery 2, MBT
  3, Heavy/Mammoth 5, Transport/Attack aircraft 3): a per-definition population cost, summed
  instead of counted, checked against the existing per-kind cap.

Implementation:
- A small definition-id-to-weight table (JSON, alongside the existing building descriptors'
  own convention) consulted by `Drain_Completed_Commander_Production`/
  `CommanderProductionService::Set_Population` instead of the current flat `+1`.

Compatibility constraints:
- None.

Network/save-load implications:
- None new -- population totals already replicate the same way.

Acceptance:
- Device-less test of the weighted-sum accounting.
- Manual: produce a mix of light and heavy units, confirm the cap is reached by weight, not
  count.

Tracker action:
- KEEP, rewritten with the concrete recommended weight table and the confirmed flat-`+1` code
  location.

Dependencies/blockers:
- None blocking. The `40/20/10/60` starting ceiling may remain until live play says otherwise,
  per the task's own instruction.

---

## Economy

### The wallet has no ceiling, so Tiberium Silos do nothing

Status:
- OBSOLETE

Classification:
- was DESIGN-BALANCE + CODE, now resolved

Current project state:
- Q-014 resolved (`awaiting_answers.md`), `completed_features.md` P34.
  `TeamPowerGridService::Get_Wallet_Capacity`/`Enforce_Wallet_Cap`, `PlayerDataClass::
  Apply_Wallet_Cap`; base 10,000 + 5,000 per active Silo; retunable via the `wallet_cap`
  console command.

Player-visible problem:
- None -- Silos now genuinely raise the credit ceiling.

Tracker action:
- DELETE.

---

### Refund policy is full-price

Status:
- PARTIALLY TRUE / REFRAMED, with a newly-found real bug

Classification:
- CODE

Current project state:
- The 100%-refund figure is real and correctly closes the wallet-cap exploit by construction,
  not merely by policy: `cCommanderProductionEvent::Act`
  (`Code/Commando/commanderproductionevent.cpp:220`) deducts `player->Increment_Money(-cost)`
  the instant a purchase is accepted into the queue, so a queued item is already-spent money
  sitting in `CommanderProductionService`'s own `_Queue` list, not money still counted against
  the player's wallet cap -- the "hidden credit storage" exploit the task worried about cannot
  occur, because queued credits are not liquid credits by construction.
  `CommanderProductionService::Cancel_Item`/`Cancel_Facility_Queue`
  (`commanderproduction.cpp:528-551`) correctly compute a 100% refund amount from the queue.
  **What this pass found genuinely broken:** `Cancel_Item` has zero callers anywhere in the tree
  -- there is no way, console command or UI, for a player to voluntarily cancel a single queued
  item today. `On_Facility_Destroyed` (`commanderproduction.cpp:553`, meant to auto-cancel and
  refund a facility's queue when the building dies) is called from nowhere except a self-check
  test (`Code/Commando/shaderselfcheck.cpp:3593`) -- a real production facility's actual
  destruction path never invokes it, so a destroyed Barracks/War Factory's queued credits are
  simply lost today, contradicting the tracker's own claim that queues "refund on destruction."

Player-visible problem:
- Cancelling a single queued item is impossible from any current player-facing surface. Losing a
  production facility mid-queue currently forfeits every credit in that queue instead of
  refunding it.

Final intended behavior:
- Wire both existing, already-correct functions into real call sites.

Implementation:
- Find the exact spot `BuildingGameObj` transitions to destroyed (health reaching zero,
  `building.cpp`) and add the `On_Facility_Destroyed` call there, crediting every affected
  player back via `player->Increment_Money(+refund)` -- owning player(s) looked up the same way
  `commanderproductionevent.cpp` already does.
- Add a `Cancel_Item` caller -- the natural home is the same building-management surface the
  sell/repair entry above is adding, plus a queue-list UI reachable there.

Compatibility constraints:
- None.

Network/save-load implications:
- The missing call sites are server-only logic; no new replication shape (`Increment_Money`
  already synchronizes normally).

Acceptance:
- `shaderselfcheck.cpp:3593` already covers `On_Facility_Destroyed`'s own arithmetic -- extend
  to prove the credit is actually applied once wired to a real destruction path.
- Manual: destroy a facility with a queue, watch the credits return; queue two items, cancel
  one.

Tracker action:
- REWRITE -- this is the single most concrete, previously-undocumented finding of this pass: the
  tracker's own claim that queues "refund on destruction" is currently false in a live game, even
  though the underlying arithmetic function exists and is correct.

Dependencies/blockers:
- None blocking -- both fixes are wiring, not design.

---

## Power

### Power only matters for buildings that have a JSON descriptor

Status:
- PARTIALLY TRUE / REFRAMED

Classification:
- CODE

Current project state:
- `BuildingDescriptorManager::Get_Descriptor(definition_name)`
  (`buildingdescriptormanager.cpp:587`) is a direct-name lookup only; no alias/preset-name-
  matching tier exists anywhere in the file, confirmed this pass. `docs/zerohour/
  BuildingDescriptors.md` (lines 116-121, read this pass) confirms the same thing from the docs
  side: `BuildingGameObjDef::DescriptorName` is the field registration keys on, and "no preset in
  this tree's own content sets it yet." The resolution order the task asks for (explicit
  `DescriptorName` -> preset-name/alias lookup -> legacy fallback) only has its first tier
  built.

Player-visible problem:
- Exactly as the tracker already says: until every relevant preset gets its `DescriptorName`
  hand-set, the grid does not own it and it falls back to stock's coarse any-plant-alive rule.

Final intended behavior:
- An alias table (project-owned, using real preset names from the same `Objects.DDB`
  enumeration Q-017 already built -- not guessed) letting a preset resolve a descriptor by
  name-matching convention without a hand-edited field on every map's preset.

Implementation:
- Extend `BuildingDescriptorManager::Get_Descriptor` with a fallback: if `DescriptorName` is
  empty, try the preset's own definition name (or a declared alias list per descriptor, e.g. a
  `"preset_aliases": [...]` array) before giving up to the legacy stock rule.

Compatibility constraints:
- This is exactly what keeps every un-annotated stock map building working exactly as today
  (the required legacy-fallback tier stays last) while adding automatic coverage for presets
  that match a known alias.

Network/save-load implications:
- None new.

Acceptance:
- Device-less test of the new alias-resolution order.
- Manual: a stock map building whose preset matches an alias registers with the grid without its
  `DescriptorName` being hand-set.

Tracker action:
- KEEP, rewritten with the precise missing tier and the concrete fix shape.

Dependencies/blockers:
- Needs the same `Objects.DDB` preset enumeration Q-017 already built -- not a new blocker.

---

### No audio or on-screen warning when power is lost

Status:
- STILL TRUE

Classification:
- CODE + CONTENT-AUTHORING (only if a specific EVA line asset is wanted)

Current project state:
- Confirmed this pass: the only power-loss feedback anywhere is
  `commandersidebarrenderer.cpp:712`'s `"LOW POWER -- %d OFFLINE"` text, drawn only inside the
  Commander sidebar. No match anywhere in `Code/Combat` or `Code/Commando` for a HUD
  announcement, EVA line, or sound cue tied to a power-state transition, and nothing reaches an
  ordinary FPS player outside Commander Mode at all.

Player-visible problem:
- Exactly as the tracker already says.

Final intended behavior:
- Transition-detected (not spammed) HUD announcement plus sound cue for every player on the
  team, Commander or not, on base power low / critical systems offline / base power restored.

Implementation:
- `TeamPowerGridService` (the existing owner of power state) gains edge-triggered "shed count
  crossed zero" / "shed count fell back to zero" detection, firing a HUD message (reusing
  `hud.cpp`'s existing message path -- needs one quick check of how that path is already fed
  before choosing new-event-vs-reuse, named as the concrete next investigation step) plus a
  sound cue (reuse an existing generic UI/alert sound if one exists; otherwise document a
  required `ow_` asset per `PROJECT_RULES.md` section 16) to every player on the affected team.
  Recommend a several-second minimum gap between repeated announcements of the same transition,
  mirroring the small-constant debounce style this session's own `RuntimeDoorService` already
  uses (`HOLD_OPEN_SECONDS`).

Compatibility constraints:
- None.

Network/save-load implications:
- A small server->client notification, or reuse of whatever HUD-message replication already
  exists.

Acceptance:
- Device-less test of the edge-detection/debounce arithmetic.
- Manual: shed enough load to trip the warning, confirm every team player sees/hears it once,
  not every frame.

Tracker action:
- KEEP as-is; already accurately described.

Dependencies/blockers:
- None blocking.

---

## Commander role

### Anyone can become Commander

Status:
- PARTIALLY TRUE / substantially REFRAMED (one confirmed real bug found)

Classification:
- was mostly CODE, now mostly resolved with specific remaining gaps

Current project state:
- Confirmed this pass: the real player-facing entry point (the `B` key,
  `INPUT_FUNCTION_COMMANDER_MODE_TOGGLE`, `input.cpp:1804`, consumed by
  `combatgmode.cpp:1324` -> `CommanderUIInput::Toggle_Commander_Mode`) already fires a real
  `cCommanderClaimEvent` (`commanderuiinput.cpp:483-507`) before opening the camera, and
  `cCommanderClaimEvent::Act` (`Code/Commando/commanderclaimevent.cpp:90-109`) is genuinely
  first-claimant-wins, one-per-team, server-authoritative, with a real release path. This
  directly contradicts the tracker's literal claim that "there is no election, no claim, no
  one-per-team enforcement." What is still genuinely true, all confirmed this pass:
  - The `commander_mode` **console command** (`Code/Commando/consolefunction.cpp:1958-1977`)
    calls `CommanderModeController::Enter_Commander_Mode` directly with no claim check at all --
    a real, confirmed bypass, though console commands are a developer/admin surface per
    `PROJECT_RULES.md` section 20's own framing, not the ordinary player's entry point.
  - No disconnect recovery exists anywhere -- zero matches for `Set_Team_Commander` being called
    from any disconnect/player-leave path. A Commander who disconnects leaves
    `Get_Team_Commander` permanently non-zero and non-matching, and
    `cCommanderClaimEvent::Act`'s own logic (`if ((current == 0) || (current == SenderId))`)
    then refuses every future claim for that team for the rest of the match. **This is a live,
    previously-undocumented bug**, not a missing feature.
  - No vote flow when multiple candidates exist simultaneously -- pure first-claimant-wins
    today, no candidacy window.
  - No resignation/transfer command -- `Toggle_Commander_Mode`'s own comment: "Leaving the mode
    is not resigning... the job is kept."
  - No skirmish/AI Commander assignment for an unclaimed side found anywhere in this pass's
    search.

Player-visible problem:
- The common case (one player presses B) already works correctly and safely. A disconnected
  Commander currently locks their team's role for the rest of the match, and there is no way to
  hand the role over voluntarily either.

Final intended behavior:
- Per the task's own recommended flow. The disconnect-recovery gap should be fixed first -- it
  is a live bug, not a missing feature.

Implementation:
- Disconnect: hook this project's existing player-disconnect path (needs one targeted search,
  not a guess) to call `CommanderProductionService::Set_Team_Commander(team,0)` for any team the
  leaving player held.
- Resignation: a small UI affordance sending `cCommanderClaimEvent::Init(false)`, which already
  exists and already works -- it just needs a deliberate trigger rather than only firing as a
  side effect of the claim protocol.
- Console-command gating: make `commander_mode 1` at the console go through the same claim path
  `Toggle_Commander_Mode` already uses, rather than calling `Enter_Commander_Mode` directly,
  closing the one confirmed real bypass.

Compatibility constraints:
- None.

Network/save-load implications:
- Disconnect handling reuses whatever this project's existing player-leave replication already
  is. Resignation reuses `cCommanderClaimEvent` unchanged.

Acceptance:
- Claim/disconnect are inherently server+network scenarios with limited device-less coverage;
  recommend a narrow device-less test of "current commander id no longer maps to a connected
  player" detection logic once written, plus a manual disconnect-and-rejoin check.

Tracker action:
- REWRITE -- replace the fully-broken framing with the precise, narrower, partially-resolved
  reality: claim/one-per-team/B-key entry already work; disconnect recovery is the one real live
  bug; console-command bypass, vote flow, resignation and skirmish AI remain real gaps.

Dependencies/blockers:
- None blocking. Disconnect handling needs locating this project's existing player-leave hook --
  a search task, not a design question.

---

### Commander Mode has no dedicated input

Status:
- OBSOLETE (pending one file check)

Classification:
- was CODE, now resolved

Current project state:
- Confirmed this pass: `INPUT_FUNCTION_COMMANDER_MODE_TOGGLE` is a real entry in the input
  function table (`input.h:249`; `input.cpp:436` names it `"CommanderModeToggle"`;
  `input.cpp:624` marks it `BUTTON_HIT` edge-triggered; `input.cpp:1804` binds it to `DIK_B` by
  default), consumed every frame in `combatgmode.cpp:1324`. Its own comment documents that it is
  on `Is_Allowed_During_Commander_UI`'s allow-list (works even while the panel is open) and read
  before the mouselook/pointer gate is applied for the frame (closing the panel is not itself
  suppressed by the panel it is closing). This is a real bindable input function on the
  project's default key exactly as the task's own "B, if still appropriate" asks for -- not a
  hard-coded keyboard read.

Player-visible problem:
- B already opens/closes Commander Mode today. Whether it also appears in the normal
  controls/config remapping screen was **not confirmed this pass** -- the function is registered
  in the same table every other rebindable function uses, so it should already be visible there,
  but the controls-menu source itself was not opened this pass to check.

Final intended behavior / remaining:
- Confirm (or, if missing, add) the controls-screen entry so a player can actually rebind it --
  a console-command-only rebind path would still fail the task's own "expose it in the normal
  controls/configuration system" requirement.

Tracker action:
- REWRITE to a single narrow manual-validation note ("confirm B appears in the rebind screen"),
  the smallest honest remaining doubt, rather than either deleting outright or guessing the
  controls-menu already lists it.

Dependencies/blockers:
- None blocking -- one file to open, not a design question.

---

## C. Implementation backlog

Ordered by dependency and shortest path to a more complete match, not by original phase number.

### 1. Playability blockers

1. **Commander disconnect leaves a team permanently Commander-less.** Live bug, one missing
   call site (`Set_Team_Commander(team,0)` on player-leave). See "Anyone can become Commander."
2. **Production-facility destruction loses queued credits instead of refunding them.**
   `On_Facility_Destroyed` exists, correct, uncalled. See "Refund policy is full-price."
3. **8 of 12 building categories cannot be built at all** (no descriptor exists). Blocks
   prerequisites, faction completeness, and most of the RTS base-building loop. See "Building
   prices" entry and Decision Log's balance table below.

### 2. Required gameplay systems

4. Build prerequisites (schema + placement check).
5. Building sell/repair/power-toggle sidebar surface (also the building-click hit-test that
   selection's building-click gap needs).
6. Two-point PATROL.
7. Shift+click additive selection and control groups.
8. Single-item production cancel wired to a real UI/console call site.
9. Console-command `commander_mode 1` claim-gating; resignation action.
10. Runtime-building MCT interactive proxy (repair-gun functionality); PT design decision.
11. Kind-specific production spawn/staging anchors + generic anchor fallback-transform schema
    field.
12. Power-loss/restoration team-wide announcement.
13. Descriptor alias-resolution tier for legacy stock preset power coverage.

### 3. Content/authoring

14. Author the 8 missing building descriptors (Decision Log balance table).
15. Air Pad descriptor + air roster table + real flying presets (needs one `Objects.DDB` pass).
16. Package building icons (needs one card-size measurement, then convert/place 33 icons).
17. Author `mct_anchor`/`purchase_terminal_anchor`/spawn-role bones or fallback transforms for
    the 8 missing categories as they're authored.

### 4. Balance

18. Per-unit population weight table (Decision Log recommended weights).
19. Sell refund formula constants (recommended, Decision Log).
20. Prerequisite graph for the full 12-category roster (Decision Log recommended starting
    shape).

### 5. Manual validation

21. Everything `awaiting_answers.md`'s Q-006/Q-012 already track remains outstanding and
    unchanged by this pass -- not repeated here.
22. Confirm `INPUT_FUNCTION_COMMANDER_MODE_TOGGLE` appears in the controls/rebind screen.
23. Live disconnect-and-rejoin check once the disconnect fix above lands.

### 6. Polish

24. This session's vertical-retract door (`RuntimeDoorService`) is a project-owned motion, not a
    baked animation match to the classic sliding door -- functionally correct (solid closed,
    real open/close, walkable open) and a net improvement over the prior inert geometry, but
    worth revisiting if a real door animation asset is ever confirmed to exist.
25. Icon source TGAs (3-6MB each) should stay authoring masters only, never shipped as-is.

---

## D. Decision log

Every design choice this pass made or reaffirmed, per the task's required format.

### Wallet cap

- Decision: base 10,000 credits, +5,000 per active Silo, non-Commander Renegade uncapped.
- Reason: already resolved (Q-014) before this pass; reaffirmed, not reopened, because current
  code proves it works as designed.
- Affected files/systems: `TeamPowerGridService`, `PlayerDataClass`.
- Compatibility: none -- Commander/RTS-scoped only.

### Silo bonus

- Decision: +5,000 per active Silo, folded into the wallet cap decision above.
- Reason: same as above.
- Affected files/systems: same as above.
- Compatibility: none.

### Runtime building architecture

- Decision: Q-016's Option A (real canonical `BuildingGameObj`, W3D-derived structure) plus this
  session's component-assembly extension (`Assembly` block) and door mechanism
  (`RuntimeDoorService`).
- Reason: already resolved and implemented before this pass; reaffirmed as working by this
  pass's own source reading (`Init_Runtime_Building`, `Build_Runtime_Assembly`,
  `TeamPowerGridService::RegisterBuilding` unconditional call).
- Affected files/systems: `building.h/.cpp`, `buildingdescriptor.h/.cpp`,
  `buildingdescriptormanager.cpp`, `runtimedoorservice.h/.cpp`.
- Compatibility: map-authored proximity/prefix collection path untouched.

### Damage/destroyed model handling

- Decision: component-assembly buildings have exactly two visual states -- assembled and
  destroyed -- no per-part damage swap, since no damaged-state art exists for any part
  (documented already in `docs/zerohour/BuildingComponentAssembly.md`).
- Reason: inventing a damaged-state swap with no art would repeat the same "don't guess content"
  mistake already ruled out project-wide.
- Affected files/systems: `BuildingGameObj::Update_Runtime_Damage_Model`.
- Compatibility: single-mesh runtime buildings keep their existing health-band table unchanged.

### Prerequisites

- Decision (recommended, not yet implemented): descriptor-data-driven prerequisite list, checked
  at placement only (not an operational dependency), starting shape:
  `Power Plant -> Refinery/early infantry; Refinery -> Silo/Vehicle Factory/Communications;
  Vehicle Factory -> Repair Pad; Communications -> advanced defenses/Air Pad/advanced tech`
  (the task's own suggested pattern, reconciled against this project's actual 8-descriptor
  roster once the missing categories are authored).
- Reason: matches the task's own suggested shape; no repository evidence contradicts it.
- Affected files/systems: `buildingdescriptor.h`, `buildingdescriptormanager.cpp`,
  `commanderplacement.cpp`.
- Compatibility: destroying a prerequisite must not cascade-destroy downstream buildings --
  gate applies to new construction only.

### Sell rule

- Decision (recommended, not yet implemented):
  `refund = build_cost * 0.50 * current_health_fraction`, 3 second cancellable countdown,
  refused during construction (already has its own 100% refund path) and for 5 seconds after
  recent damage.
- Reason: the task's own suggested first-pass rule; no existing project convention contradicts
  it, and the numbers mirror this codebase's existing small-constant style
  (`RuntimeDoorService::HOLD_OPEN_SECONDS`, etc.).
- Affected files/systems: new sell event, `CommanderProductionService` or `BuildingGameObj`.
- Compatibility: none.

### Repair model

- Decision (recommended, not yet implemented): support automatic paid-repair toggle first;
  Engineer/Technician AI-dispatch-to-MCT as a later addition; never remove the existing
  repair-gun-at-MCT FPS gameplay.
- Reason: paid-repair needs no new AI-dispatch system this codebase does not have yet, and gets
  a repair button in front of players sooner without touching working FPS gameplay.
- Affected files/systems: new repair-toggle event, `TeamPowerGridService`/`BuildingGameObj`
  health regeneration.
- Compatibility: repair-gun-at-MCT untouched; blocked entirely for runtime buildings today
  anyway (see the MCT/PT entry) independent of this decision.

### Air production model

- Decision (recommended, not yet implemented): Commander sidebar owns its own data-driven air
  catalog rather than a nonexistent stock `PurchaseSettingsDefClass::TYPE_AIRCRAFT`; a dedicated
  Air Pad facility; real presets found via the existing `Objects.DDB` enumeration method, not
  guessed.
- Reason: matches the task's own preferred architecture; consistent with Q-017's precedent for
  never guessing preset names.
- Affected files/systems: new Air Pad descriptor, new air-roster table,
  `commandersidebar.cpp`.
- Compatibility: any existing stock flying-map aircraft mechanism (not found in this pass's
  search) would be untouched.

### Population accounting

- Decision (recommended, not yet implemented): per-unit population weight
  (Infantry 1, Light vehicle 2, APC/artillery 2, MBT 3, Heavy/Mammoth 5,
  Transport/Attack aircraft 3), summed per kind against the existing per-kind cap; the current
  `40/20/10/60` ceiling stays until live play says otherwise.
- Reason: the task's own recommended weights; no repository evidence to prefer different
  numbers before a real match has been played.
- Affected files/systems: `Drain_Completed_Commander_Production`,
  `CommanderProductionService::Set_Population`.
- Compatibility: none.

### Commander election/transfer

- Decision (recommended, not yet implemented): fix disconnect-clears-ownership first (a live
  bug); add an explicit resignation action reusing the existing, already-correct
  `cCommanderClaimEvent::Init(false)`; gate the `commander_mode` console command behind the same
  claim path the real B-key entry point already uses. Candidacy-window/vote-when-contested and
  skirmish AI Commander assignment are larger, genuinely new pieces, named but not designed in
  this pass.
- Reason: the claim/one-per-team mechanism this pass found already matches the task's required
  behavior almost completely; the smallest, highest-value fix is closing the one live bug and
  the one confirmed bypass, not rebuilding a working system.
- Affected files/systems: `CommanderProductionService`, `cCommanderClaimEvent`,
  `consolefunction.cpp`, whatever this project's player-disconnect hook already is.
- Compatibility: none.

### Patrol protocol

- Decision (recommended, not yet implemented): two-point wire format
  (`cCommanderOrderEvent` gains a second `Vector3`), server-authoritative alternation loop
  shaped like `CommanderGuardAI`'s existing per-unit state, multi-waypoint explicitly deferred.
- Reason: the task's own instruction ("at least two points... future extensibility... do not
  call a single Goto a completed PATROL"); matches this codebase's existing GUARD precedent
  exactly.
- Affected files/systems: `commanderorderevent.h/.cpp`, a new small patrol-state owner.
- Compatibility: no network-compatibility burden (project-owned wire format, per
  `PROJECT_RULES.md` section 4).

### Spawn-anchor fallback behavior

- Decision (recommended, not yet implemented): add `fallback_local_position`/
  `fallback_local_yaw` to `BuildingAnchorEntry`; six named roles
  (`infantry_spawn`/`infantry_staging`/`vehicle_exit`/`vehicle_staging`/`aircraft_spawn`/
  `aircraft_staging`) resolved bone-first, then descriptor-fallback-second, mirroring
  `Get_Commander_Exit_Point`'s existing (currently hard-coded, not descriptor-driven) two-tier
  pattern for `exit_anchor`.
- Reason: matches the task's own schema sketch exactly; extends a pattern this codebase already
  proved out for one anchor rather than inventing a new one.
- Affected files/systems: `buildingdescriptor.h`, `buildingdescriptormanager.cpp`,
  `building.cpp`.
- Compatibility: `exit_anchor`'s current behavior is preserved unchanged for every descriptor
  that does not adopt the new roles.

### Descriptor alias/startup-loading behavior

- Decision (recommended, not yet implemented): add a preset-name/alias lookup tier between
  `BuildingGameObjDef::DescriptorName` (first) and the legacy stock fallback (last) in
  `BuildingDescriptorManager::Get_Descriptor`.
- Reason: the task's own preferred resolution order; `data/building_descriptors/*.json` is
  already loaded automatically at real startup (confirmed by a prior session, recorded in
  `WIP_features.md`'s reconciled P33 entry) -- only the middle alias tier is missing, not the
  loading behavior itself.
- Affected files/systems: `buildingdescriptormanager.cpp`.
- Compatibility: every currently-un-annotated stock map building keeps working exactly as today
  via the preserved legacy-fallback tier.

### Refund/reserved-credit behavior

- Decision: no change needed to the refund policy or wallet-cap-exploit design -- purchases
  already deduct credits immediately at accept time (`commanderproductionevent.cpp:220`), so
  queued credits are never double-counted against the wallet cap by construction. The only real
  fix needed is wiring the two already-correct cancel/refund functions to real call sites (see
  Playability Blockers above).
- Reason: confirmed by reading the actual credit-deduction code this pass; no invariant is
  broken, only a call site is missing.
- Affected files/systems: `CommanderProductionService::Cancel_Item`/`On_Facility_Destroyed`,
  wherever `BuildingGameObj` marks itself destroyed.
- Compatibility: none.
