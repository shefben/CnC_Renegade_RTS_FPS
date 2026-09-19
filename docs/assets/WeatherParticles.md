# Weather particles

Read by `WeatherEnvironmentRenderer::Peek_Texture` in `Code/ww3d2/weatherenvironment.cpp`,
once per mode, from the name in that mode's row of `_DefaultModes` in the same file.

Renegade's own precipitation already has art and is not listed here: rain, snow and ash were
drawn from `WeatherParticles.tga`, and snow and ash still are. What follows is the four
textures the weather layer asks for and does not get.

## What happens today without them

The renderer loads the named texture, and if the asset manager reports it missing
(`TextureClass::Is_Missing_Texture`) it **drops the texture and draws the mode untextured** --
a plain quad or streak in the mode's own colour and alpha, soft-blended and fogged like every
other weather particle. It is deliberately not left holding the asset manager's placeholder
image: a placeholder is opaque, obviously wrong, and would be drawn several thousand times a
frame.

So every mode below works right now. What is missing is only the shape inside the particle.

## The shared frame grid

Every weather texture is addressed by `PointGroupClass` as a **4x4 grid of frames**
(`FrameRowColumnLog2 = 2`), so a texture is one image divided into sixteen equal cells, and a
mode names the first cell it uses and how many it uses. Cells are numbered left to right, top
to bottom. A cell is square in UV, and is mapped to a camera-facing quad, so **a cell's
contents should be square** or they will be stretched.

Frames within a mode's band are picked per particle by a hash, so they should be
interchangeable -- four variations of the same thing, not four stages of one thing. The one
exception is a mode with a splash frame, where the **last** frame of the band is what a
landed particle shows and the others are what a falling one shows.

Alpha is what shapes a weather particle. The quad is square and always fully drawn; the
silhouette has to come from the alpha channel, with the cell's edges fully transparent so
neighbouring cells never bleed in under mipmapping.

Textures are loaded with `MIP_LEVELS_5` and clamped in both axes.

---

## `ow_weather_rain.tga`

**What it is.** A single raindrop streak: one bright, near-vertical smear, brightest along its
core and fading to nothing at both ends and both sides.

**Where it is read.** `_DefaultModes[WEATHER_MODE_RAIN].TextureName`.

**Today without it.** Rain draws as an untextured streak -- a soft-edged white quad stretched
along the direction of fall. This is close to right, and is the least wrong of the four
absences; the texture would mainly add variation and a softer core.

**Constraints.**

- Rain is the one mode drawn by the **streak** renderer (`LineGroupClass`, `PRISM`), not as a
  sprite, because a raindrop is three times longer than it is wide and a sprite has only one
  size. The streak renderer maps the texture's **U along the length of the streak** and its
  **V across the width**. It is therefore *not* on the 4x4 frame grid: the whole texture is
  one drop.
- Aspect: the drawn streak is `ParticleWidth` 0.15 by `ParticleHeight` 0.45 world units, so
  author the image at a 1:3 width-to-length ratio and let the renderer scale it.
- The **U = 0 end is the head** of the drop, the leading end as it falls. Fade both ends to
  zero alpha; a hard end reads as a tick mark rather than rain.
- Greyscale in RGB. The mode's colour multiplies it, and rain is tinted per level by fog
  rather than per texture.

## `ow_weather_dust.tga`

**What it is.** Four airborne dust motes, pale sandy brown, soft and formless.

**Where it is read.** `_DefaultModes[WEATHER_MODE_DUST].TextureName`, frames 0-3 (the top row).

**Today without it.** Dust draws as plain soft quads in the mode's colour, a dull tan at 45%
alpha. Legible as haze, but every mote is the same featureless square.

**Constraints.**

- 4x4 grid; **only the top row is used**. The other twelve cells are never sampled and can be
  left empty, but the file must still be a square power-of-two image so the grid divides it
  evenly -- 256x256 gives 64x64 cells, which is ample.
- Dust does not land and does not decay; a mote is on screen continuously and drifts, so it
  must bear being looked at. Keep it low-contrast: dust that reads as a distinct object looks
  like debris.
- Alpha well under 1 at the centre. The mode already multiplies by 0.45 and motes overlap
  heavily near the camera.
- The four cells should differ in silhouette, not brightness, or the variation will read as
  flicker.

## `ow_weather_tiberium.tga`

**What it is.** Four small green glowing specks, the airborne dust of a tiberium field.

**Where it is read.** `_DefaultModes[WEATHER_MODE_TIBERIUM].TextureName`, frames 0-3.

**Today without it.** Draws as plain additive green quads -- soft green blobs, which is
crudely the right idea and visibly untextured against a dark sky.

**Constraints.**

- This is the one weather mode drawn with an **additive** shader. RGB is what gets added and
  the alpha channel is effectively a second brightness control, so **pure black is invisible**
  and the cell's border must be black, not merely transparent.
- Because it is additive it brightens whatever is behind it, and hundreds overlap. Keep the
  peak well below full white or a tiberium field will bloom into a flat green wash.
- Green channel dominant; a little cyan in the falloff reads as glow, a little yellow reads as
  fire.
- Same 4x4 grid, top row only.

## `ow_weather_falling.tga`

**What it is.** Four generic falling flecks -- the mode a level uses when it wants something
falling that is not rain, snow, ash, dust or tiberium. Deliberately neutral: white-grey, small,
no strong shape.

**Where it is read.** `_DefaultModes[WEATHER_MODE_FALLING].TextureName`, frames 0-3.

**Today without it.** Plain white quads that settle and shrink where they land.

**Constraints.**

- Same 4x4 grid, top row only.
- This mode **decays after landing** (`DecayAfterLanding`), so a particle shrinks to nothing
  over one to two seconds where it lands, lying flat on the surface. The image should still
  read at a fraction of its size, which means no fine detail and no thin lines.
- White to light grey. The mode exists to be re-tinted by whoever uses it, so bake no colour
  in.

---

## Stock art this layer still uses

`WeatherParticles.tga` is **not** missing -- it ships with Renegade -- but the weather layer
now reads it differently, and that is worth recording next to the rest.

The stock atlas is one image divided into horizontal bands, four pages across each band: rain
occupied the top half (V 0 to 0.5), snow the third quarter (0.5 to 0.75), and ash the bottom
quarter (0.75 to 1.0). Read as a 4x4 frame grid, snow's band is exactly **row two, frames
8-11**, and ash's is exactly **row three, frames 12-15**, which is how both still use their
original art unchanged.

Rain's band is two grid rows tall and so is not addressable as frames, which is the second
reason rain became a streak with a texture of its own. Frames 0-7 of the stock atlas -- the
rain band -- are no longer read by anything.
