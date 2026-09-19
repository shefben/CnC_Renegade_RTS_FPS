# Commander right-side UI + contextual production sidebar

Roadmap Section 42, Phase 36.

`Code/Combat/commandersidebar.{h,cpp}`.

## A model, not a panel

Section 42 reads like a description of pixels, but almost everything it actually *specifies* is
state and rules: which mode is showing, which physical facility the panel is bound to, what each
card says, which page of eight is visible, what the power bars read, and — the part that matters
most — exactly when a card must refuse. All of that is decidable with no device, no `Render2DClass`
and no scene, so all of it lives in `CommanderSidebarModel` and is checked by
`renegade -shaderselfcheck sidebar`. A renderer is a consumer; it owns no rule of its own.

This is the same split `StatusOverlaySystem` (Section 32) already made, and it is what lets
*"dead/offline selected facilities cannot produce"* be proved by a check rather than by a person
watching a panel grey out.

## The content comes from the purchase definitions, so the icons do too

| Mode | Source |
|---|---|
| `INFANTRY` | `PurchaseSettingsDefClass` `TYPE_CLASSES` |
| `GROUND_VEHICLES` | `PurchaseSettingsDefClass` `TYPE_VEHICLES` |
| `AIR_VEHICLES` | nothing — see below |
| `BUILDINGS` | every loaded `BuildingModelDescriptor` whose `build.buildable` is true |

*"Reuse existing Renegade Purchase Terminal character/vehicle images whenever available"* is
therefore not something this class tries to do; it is a consequence of reading the purchase
definitions themselves, which is where those images, costs, tech levels and definition ids already
are. Availability comes from `PurchaseAvailabilityClass`, which is already replicated and already
enforces `FLAG_HIDDEN`/`FLAG_DISABLED`/`FLAG_BUSY` server-side — so a client whose menu is stale
cannot buy what it should not see, and this sidebar inherits that for free rather than inventing a
second availability rule.

**`AIR_VEHICLES` is empty**, because stock Renegade ships no air purchase page: there is no
`TYPE_AIRCRAFT` and no content behind one. The mode exists, binds, pages and refuses purchases
correctly, and fills the moment content exists. That is the project's "missing art never blocks
engine work" rule applied to a missing *content set*; recorded in `docs/assets/CommanderSidebar.md`.

## The card carries the reason, not just a boolean

`CommanderCardStatusType` is an enumerated reason rather than a bool, because Section 42 requires
the panel to display a *"prerequisite/disabled reason"*. A caller handed only `false` would have to
re-derive the reason to draw it — which is how two implementations of the same rule start.

The reasons are ordered as a player would fix them: the facility first (dead, then constructing,
then unpowered), then the entry's own availability and tech level, then the queue, then the money.
"Insufficient credits" is last because it is the only one that resolves by waiting.

A card that cannot be bought is still a card. Affordability and faction produce a *status*, never a
missing entry, so the panel can explain itself.

## Binding is a descriptor question

`Bind_Facility` decides what kind of facility a clicked building is from
`BuildingModelDescriptor::Production.Kind`, never from the model name — Section 42's *"use canonical
building definition/capability plus descriptor semantics, not raw model filenames alone"*, made
literal. That is also why *"Airstrip configured as ground factory"*, which the roadmap names
explicitly, needs no special case: it is a descriptor whose `production.kind` is `ground_vehicle`.

A building on another team, a building whose descriptor is not loaded, and a building that produces
nothing all fail to bind and change nothing — clicking them is legitimate, they just are not
production facilities.

## The facility state is read, never cached

`Refresh_Facility_State` re-reads alive from the `BuildingGameObj`, powered from
`TeamPowerGridService`, and construction-complete from `BuildingConstructionService`, every refresh.
Section 42 requires the panel to disable itself *the moment* a facility dies or loses power; a
cached answer would be exactly the stale purchase it forbids.

`CommanderFacilityState::Is_Operational()` is the single expression every purchase path asks.

## Power bars

`Get_Power_Display` fills Section 42's whole "Power display" list in one read, so a renderer never
makes six separate service calls and risks drawing two of them from different allocation passes.

Two facts are deliberately separate:

- **Headroom** is `Generation - AllocatedDemand`, and is never negative — the allocator never
  allocates past generation, so a negative headroom would be a bug being displayed.
- **Overloaded** is `RequestedDemand > Generation`: more was asked for than exists. This is the one
  Section 42 wants *"immediately obvious"*, because it is what load shedding is a symptom of.

## What this phase does not build

- ~~No renderer.~~ **Built, a later pass**: `Code/Combat/commandersidebarrenderer.{h,cpp}`, drawn
  from `HUDClass::Render` and rebuilt from `CombatManager::Think`. It is styled on the original
  Command & Conquer sidebar -- a beveled metal strip, the signature *vertical* power bar down the
  left edge with a sliding drain marker, amber readouts, the cost stamped on the icon, and recessed
  build cells that dim rather than vanish when unavailable. Section 42 fixes only the content (two
  rows of four, paging, power bars, per-card cost and reason) and asks for something "visually
  compatible with Renegade's existing UI/purchase imagery", so the reference and the spec point the
  same way; where they disagree -- C&C uses two *columns* of icons -- the roadmap wins.

  The renderer owns no rule. It colours what the model called buyable and prints the model's own
  status name as the disabled reason, so a greyed card and a refused purchase cannot disagree. It
  rebuilds only when a signature of everything it displays actually moves, and draws nothing at all
  when the panel is closed. `CommanderModeController::Enter_Commander_Mode` opens the panel and
  `Exit_Commander_Mode` closes it and cancels any ghost, so every future way into Commander Mode
  gets the panel without having to ask for it.
- **Localized names.** A purchase page's name is a unicode string id resolved through the string
  table; the card keeps `DisplayName` as a fallback label and a renderer that wants the localized
  one asks the page directly with the same index. Building cards use the descriptor's own
  `build.display_name`.
- **Per-building power controls on a selected building.** Section 42 asks the panel to expose a
  selected allied building's power state, JSON priority, manual override and manual enable/disable.
  `TeamPowerGridService` already has all four as real, immediate, authoritative calls
  (`Set_Manual_Disabled`, `Set_Priority_Override`, `Clear_Priority_Override`, `Is_Member_Powered`) —
  what is missing is only the panel surface that calls them, which arrives with the renderer.

## Checks

`renegade -shaderselfcheck sidebar`, registered as ctest `shader_sidebar` and `fds_shader_sidebar`.
Device-less. Pins: BUILDINGS is the entry default; only buildable descriptors become cards; a card
carries the descriptor's cost and its `+generation`/`-consumption`; affordability and wrong-faction
are statuses rather than missing cards; the eight-slot page size; page counts that follow content
and clamp rather than wrap at both ends; a refused bind changing nothing; no unit card reading
`AVAILABLE` with nothing bound; return-to-BUILDINGS dropping the binding; `AIR_VEHICLES` being
empty-but-well-formed; and the power bars matching the grid, including an overload reporting itself
rather than showing negative headroom.

**Not covered by any automated check**: `Bind_Facility`'s success path needs a real
`BuildingGameObj`, because the descriptor name it binds by is read off `BuildingGameObjDef` — the
rules that binding drives are exercised directly instead. Nothing here draws, so nothing about the
panel's appearance is verified.
