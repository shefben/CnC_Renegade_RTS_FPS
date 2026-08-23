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
idempotent, and that re-initialising leaves nothing alive behind it.

None of it needs a device, which is the point: those are decisions the manager makes on
its own, and every pipeline added later inherits whatever they get wrong.

The checks deliberately avoid every path that ends in `WWASSERT` -- asking for an
unregistered program, registering twice over. Those are caller mistakes, and proving
them would mean tripping an assert in the build that has them.

## What is left

The pipelines Section 15 lists are enumerated and unregistered: terrain, terrain
detail, roads, bridges, water, foliage, projected shadows, particles, tracers and
beams, status markers, ghost building tint, debug overlays. `Is_Supported` reports
false for each and `Get_Pass_Count` returns zero, so a caller that loops over them
draws nothing rather than drawing them wrongly.

They are unregistered because most of them have nothing to draw yet. Terrain, terrain
detail, roads, bridges and water arrive with the terrain framework (roadmap Sections 17
and 18); foliage, ghost tint and status markers arrive with the Commander work. Each
one registers itself when the system that draws it lands, which is the order Section 15
asks for -- the layer first, because it is a dependency of the systems, not the other
way round.
