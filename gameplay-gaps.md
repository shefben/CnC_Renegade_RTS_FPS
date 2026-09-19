# Gameplay Gaps

Gaps that a **player would notice** — missing content, missing rules, missing feedback, things
that behave oddly at the table. Engineering gaps (unwritten renderers, unwired call sites,
untested code paths) belong in `WIP_features.md`, not here.

Kept current: an entry is edited when it changes and **deleted** when it stops being true.

---

## Commander sidebar

### No air units can ever be ordered
`AIR_VEHICLES` is a real, working sidebar mode with nothing in it. Stock Renegade ships no air
purchase page — there is no `PurchaseSettingsDefClass::TYPE_AIRCRAFT` and no content behind one —
so the mode opens, binds to an Air Pad, pages and refuses every purchase correctly, and offers
zero units. A Commander can build an Air Pad and then do nothing with it.

Needs: an air purchase page per faction (name, cost, tech level, preset, icon per entry), with
canonical flying `VehicleGameObj` presets behind it. Shape and limits in
`docs/assets/CommanderSidebar.md`.

### Building prices, build times and power figures are unset
Every buildable structure's cost, build time, tech level, power draw and power output comes from
its JSON descriptor's `build` and `power` blocks. No descriptor in the tree sets them for real
content yet, so the economy of base building is entirely unauthored — nothing has a price a
player could judge.

Needs: a balance pass authoring `data/building_descriptors/*.json` for the twelve-building
coverage list. Icons now exist for both factions (see below), so the cards will render.

### Building cards will draw without pictures until the icons are packaged
33 building icons exist for both factions (`models/building images/purchase_images_2/tga/`), named
to match what a descriptor's `build.icon_texture` would ask for. They are not on the asset
manager's search path, so the sidebar cannot load them yet and every building card renders as a
frame with a name, a cost and no image. They are also 3-6 MB uncompressed TGAs — far larger than a
card needs. See `docs/assets/CommanderSidebar.md`.

### Faction rosters are not defined
`build.team` lets a descriptor be GDI-only, Nod-only or shared, and nothing says which buildings
each side should actually get. Today every authored descriptor would be buildable by both sides
unless a value is chosen per building.

---

## Base building

### Nothing physically appears when a building is placed
An accepted placement starts a real, timed, replicated procedural construction and the blocks
assemble bottom-to-top — but no canonical `BuildingGameObj` is created at the end, so the site
finishes and leaves nothing standing. A Commander can spend credits and watch a build-up that
produces no building.

This is the single largest gameplay gap in the Commander layer.

### Placement has no cursor
A building is placed with `commander_ghost <name> [x y z]` and `commander_place` at the console,
not by pointing at the ground. The ghost model, its red/valid tint and the server's verdict are
all real; what is missing is the mouse-to-world step that would let a player aim it.

### No build prerequisites
A Commander with enough credits can build anything on their faction's list in any order. There is
no "Refinery requires Power Plant" style dependency anywhere — `tech_level` is the only gate, and
it is a match-wide number a script moves, not a per-building prerequisite.

### Buildings cannot be sold or repaired from the sidebar
Classic C&C has sell and repair as sidebar verbs. Neither exists here. Manual power on/off and
priority override *do* exist as authoritative calls, but have no button.

---

## Production and units

### No unit is ever actually produced
Queues validate, charge, tick, pause on power loss, block on a busy exit and refund on
destruction — and then report a completion that nothing acts on. No `SoldierGameObj` or
`VehicleGameObj` is created, so a Barracks queue drains into nothing.

### Produced units would have nowhere to walk out of
The descriptor anchor roles (`infantry_spawn`, `infantry_staging`, `vehicle_exit`,
`vehicle_staging`, `aircraft_spawn`, `aircraft_staging`) resolve correctly against a model's
bones, but no building model in the tree has those bones named yet, and no descriptor maps them.

### Commander orders are recorded but not obeyed
`MOVE`, `ATTACK_MOVE`, `GUARD`, `PATROL` and the rest are validated, stored durably and survive a
save — and no AI reads them. An ordered unit stands still.

### No unit selection by pointing
Selection is by network id through code. There is no click-select, no drag box, no control
groups, and no on-screen indication of what is selected.

### Population caps are placeholder numbers
40 infantry / 20 ground / 10 air / 60 total per team, chosen to be plausible rather than balanced.
Nothing has been played against them.

---

## Economy

### The wallet has no ceiling, so Tiberium Silos do nothing
Silo capability is real, stacking and queryable, and raises a wallet cap that does not exist —
stock Renegade's economy never had one. Building Silos is currently a pure waste of credits.

Blocked on a design decision: **Q-014** in `awaiting_answers.md`.

### Refund policy is full-price
Cancelling or losing a queued unit refunds 100% of its cost. Deliberate and documented (there is
no partially-built-unit concept), but it means a Commander can park credits in a queue with no
risk.

---

## Power

### Power only matters for buildings that have a JSON descriptor
The deterministic grid — finite generation, priority load-shedding, manual override — only owns
buildings whose preset names a descriptor. Everything else still uses stock Renegade's coarse
"any power plant alive means everything is powered" rule. Until content is authored, a match plays
by the old rule and the sidebar's power bars read zero.

### No audio or on-screen warning when power is lost
The sidebar's bar turns red and says `LOW POWER`, and shed buildings go dark. There is no
announcement, no EVA line, and nothing at all outside Commander Mode — an FPS player never learns
the base browned out.

---

## Commander role

### Anyone can become Commander
`commander_mode 1` puts any player in the chair. There is no election, no claim, no one-per-team
enforcement at the point of entry, and no way to hand the role over. The *production* service does
enforce one Commander per team, so a second player entering Commander Mode gets the camera and the
panel but is refused every purchase — confusing rather than harmful.

### Commander Mode has no dedicated input
Panel paging, mode switching, placement and orders are all console commands. Camera pan/zoom/tilt
work. There is no key or mouse binding for any of the sidebar verbs.
