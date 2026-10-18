# Gameplay Gaps

Gaps that a **player would notice** — missing content, missing rules, missing feedback, things
that behave oddly at the table. Engineering gaps (unwritten renderers, unwired call sites,
untested code paths) belong in `WIP_features.md`, not here.

Kept current: an entry is edited when it changes and **deleted** when it stops being true. Last
full audit: `docs/GameplayGapResolutionReport.md` (cross-checked every entry against source; six
entries below were deleted as obsolete by that pass, one new entry split out, several others
narrowed to what is actually still missing).

---

## Commander sidebar

### No air units can ever be ordered
`AIR_VEHICLES` is a real, working sidebar mode with nothing in it. Stock Renegade ships no air
purchase page — there is no `PurchaseSettingsDefClass::TYPE_AIRCRAFT` and no content behind one —
so the mode opens, binds to an Air Pad, pages and refuses every purchase correctly, and offers
zero units. A Commander can build an Air Pad and then do nothing with it.

Needs: real GDI/Nod flying `VehicleGameObjDef` presets found by the same `Objects.DDB`
enumeration Q-017 already built (not guessed), an Air Pad descriptor, and a small project-owned
air-roster table the sidebar's `AIR_VEHICLES` case reads from. Shape and limits in
`docs/assets/CommanderSidebar.md`.

---

## Base building

### Runtime-constructed buildings have no working MCT or Purchase Terminal
A Commander-built building now genuinely appears, stands and is destroyable (Q-016 + this
session's component-assembly and door work). What still does not work: `mct_anchor`/
`purchase_terminal_anchor` are authored on 4 of the 8 descriptors but nothing consumes them —
every runtime part is a plain `StaticPhysClass`, never a real `BuildingAggregateClass`, so the
repair gun's `Is_MCT()` check can never fire on one. No purchase-terminal interaction mechanism
exists for any building in this codebase, stock or runtime.

Needs: a lightweight runtime-only interactive-MCT proxy (same "real class needs authored
animation data we don't have, build a project-owned wrapper instead of guessing it" pattern this
session's door fix already used); a separate design decision on whether purchase terminals get an
in-world interaction at all or route through the sidebar exclusively, which already works.

### No build prerequisites
A Commander with enough credits can build anything on their faction's list in any order. There is
no "Refinery requires Power Plant" style dependency anywhere in the schema at all —
`BuildingModelDescriptor` has no prerequisite field, and `tech_level` is the only gate, a
match-wide number a script moves, not a per-building prerequisite.

Needs: a `prerequisites` block added to the descriptor schema, checked at placement time only
(never an operational/destroy-cascades-destroy dependency). Recommended starting graph in
`docs/GameplayGapResolutionReport.md`'s Decision Log.

### Buildings cannot be sold or repaired from the sidebar
Classic C&C has sell and repair as sidebar verbs. Neither exists here. Clicking a friendly
building *does* now select it, open its power panel and — for a production facility — fork the
sidebar onto what it builds, so the surface to hang these verbs on exists. Manual power on/off
and priority override exist as authoritative calls, but still have no button.

Needs: sell and repair buttons on the selected-building panel; recommended sell formula and
repair model in the Decision Log.

---

## Production and units

### Produced units all exit through one generic point regardless of kind
Production spawn already works and already degrades safely with no art: `exit_anchor` resolves
by bone name when a descriptor authors one, and falls back to a derived direction otherwise. What
is still missing is the kind-specific half of the schema — `infantry_spawn`, `infantry_staging`,
`vehicle_exit`, `vehicle_staging`, `aircraft_spawn`, `aircraft_staging` do not exist anywhere in
code or content, so every unit kind shares the one generic exit point, and there is no
descriptor-declared fallback transform for any anchor (only one hard-coded C++ fallback,
specific to `exit_anchor`).

Needs: the six named roles, plus a generic `fallback_local_position`/`fallback_local_yaw` pair on
each anchor entry so new roles degrade safely the same way `exit_anchor` already does.

### PATROL walks to one point and stops
Every other order (`MOVE`, `ATTACK_MOVE`, `ATTACK_TARGET`, `FOLLOW`, `GUARD`,
`REPAIR_TARGET`/`RETREAT_TO_REPAIR`, `HOLD_POSITION`) is genuinely obeyed — confirmed this pass by
reading the dispatch code directly. PATROL is the one exception: the wire format carries a single
point, so it behaves exactly like MOVE, with no back-and-forth.

Needs: a second point on the order's wire format and a small per-unit alternation loop, shaped
like the existing Guard AI controller. Multi-waypoint patrol out of scope for this pass.

### No shift+click additive selection or control groups
Click-select, drag-box select, a real selection-ring overlay and right-click-move all already
work — confirmed this pass by reading the input code directly. Two pieces are deliberately not
built yet: Shift+click to add/remove a unit from the selection (a fresh box currently always
replaces it), and Ctrl+1-9/1-9 control groups.

Needs: a toggle-add path on the existing selection list plus up to 9 saved-id-list slots,
client-local, no new server authority.

### Population caps count units, not weight them
Per-kind population caps are real and enforceable. What does not exist is a per-unit weight — a
Mammoth Tank and a light vehicle currently cost the same population within their shared kind,
since every produced unit adds a flat `+1`.

Needs: a per-definition population-weight table. Recommended starting weights in the Decision
Log; the `40/20/10/60` ceiling can stay until a real match says otherwise.

---

## Economy

### Production-facility destruction loses a queue's credits instead of refunding them
The 100%-refund arithmetic is real and correct (`CommanderProductionService::Cancel_Item`/
`On_Facility_Destroyed`), and the wallet-cap exploit the tracker used to worry about cannot
happen — credits are already spent the instant a purchase is queued, not held anywhere they
could double-count. But `On_Facility_Destroyed` is called from nowhere in the actual game —
confirmed this pass, its only caller anywhere in the tree is a self-check test. A destroyed
Barracks or War Factory currently forfeits every credit in its queue. Separately, there is no
player-facing way to cancel a single queued item either — `Cancel_Item` also has zero callers.

Needs: wire `On_Facility_Destroyed` into wherever a building actually transitions to destroyed,
crediting the refund back to its owner; wire `Cancel_Item` to a real UI/console call site.

---

## Power

### Power only matters for buildings that have an explicit descriptor name set
`data/building_descriptors/*.json` is loaded automatically at real startup — this is no longer a
gap. What is still missing is the middle tier of the intended resolution order: a preset with no
hand-set `BuildingGameObjDef::DescriptorName` has no way to find its descriptor by name/alias, so
it falls back to stock's coarse "any plant alive" rule until someone hand-edits that field.

Needs: an alias-lookup tier in `BuildingDescriptorManager::Get_Descriptor`, using real preset
names from the same `Objects.DDB` enumeration Q-017 already built.

### No audio or on-screen warning when power is lost
The sidebar's bar turns red and says `LOW POWER`, and shed buildings go dark. There is no
announcement, no EVA line, and nothing at all outside Commander Mode — an FPS player never learns
the base browned out.

---

## Commander role

### A disconnected Commander locks their team's role for the rest of the match
The real entry point (the `B` key) already claims the role correctly — first-claimant-wins,
one-per-team, server-authoritative, confirmed this pass by reading the claim event directly. What
is broken: nothing anywhere clears `Get_Team_Commander` when the holder disconnects, so a
departed Commander's id stays claimed and every future claim for that team is refused for the
rest of the match. This is a live bug, not a missing feature.

Needs: clear the team's commander slot on player-disconnect (one call site, needs locating the
existing disconnect hook). Also still missing: a deliberate resignation action (leaving Commander
Mode today keeps the job), a vote when multiple players claim at once, and the `commander_mode`
console command bypasses the claim system entirely (unlike the real `B`-key path).

### Confirm the Commander Mode key appears in the rebind screen
`INPUT_FUNCTION_COMMANDER_MODE_TOGGLE` is a real, bindable input function on `B` by default,
wired through the normal per-frame input table exactly like every other rebindable function —
this is no longer "no dedicated input." Not confirmed this pass: whether it is actually listed in
the controls/config remapping screen. One file to check, not a design question.

## Procedural Skirmish

### The native generated world is still refused for every seed
Start Procedural Game asks for the native world (`ow_procedural.mix`, which has no level file under it) and generates
everything in `CombatManager::Post_Load_Level`. A player no longer ends up in an empty skybox: a refused generation
abandons the load, reports its reason and loads a fit-to-level world over a stock skirmish map instead (D-038), which
the generator does build. So the button produces a playable match — just not yet the native one it asks for first.

What is still missing is native generation succeeding. Census over requested seeds 1..256 (768 attempts, generator
identity 16, D-037 and D-039 included): no seed reaches Phase 12, and the 256 kept worlds stop at Phase 2 (32),
Phase 3 (39), Phase 4 (6), Phase 5 (7), Phase 6 (84), Phase 7 (24), Phase 8 (29), Phase 9 (21) and Phase 10 (14). By
reason, over all attempts: Phase 6 `lake_quota` 213, Phase 2 `NO_AVAILABLE_BIOME_PAIR` 94, Phase 3
`NO_LEGAL_BASE_POSITION` 87, Phase 7 `feature_lost` 72, Phase 8 `base_hard_overlap` 63, Phase 9 `bridge_quota` 48,
Phase 10 `no_spline` 44, Phase 6 `water_in_reserve` 33, Phase 3 `NO_LEGAL_RESOURCE_POSITION` 30, Phase 5
`candidates_exhausted` 21, Phase 4 `BASE_ON_HARD_INTENT` 18, Phase 8 `resource_illegal` 18, Phase 9
`obligation_unbound` 15, Phase 6 `no_crossing_window` 6.

Needs: the walls worked in census order. D-039's budget raise took Phase 5 from 66 kept worlds to 7 and moved 59 seeds
deeper, so **Phase 6 `lake_quota` is now the wall**. Unlike Phase 5's, it is a generator rule rather than a search
budget, so it needs a decision of its own.
