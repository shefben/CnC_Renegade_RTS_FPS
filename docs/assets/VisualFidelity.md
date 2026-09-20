# Visual fidelity (roadmap Section 52A)

Section 52A's own program-wide constraint: "no new authored art is required... generated
procedurally... recorded in `docs/assets/VisualFidelity.md` as improvements, never
prerequisites." This file is that record.

## Phases 46-50

No entries. Every one of the five phases works from what the engine already computes or
loads -- device capability, the existing per-vertex light solve, the sun's own transform,
a light's own existing `Intensity`/`GroupID` -- or from a level author's own placement
decisions (a `LightPhysClass`'s `Group ID`, associating it with a building). None of the
five needed a texture, model, or any other asset that does not already exist, and none is
blocked on one. See `docs/zerohour/VisualFidelityProgram.md` for what each phase actually
does.

An asset *would* improve, not gate, one of these if it existed:

- A dedicated flicker/pulse/warmup/fail profile authored per light fixture (rather than the
  one fixed timing each curve uses today) would be a data file, not new code -- Phase 50's
  `LightClass::Set_Light_Curve` already takes the curve type as a parameter; only the
  per-curve constants (`FAIL_SECONDS`, the `PULSE` period, and so on) are fixed rather than
  read from anywhere yet. Not a prerequisite for the phase's own acceptance line.

## Phases 51-55

Two procedural, in-memory textures, neither loaded from disk, both prefixed `ow_` for
consistency with the rest of this folder even though nothing here reads either name from the
mix files:

- **`ow_cloud_shadow_mask`** -- Phase 53's ground-shadow density mask. Generated once at
  `CloudLayerClass::Build_Shadow_Mask_Texture` (`Code/Combat/backgroundmgr.cpp`): a 128x128
  tiling value-noise texture, coarse 8x8 random grid bilinearly interpolated and squared so it
  reads as broken cloud cover rather than an even grey wash. An authored replacement --
  hand-painted cloud-shaped density, matching whatever `CloudLayerClass`'s own sky texture
  looks like on a given level -- would be a straightforward drop-in: `Build_Shadow_Mask_Texture`
  would just load it instead of generating it, and everything downstream (the projector, the
  scroll, the multiplicative blend) is unchanged either way. Not a prerequisite; Phase 53's
  acceptance does not ask for a particular cloud shape, only that shade crosses terrain in the
  wind direction.
- **`ow_sun_shaft_glow`** -- Phase 55's glow sprite. Generated once at
  `BackgroundMgrClass::Build_Sun_Shaft_Glow_Texture`: a 32x32 radial falloff, squared for a
  soft edge. An authored lens-flare-style glow (with the small chromatic fringing real optics
  give a strong backlight) would read better at close range than the flat radial falloff does,
  but is a texture swap only -- the streak placement, count and fade logic in
  `Render_Sun_Shafts` do not change. Not a prerequisite.

No other new asset is needed for Phases 51-55. P51 (height fog) and P52 (sky/contact
occlusion) work entirely from geometry and device state already present; P54 (environment
reflections) reuses `MetalMapManagerClass`'s existing procedural generator unchanged, wiring
it into the runtime rather than adding anything new to generate.
