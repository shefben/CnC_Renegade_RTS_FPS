# Atmospheric weather textures

Roadmap Section 36. Read by `WorldParticleBatchManager::Attach_Group_Object`
(`Code/WWPhys/worldparticlebatchmanager.cpp`) for the kinds
`WeatherEnvironmentRenderer::Init` defines (`Code/Combat/weatherenvironment.cpp`), through
`WW3DAssetManager::Get_Instance()->Get_Texture(def.Get_Texture())`.

Three textures, for the three kinds of weather that hang in the air rather than falling and
landing. **Rain, snow and ash need nothing**: they are Renegade's own precipitation, they are
drawn by `WeatherSystemClass`, and they come out of the shipped `WeatherParticles.tga`, which
divides one image into bands -- rain across the bottom half, snow and ash a quarter each above
it. Nothing here touches that texture or those three modes.

## The textures

| Name | What it is |
|------|------------|
| `ow_weather_dust.tga` | A soft, almost shapeless dust puff. What hangs in the air of a hot dry place: pale sandy brown, very low contrast, no hard edge anywhere. It is drawn a third of a metre across and there may be five hundred of them on screen, so detail in it is wasted. |
| `ow_weather_tiberium.tga` | A small green glint. Drawn additively, so it is a bright core falling off to black well inside the image; black is transparent and the shape lives in the falloff rather than in the alpha. The one weather texture with a colour of its own. |
| `ow_weather_falling.tga` | A neutral fleck, slightly taller than wide, for whatever a level wants falling slowly through the air near the player: ash from a fire, leaves, paper. Deliberately characterless, because the colour and the motion come from the mode. |

## The modes that read them

Set by `WeatherEnvironmentRenderer::Init`. Population is how many motes the air holds at
density one, which is the number the budget is measured against; rise is metres per second
upward, so a negative one falls.

| Mode | Shape | Population | Life | Size | Rise | Wind | Blend |
|------|-------|-----------:|------|------|------|------|-------|
| `dust` | sprite | 500 | 6.0 s | 0.35 → 0.55 m | -0.15 | full | alpha |
| `tiberium` | point | 300 | 4.0 s | 0.10 → 0.04 m | +0.35 | a quarter | additive |
| `falling` | sprite | 700 | 3.5 s | 0.12 → 0.10 m | -1.60 | most | alpha |

All three are in the particle pool's `ENVIRONMENT` budget class, which is what makes weather
the first thing a busy frame stops drawing.

## Without them

Nothing of the three atmospheric modes is drawn, and everything else works. The motes are still
emitted, budgeted, moved, aged and counted, so `weather_status` reports real live, target and
budget numbers and `terrain_weather` proves the acceptance; the modes simply never reach a
screen. Rain, snow and ash are unaffected and draw normally.

The kinds ship with an **empty** texture name, for the reason the rest of
`docs/assets/ParticleSprites.md` gives: `Get_Texture` answers a name that does not resolve with
the engine's placeholder rather than with nothing, so naming `ow_weather_dust.tga` before it
exists would hang white squares in the air. When the art lands, the names above go into the
`_Modes` table in `weatherenvironment.cpp` and that is the whole of the work.

To see the layer before then: `weather_texture <a texture name from the loaded level>` points
all three modes at one that resolves, and `weather dust 1` fills the air around the camera.

## Constraints

**The image is the mote, with `u` and `v` running 0 to 1 across it.** Nothing tiles, nothing is
paged: these are drawn by `PointGroupClass` as camera-facing quads and triangles built around
the particle's position, so the centre of the image is the mote.

**Alpha is the shape for the two alpha-blended modes, and it is multiplied by the fade.** Dust
and falling motes spend their whole lives fading from their start alpha to nothing; a texture
opaque to its edges reads as a square. The tiberium mode is additive, where black is
transparent and the alpha channel matters much less, but the falloff must still be inside the
image.

**Small.** 64x64 is enough for all three. A mote is a few pixels across on screen and there can
be nine hundred of them.

**No motion of its own.** These textures are not flipbooks -- none of the modes sets a sprite
grid -- so anything that looks like it should animate will instead sit still while it drifts.
