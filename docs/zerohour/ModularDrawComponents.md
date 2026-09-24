# Modular draw components + composite attachments

Roadmap Section 29, Zero Hour / SAGE feature 18. Donor concepts: `W3DModuleFactory`,
`DrawModule`, `W3DModelDraw`, and Zero Hour's expanded specialized draw-module set including the
Overlord attachment-oriented render modules.

`Code/Combat/rendercomponent.h` (the component contracts) and
`Code/Combat/compositeattachment.{h,cpp}` (the one generalized attachment implementation),
consumed by `VehicleGameObj` (`Code/Combat/vehicle.{h,cpp}`) and `BuildingGameObj`
(`Code/Combat/building.{h,cpp}`).

## The rule this phase is shaped by

The section's own rule is a prohibition, not a design: **do not replace OpenW3D `GameObj` with
the SAGE `Thing` hierarchy.** That rules out the donor's actual architecture. In Zero Hour a
`Thing` owns a list of modules built by a factory from INI, and *all* behaviour lives in those
modules; porting that shape would mean rewriting every `GameObj` subclass in the game to own a
module list, which is exactly what the rule forbids and exactly what the acceptance does not ask
for. The acceptance asks only that *new rendering behaviour can be attached without continuously
expanding every `GameObj` subclass* -- a statement about what adding a feature costs, not about
what a game object is made of.

So this is deliberately two separate, small things rather than one framework:

1. **Contracts, no container.** Five interfaces a class implements as extra inheritance,
   keeping whatever base class it already has. There is no registry, no factory, no dispatch
   loop and no per-object component list anywhere -- adding one would be the `Thing` hierarchy
   arriving by the back door.
2. **One generalized attachment implementation**, because that is the part of the donor that is
   genuinely missing here and genuinely reusable.

## Part 1 -- the component contracts

`Code/Combat/rendercomponent.h`:

| Interface | Method(s) | Implemented today by |
|---|---|---|
| `IRenderComponent` | `Render_Component(RenderInfoClass &)` | `TracerSystemClass`, `DebrisSystemClass` |
| `IClientUpdateComponent` | `Update_Component(float seconds)` | nothing yet |
| `IRTSOverlayComponent` | `Render_RTS_Overlay`, `Is_RTS_Overlay_Visible` | nothing yet |
| `ISelectionVisualComponent` | `Set_Selected`, `Is_Selected` | nothing yet |
| `IWorldInteractionVisualComponent` | `Set_Interaction_Highlight` | nothing yet |

`IRenderComponent`'s signature is `RenderObjClass::Render`'s on purpose: the two pooled renderers
this project already built (P24's tracer, P25's debris) each wrap their pool in a private
`RenderObjClass` subclass so the scene can draw it, and both now also declare `IRenderComponent`
and forward to that same `Render`. Nothing about their behaviour changed -- the point is that the
two things the roadmap's Initial-components list already names as components (`tracer renderer`,
`debris renderer`) now *say so in the type system*, so a future caller can hold "a thing that
draws" without holding "a scene object".

**Four of the five have no implementer yet, and that is the honest state.** Their consumers --
the selection/status renderer, the waypoint/order renderer, the ghost-building renderer, the
Commander sidebar that would gate an RTS overlay -- are all later phases in the roadmap's own
dependency order (`ghost placement`, `Commander sidebar`). Writing a contract now and leaving it
unimplemented until the feature that needs it exists is the same discipline the project already
applies to missing art: the absent case behaves sanely rather than being invented early to look
finished. What this phase buys is that when those phases land they add a component, not another
virtual on `GameObj`.

## Part 2 -- `CompositeAttachmentClass`, the generalized attachment layer

The section lists seven mechanics to extract. **Two of them needed no new code at all**, and
saying so is more useful than writing a wrapper that pretends otherwise:

- *parent-relative transform propagation* -- already automatic. A sub-object added with
  `RenderObjClass::Add_Sub_Object_To_Bone` rides its bone through the `HTree` with no per-tick
  reposition anywhere. The stock soldier weapon attachment (`Code/Combat/soldier.cpp`,
  `WeaponRenderModel` on `GUNBONE`) has never had one, and neither does this.
  (`PhysicalGameObj::Attach_To_Object_Bone` *does* re-teleport every `Post_Think`, but that is a
  different mechanism: it attaches a whole physics object, not a render sub-object.)
- *LOD/culling propagation* -- already `HLodClass`'s job (`Code/ww3d2/hlod.h`:
  `Prepare_LOD`/`Increment_LOD`/`Decrement_LOD`/`Set_LOD_Level`), which walks sub-objects for
  every bone-attached child, stock Renegade's included.

The remaining five are what the class implements:

| Requirement | API |
|---|---|
| child model attached to a named W3D node/bone | `Attach(host, slot, bone, model_name)` |
| runtime add/remove child visual | `Attach` / `Detach(slot)` / `Detach_All()` |
| independent child animation state | `Set_Slot_Animation(slot, anim, looping)` |
| visibility/damage-state propagation | `Set_Slot_Hidden(slot, hidden)` |
| turret/module attachment transforms | the bone name is the transform -- see below |

### Slots own their host

Each slot records its own host model and holds a reference to both host and child. This is what
makes one implementation serve both consumers: a vehicle has exactly one model to attach to, a
**building has none** -- it is a set of separately-collected aggregates and meshes with no model
of its own (`building.h`'s own note above `Get_Bounds`). A controller whose slots each remember a
different host handles both without a second class, which is the section's explicit instruction:
*one generalized attachment implementation, not one special class per donor unit.*

It also removes a footgun: `Detach` and the destructor need no host argument and cannot be given
the wrong one.

### Missing art

`Attach` resolves the child through `WW3DAssetManager::Create_Render_Obj`. A name with no art
yet returns `false`, occupies no slot, logs through `WWDEBUG_SAY`, and leaves the controller
untouched -- the project's missing-art rule, and the behaviour the automated check pins. No
placeholder model is ever substituted.

### Turret/module attachment transforms

There is no separate turret API, because there does not need to be one. A turret attachment *is*
a child on a named bone; the bone's animation or the existing `VehicleGameObj` turret bone
capture (`TurretBone`/`BarrelBone`, `Aquire_Turret_Bones`) supplies the motion, and the
attachment rides it. Adding a parallel turret-transform path would be a second implementation of
something `HTree` already does.

## Consumers wired this pass

- **Vehicles.** `VehicleGameObj::Attach_Visual_Module(slot, bone, model)` /
  `Detach_Visual_Module(slot)` / `Set_Visual_Module_Damaged(slot, damaged)`. The controller is
  created on first use, so a vehicle that never carries an extra module pays nothing. This is
  the "special vehicle visual controller" from the Initial-components list, and it is what a
  Zero Hour Overlord-style upgrade (a bunker, a propaganda tower, a gattling turret) would hang
  off -- as data passed in, not as a subclass.
- **Buildings.** `BuildingGameObj::Attach_Visual_Module(slot, bone, model)` and the same two
  companions. Because a building has no model, **the bone name alone selects the host**: the call
  searches the collected `Aggregates`, then `ExteriorMeshes`, for the first component whose model
  actually carries that bone. That is what makes the layer "consumable by the building semantic
  descriptor system", which addresses building geometry by W3D node name rather than by model --
  the same addressing the roadmap's automatic PCT/MCT placement will need.
  `Get_Bone_Index` answers `0` both for "root" and "no such bone" (the convention
  `VehicleGameObj::Aquire_Turret_Bones` already reads it by), so a module cannot be attached to a
  component's root -- the right refusal, since a module with no named place on the geometry has
  nothing to be positioned by.

Neither consumer names a model of its own, so **no new asset names were invented this pass** and
there is no `docs/assets/` entry to make: every model name is supplied by whatever calls
`Attach_Visual_Module`. The first caller that hardcodes one owes that file an entry.

## Checks

`renegade -shaderselfcheck attachments`, registered as ctest `shader_attachments` and
`fds_shader_attachments`. Device-less. It constructs a bare `WW3DAssetManager` -- safe here
because its constructor only registers prototype loaders and allocates a hash table, touches no
DirectX, and defaults load-on-demand off, so every name resolves against an empty prototype table
-- and a minimal `RenderObjClass` subclass as a host. What it pins:

- a fresh controller holds no slots and reports none attached;
- attaching a child model with no art returns false, leaves zero slots, and does not report the
  slot attached -- the missing-art refusal;
- `Detach`, `Set_Slot_Hidden` and `Set_Slot_Animation` against a slot that was never filled are
  safe no-ops, and `Set_Slot_Animation` reports false rather than succeeding;
- `Detach_All` on an empty controller is a no-op.

**Not covered by any automated check**, and worth being explicit about: every *success* path.
Attaching a real model to a real bone, the child's independent animation actually playing,
`Set_Slot_Hidden` actually hiding a visible module, a building's bone search actually finding a
component, and reference release on detach all need real `.w3d` art and, for the building path, a
live combat scene with collected components -- neither of which `-shaderselfcheck` has. The code
follows the verified stock precedent for each (`soldier.cpp`'s `Add_Sub_Object_To_Bone` +
`Remove_Sub_Object` + `Release_Ref` sequence for attach/detach, `explosion.cpp`'s
`Get_HAnim`/`Set_Animation`/`Release_Ref` sequence for animation), but none of it has run.

## D-001 P26: the contracts now have implementers

Each contract now has a real implementer, adopted once the feature it was written for existed:

| Contract | Implementer | Reached from |
| --- | --- | --- |
| `IClientUpdateComponent` | `CommanderSelectionOverlays`' ring manager | `CombatManager`'s frame, through `Get_Update_Component()` |
| `IRTSOverlayComponent` | The command palette, the power panel and the governance panel | `HUDClass::Render`, which draws each only when `Is_RTS_Overlay_Visible` |
| `ISelectionVisualComponent` | One status ring per selected unit or building | The ring manager; `Set_Selected` creates or removes the ring |
| `IWorldInteractionVisualComponent` | The placement ghost's valid/invalid tint | `CommanderGhostRenderer::Update` |

The success-path attachment test is still open: a real model attached to a real bone, a child
animation playing, `Set_Slot_Hidden`, and a building's bone search. It is listed under D-001
priority 6 in `WIP_features.md`.

## Success path on real art (D-001 priority 6)

`shader_attachments` now also loads the retail Barracks interior (`mgbar_int.w3d`, hierarchy with
the `MGBAR_MCT~` pivot) and doors (`mgbar_doors.w3d`, its own embedded animation) from
`data/building_models`, headless, and checks: the doors attach on the MCT bone and appear among
the host's sub-objects; their animation plays and a missing one is refused; hiding and showing the
slot hides and shows the model; a descriptor's `mct_anchor` role resolves through the same
hierarchy to the bone's transform; and detaching removes the model from the host.
