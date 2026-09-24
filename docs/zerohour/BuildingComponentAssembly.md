# Real multi-part building models (Q-017 follow-up)

Roadmap Section 44. Records what `models/mp buildings/c&c_mode/` actually contains, why
Commander-built GDI/Nod Power Plant now assemble from several real parts instead of one
placeholder mesh, and why GDI/Nod Communications Center did not both get the same treatment.

## Where this data came from

The user pointed at `models/mp buildings/c&c_mode/`: real GDI/Nod building art, for 17
buildings catalogued into a `buildings.json` index plus one `building.json` manifest per
building (schema: `building-assembly.schema.json`), separating each building into an
exterior/interior shell, several "building aggregate" meshes (already-compiled MCT/PT-proxy-
bearing pieces -- the manifest's own note: "the interior and aggregate sources contain PT/MCT
proxy naming. Preserve those proxies... prefer their transforms over hand-authored guesses"),
an animated door part, and night-variant/authoring-only extras that are never loaded at
runtime.

Asked whether Commander-built buildings should (a) assemble from these parts the way a real
map-authored Renegade building does, (b) have everything compiled down to one merged model
per damage state first, or (c) just get a report of what is usable -- the user chose (a).

## What actually got used, and why not everything did

| Building | Manifest state | What this project did |
| --- | --- | --- |
| GDI Power Plant (`gdi/mgpwr`) | Clean, curated `building.json`: shell (ext/int) + 4 aggregates + 1 door part, all at identity position/rotation | Real component assembly (`data/building_descriptors/gdi_power_plant.json`'s `assembly` block); files copied to `data/building_models/gdi/mgpwr/` |
| Nod Power Plant (`nod/mnpwr`) | Same shape, Nod-prefixed | Same treatment; `data/building_models/nod/mnpwr/` |
| GDI Comm Center (`gdi/GDI comm`) | One already-compiled file, `G_Comcntr.w3d`, no manifest -- nothing to assemble | Single-mesh path, pointed at the real file (`data/building_models/gdi/gdi_comm/`); no `assembly` block needed |
| Nod Comm Center (`nod/NOD communications center`) | ~25 loose files directly under `parts/` (`com_ag_mnfrme`, `com_ag_cnf`, `com_lift`, `mct_loca`, `tmp.w3d`, `Com_Temp Grnd.w3d`, ...), **no shell/aggregate/temp split done by anyone** | **Left alone.** Picking which of ~25 uncurated files are real building geometry versus authoring scratch is exactly the guess Q-017's own instructions forbade, applied to asset curation instead of preset names. Still names the `ow_nod_comm_center*.w3d` placeholders from Q-016; still missing art, same as before this pass. Needs the same hand curation `mgpwr/building.json` already received (someone opening each `.gmax`/`.w3d` and sorting shell from aggregate from scratch) before it can follow the Power Plant's path. |
| Construction Yard, Barracks, Refinery, Weapons Factory, Repair Bay (both factions), Advanced Guard Tower, Airstrip, Hand of Nod, Obelisk, SAM Site, Turret | Curated `building.json` manifests | Real component assembly, the Power Plant's treatment: every `loadByDefault` part, copied to `data/building_models/<team>/<prefix>/` (P33; `docs/zerohour/BuildingPresets.md` has the per-building detail) |
| Nod Shrine, and the neutral church/bunkers/hedgehog props | Curated manifests | Not authored: the Shrine has no building definition in the retail data, and the props are not Commander buildings |

GDI/Nod Advanced Communications Center and Tiberium Silo are unaffected by this pass: they
are project-owned (`Code/Combat/projectbuildingdefs.cpp`, Q-017) with no stock or mod-asset
equivalent at all, and keep the single-mesh `ow_*` placeholder path exactly as Q-017 left it.

## Why component assembly is static geometry only -- no `BuildingAggregateClass`, no `DoorPhysClass`

A real map-authored building's aggregates and doors are not "just meshes" in the engine:

- `BuildingAggregateClass::Init` (`Code/Combat/buildingaggregate.h`) takes a
  `BuildingAggregateDefClass`, which carries per-`BuildingStateClass::STATE_COUNT` animation
  frame ranges (`Frame0`/`Frame1`/`AnimationEnabled`) and an `IsMCT` flag -- authored data a
  level's own aggregate compiler normally produces.
- `DoorPhysClass::Init` (`Code/Combat/doors.h`) takes a `DoorPhysDefClass`, which carries two
  `OBBoxClass` trigger zones and three sound definition IDs.

Neither exists anywhere in the `building-assembly.schema.json` manifest (a `modelPart` is
only id/kind/model/position/rotation/loadByDefault/behavior/replaces/notes) or anywhere else
in the asset drop. Constructing either class at runtime from what is actually there would
mean inventing frame ranges and trigger boxes -- the same mistake forbidden for preset names,
applied to animation/trigger data instead. So every assembly part -- aggregates and the door
part alike -- loads as plain static geometry: correct model, correct position, correct
collision, no per-part animation. `Kind` in the JSON (`"exterior"`/`"interior"`/`"aggregate"`/
`"door"`) is informational plus routing (`Code/Combat/building.cpp`,
`BuildingGameObj::Build_Runtime_Assembly`, files each part into `InteriorMeshes` or
`ExteriorMeshes`), not a promise of aggregate/door behavior.

## How the runtime side works

`BuildingModelDescriptor::Assembly` (`Code/Combat/buildingdescriptor.h`) is an optional list
of parts, empty by default. Empty -- true for every descriptor except the two Power Plants --
means nothing changed: the single-mesh path (`BuildingGameObj::Build_Runtime_Components`) is
exactly what it always was.

When `Assembly.Parts` is non-empty, `BuildingGameObj::Build_Runtime_Assembly` loads every
`load_by_default` part, builds each its own `StaticPhysClass` at
`RuntimeTransform * part_local_transform`, and records it in the (renamed, from the
single-`RuntimePhys` pointer) `RuntimeParts` list -- the single ownership list
`Release_Runtime_Components`, `Peek_Runtime_Model`, `Derive_Runtime_Structure` and
`Resolve_Runtime_Anchor` all now iterate, so the single-mesh path (a one-element
`RuntimeParts`) and the assembly path (several elements) share every consumer.

`BuildingModelDescriptor::Resolve_Anchor_World_Transform` changed from taking one
`RenderObjClass *` to taking every currently-loaded part's model
(`DynamicVectorClass<RenderObjClass *>`), tried in build order until one of them actually
carries the named bone. This is what actually finds the real MCT/PT proxies the asset already
carries (per the manifest's own `proxySources` note) -- the whole reason component assembly
was worth doing over the merged-file shortcut.

### Damage states: assembled, or destroyed -- nothing in between, honestly

Nothing in the real asset drop has a damaged-state variant for any individual part -- the
`stateVariants` the manifest does declare (`mgpwr_ext_n.w3d`, `MGpwr_ag_3n.w3d`, ...) are
*night* variants, a lighting axis this project does not consume anywhere yet, not damage.
Inventing a damaged-state swap with no art to back it would be exactly the kind of gap Q-016
already ruled out. So a component-assembly building has exactly two visual states:

- **assembled** -- built once (`Build_Runtime_Assembly`), kept for the building's life,
  regardless of health;
- **destroyed** -- the existing single-mesh `DamageVisual.DestroyedModel` path
  (`Build_Runtime_Components`), unchanged. "Destroyed" was already one whole-building model
  before this pass, assembly or not, so reusing it needed no new mechanism.

`BuildingGameObj::Update_Runtime_Damage_Model` branches on `descriptor->Assembly.Parts.Count
() > 0` before it ever reaches the single-mesh health-band table, so this is a genuinely
separate, simpler path, not a special case bolted onto the table-driven one.

## Real door behavior, not inert geometry

Shipped as a follow-up in the same pass, after being asked directly: a player walking up to a
component-assembly building's door part hit solid, permanently-closed geometry -- the building
was not actually enterable. Fixed for real, not with a collision-group hack:

**Checked whether the real asset has a usable baked animation before designing around one.**
`data/building_models/gdi/mgpwr/mgpwr_doors.w3d` (and its Nod equivalent) were ASCII-string-
scanned (no W3D inspector tool exists in this environment) -- 796 bytes, a hierarchy
(`MGPWR_DOORS`, bone `ROOTTRANSFORM`, door leaf bone `MPGDR_0`) and one stray string
(`MNHND_ELEV01`), nothing sized like real per-frame animation channel data. `AnimCollisionManagerClass`
(`Code/wwphys/animcollisionmanager.h`), the engine's real animated-door mechanism, needs a
named `HAnimClass` to play; there is no confirmed one here, and inventing an animation name
to point it at would repeat Q-017's own "do not guess" rule aimed at animation data instead of
preset names. So this is real, project-owned motion computed from the door's own measured
geometry, not from an animation asset that cannot be confirmed to exist.

**Vertical retract, not a horizontal slide.** The door's own object-space bounding box
(`RenderObjClass::Get_Obj_Space_Bounding_Box`, the same call `runtimebuildingfactory.cpp`
already uses for construction bounds) gives a real, measured rise distance: the panel
retracts straight up by its own height (`2 * Extent.Z`) plus a 10% margin
(`RuntimeDoorService::RISE_HEIGHT_MARGIN`). A horizontal direction would have to guess which
way clears the opening versus slides into solid wall; a vertical rise clears its own footprint
for any wall orientation and degrades to "looks like a roll-up door" at worst, never a wrong-
direction clipping failure. Reversible later once someone inspects the real geometry or
supplies a proper animation.

**`RuntimeDoorService`** (`Code/Combat/runtimedoorservice.h/.cpp`) is the whole mechanism,
following the exact shape `BuildingConstructionService` already established for "timed,
server-authoritative, deterministically replayable on a client": registered per
(building id, assembly-part index) when `Build_Runtime_Assembly` builds a `"door"`-kind part,
advanced once a frame from `combat.cpp`'s existing Commander/RTS update block against
`CommanderStateClass::Get_Estimated_Server_Time()` -- the same clock
`BuildingConstructionService::Update` reads, right above it. Proximity
(`GameObjManager::Get_Smart_Game_Obj_List`, the engine's own soldier/vehicle enumeration, no
new spatial query invented) and the open/close decision are server-only; both sides then
recompute every registered door's current position as a pure function of (direction,
fraction-at-last-direction-change, time-of-last-change, now) -- `RuntimeDoorService::
Compute_Fraction`, which needs no scene/clock/registration and is exercised directly by the
`runtimedoors` self check. A direction reversal resumes from wherever the door physically was,
not from an endpoint, so reversing mid-swing does not visibly snap.

Only the moment a direction changes crosses the network: `BuildingGameObj::Export_Rare`/
`Import_Rare` gained a block, present only for a runtime-constructed building, that walks its
descriptor's `Assembly.Parts` (deterministic on both sides, same JSON, no count needs to be
sent) and reads/writes `RuntimeDoorService::Get_State`/`Set_State` for each `"door"`-kind
entry. `BuildingGameObj::Mark_Runtime_Door_State_Dirty` is the one-liner the service calls
back into on a state change, since only a `GameObj` can dirty itself over the network.

Collision is a consequence of the real transform, not a separate toggle: once risen, the
panel's own collision no longer overlaps the doorway footprint.

## Asset placement

The real files this pass actually uses are copied into the repo at
`data/building_models/<faction>/<sourcePrefix>/<filename>`, unrenamed (these are real names,
not invented ones -- the `ow_` prefix convention in `docs/assets/README.md` is for invented
placeholder names only, and does not apply here):

- `data/building_models/gdi/mgpwr/` -- `mgpwr_ext.w3d`, `mgpwr_int.w3d`,
  `MGpwr_ag_{1,2,3,4}.w3d`, `mgpwr_doors.w3d`
- `data/building_models/nod/mnpwr/` -- the `mnpwr` equivalents
- `data/building_models/gdi/gdi_comm/` -- `G_Comcntr.w3d`

`WW3DAssetManager::Create_Render_Obj` finds a bare filename through whatever mix files and
loose-file search paths are mounted (`Code/Commando/init.cpp`, `DATA_SUBDIRECTORY = "DATA/"`)
-- the same mechanism `docs/assets/README.md` already documents for textures, with zero
engine changes needed. **This repo location is not yet wired into any packaging/deploy step**
(none exists for loose runtime assets at all today -- nothing shipped one before this pass);
whoever stages a build's `DATA/` folder needs to also copy `data/building_models/**` into it.
That gap is packaging, not code, and is out of this pass's scope.
