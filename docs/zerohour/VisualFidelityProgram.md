# The Section 52A visual-fidelity program: Phases 46-55

Roadmap Section 52A, the first ten phases (of seventeen). Program-wide constraints that
bind every phase in the section: no new authored art required; no `.w3d` re-export or
LevelEdit re-solve; every phase is a runtime switch defaulting to current appearance until
enabled, and disabling it restores stock rendering exactly; appearance is not intelligence
(Section 30's firewall still holds -- nothing here changes what is knowable to a player or
server); shared state that must agree across a match is server-authoritative and replicated
as a small scalar set, never per-object; each phase extends its named canonical owner, it
does not fork a second pipeline behind a flag.

All ten phases build and link clean across `ww3d2`, `wwphys`, `combat`, `renegade` and
`renegadeserver`. Six device-less `-shaderselfcheck` families cover the state-machine surface
(`visualsettings`, `skyclock`, `lightcurves` for P46-50; `heightfog`, `skyocclusion`,
`skydriventoggles` for P51-55), each with an `fds_` twin, per
`docs/zerohour/TestCoverageCatalog.md`'s own convention.

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

## What this pass did not attempt

- **Runtime visual verification of any of the ten phases.** No screenshot, no frame capture,
  no live match -- the standing "do not run the game" instruction covers this pass the same
  way it already covers the twenty-two self-check families **Q-012** tracks. Everything above
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
