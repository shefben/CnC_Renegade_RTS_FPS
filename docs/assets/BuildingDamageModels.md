# Building damage models (runtime-constructed buildings)

Roadmap Section 44 / **Q-016**. These are the whole-model damage states a
*Commander-constructed* building wears. Map-authored buildings are untouched by this
system: they keep the legacy alternate-material swap across the statics
`BuildingGameObj::Collect_Building_Components` claims, and need none of the models below.

## Why whole models rather than a material swap

A map-authored building is a scatter of separately placed `StaticPhysClass` tiles whose
names carry `#interior` / `^exterior`, and damage is expressed by flipping each tile to its
alternate material set. A Commander-constructed building has no such scatter — it is a
single `.w3d` loaded at runtime — so there is nothing to flip. Its damage states are
therefore separate models named in the descriptor JSON, and entering a state rebuilds the
physics object so collision matches what is drawn.

## Where the code reads these

| What | Where |
| --- | --- |
| Table assembled from JSON | `Code/Combat/buildingdescriptormanager.cpp`, the `damage_visual` block in `Parse_And_Validate` |
| Table validated | same file, `Validate_Descriptor` |
| State chosen from health | `Code/Combat/buildingdescriptor.cpp`, `BuildingDamageVisualDefinition::Find_State_Index` / `BuildingModelDescriptor::Get_Damage_Model` |
| Model loaded and swapped | `Code/Combat/building.cpp`, `BuildingGameObj::Update_Runtime_Damage_Model` → `Build_Runtime_Components` |

## Naming

One model per state, per building, all `ow_`-prefixed so nothing here can shadow a stock
Renegade asset:

| State | Name | Shown while |
| --- | --- | --- |
| intact | `ow_<building>.w3d` | health at or above the highest transition |
| damaged | `ow_<building>_d.w3d` | health below `damaged_health_fraction` |
| destroyed | `ow_<building>_x.w3d` | the building has been declared destroyed |
| extra states (optional) | `ow_<building>_s1.w3d`, `_s2.w3d`, … | between the transitions that name them |

Currently declared, none of which exist yet (Q-017 gave both factions real presets, so
both factions now have a full damage-visual declaration):

- `ow_gdi_power_plant_d.w3d`, `ow_gdi_power_plant_x.w3d`
- `ow_gdi_comm_center_d.w3d`, `ow_gdi_comm_center_x.w3d`
- `ow_gdi_adv_comm_center_d.w3d`, `ow_gdi_adv_comm_center_x.w3d`
- `ow_gdi_silo_d.w3d`, `ow_gdi_silo_x.w3d`
- `ow_nod_power_plant_d.w3d`, `ow_nod_power_plant_x.w3d`
- `ow_nod_comm_center_d.w3d`, `ow_nod_comm_center_x.w3d`
- `ow_nod_adv_comm_center_d.w3d`, `ow_nod_adv_comm_center_x.w3d`
- `ow_nod_silo_d.w3d`, `ow_nod_silo_x.w3d`

…alongside the intact models `ow_gdi_power_plant.w3d`, `ow_gdi_comm_center.w3d`,
`ow_gdi_adv_comm_center.w3d`, `ow_gdi_silo.w3d`, `ow_nod_power_plant.w3d`,
`ow_nod_comm_center.w3d`, `ow_nod_adv_comm_center.w3d` and `ow_nod_silo.w3d`, none of
which exist either. See `docs/zerohour/BuildingPresets.md` for which `BuildingGameObjDef`
preset each of these eight descriptors now names.

## What happens today without them

`WW3DAssetManager::Create_Render_Obj` returns null, `Build_Runtime_Components` logs the
exact name it wanted and returns false, and the building **still exists**: it occupies its
footprint, replicates, takes damage, produces, and can be destroyed. It simply draws
nothing. No crash, no blocking error, no stand-in model checked into the tree. A state
change with no art is a no-op rather than a failure, so a descriptor that names only an
intact model is a building that never changes appearance — which is the expected state of
every one of these until the art lands.

## Constraints the code imposes

- **One file per state, a complete building.** Each state model is loaded on its own and
  replaces the previous one outright. A state model is not a damage overlay and is not
  merged with the intact model.
- **Shared origin and orientation.** All states of one building are placed with the same
  `Matrix3D`, so they must share an origin and a facing. A destroyed state whose rubble is
  modelled off-origin will appear to jump when the building dies.
- **Collision travels with the model.** Whatever collision the meshes carry becomes the
  building's collision the moment that state is entered — so a destroyed state must be
  walkable-over or walkable-through as intended, because nothing else adjusts it.
- **`#` / `^` sub-object naming still means something.** `Walk_Runtime_Hierarchy` reads the
  model's own sub-object names: a sub-object named with `#` marks an enterable interior and
  is what the building's announcement sphere is measured from. A model with no `#`
  sub-object is treated as solid, and announcements are sized to the whole building. Name
  them the same way a level author would.
- **Bones are the anchors.** `mct_anchor`, `purchase_terminal_anchor` and `exit_anchor` are
  resolved by bone name against the state model that is currently worn, via
  `BuildingModelDescriptor::Resolve_Anchor_World_Transform`. A damaged state that keeps the
  MCT reachable must keep the bone; a destroyed state need not, since
  `BuildingGameObj::Resolve_Runtime_Anchor` refuses every anchor once destroyed.
- **Bounds drive the construction blocks.** The intact model's object-space bounding box is
  what Section 44's procedural build-up is generated over
  (`RuntimeBuildingFactory::Get_Construction_Bounds`), so an intact model whose box is much
  larger than the visible building will build up over empty ground.

## JSON shape

A declared fraction is a **transition**: the health you fall *below* to enter the state it
names. The intact model has no fraction of its own — it is whatever is worn above the
highest transition — which is what stops an extra state from accidentally outranking it.

```json
"damage_visual": {
  "intact_model": "ow_gdi_power_plant.w3d",
  "damaged_model": "ow_gdi_power_plant_d.w3d",
  "destroyed_model": "ow_gdi_power_plant_x.w3d",
  "damaged_health_fraction": 0.5,
  "extra_states": [
    { "health_fraction": 0.8, "model": "ow_gdi_power_plant_s1.w3d" }
  ]
}
```

That example yields three living bands — intact down to 0.8, `_s1` down to 0.5, `_d` down
to 0 — plus the destroyed model, which is not a band and is reached only by the building
being declared destroyed. Omitting the whole block is legal and means the building always
wears `model_name`.
