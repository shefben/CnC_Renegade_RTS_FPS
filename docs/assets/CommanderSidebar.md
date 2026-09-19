# Commander sidebar content and icons

Roadmap Section 42, Phase 36. Read by `Code/Combat/commandersidebar.cpp`.

This is mostly a **content** gap rather than an art gap, and it is worth being precise about the
difference: three of the sidebar's four modes already have real Renegade content behind them and
need nothing new. Only two things are missing, and one of them is a purchase page rather than a
picture.

## Already satisfied — no new asset required

`INFANTRY` and `GROUND_VEHICLES` read `PurchaseSettingsDefClass` (`TYPE_CLASSES` and
`TYPE_VEHICLES`), which already carries, per entry, a cost, a tech level, a preset id and a
Purchase Terminal image name. Section 42's *"reuse existing Renegade Purchase Terminal
character/vehicle images whenever available"* is therefore met by reading the definitions rather
than by anyone drawing anything.

---

## 1. Air production purchase page

**Name** — none exists to pass. There is no `PurchaseSettingsDefClass::TYPE_AIRCRAFT`, and no
`PurchaseSettingsDefClass` instance for air units on either team.

**What it is for** — the `AIR_VEHICLES` sidebar mode: the list of aircraft a Commander may order
from a GDI or Nod Air Pad, with a name, a cost, a tech level, a preset and an icon per entry,
exactly as the infantry and vehicle pages already have.

**Where it is read** — `CommanderSidebarModel::Refresh`, `Code/Combat/commandersidebar.cpp`, the
`SIDEBAR_MODE_AIR_VEHICLES` case. Today that case is an explicit, commented no-op.

**Without it** — `AIR_VEHICLES` is a fully working empty mode. It can be entered, it binds to an Air
Pad, it pages, its power bars read correctly, and every purchase against it is refused with a real
reason. Nothing crashes, nothing draws a missing icon, and nothing is faked by borrowing the ground
vehicle page. `renegade -shaderselfcheck sidebar` asserts the emptiness, so it is a recorded fact
rather than a surprise.

**Constraints** — whatever is authored must be a `PurchaseSettingsDefClass`-shaped page to be read
without a second code path: at most `ENTRY_COUNT` (10) entries per team, each with a cost, a tech
level, a preset definition id and a texture name. The presets must be canonical flying
`VehicleGameObj` derivatives, since Section 45 requires *"canonical OpenW3D/TT VTOL/flying
behavior"* and not a new aircraft class. Icons follow the existing PT image convention — same
dimensions and framing as the vehicle page's images, so the two modes' cards match.

---

## 2. Building icons for buildable descriptors

**Name** — whatever a descriptor's `build.icon_texture` names. Invented names use the `ow_` prefix,
e.g. `ow_pt_powerplant.tga`, `ow_pt_barracks.tga`.

**What it is for** — the icon on a `BUILDINGS`-mode card. Stock Renegade has no buildable-structure
concept at all, so there is no existing PT image for a *building* the way there is for a character
or a vehicle.

**Where it is read** — `Add_Building_Cards`, `Code/Combat/commandersidebar.cpp`, copying
`BuildingModelDescriptor::Build.IconTexture` onto the card. The card is then a renderer's input; no
renderer exists yet, so nothing currently loads the texture.

**Without it** — a descriptor that names no icon gets a card with an empty `IconTexture` and every
other field intact: name, cost, `+generation`, `-consumption`, and its availability reason. A
renderer is expected to draw the card without an image rather than to skip the card or substitute a
placeholder file. No stand-in texture is checked into the tree.

**Constraints** — match the existing Purchase Terminal images so the four modes read as one panel:
same pixel dimensions, same aspect, same framing and margin conventions as the `TYPE_VEHICLES`
entries. The subject is the finished building, read at card size — roughly three-quarter view,
consistent light direction across the set, and legible as a silhouette, since a card is small and a
player picks from eight at once. A team-coloured variant is not required; the card's own faction is
already known from `build.team`.
