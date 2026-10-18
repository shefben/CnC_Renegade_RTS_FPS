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

## 2. Building icons for buildable descriptors — **done**

**Status**: the art is done. `models/building images/purchase_images_2/tga/` holds 33 building
icons covering both factions and the neutral props, named by faction and structure:

```
gdi_power_plant   gdi_barracks      gdi_warfactory    gdi_ref          gdi_comm_center
gdi_sil           gdi_agt           gdi_guard_tower_r gdi_repair_pad   gdi_landing_platform
gdi_con           gdi_harvester

nod_power_plant   nod_hon           nod_airstrip      nod_ref          nod_comm_center
nod_sil           nod_obe           nod_turret        nod_sam_site     nod_repair_pad
nod_landing_platform                nod_con           nod_harvester

neutral_round_bunker  neutral_square_bunker  neutral_stone_bunker  neutral_sniper_tower
neutral_platform      neutral_czech_hedgehog neutral_barbed_wire_roll
```

**Name** — whatever a descriptor's `build.icon_texture` names. The packaged cameo carries the
project's `ow_` prefix so it cannot shadow a stock Renegade texture, so a descriptor sets e.g.
`"icon_texture": "ow_bld_gdi_power_plant.tga"`. All 22 descriptors under
`data/building_descriptors/` now name one; the two Communications Centers share their faction's
`comm_center` cameo, because the art has no separate Advanced variant.

**Where it is read** — `Add_Building_Cards`, `Code/Combat/commandersidebar.cpp`, onto
`CommanderSidebarCard::IconTexture`; loaded by `Set_Icon_Texture`,
`Code/Combat/commandersidebarrenderer.cpp`, through
`WW3DAssetManager::Get_Texture(name, MIP_LEVELS_1)`.

**Without it** — a card whose texture the asset manager cannot find draws its cell, its name, its
cost and its status with no picture, and the miss is counted in
`CommanderSidebarRenderer::Get_Missing_Icon_Count()`. Never a crash, never a stand-in file.

**Packaging** — `tools/build_commander_icons.py`. `models/building images/` is not on the asset
manager's search path, and the authored files are uncompressed 1254x1254 TGAs of 3-6 MB each,
which is far more resolution than a card draws and far more bytes than the client should hold
resident. The tool resizes each to 128x128, converts to 32-bit, renames it `ow_bld_<stem>.tga`
and writes it loose into the game's `Data` directory, where `Get_Texture` already finds every
other `ow_` texture this project ships. 128x128x32 matches the cameos already in `Data`
(`hud_rsf_*.tga`), so the four sidebar modes read as one panel.

Re-run it after re-exporting the art:

```text
python -I tools/build_commander_icons.py
```

**Constraints** — match the existing Purchase Terminal images so the four sidebar modes read as
one panel: same pixel dimensions, same aspect, same framing and margin conventions as the
`TYPE_VEHICLES` entries. The renderer draws the icon into the upper ~60% of a cell with the cost
stamped across the bottom of the icon, so the lower strip of each image should not carry detail
that matters.
