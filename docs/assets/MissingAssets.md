# Missing Assets

Every texture, material, model, animation and content page the engine asks for by name and that
does not exist yet. It is compiled from the code and from the per-system asset notes in this
folder. Each entry says what is used in the meantime. Missing art never blocks engine code: every
item below degrades to something safe, named in the "Without it" column.

A `.tga` name can ship as `.tga` or as `.dds`; the asset manager resolves either. Put new files
where every other game texture lives: in `always.dat`, in the level's own mix, or in the loose
texture directory the client runs with. Models go in the same places.

Checked against the retail `always.dat` (15,342 entries) and against the models already in this
repository's `data/building_models`. The list was compiled at the commit that added this file.

---

## 1. Textures

### Terrain (`docs/assets/TerrainLayers.md`)

A stock stand-in is drawn until each `ow_ter_*` file ships. The detail layer is the exception: it
has no stand-in.

| Name | For | Stand-in used now | Without it |
| --- | --- | --- | --- |
| `ow_ter_ground.tga` | base ground, 8 m tiling | `cc_grass1` | stand-in |
| `ow_ter_macro.tga` | large-scale variation, 64 m | `l05_grass` | stand-in |
| `ow_ter_rock.tga` | steep ground, 4 m | `l01-rock2` | stand-in |
| `ow_ter_cliff.tga` | cliff faces, boundary ring, 6 m | `l03_mossrock` | stand-in |
| `ow_ter_shore.tga` | wet shoreline, 4 m | `lv8_sand3` | stand-in |
| `ow_ter_road.tga` | ground under roads, 4 m | `l11_gravel` | stand-in |
| `ow_ter_detail.tga` | close-range grain, 2 m, multiplied over every layer | none (always.dat has no ground detail texture) | no detail stage; ground draws as before |

`ow_ter_detail` must stay near white (mean about 230/255, greyscale, low contrast), because the
detail stage multiplies (`DETAILCOLOR_SCALE`, not 2x).

### Roads (`docs/assets/RoadSurfaces.md`)

| Name | For | Without it |
| --- | --- | --- |
| `ow_road_highway.tga` | highway carriageway (`u` spans the full width once) | road draws untextured |
| `ow_road_street.tga` | street carriageway | road draws untextured |
| `ow_road_track.tga` | dirt track | road draws untextured |

No stock road texture fits this UV layout.

### Water (`docs/assets/WaterModels.md`)

| Name | Stand-in used now |
| --- | --- |
| `ow_water_river.tga` | `water_texture` |
| `ow_water_stream.tga` | `water_texture` |
| `ow_water_lake.tga` | `water_texture` |
| `ow_water_pond.tga` | `water_texture` |
| `ow_water_coast.tga` | `water_texture` |
| `ow_water_ocean.tga` | `water_texture` |

### Surface marks (`docs/assets/SurfaceMarks.md`)

| Name | Kind | Stand-in used now | Without it |
| --- | --- | --- | --- |
| `ow_mark_construction.tga` | construction site | `hnd_pad` | stand-in |
| `ow_mark_stain.tga` | oil and stain (vehicle wrecks) | `oil-2` | stand-in |
| `ow_mark_scorch.tga` | scorch | none | no mark drawn (no event triggers it yet either) |
| `ow_mark_blast.tga` | blast | none | no mark drawn |
| `ow_mark_impact.tga` | impact | none | no mark drawn |
| `ow_mark_dirt.tga` | dirt | none | no mark drawn (no event triggers it yet either) |
| `ow_mark_world.tga` | generic world mark | none | no mark drawn |

### Track and trail ribbons (`docs/assets/RibbonSurfaces.md`)

| Name | Stand-in used now | Without it |
| --- | --- | --- |
| `ow_ribbon_tank_track.tga` | `d_tk_dirt` (Renegade's tank-tread print) | stand-in |
| `ow_ribbon_harvester_track.tga` | `d_tk_dirt` | stand-in |
| `ow_ribbon_tire_track.tga` | none | wheeled vehicles lay invisible tracks |
| `ow_ribbon_scorch_trail.tga` | none | no scorch trail drawn |
| `ow_ribbon_drag_mark.tga` | none | no drag mark drawn |

### Weather (`docs/assets/WeatherParticles.md`)

| Name | Mode | Without it |
| --- | --- | --- |
| `ow_weather_rain.tga` | rain streaks | rain draws untextured (plain coloured streaks) |
| `ow_weather_dust.tga` | dust (profile `weather.dust_density`) | untextured sprites |
| `ow_weather_tiberium.tga` | Tiberium haze near fields (additive) | untextured sprites |
| `ow_weather_falling.tga` | scripted falling particles | untextured sprites |

Snow and ash use the stock `WeatherParticles.tga` atlas and need nothing.

### Foliage (`docs/assets/FoliageModels.md`)

Each plant needs a material texture and a far-LOD impostor.

| Material | Impostor |
| --- | --- |
| `ow_tree_conifer.tga` | `ow_tree_conifer_impostor.tga` |
| `ow_bush_scrub.tga` | `ow_bush_scrub_impostor.tga` |
| `ow_tree_dead.tga` | `ow_tree_dead_impostor.tga` |
| `ow_grass_clump.tga` | `ow_grass_clump_impostor.tga` |
| `ow_rock_boulder.tga` | `ow_rock_boulder_impostor.tga` |

Without them, foliage still places, blocks and breaks, and draws nothing.

### Procedural textures (no file needed)

`ow_cloud_shadow_mask` and `ow_bloom_glow` are generated in memory
(`docs/assets/VisualFidelity.md`). A file with the same name would replace the generated one, but
none is required.

---

## 2. Models

### Building damage states (`docs/assets/BuildingDamageModels.md`)

Each Commander-buildable building swaps to a damaged model below its `damaged_health_fraction` and
to a destroyed model when it dies. Without them it keeps its intact model; its health, capability
loss and death still apply.

| Building | Damaged | Destroyed |
| --- | --- | --- |
| GDI Barracks | `ow_gdi_barracks_d.w3d` | `ow_gdi_barracks_x.w3d` |
| GDI Weapons Factory | `ow_gdi_weapons_factory_d.w3d` | `ow_gdi_weapons_factory_x.w3d` |
| GDI Refinery | `ow_gdi_refinery_d.w3d` | `ow_gdi_refinery_x.w3d` |
| GDI Repair Facility | `ow_gdi_repair_facility_d.w3d` | `ow_gdi_repair_facility_x.w3d` |
| GDI Construction Yard | `ow_gdi_construction_yard_d.w3d` | `ow_gdi_construction_yard_x.w3d` |
| GDI Advanced Guard Tower | `ow_gdi_advanced_guard_tower_d.w3d` | `ow_gdi_advanced_guard_tower_x.w3d` |
| GDI Power Plant | `ow_gdi_power_plant_d.w3d` | `ow_gdi_power_plant_x.w3d` |
| GDI Communications Center | `ow_gdi_comm_center_d.w3d` | `ow_gdi_comm_center_x.w3d` |
| Nod Hand of Nod | `ow_nod_hand_of_nod_d.w3d` | `ow_nod_hand_of_nod_x.w3d` |
| Nod Airstrip | `ow_nod_airstrip_d.w3d` | `ow_nod_airstrip_x.w3d` |
| Nod Refinery | `ow_nod_refinery_d.w3d` | `ow_nod_refinery_x.w3d` |
| Nod Repair Facility | `ow_nod_repair_facility_d.w3d` | `ow_nod_repair_facility_x.w3d` |
| Nod Construction Yard | `ow_nod_construction_yard_d.w3d` | `ow_nod_construction_yard_x.w3d` |
| Nod Obelisk | `ow_nod_obelisk_d.w3d` | `ow_nod_obelisk_x.w3d` |
| Nod Turret | `ow_nod_turret_d.w3d` | `ow_nod_turret_x.w3d` |
| Nod SAM Site | `ow_nod_sam_site_d.w3d` | `ow_nod_sam_site_x.w3d` |
| Nod Power Plant | `ow_nod_power_plant_d.w3d` | `ow_nod_power_plant_x.w3d` |

### Buildings with no model at all

These descriptors have no retail model to assemble. Without one, the building exists, has health
and capabilities, and draws nothing.

| Building | Intact | Damaged | Destroyed |
| --- | --- | --- | --- |
| GDI Tiberium Silo | `ow_gdi_silo.w3d` | `ow_gdi_silo_d.w3d` | `ow_gdi_silo_x.w3d` |
| Nod Tiberium Silo | `ow_nod_silo.w3d` | `ow_nod_silo_d.w3d` | `ow_nod_silo_x.w3d` |
| GDI Advanced Communications Center | `ow_gdi_adv_comm_center.w3d` | `ow_gdi_adv_comm_center_d.w3d` | `ow_gdi_adv_comm_center_x.w3d` |
| Nod Advanced Communications Center | `ow_nod_adv_comm_center.w3d` | `ow_nod_adv_comm_center_d.w3d` | `ow_nod_adv_comm_center_x.w3d` |
| Nod Communications Center | `ow_nod_comm_center.w3d` | `ow_nod_comm_center_d.w3d` | `ow_nod_comm_center_x.w3d` |

The Nod Communications Center has retail parts, but they are uncurated (see
`WIP_features.md`, deferred). Its descriptor still points at the `ow_` model until they are curated.

### Bridge sections (`docs/assets/BridgeSections.md`)

Three bridge types, each with four section kinds. Each kind has an intact and a `_damaged`
variant: 24 models. Without them, each section is built as a box of the section's exact dimensions.

| Type | Sections (each also `_damaged`) |
| --- | --- |
| `ow_bridge_timber` | `_start_cap`, `_span`, `_end_cap`, `_support` |
| `ow_bridge_concrete` | `_start_cap`, `_span`, `_end_cap`, `_support` |
| `ow_bridge_steel_truss` | `_start_cap`, `_span`, `_end_cap`, `_support` |

Rubble from a broken span: `ow_bridge_timber_rubble`, `ow_bridge_concrete_rubble` and
`ow_bridge_steel_truss_rubble`. Without them the debris falls back to pooled visual chips, which
cannot be stood on.

Optional decorations (lamps, railings, cable stays) are laid out only when a definition names
them, so none is missing. No repair scaffold is needed until something repairs a bridge.

### Tunnels (`docs/assets/TunnelModules.md`)

| Name | For | Without it |
| --- | --- | --- |
| `ow_tunnel_lined_straight` | lined tunnel bore (infantry, vehicle, mixed, underpass) | procedural tube with the bridge material |
| `ow_cave_rock_straight` | cave shortcut | procedural tube with the terrain material |

### Foliage and street props (`docs/assets/FoliageModels.md`)

| Name | Category |
| --- | --- |
| `ow_tree_conifer` | tree |
| `ow_bush_scrub` | bush |
| `ow_tree_dead` | dead tree |
| `ow_grass_clump` | grass clump |
| `ow_rock_boulder` | decorative rock |
| `ow_street_lamp` | street prop (settlement streets) |

Without a model, each type is still placed and its proxy still blocks, but nothing draws.

### Settlements and city props

Generated settlements need building models for towns, industrial areas and abandoned buildings.
Destructible props are needed before city debris can exist (P25). This is a content dependency
with no name list yet: settlement structures stay data-only until assets are chosen.

---

## 3. Animations

Nothing the engine runs today asks for an animation that is missing. The embedded animations of
the retail building parts (`data/building_models/**/*.w3d`) are present. These would only be
needed by features that are not built:

- foliage wind sway and topple (the merged foliage mesh cannot animate one plant);
- a bridge repair scaffold (nothing repairs bridges yet).

---

## 4. Content pages (not art)

| What | For | Without it |
| --- | --- | --- |
| Air production purchase page (a `PurchaseSettingsDefClass` for aircraft, one per team) | the Commander sidebar's `AIR_VEHICLES` mode | the mode works and is empty; every purchase is refused with a reason |
| Building icons for buildable descriptors | the sidebar's building cards | cards draw without an icon |

The building icon art already exists in `models/building images/`. It needs packaging:

- move the files onto the asset manager's search path;
- resize them to the Purchase Terminal image size.

See `docs/assets/CommanderSidebar.md`.

---

## 5. Not missing, for the record

These have real retail assets behind them and need nothing:

- the models and embedded animations of every GDI and Nod base building listed under
  "Building damage states";
- snow and ash particles;
- stock water, terrain and mark stand-ins;
- tank tread prints;
- the nine projectile trail emitters that retail ammo definitions name.
