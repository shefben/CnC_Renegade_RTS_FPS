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
