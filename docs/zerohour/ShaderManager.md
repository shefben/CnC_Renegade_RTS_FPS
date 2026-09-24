# The shader and state management layer

Roadmap Section 15, Zero Hour / SAGE Feature 14. Donor: `W3DShaderManager` in
`GeneralsGameCode/Core/GameEngineDevice`, migration class `ADAPTED_BACKPORT`.

The donor system exists because W3D2 could not express what SAGE terrain, roads and
water needed, and because every card of that era needed a different route to the same
picture. The first half of that is still true here and is the reason to have this at
all. The second half is not, and is the reason this is an adaptation rather than a
port.

## What it is

`ShaderManagerClass` in `Code/ww3d2/shadermgr.h`. A registry of named material
programs, one current at a time, plus the textures those programs read.

A `MaterialProgramClass` answers four questions: what it is called, whether it can run
on this device, how many passes it needs, and what device state each pass wants. It is
also required to put back what it changed, which is the whole point of routing through
one layer rather than letting each system reach into `DX8Wrapper` on its own.

`Set_Program` resets the previously current program before making a new one current, so
state never leaks from one pipeline into the next. Moving between passes of one program
does not reset it: a program setting up its own second pass knows what its first one
did, and forcing it to start from nothing would be the layer getting in the way of the
pipeline it exists to serve.

## Stock content is a program

`MATERIAL_PROGRAM_LEGACY_W3D` is every Renegade material ever exported. One pass, and
that pass sets nothing, because a W3D material already carries its own `ShaderClass`
and `VertexMaterialClass` and the renderer applies them as it draws.

It is registered rather than special-cased on purpose. If stock rendering were a bypass
around the manager there would be two layers, which is exactly what directive 0.4
forbids, and the question "did the new layer change how a 2002 model looks" would have
no structural answer. As it stands the answer is that the layer has nothing to say
about it: a stock model renders in precisely the state it rendered in before this class
existed.

## Debug overlays, the first real pipeline

`MATERIAL_PROGRAM_DEBUG_OVERLAY` is the boxes the engine draws over the world when a
display mask is on — collision boxes, bounding boxes, vis sectors. It is registered
in `ShaderManagerClass::Init` alongside stock content because, alone in the list
Section 15 gives, its consumer was already in this tree.

That consumer is `BoxRenderObjClass::render_box`, which set a material, a shader and a
texture stage straight on `DX8Wrapper` and then left — the exact arrangement the
layer exists to replace. The material it used was a file static built in
`BoxRenderObjClass::Init`; the program owns it now, and the static is gone rather than
kept beside it, which is directive 0.4 applied to a path this project wrote itself.

The program is worth having as more than a demonstration: it proves the layer works
against something that really draws, and it is the first program that builds a device
resource in its `Init`, which is why the check below registers it in a process with no
device rather than assuming that is safe.

Its `Reset` clears the texture stage and nothing else. Restoring a null material would
be inventing a state nobody asked for: every draw in this renderer sets its own
material and shader, so what the next pipeline needs is a stage it did not inherit.

## The pipelines, and why they are all in one file

Six more programs are registered, each with a consumer that already draws: terrain, roads,
bridges, water, foliage and projected shadows. They are all `FixedFunctionProgramClass` —
a shader, a material and a count of texture stages — because that is genuinely all any
surface in this engine is drawn by, and it is what lets them be compared.

They are built in `ShaderManagerClass::Init` rather than by the systems that draw them.
The earlier sketch had each system register its own on the way up, and that was the wrong
shape once there were six of them: a pipeline is a description of device state in ww3d2's
own vocabulary, its lifetime is the manager's rather than any system's, and splitting the
six across six files in wwphys reproduces exactly the scatter this layer exists to remove
— it would only be scatter that called `Register_Program` on the way past. The dependency
still runs the way Section 15 asks: the systems know about the layer, and the layer knows
nothing about the systems.

Having them side by side is the point rather than a tidiness. Water blends and does not
write depth; foliage alpha-tests and does. Both disable culling, for different reasons —
a lake seen from underneath, a leaf sheet seen from behind. Those two facts are eleven
lines apart in `shadermgr.cpp`, and were previously in two files that had never been read
together.

## Two ways to consume a pipeline

This renderer submits geometry two ways, and a pipeline has to serve both.

Terrain, the debug overlays and the projected shadow pass set device state and then draw.
That is `Set_Program`/`Reset_Program`, which is what the class was built around.

Roads, bridges, water and foliage build a `DynamicMeshClass` and carry the state on it;
what the device ends up in is set by the mesh as it draws, long after any call this class
could make. They ask `Get_Pass_State`, which hands out the identical `ShaderClass` and
`VertexMaterialClass` that `Set_Pass` would have installed — the same stored objects, not
a second description of them. A road mesh and a terrain draw therefore cannot disagree
about what their pipeline is, because there is only one of it.

`Get_Pass_State` is also the one accessor that brings the manager up on its own. Nothing
in the table is a device resource, and the dedicated server builds the same road, bridge,
water and foliage geometry a client does — it is the same code and the same collision —
while never reaching the device init that normally starts this class. `Detect_Tier`
answers fixed function with no device, so coming up early is correct and not merely safe;
a device arriving afterwards re-detects the tier and rebuilds the table, which is what it
already did when a device changed.

For the same reason the materials are built here rather than fetched from
`VertexMaterialClass::Get_Preset`. The presets are created by `VertexMaterialClass::Init`,
which the server never reaches, and `Get_Preset` dereferences a null pointer there. It
also means no pipeline can reach through a preset and change a material every mesh in the
engine shares.

## What terrain gave up

`RenegadeTerrainPatchClass` used to carry four members — two `VertexMaterialClass` and two
`ShaderClass` — built per patch in `Initialize_Material`. Every patch on a map held its own
copy of two materials that were always identical, and `Render` mutated one of them in place
when the patch turned out to be prelit, which nothing put back.

That is now the terrain program's two passes: base and layer, differing in exactly one
thing, opaque against alpha, and agreeing about everything else including fog — a hillside
that stopped being fogged where its second layer started would be a visible seam. Whether
the base pass's colour comes from the light environment or from a vertex the lighting solve
baked is a fact about the patch and not about terrain, so the caller says which with
`Set_Program_Variant`, staged the way a texture is. The layer pass always takes colour from
the vertex; that is where its blend weight lives, which is why the vertex-lit base turns out
to be the same material object the layer draws with.

The patch stages its layer texture through `Set_Texture` rather than setting stage zero
itself, so the staging API this class was written with finally has a consumer. `Reset` gives
the stages back *and* clears the staging area, so the next pipeline has to say what it draws
with rather than inheriting what the last one left — and so a level's texture is not held
alive here by a system that has since been torn down.

## Tiers instead of a card table

The donor chose implementations from a table of chipsets, vendor IDs and driver
versions. That was a reasonable way to survive 2001 hardware and it is a liability now,
because such a table answers wrongly for every card released after it was written.

`Detect_Tier` asks the device instead. Two tiers: `SHADER_TIER_FIXED_FUNCTION`, which
is texture stages and render state, and `SHADER_TIER_PROGRAMMABLE`, which additionally
has vertex and pixel shaders. Both halves have to be present for the higher tier -- a
vertex shader with no pixel shader to receive its output is not a tier worth writing
against.

With no device -- a dedicated server, or before the renderer comes up -- the answer is
fixed function, which is both the safe reading and the true one, since every program
has to be able to fall back to it anyway.

A program that wants hardware the tier does not have simply returns `false` from
`Init`, and is dropped. That is a normal answer rather than a failure, and it is how a
pipeline offers a shader path and a fixed-function path without the manager knowing
anything about either.

## Wiring

| Site | Call |
| --- | --- |
| `Code/ww3d2/dx8wrapper.cpp`, `Do_Onetime_Device_Dependent_Inits` | `ShaderManagerClass::Init()` after `Compute_Caps` and the subsystem inits |
| `Code/ww3d2/dx8wrapper.cpp`, `Do_Onetime_Device_Dependent_Shutdowns` | `ShaderManagerClass::Shutdown()` before the texture manager goes |
| `Code/ww3d2/boxrobj.cpp`, `render_box` | `Set_Program(MATERIAL_PROGRAM_DEBUG_OVERLAY,0)` and `Reset_Program()` around the draw |
| `Code/wwphys/renegadeterrainpatch.cpp`, `Render_By_Texture` | stages the layer texture, then `Set_Program(MATERIAL_PROGRAM_TERRAIN,pass)`; `Set_Program_Variant` from `Render` |
| `Code/wwphys/pscene_projectors.cpp` | `Set_Program(MATERIAL_PROGRAM_PROJECTED_SHADOW,0)` around the coverage draw |
| `Code/wwphys/roadsystem.cpp`, `bridgesystem.cpp`, `watersystem.cpp`, `foliagesystem.cpp` | `Get_Pass_State` onto the batch mesh as it is built |

The device lifecycle already owns the other ww3d2 subsystems, so the layer comes up and
goes down with the device rather than with the process. Coming up again after a device
change tears down what was registered first.

## Pixel shader version

`DX8Caps::Get_Pixel_Shader_Majon_Version` and its minor counterpart read
`VertexShaderVersion`, so both pixel shader accessors reported the vertex shader
version. Nothing called them, so nothing was wrong until the tier detection above
became the first caller. Fixed with them.

## What is checked

`renegade -shaderselfcheck programs`, registered as `shader_programs` and
`fds_shader_programs`. Registration, tier refusal and destruction of a refused program,
pass counts, which program is current, that handing over resets the previous program
exactly once, that moving between passes does not, that giving the device back is
idempotent, and that re-initialising leaves nothing alive behind it. It also checks that
*exactly* the pipelines with consumers are registered and no others — both halves matter,
since a missing one means a system draws in whatever state it inherits, and an early one
is a claim that something draws it.

`renegade -shaderselfcheck pipelines`, registered as `shader_pipelines` and
`fds_shader_pipelines`, pins what each pipeline actually draws in. That the manager comes
up on its own when a pipeline is asked with no device. That the material comes back
referenced, and twice asking gives the same object. That an unregistered pipeline and an
out-of-range pass both answer no without leaving a material behind. That terrain is two
passes, opaque then alpha, both fogged; that the variant switch changes the base material
and switches back; and that the vertex-lit base is the object the layer uses. That water
and foliage still differ in the way the design says they do. That roads and bridges are
solid and write depth. That the shadow pass is additive, untextured, depth ignored, and
has no material. And that stock W3D content has no pipeline state of its own, because a
Renegade material carries its own.

That last group is worth having rather than merely counting programs: the whole claim of
this layer is that "how is a road surface drawn" has one answer, and a check that only
asked whether the road program existed would pass just as happily if that answer quietly
became foliage's.

None of it needs a device, which is the point: those are decisions the manager makes on
its own, and every pipeline added later inherits whatever they get wrong.

The checks deliberately avoid every path that ends in `WWASSERT` -- asking for an
unregistered program, registering twice over. Those are caller mistakes, and proving
them would mean tripping an assert in the build that has them. The refused-tier program is
offered into `MATERIAL_PROGRAM_PARTICLE` for that reason: a slot nothing claims.

## What is left

**D-001 P09 update.** Three of the pipelines that used to be enumerated but unregistered now
have their renderers, and are registered:

| Program | Renderer | State |
| --- | --- | --- |
| `MATERIAL_PROGRAM_TRACER` | `TracerRenderer` | alpha, modulated by vertex colour, both sides |
| `MATERIAL_PROGRAM_STATUS_MARKER` | `StatusOverlayRenderer` | the same |
| `MATERIAL_PROGRAM_GHOST_TINT` | `CommanderGhostRenderer` | alpha, no depth write; the verdict's tint material stays the renderer's |

Each renderer now takes its state from `Get_Pass_State` and adds only its own fog name. Each
program answers with exactly the state its renderer used to build for itself, and
`shader_pipelines` asserts that bit for bit, so the move changes no picture.

Two pipelines remain unregistered, each for a reason:

- **Particles are retired, not faked.** Every emitter carries the shader its W3D definition
  authored (additive, alpha, screen...), so no single program can describe them. They keep the
  stock per-emitter path.
- **Terrain detail** is the one that looks ready and is not. It needs a second UV rate on a
  single layer, which the terrain pass cannot carry, and there is no detail entry in the layer
  table for it to draw.

`Is_Supported` reports false for both and `Get_Pass_Count` returns zero, so a caller that
loops over them draws nothing rather than drawing them wrongly.

The acceptance line *new donor systems share one state/shader management layer* is now met
in substance — six donor-derived systems draw through one layer and no longer decide their
own state — but the layer has still never been seen. The checks run without a device, so
whether terrain, water, foliage and the shadows still look the way they looked before this
change is a question only a run with a display can answer. That is the same manual step
already recorded against those systems.

Ground decals are a deliberate omission rather than an oversight. `SurfaceRibbonSystem` and
`WorldSurfaceMarkManager` each keep a private shader function of the same shape the six
above gave up, and the two are near enough identical to each other to be a real duplicate.
Section 15 enumerates no decal pipeline, and the program enum is a contract; adding one is a
design change belonging to whichever section owns decals, not something to slip in here.
