# Projected / cached shadows

Roadmap Section 24. Donors: Zero Hour `W3DProjectedShadow`, `W3DShadow`,
`W3DVolumetricShadow`, `W3DBufferManager`.

Acceptance: *there is one shadow implementation serving both TT-facing APIs and new world
systems.*

## What is here

| File | Holds |
|------|-------|
| `Code/WWPhys/worldshadowmanager.h/.cpp` | `WorldShadowManager` — the one implementation |
| `Code/WWPhys/pscene_projectors.cpp` | the projector pass, and nothing else about shadows |
| `Code/WWPhys/pscene.h`, `pscene.cpp` | the TT-facing names as forwarders; the settings members are gone |
| `Code/WWPhys/dynamicshadowmanager.h/.cpp` | the TT-facing per-object class, now three lines of delegation |
| `Code/WWPhys/dyntexproject.cpp` | clears its dirty flag when the picture is actually taken |
| `Code/ww3d2/rendobj.h`, `animobj.h` | `Get_Animation_Frame` — the pose stamp a cache needs |
| `Code/Commando/terrainselfcheck.cpp` | `Check_Shadows`, run as `terrain_shadows` and `fds_terrain_shadows` |
| `Code/Commando/consolefunction.cpp` | `shadow_status`, `shadow_mode`, `shadow_invalidate` |

## There were four shadow systems, and none of them was the owner

Before this, the shadow system was spread across four places, none of which could answer a
question about it on its own:

- **`PhysicsSceneClass`** held the mode, the attenuation distances and the intensity as
  member variables, and exposed them under the names TT spells.
- **`pscene_projectors.cpp`** held two texture managers as *file statics*
  (`_StaticShadowTexMgr`, `_DynamicShadowTexMgr`), which is where the resolutions, the render
  targets and the shared static shadow textures actually lived. Nothing outside that
  translation unit could see them.
- **`DynamicShadowManagerClass`** held the per-object policy — which light, blob or rendered,
  how far is too far — and only two classes in the engine embed one.
- **The `STATIC_PROJECTOR_RESOLUTION` constant** was a `const int` in a `.cpp`, so the static
  half of the system had a resolution nothing could set, while TT's header names
  `Set_Static_Shadow_Resolution` as though something could.

All of it is now in `WorldShadowManager`, and each of those four is a caller. The scene's
`Set_Shadow_Mode`, `Set_Shadow_Attenuation`, `Set_Shadow_Normal_Intensity`,
`Set_Shadow_Resolution` and `Set_Max_Simultaneous_Shadows` still exist and still mean what
they meant — they forward. TT's `Set_Static_Shadow_Resolution`,
`Get_Static_Shadow_Resolution`, `Set_Dynamic_Shadow_Resolution`,
`Get_Dynamic_Shadow_Resolution` and `Generate_Static_Directional_Shadow(obj, light_dir)` were
named in TT's headers and missing from OpenW3D; they exist now, on the same owner.

That the two vocabularies still agree is not left to inspection. The self-check asserts that
every `PhysicsSceneClass::SHADOW_MODE_*` equals its `WorldShadowManager::SHADOW_MODE_*` and
that the two enums are the same length, because a silent divergence there is exactly how a
claim of "one implementation" dies.

## The other word in the section title is "cached"

Stock Renegade re-rendered **every visible shadow, every frame**. `Update_Shadow` ended with
an unconditional `Set_Texture_Dirty(true)`, and `Pre_Render_Update` obeyed it.

That was not laziness, it was forced. The render targets came from
`DynamicShadowTexMgrClass`, which handed them out from a ring reset by `Per_Frame_Reset()`
every frame. A projector never held the same target twice running, so whatever it rendered
last frame had already been overwritten by somebody else. Caching was impossible, so the
dirty flag was meaningless, so it was always set.

The fix is ownership. A projector now **holds** its render target for as long as it keeps
being drawn:

- `Acquire_Render_Target` returns the slot this projector already had, if it has one.
- A slot is only taken away from a projector that stopped asking for it, or — if every slot
  was asked for this frame — from the holder furthest from the camera, which is the same rule
  the projector pass already used to decide which shadows are worth drawing.
- Every eviction is counted.

With ownership real, the dirty flag becomes real, and the question "does this picture need
taking again?" can finally be asked.

## What makes a shadow picture stale

`ShadowStampClass` is the whole answer, and it is deliberately small:

| Field | Why it invalidates |
|-------|--------------------|
| `LightVector` | the sun moved, so the silhouette moved |
| `Orientation` | the object turned; under an orthographic light this is the only motion that changes the picture |
| `Model` | it is a picture of a different model now |
| `AnimFrame` | the pose advanced |
| `Generation` | something invalidated everything — a mode change, a resolution change, `shadow_invalidate` |
| `Perspective` | never matches, because a perspective projection depends on *position* too |

**Translation is deliberately absent.** The sun is a directional light and the projection is
orthographic, so a vehicle driving in a straight line across flat ground casts exactly the
same picture the whole way. The projection *matrix* is rebuilt every frame regardless — it is
a handful of multiplies — and only the render-to-texture is skipped. That is the same split
the donor makes: `W3DProjectedShadow::update()` re-renders when the light moved and only
recomputes the projection when the object moved.

`Orientation` compares as a quaternion dot, which is the cosine of half the angle between two
rotations, against `0.9999` — a little under two degrees. `q` and `-q` are the same rotation
so the sign is dropped.

Zero Hour got the same effect by rendering into one scratch target and **copying** the result
into a permanent per-shadow texture. That works but pays a surface copy on every update. Here
the shadow renders straight into the texture it owns, which is cheaper and needs no copy path
— the difference is only possible because OpenW3D can afford one render target per
simultaneous shadow, and the number of simultaneous shadows was already bounded.

### The one thing the flag had to learn

`DynTexProjectClass::Pre_Render_Update` never cleared `TEXTURE_DIRTY`, because nothing ever
needed it cleared. It clears it now — **and only when `Compute_Texture` returned true.** A
render that did not happen must not be recorded as a fresh picture, so the manager reads the
flag back afterwards and commits the new stamp only if the flag actually went down. A shadow
that found no receiver, or had no render target, keeps its old stamp and stays stale.

## Terrain reception, traced rather than assumed

Section 24 asks for terrain reception and "arbitrary receiver support where practical". For
generated terrain this already works, and the chain is worth writing down because it is the
kind of thing that is assumed and then quietly untrue:

`WorldTerrainSystem::Build_Collision_Patch` wraps each `RenegadeTerrainPatchClass` in a
`StaticPhysClass` and calls `Add_Static_Object`, which puts it in `StaticCullingSystem`.
`Collect_Visible_Objects` sorts everything in that system into `VisibleStaticObjectList` or
`VisibleWSMeshList`. `Apply_Projector_To_Objects` collects static objects intersecting the
projector volume and accepts any that appear in either list, calling `Add_Effect_To_Me`.
`PhysClass::Push_Effects` turns that into `rinfo.Push_Material_Pass`. And
`RenegadeTerrainPatchClass::Render` loops `rinfo.Additional_Pass_Count()` and draws each one
through `Render_Procedural_Material_Pass`.

Every link exists. Roads, bridges and water reach the same place by the same route, because
they are all `StaticPhysClass` in the same culling system.

## What a world system does that is not one of the two classes

Casting was the narrow part. A dynamic shadow existed only if the object was a
`MovePhysClass` or a `DynamicAnimPhysClass`, because those are the only two classes that
embed a `DynamicShadowManagerClass`. Nothing else in the engine had any way in.

`Register_Caster` is the way in. A registered object is driven by
`WorldShadowManager::Timestep`, which runs next to the ribbon and mark ticks in
`PhysicsSceneClass::Update`, and it goes through **the same `Update_Object_Shadow`** the
embedded managers call. There is one function that decides what an object's shadow looks like
this frame, and gaining a second kind of caster did not add a second copy of it.

The table is capped at 64 and refusals are counted, for the same reason every other table in
this tree is capped: a shadow needs a render target, there are at most 32 of those, and a
caster list that grows with the content is a list that will one day be longer than anything
that can draw it.

## Bounds

| Bound | Value | What it stops |
|-------|-------|---------------|
| Render-target slots | ≤ 32, mode-driven default 0/1/4 | the number of shadows rendered per frame |
| Resolution, both kinds | power of two, 16..256 | a mod asking for a 4096 shadow map |
| Registered casters | 64 | a world system registering per-instance |

`Set_Max_Simultaneous_Shadows` is still driven by the mode, exactly as stock Renegade drove
it: none and blobs get zero, blobs-plus gets one, hardware gets four. That is the knob a
player actually turns, and it remains the knob.

## The dedicated server needs no special case

`Create_Render_Target` asks the device for a format and returns null when there is no device
or when the device refuses. `Acquire_Render_Target` counts that as a missing target, gives
back the slot and returns false, and the projector pass simply does not draw that shadow.
Nothing asserts, nothing is created, and a headless build allocates no shadow memory at all —
without a single `if (dedicated)` anywhere in the system.

That is also what makes the self-check meaningful: it runs with no device, so the refusal
path is the path it exercises.

## What was taken from the donor and what was not

**Taken — cache-and-invalidate.** `W3DProjectedShadow::update()` re-renders only when the
light has moved and recomputes only the projection when the object has moved. That split is
the core idea and it is the one adopted, widened to also notice the object turning, the model
changing and the pose advancing.

**Taken — sharing by kind.** `W3DShadowTextureManager` keys cached shadow textures so
instances of the same model share one. OpenW3D already did this for static shadows, keyed by
definition id plus orientation within ten degrees; it moved here intact and is now reachable
rather than being a file static.

**Not taken — `W3DVolumetricShadow`.** Stencil shadow volumes are a different technique with
a different cost model and a different look, and Section 24's Implement list asks for
projectors, caching and batching, none of which describe a volume. Renegade's world is
authored around projected shadows and a volume pass would have to be given its own geometry
preparation for every model in `always.dat`.

**Not taken — `W3DBufferManager`.** It is a vertex/index buffer suballocator for the SAGE
renderer. OpenW3D's shadows do not build geometry — they render an existing model into a
texture and then project it — so there is nothing for it to suballocate. The batching Section
24 asks for is already what the projector pass does: one material pass, applied to every
receiver the projector volume touched, in one collection.

## Not done

- **Nothing registers a caster yet.** The path exists, is bounded, is counted and is checked;
  what it is for is the world systems that move — the Commander ghost building, procedural
  construction blocks, produced AI that is not a `MovePhysClass`. Everything built so far
  (terrain, roads, bridges, foliage, water) is static geometry that wants to *receive*, and
  does.
- **Local lights still do not cast.** Stock Renegade carried a disabled local-light branch
  inside `Update_Shadow`, behind `#if 0` with a "FIXME (gth) Disabling local shadows" note.
  It did not move here; a superseded path is not kept. Adding it back means collecting
  shadow-casting lights near the object and calling `Enable_Perspective(true)` with the light
  position, and it would be correct from the first frame — `ShadowStampClass` already refuses
  to consider a perspective projection cacheable, because such a picture depends on where the
  object is and not only which way it faces.
- **No shadow has been looked at.** The checks run device-less. `shadow_mode 3`, then
  `shadow_status` while standing still and again while running, is the thing that says
  whether the renders really drop to the cache hits.
- **Static shadows are still generated in one batch at load.** `Generate_Static_Shadow_Projectors`
  walks every `StaticAnimPhysClass` that wants one. `Generate_Static_Directional_Shadow` now
  exists for re-shadowing a single object, but nothing calls it — a moving sun would.
- **No art gap.** The blob shadow texture is stock Renegade's `shadowblob.tga`; every other
  shadow in the system is a picture of a model taken at runtime. There is nothing for
  `docs/assets/` to list.
