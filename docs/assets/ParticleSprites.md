# Batched particle textures

Roadmap Section 26. Read by `WorldParticleBatchManager::Attach_Group_Object`
(`Code/WWPhys/worldparticlebatchmanager.cpp`), through
`WW3DAssetManager::Get_Instance()->Get_Texture(def.Get_Texture())`.

Six textures, one per default kind of particle. None of them is urgent, for the same reason
the surface marks are not: shipped Renegade content already names its own particle art, and
the authored W3D emitters that use it keep their own implementation (`ParticleBufferClass`)
and are not affected by anything here. What is missing is the six **named** kinds — the ones
engine code can ask for by concept when there is no asset pipeline in the way, which is what
the Commander feedback, weather, tracer and debris work will all be asking for.

## The textures

| Name | What it is |
|------|------------|
| `ow_part_signal.tga` | A soft smoke puff with a bright core, read as a plume when stacked. The one particle a player has to be able to see across a map: a beacon's column, a gas cloud, a building about to fall. Pale, high contrast against both daylight terrain and interiors. |
| `ow_part_impact_dust.tga` | A small ragged dust puff. Thrown off a bullet strike on ground or wall; greyish-brown, no colour of its own, so a tint can make it dirt, sand or concrete. |
| `ow_part_spark.tga` | A single hot spark: a bright centre falling off to nothing well inside the image. Drawn as a triangle, additive, and only ever a few pixels across on screen. |
| `ow_part_debris_streak.tga` | A lengthwise streak, brightest at one end. Stretched along a line between where a burning fragment is and where it was, so the image is read along `u` and not around a centre. |
| `ow_part_environment.tga` | Wind-blown dust, snow or rain motes — whatever the weather layer asks for. Deliberately characterless: the colour and the motion come from the definition. |
| `ow_part_ember.tga` | A dim mote. An ash fleck or a drifting ember; the smallest and least important thing on screen. |

## The kinds that read them

Set by `WorldParticleBatchManager::Define_Default_Batches`. Size is the particle's width in
metres at birth and at death; life is how long it lasts; gravity is a downward acceleration,
so a negative one is a thing that rises.

| Kind | Shape | Budget class | Life | Size | Gravity | Blend |
|------|-------|--------------|------|------|---------|-------|
| `ow_part_signal` | sprite | `CRITICAL_GAMEPLAY` | 3.0 s | 0.60 → 2.50 m | -0.60 (rises) | alpha |
| `ow_part_impact_dust` | sprite | `COMBAT_NEAR` | 1.1 s | 0.25 → 1.00 m | 1.50 | alpha |
| `ow_part_spark` | point | `COMBAT_NEAR` | 0.6 s | 0.09 → 0.02 m | 9.00 | additive |
| `ow_part_debris_streak` | streak | `COMBAT_FAR` | 1.6 s | 0.10 → 0.04 m | 9.00 | additive |
| `ow_part_environment` | sprite | `ENVIRONMENT` | 4.0 s | 0.40 → 0.60 m | 0.40 | alpha |
| `ow_part_ember` | point | `AMBIENT` | 2.5 s | 0.05 → 0.02 m | -0.35 (rises) | additive |

A group interned from a content texture name through `Find_Or_Define_Texture_Group` does not
use this table at all; it carries whatever the caller asked for.

## Without them

Nothing of those six kinds is drawn, and everything else works. Particles of those kinds are
still emitted, budgeted, shed, moved, aged and counted; their buffers are still filled, so
`particle_status` reports real particle, submission and headroom numbers, and
`Get_Missing_Texture_Count()` says how many groups hold particles with nothing to draw them
with. No render object is created at all until a texture resolves, so a missing particle
texture costs nothing rather than a white square.

The definitions ship with an **empty** texture name, deliberately. `Get_Texture` answers a
name that does not resolve with the engine's placeholder rather than with nothing, so naming
`ow_part_spark.tga` before it exists would fill the screen with white quads — which is worse
than drawing nothing and harder to diagnose. When the art lands, the names above go into
`Define_Default_Batches` and that is the whole of the work.

To see the system before then: `particle_texture <a texture name from the loaded level>`
points every default kind at one that does resolve, and `particle_test 600` throws a burst of
each kind up around the camera.

## Constraints

**A sprite or point is the whole image, with `u` and `v` running 0 to 1 across the quad.**
Nothing tiles. `PointGroupClass` builds the quad around the particle's position from
precalculated corner offsets, so the image's centre is the particle and the corners are the
corners.

**Alpha is the shape, and it is multiplied by the particle's own fade.** The groups are drawn
with the engine's sprite presets (`_PresetAdditiveSpriteShader` for additive kinds,
`_PresetAlphaSpriteShader` for the rest) and the per-vertex diffuse carries the colour and the
alpha the particle has reached on its curve. An image opaque to its edges reads as a square,
which is the one thing a smoke puff must not be: the edge belongs in the alpha channel. For
the additive kinds, black is transparent and the alpha channel matters less, but the falloff
still has to be inside the image.

**Square, and small.** These are drawn a few dozen pixels across at most. 64x64 is enough for
a spark or an ember and 128x128 for the smoke and dust; a 512 texture would be eight times the
memory for the same pixels on screen, across up to four thousand particles at once.

**A streak reads along `u`.** The streak kinds are drawn by `LineGroupClass` as a tetrahedron
from the particle's position to where it was `Get_Streak_Seconds()` ago, so the texture is
stretched along the line of travel. An image designed around a centre will look like a smear
of its middle row.

**A sprite sheet is 2^n by 2^n cells, and plays once over the particle's life.** A definition
with `Set_Frame_Grid_Log2(n)` divides its texture into a (2^n)x(2^n) grid — up to 16x16 — and
the manager walks the cells from the first to the last as the particle ages. None of the
default kinds uses one; a flipbook explosion puff would. The cells must be laid out left to
right, top to bottom, and every cell has to carry the same silhouette scale, because the
particle's size curve is applied on top of the cell rather than per cell.
