# Weather / environment particle layer

Roadmap Section 36, Zero Hour / SAGE feature 23. Port matrix row ZH-23
(`PORT_RENDER_FEATURE`, `ADAPTED_BACKPORT`).

Donor, per the frozen inventory in `ZeroHourCapabilitySources.tsv`:
`Core/GameEngine/{Include,Source}/GameClient/Snow.{h,cpp}` and
`Core/GameEngineDevice/{Include,Source}/W3DDevice/GameClient/W3DSnow.{h,cpp}`.

Owners: `WeatherMgrClass` (`Code/Combat/WeatherMgr.{h,cpp}`) for weather **state**, and
`WeatherEnvironmentRenderer` (`Code/Combat/weatherenvironment.{h,cpp}`) for the atmospheric
half of what draws it.

## What was already here, which is most of it

The port matrix recorded this row as "Combat/WeatherMgr exists; the screen-space particle layer
does not", and reading `WeatherMgr.cpp` bears that out. Renegade shipped a weather system that
already answers most of what Section 36 asks for:

| Section 36 asks for | Renegade already had |
|---|---|
| camera-centred emitters | `WeatherSystemClass` emits from a square box a fixed height above the camera |
| batch rendering through the current renderer | one index buffer and one draw per system, through `DX8Wrapper` |
| distance/visibility culling | the system keeps its own world bounding box and is culled with it |
| optional wind vector | `WindClass`, with heading, speed, variability and a sound |
| density/intensity controls | `Set_Density`, and a `WeatherParameterClass` per setting that ramps to a target |
| no dedicated-server renderer dependency | `Update` refuses to create a system when `CombatManager::I_Am_Only_Server()` |
| resource recreation after device reset | nothing to recreate: the index buffer is `D3DPOOL_MANAGED`, which survives a reset |

It has something the donor does not, too. `WeatherSystemClass` casts a grid of rays down from
its emitter box and spawns a particle on a ray **only if that ray can see the sky**
(`Can_Spawn`), and a particle dies where its ray hit. That is why snow does not fall inside a
Renegade building interior, which is the one constraint the port matrix wrote down for this row,
and it is a better answer than a screen-space snow layer can give. So the donor's
implementation is not ported: its architecture is, where Renegade was missing a piece.

## What was missing

**Three of the six initial visual modes.** Rain, snow and ash existed. Dust, tiberium
atmospheric particles and generic falling particles did not.

**A particle budget anybody chose.** `WeatherSystemClass::Spawn` stopped at
`USHRT_MAX / 6` = 10922 particles across every weather system -- a limit of the index buffer
rather than a decision about what a frame can afford -- and whichever system asked first was
free to spend all of it, so heavy rain could leave snow with nothing. Section 36 asks for
bounded particle budgets, and a bound one claimant can empty is not one.

**World-region emitters**, as an alternative to camera-centred ones.

## The division

Weather that **falls and lands** stays with `WeatherSystemClass`: rain, snow, ash, and nothing
new. Collision with the world is the whole value of that implementation and an atmospheric mote
has no use for it.

Weather that **hangs in the air** is the new layer: dust, tiberium haze, generic falling motes.
It holds no particles and no geometry. It decides how many motes of each kind the air should
hold and asks the batched particle pool of Section 26 for them, which is where the pool, the
budget classes, the batching and the distance culling already are. A mote is therefore one slot
in that pool's `ENVIRONMENT` budget class, drawn in the same submissions as everything else of
its texture.

`WeatherMgrClass` remains the one owner of weather state. The three new modes are precipitation
types like the other three: the same `Set_Precipitation`, the same density parameter with the
same ramping and override, the same micro-chunks in a saved game and the same fields in the
`Export_Rare` packet. They are appended to `PrecipitationEnum`, so the numbering of the three a
script or a level already names cannot move.

What each type **is** -- which parameter it ramps, whether it falls or hangs, which atmospheric
mode draws it -- is now one table, `WeatherMgrClass::Get_Descriptor`. The stock update loop
asked that question with one switch for the parameter and a second for the constructor a few
lines later; two switches over one enumeration are two places to forget.

## The population is a count, not a rate

The layer's unit is motes, not motes per second. Each mode has a population at density one --
500 dust, 300 tiberium, 700 falling -- and each step emits the difference between that target
and what the pool says is alive, bounded to 48 a step so that a density going from nothing to
full arrives over a second rather than in one frame.

That is what makes the acceptance true by construction: the steady state does not depend on the
frame rate, on how long the weather has been running, or on how large the scene is. The volume
travels with the camera, so a bounded number of motes covers an unbounded outdoor scene.

Nothing is ever removed on purpose. A density that falls, or a share that is divided with
another mode, is a population that thins out as its motes reach the end of their lives -- which
is how weather stops looking like a switch, and is why the self check runs frames rather than
steps.

Where the motes go inside the volume is a three-dimensional low-discrepancy sequence (additive
recurrences of the generalised golden ratios), not a random number generator: it covers the
volume more evenly than the same number of random points, and the same weather looks the same
on every machine and in a replay.

## The two budgets

**The air**: `WeatherEnvironmentRenderer::Set_Budget`, 900 motes by default, which is less than
the pool's `ENVIRONMENT` class cap of 1536 on purpose -- that class is also where a Commander's
smoke and a burning building's haze will be spawned, and weather allowed to fill it would be
weather that turns those off. When the modes between them want more than the budget, every one
of them is scaled by the same fraction rather than the first one asked taking what it likes.

**The precipitation**: `WeatherSystemClass::Set_Global_Particle_Budget`, 8192 by default,
clamped to `Get_Particle_Limit()` -- the index-buffer limit, which is real and which nothing can
raise. Each live system is given an equal share of it every update, so turning snow on halves
what rain may hold instead of starving it.

## The acceptance, as numbers

> Weather can cover a large outdoor scene at a stable bounded particle count without creating a
> `GameObj` per particle.

`terrain_weather` (and `fds_terrain_weather`) asks for four times full density on all three
atmospheric modes -- six thousand motes' worth -- and reads back 900: the budget, divided in
proportion, with no mode starved to nothing and the order of the shares matching the order of
what they asked for. Before that it drives one mode to its target and keeps stepping, and the
population stays exactly on the target rather than creeping; after it, the dust that is suddenly
above its share drains to it over its own six-second lifetime. The pool's `ENVIRONMENT` class
never exceeds its cap at any point.

No `GameObj` is created anywhere in this feature, and none could be: the layer's only output is
`WorldParticleBatchManager::Emit`.

The precipitation budget is checked in the same place: nothing can be set above the renderer's
limit, a budget inside it is kept, and the limit itself is what it has always been.

None of it needs a graphics device, a level or a physics scene.

## Console

| Command | What it does |
|---------|--------------|
| `weather` | Lists every kind of weather, its density, and whether it falls or hangs. |
| `weather <kind> <density> [ramptime]` | Sets one, by name, through the same server-authoritative path the stock `rain`, `snow` and `ash` commands use. |
| `weather_status` | Wind; the precipitation particle count against its budget and the renderer's limit, per kind; the air's live count against its target and budget, per mode; and where the volume is. |
| `weather_texture <texture>` | Points the three atmospheric modes at a texture that exists, so the layer can be seen. |
| `weather_budget <precipitation> [air]` | Retunes either budget. No argument prints both. |

`rain`, `snow` and `ash` still do exactly what they did.

## Not done

- **Nothing has been drawn.** The three atmospheric modes name no texture
  (`docs/assets/WeatherSprites.md`), so no mote has been on a screen. `weather_texture` plus
  `weather dust 1` is the manual check.
- **No script commands for the new modes.** `Set_Rain`, `Set_Snow` and `Set_Ash` exist in the
  script API; the three new kinds are reachable from the console and from C++ but not yet from a
  level's scripts. Adding them is a line each, and belongs with the script-API work rather than
  here.
- **No LevelEdit page for them.** The editor's weather page offers none/rain/snow/ash as radio
  buttons, which is a dialog resource rather than code.
- **The atmospheric modes do not test for a roof.** Dust in a room is not a bug, so they spawn
  wherever the volume is. Precipitation still cannot fall indoors, because that is the stock ray
  grid's job and it still does it.
- **A region is set from code only.** `Set_Region` pins the air to a part of the world, which is
  what a tiberium field wants, but nothing reads a region out of a level yet -- that is a job for
  the zone work, and the console has no command for it.
- **Generic falling motes do not land.** They have no collision, because the pool has none; they
  are given a short life and a slow fall and live in the air near the viewer. Something that has
  to pile up on the ground is precipitation and belongs in `WeatherSystemClass`.
