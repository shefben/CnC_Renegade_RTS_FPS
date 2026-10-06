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

### Building prices, build times and power figures are unset for 8 of 12 categories
All 8 existing descriptors (GDI/Nod Power Plant, Communications Center, Advanced Communications
Center, Tiberium Silo) already carry real authored cost, tech level, faction and power figures —
this is no longer true of them. The real gap: the other 8 of the roadmap's 12 named categories
(Barracks/Hand of Nod, Refinery, Repair Bay, War/Vehicle Factory, Airstrip, Construction Yard,
faction defense structures) have no `data/building_descriptors/*.json` at all, so none of them
can be built yet, at any price.

Needs: a balance pass authoring the remaining 8 categories — recommended starting numbers in
`docs/GameplayGapResolutionReport.md`'s Decision Log. Each new descriptor should set `build.team`
explicitly the same way the existing 8 already do.

### Building cards will draw without pictures until the icons are packaged
33 building icons exist for both factions (`models/building images/purchase_images_2/tga/`), named
to match what a descriptor's `build.icon_texture` would ask for. They are not on the asset
manager's search path (confirmed empty this pass), so the sidebar cannot load them yet and every
building card renders as a frame with a name, a cost and no image. They are also 3-6 MB
uncompressed TGAs — far larger than a card needs. See `docs/assets/CommanderSidebar.md`.

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
Classic C&C has sell and repair as sidebar verbs. Neither exists here, and there is no way to
even click a building in the world to select it for management — only units are hit-tested.
Manual power on/off and priority override *do* exist as authoritative calls, but have no button.

Needs: a building-management surface reached by clicking a friendly building; recommended sell
formula and repair model in the Decision Log.

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

### A Procedural Skirmish opens on an empty world with no terrain
Start Procedural Game loads the native map (`ow_procedural.mix`, which has no level file under it) and generates the
whole world in `CombatManager::Post_Load_Level`. When generation is refused, the failure is only logged and the level
carries on, so the player stands in an empty scene with no land, roads or bases, and no message saying why.

Today every native request is refused at the content preflight, before any terrain exists. It stopped at the Humvee
metric until D-034, and now stops at `NO_HEAVY_CAPABLE_BRIDGE`: the game process registers no bridge definitions (D-034).

Needs: generation to succeed (D-034's blocker), and a failed generation to return to the menu with its reason instead of
loading the empty native level.
