# The Section 52A visual-fidelity program: Phases 46-62

Roadmap Section 52A, complete -- all seventeen phases. Program-wide constraints that
bind every phase in the section: no new authored art required; no `.w3d` re-export or
LevelEdit re-solve; every phase is a runtime switch defaulting to current appearance until
enabled, and disabling it restores stock rendering exactly; appearance is not intelligence
(Section 30's firewall still holds -- nothing here changes what is knowable to a player or
server); shared state that must agree across a match is server-authoritative and replicated
as a small scalar set, never per-object; each phase extends its named canonical owner, it
does not fork a second pipeline behind a flag.

All seventeen phases build and link clean across `ww3d2`, `wwphys`, `combat`, `renegade` and
`renegadeserver`. Twelve device-less `-shaderselfcheck` families cover the state-machine and
arithmetic surface (`visualsettings`, `skyclock`, `lightcurves` for P46-50; `heightfog`,
`skyocclusion`, `skydriventoggles` for P51-55; `tonemap`, `bloom`, `windfield`, `wetness`,
`damagetint`, `weaponkick` for P56-62), each with an `fds_` twin, per
`docs/zerohour/TestCoverageCatalog.md`'s own convention.

**One correction to an assumption this document carried through P55.** The tree is Direct3D 9,
not Direct3D 8, despite every filename saying `dx8`; and `DX8Wrapper::Create_Render_Target`,
`Set_Render_Target` and `_Get_DX8_Back_Buffer` have existed all along -- the projected-shadow
system in `wwphys/pscene_projectors.cpp` has been using them since Section 24. What does *not*
exist is any programmable-shader path: `CreatePixelShader` appears nowhere in the tree, and no
material program uses the programmable tier even on hardware whose caps report it. P55's note
that "this codebase has no screen-space render-target post-process chain" was right about the
chain and wrong about the primitives. P56 and P57 build the chain out of what was there.

## P46: sampler/filtering/antialias quality defaults

**Owner:** `Code/ww3d2/dx8wrapper.cpp`, `texture.cpp`, `ww3d.h`/`.cpp`.

Three settings, each a static on `WW3D`, each defaulting to exactly what the engine already
did:

- `Set_Anisotropic_Level(int)` (default 2, clamped 1-16 against `dx8caps.MaxAnisotropy` at
  apply time) replaces the hardcoded `D3DSAMP_MAXANISOTROPY,2` `TextureClass::_Init_Filters`
  always wrote.
- `Set_Texture_LOD_Bias(float)` (default 0.0, the hardware default the engine never touched
  before) writes `D3DSAMP_MIPMAPLODBIAS` for the first time, bit-cast through the same
  float-as-DWORD convention D3D9 itself expects.
- `Set_Antialias_Quality(int)` (default 0 = `D3DMULTISAMPLE_NONE`, stock) is applied in
  `DX8Wrapper::Set_Render_Device`, right before `Create_Device()`, by walking down from the
  requested level through `IDirect3D9::CheckDeviceMultiSampleType` until one the device
  actually reports support for is found -- never a hard failure, only a graceful step down to
  0. Multisampling is a `D3DSWAPEFFECT_DISCARD`-only feature in D3D9, so the check is gated on
  that swap effect (true whenever windowed; fullscreen keeps `D3DSWAPEFFECT_FLIP` and stays
  unaffected, matching its own pre-existing behaviour).

**Not built:** `Create_Additional_Swap_Chain`'s secondary swap chains (tool/editor
multi-viewport use) stay `D3DMULTISAMPLE_NONE` -- out of scope for a first pass, and not
reachable from a normal client or server run.

## P47: level-of-detail and far-terrain cross-fade

**Owner:** `Code/ww3d2/distlod.cpp`, `Code/wwphys/worldterrainsystem.cpp`. Shared switch:
`WW3D::Get_LOD_Dissolve_Enabled()` (default off).

Neither owner has a generic per-object alpha to cross-fade with -- `RenderObjClass` exposes
no opacity control in this engine at all -- so both use the same substitute: a bounded
*overlap window* where the outgoing and incoming representations draw together instead of
the old hard, single-frame swap. Because both LOD tiers already agree exactly at the seam
(`DistLODClass`'s own two levels share geometry topology across LOD bands; the far/near
terrain split shares literal grid vertices, per `WorldTerrainSystem`'s existing
`FarTerrainDrop` margin), the overlap can only double-draw the same ground for its window,
never open a gap.

- `DistLODClass::Increment_Lod`/`Decrement_Lod` capture `PrevLod`/`OverlapTimer` (0.35s) when
  the dissolve switch is on; `Render()` draws `Lods[PrevLod]` alongside `Lods[CurLod]` for
  that window.
- `WorldTerrainSystem::Update_Terrain_LOD` gained a parallel `FarBlockLastNear`/
  `FarBlockOverlapTimer` array (same lifetime as `FarBlocks`/`FarBlockDirty`, allocated and
  freed alongside them) driving the same overlap-window logic per far/near block.

**Known limitation, recorded rather than hidden:** `DistLODClass` is flagged obsolete at
load -- `DistLODPrototypeClass::Create` converts every loaded W3D LOD chunk straight to
`HLodClass` and logs a warning. Stock content therefore never reaches `DistLODClass::Render`
at all; this phase's mesh-LOD half is real, tested, and exactly what the roadmap names as the
owner, but has no effect on today's actual assets until something is authored to use
`DistLODClass` directly, or a later phase extends the same idea to `HLodClass`. The
far/near terrain half has no such caveat -- it is the live path every level already uses.

## P48: server-authoritative sky clock

**Owner:** `Code/Combat/backgroundmgr.cpp` (`BackgroundMgrClass`), consuming into
`Code/wwphys/pscene_lighting.cpp` (`PhysicsSceneClass::Set_Sun_Light_Orientation`).

`BackgroundMgrClass` already replicated a weather-style parameter set through
`Export_Rare`/`Import_Rare` (`NETID_SERVER_BACKGROUND`/`APPPACKETTYPE_NETBACKGROUND`, both
pre-existing) but never replicated `_Hours`/`_Minutes`, and nothing before this phase ever
drove `PhysicsSceneClass`'s actual sun angle from them -- the sky dome's sun/moon sprite
position and the scene's real lighting direction were two independently-authored, never
reconciled things. This phase:

- Adds `_ClockDrivesSun` (bool), `_ClockRateHoursPerSecond` (float), `_ClockFractionalHours`
  (float), all new statics, all defaulting to off/0/0.
- `Set_Sky_Clock_Rate(float hours_per_second)` is the runtime switch: calling it at all --
  even with 0 -- turns the clock on. Server-only (`WWASSERT(I_Am_Server())`, matching every
  other `Export_Rare`-adjacent setter in this file).
- `BackgroundMgrClass::Update` advances the clock (server only, when the rate is nonzero),
  then -- whenever the clock is active, on both sides -- computes a stylised day arc (azimuth
  sweeps once per day, elevation peaks at noon, dips below the horizon at night; not
  astronomically exact, nothing in Section 52A asks for that) and calls
  `PhysicsSceneClass::Set_Sun_Light_Orientation`. `WorldShadowManager` and
  `PhysicsSceneClass::Compute_Static_Lighting` both already re-read the scene's sun fresh on
  every use (confirmed by source read, not assumed), so neither needed a single line changed
  for shadows to rotate and static lighting to respond.
- `_Hours`/`_Minutes`/`_ClockDrivesSun`/`_ClockRateHoursPerSecond` are now part of the
  `Export_Rare`/`Import_Rare` packet, so a late-joining client agrees with the hour from its
  first packet, not just from then on.

**Acceptance, checked against the actual text:** sun crosses the sky (day-fraction-driven
yaw) -- yes; shadows rotate and lengthen (consumers already re-read fresh) -- yes, by
construction, not by re-verifying each consumer's render output; night falls (elevation goes
negative, `SkyClass`'s own pre-existing day/night rendering already responds to `Hours`/
`Minutes`) -- yes; clients agree on the hour (replicated) -- yes; a zero rate reproduces
authored appearance -- yes, and so does never calling `Set_Sky_Clock_Rate` at all, which is
the actual default every level starts in.

## P49: time-of-day relighting of baked static lighting

**Owner:** `Code/wwphys/lightsolve.cpp` (the bake), `Code/ww3d2/mesh.cpp` (storage and
composition), driven from `BackgroundMgrClass::Update` (P48's owner) once the clock is
active.

`VertexSolveClass::Light_Mesh` already baked one `Vector4` per vertex (`UserLighting`,
installed unchanged, still exactly what renders when this phase is off). This phase adds a
second, independent bake computed in the same pass:

- `SunVis[vi]`: a geometric-only 0..255 scalar -- `N.L` against the sun's direction, zeroed by
  the exact same occlusion ray-cast `Add_Light_To_Vertex` already casts for every other
  light -- independent of the sun's actual colour, so it can be recoloured at runtime with
  whatever colour the sky clock says the sun is *right now*.
- `BaseSolve[vi]`: the same vertex, minus exactly what the sun alone contributed (captured by
  snapshotting `AmbientSolve`/`DiffuseSolve` immediately after the sun's own
  `Add_Light_To_Vertex` call and before any other light's). An approximation where the
  pre-clamp totals would have saturated (several stacked lights over 1.0) -- the subtraction
  happens before `Solve`'s own clamp, not after -- accepted as reasonable for a cosmetic-only
  term; noted rather than hidden.

`MeshClass::Install_Sun_Relighting_Data` stores both (new `SunRelightBase`/`SunVisibility`
arrays, same lifetime discipline as `UserLighting`) and registers the mesh in a small
file-local registry. Installing this data changes nothing by itself. `Apply_Sun_Relighting
(sun_color)` recomposes `UserLighting[vi] = BaseSolve[vi] + SunVisibility[vi]*sun_color`,
clamped, and rewrites the same array the render path already consumes -- no new vertex-colour
channel, no `VertexMaterialClass` rebinding, no second pipeline. Indoor vertices have
`SunVisibility == 0` by construction (occluded from the sun by definition), so they are
mathematically unchanged by any call to `Apply_Sun_Relighting` -- "indoor lighting does not
change" is not a claim to verify per-level, it falls out of the formula.

`BackgroundMgrClass::Update`, only while the sky clock is active, calls
`MeshClass::Update_Sun_Relighting(current_sun_color)` on a 2-second throttle (not per-frame --
the whole registry is walked, and the sun moves slowly enough that a two-second cadence is
not visible as a step).

**Not persisted.** Deliberately: it is cheap to regenerate at the same load-time pass that
bakes `UserLighting` in the first place, and Section 52A already rules out a second `.w3d`;
persisting it would mean a save-format change for derived data, which is the second-truth
problem this project's own rules argue against elsewhere.

**Not covered by the device-less self-checks:** constructing a real `MeshModelClass` needs
loaded `.w3d` content this binary does not have. What is proved by source reading rather than
by an automated check: the formula's indoor invariance, and that installing the data changes
nothing until something calls `Apply_Sun_Relighting`.

## P50: animated static lights

**Owner:** `Code/ww3d2/light.h`/`.cpp` (the curve), `Code/wwphys/worldlightmanager.h`/`.cpp`
(the group-id association), `Code/Combat/building.cpp` (the power/damage hook).

`LightClass` gains `LightCurveType` (`STEADY`/`FLICKER`/`PULSE`/`WARMUP`/`FAIL`, default
`STEADY`) and `Update_Light_Curve(dt)`, a no-op for `STEADY` at any cost. `Set_Light_Curve`
is careful about two things a naive version would get wrong, both caught and fixed by this
phase's own device-less self-check before being trusted:

- Setting the *same* curve twice is a no-op. `BuildingGameObj::Update_Group_Lighting` calls
  it on every `Enable_Power`, changed or not; without this, the very first such call would
  have stomped a level-authored `Intensity` back to the construction-time default the moment
  any building's power state was ever touched.
- Restoring `STEADY` restores the intensity the curve *started* from
  (`CurveBaseIntensity`), not whatever the curve's animation currently reads -- a `FAIL`
  curve that had ramped to zero and was then repaired must come back at full brightness, not
  stay dark because `STEADY` re-snapshotted a zero.

`WorldLightManager` gained a flat registry (every `LightPhysClass` in existence, static or
dynamic, registers/unregisters itself in its constructor/destructor -- cheap: a pointer chase
and an enum compare per light per frame for the ones nobody ever curved) and
`Set_Group_Curve(group_id, curve)`, which walks it filtering by `LightPhysClass::GroupID`.
`lightphys.h` has carried a `GroupID` field with a doc comment describing exactly this use
("the building code uses the Group ID to differentiate...") since before this project started
-- this phase is what finally implements the comment, not a new invention.

`BuildingGameObj::Update_Group_Lighting` (new, private) computes
`should_glow = IsPowerOn && !IsDestroyed` and calls
`WorldLightManager::Set_Group_Curve(Get_ID(), should_glow ? STEADY : FAIL)`. Wired into
`Enable_Power`, `On_Destroyed` and `On_Revived` -- every place those two flags actually
change, on both client and server (all three already run unconditionally on both; only the
network-authority half of each is server-gated, unchanged by this phase). Event-driven, not
polled: no new per-frame cost for buildings whose power never changes.

**What this depends on that is authoring, not code:** a light only follows a building when a
level author (or a spawn-time script) sets that `LightPhysClass`'s `Group ID` to that
building's own `Get_ID()`. No engine code can derive that association from geometry alone
without guessing a convention nothing has ever authored to; per this project's own
`docs/assets/` rule for exactly this situation, the mechanism is built, tested, and
documented, and the association is recorded as a data gap, not treated as a reason to leave
the code unwritten.

## P51: height fog tied to the horizon colour

**Owner:** `Code/ww3d2/dx8wrapper.cpp`/`.h` (the term), `ww3d.h`/`.cpp` (the settings),
`scene.cpp` (the one call site).

The fog colour already moved with the sky clock before this phase -- `SkyClass::Update`
calls `mainscene->Set_Fog_Color(Haze->Get_Horizon_Color() + colorsum)` every frame, and
`Haze`'s own colour already comes from `SkyClass::Set_Time_Of_Day`'s interpolation table, so
P48's clock was already driving it. What was missing, confirmed by reading `DX8Wrapper::
Set_Fog` and `SceneClass::Render`, was any height term at all: fixed-function `D3DFOG_LINEAR`
vertex fog, pure camera-space distance, one flat colour.

Rather than the specular-alpha-as-explicit-fog-factor technique (the only fixed-function way
to get true per-vertex height fog), which would mean touching every vertex format this engine
writes, this phase uses the same *approximation* class of technique 2002-era engines without
per-pixel shaders used: `DX8Wrapper::Set_Fog` now takes the viewer's world Z (the one call
site, `SceneClass::Render`, already has `rinfo.Camera.Get_Position()`) and pushes the
authored `start`/`end` distances out toward "never reached" as the viewer rises past a
configurable ceiling, scaled by `WW3D::Compute_Height_Fog_Scale` -- a pure function, 1.0 at or
below the ceiling minus the falloff band (byte-identical to today's fog), 0.0 at or above it.
Low ground keeps the level's authored fog exactly; high ground pushes it out of range. This is
camera-relative, not per-object -- a valley visible from a hilltop still reads by the camera's
own height, not the valley floor's -- an honest limitation of not touching per-vertex data,
not an oversight.

`WW3D::Set_Height_Fog_Reference(ceiling_z, falloff)` is a plain setting, not an authored
asset; nothing computes it from a level's actual terrain bounds this pass (a heightfield-
derived default is a reasonable follow-on, not attempted here since the feature is off by
default regardless of what the reference values are).

## P52: terrain and contact ambient occlusion

**Owner:** `Code/wwphys/lightsolve.cpp` (`VertexSolveClass::Light_Terrain`), `Code/wwphys/
lightsolvecontext.h`/`.cpp` (the new switch), `Code/wwphys/worldshadowmanager.cpp` (already
complete -- see below).

Every heightfield-generated patch (P11-B: `Build_Collision` turns each one into a
`RenegadeTerrainPatchClass`) and every stock, author-placed terrain patch light through the
same `Light_Terrain` pass and the same `VertexColors` array -- one lighting path for both, so
adding ambient occlusion here covers "generated terrain" and stock terrain identically, with
no second implementation for either.

`Compute_Sky_Occlusion` casts six rays per vertex (straight up the vertex normal, plus five
spread at a fixed cone angle), cosine-weighted, against `PhysicsSceneClass::Cast_Ray` -- the
same kind of ray `Compute_Sun_Visibility` (P49) already casts, just aimed at a sampled
hemisphere instead of one sun direction, and bounded to a 6-unit local test distance (creases
and nearby slopes, not distant terrain). The result multiplies the *ambient* term only, never
the direct sun/light terms, which already have their own per-light shadow test.

Gated by a new `LightSolveContextClass::Enable_Sky_Occlusion`/`Is_Sky_Occlusion_Enabled`,
deliberately distinct from the pre-existing `Enable_Occlusion`/`Is_Occlusion_Enabled` (the
direct-light shadow test, which has always defaulted **on**). Reusing that existing switch
would have silently added a new visual term to every solve already running with it on --
sky occlusion needed its own switch, defaulting off, so an ordinary solve reproduces today's
terrain lighting exactly.

**The dynamic-object contact-darkening half needed no new code.** `WorldShadowManager`
already has a complete, working blob-shadow tier (`SHADOW_MODE_BLOBS`): an orthographic blob
projected straight down under any dynamic object, affecting only static geometry
(`Enable_Affect_Dynamic_Objects(false)`), already wired to the console, the performance
settings dialog and `SystemSettingsClass` (`PhysicsSceneClass::Set_Shadow_Mode`). It defaults
to `SHADOW_MODE_NONE`, which this phase leaves alone -- Section 52A's own "defaults to current
appearance" rule means the default cannot change, only what a caller can opt into, and that
opt-in already existed before this phase touched anything.

## P53: moving cloud shadows

**Owner:** `Code/Combat/backgroundmgr.cpp` (`CloudLayerClass`).

`CloudLayerClass` already scrolls its own sky-dome texture by `Velocity` every frame
(`Render()`, pre-existing). This phase gives it a second, independent capability: up to three
`TexProjectClass` ground projectors (`GROUND_SHADOW_PATCH_COUNT`), each a multiplicative,
depth-independent ortho projection (`Compute_Ortho_Projection`, looking straight down,
`Init_Multiplicative`), staggered along the same `Velocity` direction and drifting at a fixed
world-space pace, wrapping back around a bounded radius from the origin -- the same technique
`WorldShadowManager` already uses for its own projector (`Compute_Ortho_Projection` called
fresh every frame; no render target, no texture regeneration, only the projection matrix
changes).

No authored mask exists or is required: `Build_Shadow_Mask_Texture` generates a small (128x128)
tiling value-noise texture once, named `ow_cloud_shadow_mask` (procedural, never loaded from
disk -- the `ow_` prefix is kept for consistency with `docs/assets/` even though nothing here
reads it from a file). A purpose-made replacement is recorded in `docs/assets/
VisualFidelity.md` as an improvement, never a prerequisite, exactly as Section 52A's own text
for this phase says.

Deliberately reused `CloudLayerClass::Velocity` -- the direction its own sky texture already
scrolls by -- rather than `WeatherMgrClass`'s separate wind (also real, also already
replicated, and the more literal reading of the roadmap's "same wind field as P58", which
does not exist yet). Using the cloud layer's own existing scroll keeps "the clouds that are
already drawn cast something" true in the most direct sense: the shadow moves with the same
value the clouds themselves do, not a second, independently-authored direction. `Enable_
Ground_Shadow`/`Update_Ground_Shadow` are new; `BackgroundMgrClass::Set_Cloud_Shadows_Enabled`
(default off) is the runtime switch, applied idempotently every frame from `Update()`.

## P54: sky-driven environment reflections

**Owner:** `Code/ww3d2/metalmap.cpp` (`MetalMapManagerClass`, unchanged), `Code/Combat/
backgroundmgr.cpp` (the new caller).

`MetalMapManagerClass::Update_Lighting`/`Update_Textures` were already complete, already
generate real procedural environment maps (`!m%02d.tga`, already wired into every metal
material through `WW3DAssetManager::Load_Procedural_Textures`) -- and already never called
anywhere but the standalone `W3DView` tool. Every metal surface in the actual game has
rendered with whatever the constructor's zero-initialized lighting left it since this system
shipped. This phase's entire job was wiring the existing generator into the runtime: a
2-second-throttled call from `BackgroundMgrClass::Update`, feeding `PhysicsSceneClass::
Get_Ambient_Light()`/`Get_Sun_Light()` -- the same source `lightsolve.cpp` already uses for
the terrain/mesh solve -- plus the camera's own forward direction as the Phong view term.
`Set_Metal_Reflections_Enabled` (default off) gates it; enabling it is the only thing that
changes.

## P55: sun shafts

**Owner:** `Code/ww3d2/dazzle.h`/`.cpp` (the sun position/visibility, extended), `Code/Combat/
backgroundmgr.cpp` (the draw, new), `Code/Combat/combat.cpp` (the one call site).

The roadmap names `SkyGlowClass` as already knowing the sun's on-screen position; reading it,
`SkyGlowClass` is a lightning/war-blitz billboard effect with no connection to the sun at all.
The actual owner of the sun's screen position is `_Dazzle`, a `DazzleRenderObjClass` that
`BackgroundMgrClass` already positions at the sun's world location every frame -- its
`Render()` already computes a clip-space position (`current_vloc`) and an occlusion factor
(`visibility`) each frame, both previously private and unread outside the class. This phase
adds public `Get_Screen_Position()`/`Get_Visibility()` getters, nothing else, to the owner
that already does the work.

**What this draws is a streak-sprite approximation, not a per-pixel radial blur.** A true
depth-occluded radial blur needs a screen-space render-target chain this codebase has not
built yet -- confirmed by reading `shadermgr.cpp`: Section 15's `ShaderManagerClass` is a
per-surface material/shading-state registry (terrain, road, water, particle passes), not a
post-process framework, and nothing in the tree does full-screen render-target ping-pong
(`DX8Wrapper::Create_Render_Target` exists but is used only for shadow-projector textures).
Building that framework from nothing, untestable without a device, was judged disproportionate
to one phase in a five-phase batch. Instead, `BackgroundMgrClass::Render_Sun_Shafts` -- called
once per frame from `combat.cpp`, right after the dazzle layer renders so the position/
visibility are this frame's, not last frame's -- draws six small additive glow quads (a
`Render2DClass`, the same 2D overlay primitive HUD elements already use) stepping from the
sun's screen position toward screen centre, sized and faded by distance along that step and
by `_Dazzle`'s own `Get_Visibility()` -- the same occlusion test the sun dazzle itself already
uses, not a second one. Gated off entirely (nothing drawn) when the sun is at or below the
horizon (`_UnitLightVector.Z <= 0`) or the screen position falls outside the view volume,
matching the acceptance line's wording exactly. The glow texture (`ow_sun_shaft_glow`, a
32x32 radial falloff) is generated once, procedurally, the same way P53's mask is.

**Recorded, not hidden:** this is real, visible, gated correctly, and reuses the sun's actual
occlusion test -- but it is a cheaper effect than "radially blur bright pixels... strongest
through windows, doorways and tree canopy" describes. A true per-pixel version is the natural
target once P56/P57's post-processing chain exists to share a render-target owner with, which
is exactly what Section 52A's own ordering note says P56/P57 are for.

## P56: tonemapping and eye adaptation

**Owner.** `ww3d2/postprocess.cpp` (new, and the shared owner P57 also uses),
`ww3d2/dx8wrapper.cpp`, `Combat/backgroundmgr.cpp`.

**What the pipeline allows.** Render targets are real and usable. Pixel shaders are not
available at all. So a per-pixel filmic tonemap operator is not on the table, and writing a
shader pipeline as a side effect of a tonemap phase would be a far larger and riskier change
than the phase asks for.

What *is* available is the hardware gamma ramp: a 256-entry per-channel lookup the hardware
applies to every pixel on the way out. That is a colour-grading ramp in the literal sense, and
it is where the exposure and the grading go.

**Implement.** `DX8Wrapper::Set_Gamma` was refactored to remember its arguments and to build
its ramp through a new `Apply_Gamma_Ramp`. Alongside it, `Set_Tone_Scale(exposure, tint)` folds
an exposure multiplier and a per-channel tint into the same ramp. An exposure of 1 and a white
tint reproduce `Set_Gamma`'s own output bit for bit, which is what makes the feature disabled
pixel-identical rather than approximately so; and the player's own gamma slider composes with
the tonemap instead of fighting it.

`PostProcessChainClass` owns the adaptation. Scene brightness is *not* measured off the frame
buffer -- reading a render target back costs a pipeline stall this engine has no budget for.
It is composed instead from three things `BackgroundMgrClass::Update` already holds: the sun's
colour after cloud attenuation, the scene ambient, and how much sky the camera can see.

That third term is what makes the phase work. At noon, indoors and outdoors have the same sun
and the same ambient; the only thing that differs is the roof.
`BackgroundMgrClass::Compute_Sky_View` casts five rays (straight up and four leaning out) using
the same static-geometry, projectile-group cast P52's terrain occlusion uses, so a soldier
walking past never darkens the world. Five rays a frame is far below what one bullet costs.

Exposure then approaches its target with asymmetric time constants -- 0.45s into the light,
2.5s into the dark -- because that asymmetry is what the eye actually has and what makes
stepping into a building read as adaptation rather than as a brightness slider moving.

The grading ramp comes from the sky clock: warm near the horizon, neutral overhead, a slight
cool cast at night. Deliberately gentle. A grade strong enough to notice on its own is strong
enough to make somebody's screenshot look wrong, and Section 52A's rule is that this may look
better and must never look worse.

**The shoulder, which is what makes this an operator rather than a brightness slider.** Raising
the exposure to bring a dark interior up does not help if everything it lifts past 1.0 is then
clipped flat: the shadows open and the sky turns into a white sheet, which is worse than the
picture we started with. So `Tone_Curve` rolls the top of the range off instead of clipping it,
using extended Reinhard -- `v * (1 + v/W^2) / (1 + v)`, which maps 0 to 0 and the white point to
exactly 1, is monotonic between, and is nearly linear at the bottom so shadows are not crushed to
buy the highlights back. That is the operator itself, not an approximation of one, and it is what
the acceptance line's "hold detail in the sky and in shadow at once" actually asks for.

A shoulder of 0 is a straight clamp, bit for bit what the ramp did before, so the default is
still exactly today's image.

**What remains global.** One curve applies to every pixel, because a gamma ramp is a per-channel
lookup and cannot be anything else. A *local* operator -- one part of the frame on a different
curve from another -- needs the programmable tier this build does not have. In practice the
difference shows only in a frame containing both a deep interior and open sky at once; the global
curve handles each well and the pair as a compromise. Recorded, not hidden.

---

## P57: bloom on emissive and dazzle sources

**Owner.** `ww3d2/postprocess.cpp` (shared with P56, as the phase text requires),
`ww3d2/dazzle.cpp`, `Combat/combat.cpp`.

**Implement.** The threshold stage is free. `dazzle.cpp` already marks every glare source in
the world and already computes, for its own draw, each one's screen position, visibility and
intensity. `DazzleRenderObjClass::Render` now hands those to `PostProcessChainClass::Add_Glare`
at the same point it submits itself to the dazzle layer -- so muzzle flashes, lamps, the sun,
Tiberium and beam weapons pick bloom up with no per-effect work, exactly as the phase says they
should. `Add_Glare` does its own threshold and off-screen rejection and returns immediately
while bloom is off, which is the default.

Blur and add happen together. Each source is drawn once as a soft radial sprite into the shared
render target; the target is then composited back over the frame four times, each tap wider and
dimmer than the last. The sum of those taps is the wide soft falloff a blur kernel would have
produced, obtained on a pipeline with no way to run one -- bilinear filtering on the way out of
the target does the smoothing. The chain allocates its target once, on first use, and
`Release_Targets` drops it on shutdown.

`Combat/combat.cpp` brackets it: `Begin_Glare` before the scene renders, `Render_Bloom` after
the dazzle layer has drawn and before the HUD, so the interface does not bloom.

**The chain's second customer.** Phase 55's sun shafts were implemented, in an earlier pass, as
a row of additive streak sprites, and `WIP_features.md` recorded plainly that this was an
approximation standing in for the per-pixel radial blur the acceptance text describes -- there
was no render target to blur into at the time. There is now, and Phase 55 has been reimplemented
on it: see the revision note at the end of this document. The streak-sprite path and its
procedural glow texture were **deleted**, not left beside the new one.

**Not verified on a device.** The mid-frame render-target switch follows the bracket pattern
`pscene_projectors.cpp` already uses (set, clear, draw, restore), but unlike the projectors this
runs inside the frame already being rendered rather than in its own `Begin_Render`/`End_Render`
pair. That difference is the one thing about this phase -- and about the Phase 55 rework that
shares it -- which genuinely needs a screen before it is turned on for anybody. Both are off by
default.

---

## P58: wind field applied to geometry

**Owner.** `Combat/WeatherMgr.cpp` (`WindClass`), `wwphys/foliagesystem.cpp`,
`ww3d2/part_buf.cpp`, `Combat/backgroundmgr.cpp`.

**The wind was already replicated.** This is the phase's most useful finding.
`WeatherMgrClass` is a `NetworkObjectClass`; its wind heading, speed and variability go over
`Export_Rare`; and both ends run the same two-octave gust function over them. Every client
already agrees about which way the wind blows, to the frame. Section 52A's "shared state is
server-owned, replicated as a small scalar set" was satisfied before this phase started, by
something written for rain in 2002. Not one new byte goes on the wire.

What was missing was consumers. `WindClass::Get_Velocity` had exactly one reader --
`WeatherEnvironmentRenderer::Set_Wind`, for precipitation.

**Implement.** `WeatherMgrClass::Update_Wind_Consumers` hands the live gusting vector to three
systems, each behind its own scale defaulting to zero.

*Foliage.* Sway is baked into the batch geometry at build time, keyed on each vertex's height
above its own instance's base and quadratic in that fraction, so a trunk is pinned exactly and a
crown moves most. `FoliageModelCacheClass` gained a `Height` measured once when the model is
cached. Because the sway is baked, drawing costs exactly what it cost before: no per-frame
vertex work, no second render path, no shader.

What makes it *move* is `FoliageSystem::Update_Wind`, which rebuilds two visible cells per
frame, round robin, and only when the wind has changed by more than a threshold. That is a
deliberate trade: the sway lags a gust by however long a full sweep takes, and in exchange the
cost is a hard constant that does not grow with the size of the forest.

Section 21's merged collision posts cannot move even in principle. They are separate objects
built by `Build_Proxy_Geometry` from the instance positions alone and are never touched by any
of this -- the wind moves the picture, not the blocker, kept structurally rather than by being
careful.

*Particles.* `ParticleBufferClass::Set_Global_Wind` adds a world acceleration to the one each
buffer already integrates. Added rather than replacing: an effect that deliberately rises or
falls keeps doing so and simply drifts while it does, which is what wind does to smoke.

*Clouds.* `SkyClass::Set_Cloud_Velocity` drives both layers from the same wind, the upper at
55% of the lower so the sky keeps its depth. P53's ground shadows already drift along
`CloudLayerClass::Velocity`, so the clouds and the shade they cast now move together.

---

## P59: wetness response

**Owner.** `Combat/WeatherMgr.cpp`, `wwphys/terraintexturesystem.cpp`.

**Implement.** One scalar, 0 dry to 1 soaked, integrated from the replicated rain density:
about twelve seconds to soak, about a minute to dry. Linear rather than exponential, because a
puddle fills and empties at a rate rather than a proportion -- and because linear actually
*reaches* zero, which is what makes "they dry out afterwards" true rather than asymptotically
nearly true. Like the wind, it needs no replication of its own: the input is already replicated
and both ends run the same integrator.

`Get_Sheltered_Wetness(sky_view)` is the shelter term. Rain falls downward, so how wet a surface
gets is how much sky it can see, and callers that know their own sky view ask with it.

The terrain response is the roadmap's own words made mechanical: wetness biases the layer table
toward its wet variants on low-slope ground. `TERRAIN_LAYER_SHORELINE` is already what "wet
ground" means in Section 18's table, so rain simply extends it inland. Water runs off anything
steeper than twenty degrees, so the bias falls to nothing there -- and it is capped at 60% of
the remaining dry weight, because a rained-on field is a darker field, not a beach.

**The object half.** Soldiers, vehicles and buildings respond through
`SurfaceTintEffectClass`, the same material effect Phase 61 uses -- and sharing it is the point.
Both phases want the same two passes over the same object (something multiplicative to darken it,
something additive on top), so giving each its own effect would mean two entries in every
object's effect list, two material passes pushed where one would do, and the same object walked
twice a frame. One effect carrying both terms is cheaper and more honest, which is why Phase 61's
`DamageTintEffectClass` was renamed rather than duplicated.

Two things happen to a wet surface, and both matter:

- **Albedo darkens**, multiplicatively. Water fills the surface's microscopic roughness, so light
  that would have scattered straight back out is bounced around inside the film and absorbed.
  That is why a wet pavement is close to half the brightness of a dry one, and why this is a
  multiply -- it takes light away rather than adding a colour. Slightly cool with it, because the
  film is a weak blue filter.
- **Specular lifts.** The additive pass runs with lighting *on* and its ambient and diffuse set
  to black, so the only thing lighting can contribute through it is the specular highlight.
  That gives a genuine view-and-light-dependent sheen that moves as the player moves. Without it
  the surface would only darken, and a surface that only darkens reads as being in shadow rather
  than as being wet.

**Shelter is per object, and is the reason the value is not global.** A truck parked under a
bridge stays dry; a soldier indoors stays dry.
`DamageableGameObj::Compute_Sky_View` casts one ray straight up against static geometry only --
the same cast Phase 52's terrain occlusion and Phase 56's camera sky view use, so a soldier
walking over a puddle never shelters it. Cached and refreshed every two seconds rather than every
frame: objects move between shelter and open sky rarely, and a raycast per object per frame would
be the most expensive thing in the phase by a wide margin.

Attached from `PhysicalGameObj::Post_Think`, because wetness is not a damage event and cannot
ride the damage path the way Phase 61's soot does -- an object that has never been shot still
gets rained on. Two compares per object per frame while the feature is off.

---

## P60: surface marks age with the weather

**Owner.** `wwphys/worldsurfacemarkmanager.cpp`, `wwphys/surfaceribbonsystem.cpp`.

**Implement.** The smallest change in the program, and the cleanest, because Section 35 had
already put the mechanism in place without knowing it. `Mark_Alpha` already fades a mark on its
age against its lifetime, and `Timestep` already ages it. Multiply the aging by a function of
wetness and everything downstream follows: marks laid before a storm fade sooner, clear sooner,
and free their slots sooner, with `Mark_Alpha` knowing nothing about weather.

`Compute_Weather_Fade_Rate` is exactly 1.0 at zero wetness -- so dry weather reproduces the
current fade timing by the multiplier being one rather than by anything being special-cased --
and 4.0 at full soaking. Pool size, group caps and eviction policy are Section 35's and are
untouched; a mark cleared early simply frees its slot early, which is what a mark reaching its
lifetime has always done.

`SurfaceRibbonSystem` (tyre tracks) uses the same function, so tracks and scorches wash out at
the same rate for the same reason.

---

## P61: progressive damage appearance

**Owner.** `wwphys/damagetinteffect.cpp` (new, beside `materialeffect.cpp` and
`transitioneffect.cpp`), `Combat/damageablegameobj.cpp`.

**The gap.** A building's entire visual response to damage today is one binary material swap at
zero health (`BuildingGameObj::Update_State` calling `Enable_Alternate_Materials`). A structure
one shot from collapse is pixel-identical to an untouched one.

**Implement.** `SurfaceTintEffectClass` is a `MaterialEffectClass`, so it hangs on the effect
list `PhysClass` has carried since stealth and transitions were written and needs no new
plumbing at all. It is shared with Phase 59 -- see that phase for why one effect carries both
terms rather than each having its own. Two passes over whatever the object already draws:

- a **multiplicative** soot pass, which darkens and browns as health falls. Multiplicative
  matters: an additive pass can only ever brighten, which is why the transition and stealth
  effects could not be reused for this however similar they look.
- an **additive** pass carrying the embers (as its emissive term) and Phase 59's wet sheen (as
  its specular term). Both are additive and neither needs a pass of its own.

The night factor the embers key on comes from the sky clock:
`BackgroundMgrClass::Compute_Night_Factor` reads the sun's elevation, which is already computed
there, and ramps across the shallow band either side of the horizon so embers come up during dusk
rather than switching on at it.

Both are untextured; their colour is the vertex material's emissive with lighting off, so the
tint is exactly the number computed rather than that number lit. Tint only -- no damage-state
models, no damage textures, nothing to author, which is the phase's own constraint and also why
it works on content that shipped in 2002.

The soot starts at 85% health (one bullet should not char a building) and the embers at 35%
(a structure is scorched long before anything in it is burning). Nothing multiplies to black:
a building reduced to a silhouette is a worse picture than the stock one.

Wired at **one** site, `DamageableGameObj::Apply_Damage`, because that is where health changes
for buildings, vehicles and soldiers alike -- hooking each separately would be three places to
forget. Repair comes through the same path as negative damage, so the tint reverses there too,
ramping rather than snapping so a repair gun reads as healing rather than as a strobe.

The effect is attached lazily, on the first damage taken while the feature is on, so an
untouched world carries no extra objects.

---

## P62: weapon-fire camera response

**Owner.** `wwphys/camerashakesystem.cpp`, `Combat/ccamera.cpp`, `Combat/weapons.cpp`.

**Implement.** A per-shot field-of-view kick (1.6 degrees at full scale) and roll (0.5 degrees),
recovering along a cubed curve so most of the movement is gone almost immediately and the last
of it eases out. A linear recovery reads as the camera being dragged back rather than springing
back.

Kept as a separate track from the positional shakers, because a shot is not an explosion
somewhere in the world that the camera happens to be near: it has no position, no radius, and
recovers in a fraction of the time. A second shot **restarts** the kick rather than stacking
into it -- stacking means a machine gun walks the view off the screen, which is the exact
failure that makes a camera response stop being cosmetic.

Scaled per weapon from `WeaponDefinitionClass::RecoilScale` and `RecoilTime`, which already
exist, are already editor-exposed, and already drive the weapon's bone recoil. A heavy weapon
moves the view more than a pistol without anybody authoring a second set of values.

Triggered from `WeaponClass::Fire_Bullet`, beside the existing `Start_Recoil` call, and only
for the local player's own weapon -- somebody else firing across the map has no business moving
this client's camera, and the existing shake system already covers the case where a nearby
explosion should.

**Provably cosmetic, structurally.** The shake system only ever writes the camera transform, and
it does so in `PhysicsSceneClass::Apply_Camera_Shakes`, which runs after the simulation has
already decided where the shot went. Aim direction, spread and hit registration are the
server's and never see any of this -- the same guarantee the explosion shake has always had, for
the same reason. The roll is added before the existing clamp, so it shares the ceiling the
explosion shake has and cannot combine with one to tip the view over.

## What this pass did not attempt

- **Runtime visual verification of any of the seventeen phases.** No screenshot, no frame
  capture, no live match -- the standing "do not run the game" instruction covers this pass the
  same way it already covers the thirty-four self-check families **Q-012** tracks. P57's
  mid-frame render-target switch is the single item on this list most in need of a screen. Everything above
  that is stated as a visual outcome ("shadows rotate", "night falls", "broad patches of
  shade") is a claim about what the code does given what its consumers were read to already
  do, not a claim that has been seen on screen.
- **HLodClass mesh-LOD dissolve.** P47's DistLOD half is real and correct against the class
  the roadmap names, but does not reach the LOD system stock content actually uses
  (`HLodClass`). Extending the same overlap-window technique there is a natural follow-on,
  not attempted here to keep this pass's diff bounded to the named owners.
- **The P49 BaseSolve saturation approximation** is exactly that -- an approximation, not
  energy-conserving in scenes with several stacked over-bright lights. Acceptable for a
  cosmetic relighting term; not re-derived to be exact this pass.
- **P51's height fog is camera-relative, not per-vertex.** True height-varying fog (a valley
  foggy while a hilltop the camera can also see is not) needs per-vertex or per-pixel data
  this pass did not add to every renderer; recorded above, not hidden.
- **P55's shaft is a streak-sprite approximation**, not the per-pixel radial blur the acceptance
  text describes; recorded above with the reasoning, and the natural follow-on (P56/P57's
  post-processing render-target chain) named.
- **Local tone mapping.** P56's curve is a real operator with a real shoulder, but one curve
  applies to the whole frame. A local operator needs the programmable tier; recorded above under
  P56, and the only part of that phase's acceptance not fully met.
- **Anything on a device.** P57's composite and the P55 rework that shares it both switch render
  targets mid-frame, which is the one construct in this program that has no precedent inside a
  frame already being rendered. Both off by default; first thing to look at when somebody can run
  the game.

---

## Revision: Phase 55's sun shafts, redone on the chain

Phase 55 shipped in an earlier pass as a row of additive streak sprites stepping from the sun's
screen position toward screen centre. That was recorded at the time, in this document and in
`WIP_features.md`, as an approximation standing in for the per-pixel radial blur the acceptance
text describes -- with the reason given (no render-target post-process chain existed) and the
follow-on named (P56/P57 would build one).

They did, so the follow-on has been taken.

`PostProcessChainClass::Render_Radial_Streaks` draws the sun into the shared target and then
composites that target back over the frame six times, each pass scaled *about the sun's own
screen position*. Because the anchor is a fixed point of the transform, every bright pixel lands
a little further from it each pass, and the sum is a streak pointing away from the source. That
is a radial blur, built out of the one operation this pipeline has, and it is the thing the
acceptance line asked for rather than something that resembles it.

The gating is unchanged and still comes from the sun dazzle's own data -- below the horizon, off
screen, or occluded all draw nothing -- but it now lives beside the drawing it controls.

The streak-sprite path and its `ow_sun_shaft_glow` procedural texture were **deleted**. Keeping
both would have left two implementations of one phase selected by which function the caller
happened to reach, which is exactly what the project's own rules forbid.
