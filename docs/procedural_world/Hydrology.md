# Hydrology (V2 Phase 6)

Roadmap Section 34 Phase 6 -- "drainage, rivers, streams, lakes, ponds, and internal coastlines"
(Section 9). See `MacroTerrainGeneration.md` for the terrain this phase drains and
`InteriorCliffs.md` for the sibling generator this one mirrors in shape, not in mechanism.

## One water renderer, used a second way

`WaterSystem`/`WaterAreaClass` (roadmap Section 22, complete long before this roadmap started)
already carve, mask and draw a body of water from a placed shape -- what it never had was
anything computing that shape procedurally. Its own documentation says so plainly: "nothing
plants anything... reading water areas out of a level's own data is a level-format question, not
this one." `HydrologyGenerator` (`Code/wwphys/hydrologygenerator.h/.cpp`) is exactly that
missing caller: it computes drainage, turns high-accumulation runs into open `WaterAreaClass`
centre lines and closed basins into rings, and hands both to `WaterSystem::Add_Area`/
`Shape_Terrain`/`Stamp_Mask`/`Build_Geometry` -- the same entry points the console command and
self check already use. There is no second carving primitive: `WorldTerrainSystem::
Apply_River_Cut`, used internally by `WaterSystem::Shape_Terrain`, is untouched by this file.

## Drainage at chunk resolution

A full per-vertex D8 flow-accumulation pass -- the real hydrology-simulation technique -- is not
attempted here. Phase 4 and Phase 5 already established the chunk grid (one chunk = one
heightfield patch) as the unit a generator reasons about, and this generator reuses it:

1. **Height.** One representative height per chunk, the average of the chunk registry's own
   `MinHeight`/`MaxHeight`.
2. **Flow direction.** Steepest descent among the four neighbouring chunks (east, north, west,
   south, in that fixed order for determinism), never toward a `CliffBand` chunk -- step 2's
   "exclude protected boundary/hard-boundary zones from traversable water outlets", kept
   structurally: a cliff-band neighbour is never even considered as a flow target, so a channel
   can never be steered toward one.
3. **Accumulation.** Every non-cliff-band chunk starts at 1 (itself); chunks are processed in one
   pass, height-descending, each adding its own running total into whatever chunk its flow
   points to. This is the standard sorted-list D8 algorithm, at chunk rather than vertex
   granularity.

A chunk with no lower non-cliff-band neighbour is a sink -- Phase 6 step 1's "basin/depression
identification." Its own accumulated value is treated as the size of what drains into it,
Phase 6 step 1's "watershed" answered as a number rather than as a delineated polygon; true
watershed-boundary delineation was not attempted, the same honest scope decision Phase 5 makes
about ridge-line extraction.

## Rivers, streams, and the walk between them

A chunk whose own accumulation clears a third of `water.river_accumulation_chunks` is
channel-eligible. A channel-eligible chunk nothing else channel-eligible flows into is a
headwater; headwaters are taken in accumulation-descending order, bounded at
`water.max_river_count` -- the same bounded-output discipline every generator in this tree
follows. From each accepted headwater, the walk simply follows flow-direction pointers
chunk-to-chunk until it reaches a sink (its own termination, below) or an already-claimed chunk
(a tributary joining a bigger, earlier-built channel -- the walk stops there rather than
duplicate the parent river's own coverage downstream of the join).

A channel whose peak accumulation along its own path clears `water.river_accumulation_chunks`
is a river; below that, a stream/tributary (Section 9.2 treats the two as one network, gated by
the single `water.rivers` flag). Width grows with accumulation: `lerp(water.stream_width,
water.river_width, accumulation / river_accumulation_chunks)` per station, so a channel widens
downstream as real drainage would, rather than holding one constant width along its whole
length. Station height comes from a real `WorldTerrainSystem::Sample_Height` at each chunk's own
centre, clamped non-increasing against the previous station as a defensive backstop against the
coarse chunk-average flow direction and the finer per-point terrain sample disagreeing at any
one point -- the guarantee `Check_River_Never_Climbs` proves is one `Generate` actually keeps,
not merely hopes for.

## Section 5.9, kept structurally

A channel never reaches a cliff-band chunk, because cliff-band chunks are never candidate flow
targets in the first place (above). What happens instead: the chunk whose only lower neighbours
were cliff-band chunks becomes an ordinary sink, exactly as a genuine local minimum would, and
the channel terminates there with a real, visible terminus -- an enclosed lake or pond built at
that sink's own footprint, one of the four options Section 5.9 names ("enclosed lake/basin").
With both `water.lakes` and `water.ponds` disabled, the channel simply stops carving at the sink
with no closed body built; since nothing carved past that point, the ground there is untouched
and ordinary, which satisfies "rivers terminate as unexplained traversable holes" the same way a
channel that was never drawn there in the first place would -- there is no hole because there is
no channel, not because a hole was patched.

The river's own last station and its terminal lake/pond's own footprint deliberately overlap at
the sink chunk: the lake's own carve runs after the channel's and re-levels that same ground to
the lake's flat surface, which is the correct visual outcome (a river running into a lake, not
stopping short of its shore) rather than an oversight. One minor, accepted consequence: a point
query exactly at that overlap (`WaterSystem::Find_Area_At`) returns whichever area was added
first -- the channel -- rather than the lake sitting on top of it there. Not fixed this phase;
noted rather than silently left for someone to rediscover.

## Basins: lake or pond, by size

A sink's own accumulated inflow decides lake versus pond -- at or above
`water.river_accumulation_chunks`, a lake; below it, a pond -- the same threshold already
meaningful for the river/stream split, rather than a second arbitrary number. A basin is built as
a closed ring exactly matching its own chunk's footprint (`WorldTerrainSystem::Get_Patch_Bounds`)
at a flat surface height of that chunk's own sampled minimum plus a small margin
(`BASIN_HEIGHT_MARGIN`), so the carved bed (`WaterDefinitionClass::BedDepth`, cut down from that
surface) has real room to sit in without the surface itself floating above the ground it fills.
This is a coarse approximation of a basin's true shape -- one chunk's own square footprint,
never a fitted polygon around the actual depression -- the same per-chunk granularity Phase 5's
own interior cliff stations already accept as a documented simplification.

## Depth is not a profile field

`WaterDefinitionClass::BedDepth`, already set per category by `WaterSystem::Define_Default_
Water` (`ow_water_river`: 2.5m, `ow_water_stream`: 1.0m, `ow_water_lake`: 3.0m, `ow_water_pond`:
1.5m), is carve depth's one source of truth -- `WaterSystem::Shape_Terrain` reads it directly
from the named definition. A second depth field on the world profile could only ever disagree
with it, so Schema 4 does not add one; only width and the accumulation threshold are profile
fields. See `docs/procedural_world/WorldProfileSchema.md`'s Schema-4 section.

## Shore classification and crossing candidates

Bank slope is sampled once per feature (its own midpoint, offset outward from the channel or the
basin's own edge), not once per station -- the same coarser-than-per-edge granularity Phase 5
accepts for interior cliff traversability, and for the same reason: a per-station classification
would need per-station consumers that do not exist yet. `WorldTerrainSystem::Sample_Slope`
against two fixed thresholds (roughly 15 and 35 degrees) gives `WALKABLE_BANK`/`STEEP_BANK`/
`CLIFF_SHORE`; a channel narrow enough to just drive across overrides up to
`VEHICLE_CROSSABLE`, and a channel too wide with a cliff shore overrides down to
`NON_CROSSABLE` -- Section 9.5's own crossing-evaluation language, made concrete rather than left
as five names with no rule connecting them.

Every feature records exactly one crossing candidate: a river/stream gets a `FORD` (narrow
enough at its own midpoint) or a `BRIDGE` (everything else); an enclosed lake/pond gets a
`ROUTE_AROUND`. `TUNNEL` stays in `HydrologyCrossingType` as a legal value this generator simply
never picks -- deciding a tunnel needs terrain-depth judgement this hydrology pass does not have,
and is explicitly Phase 10's call, not this one's. Building the actual ford/bridge/tunnel is
explicitly out of scope here too (Phase 6 step 11 records candidates; Phases 8-10 build
infrastructure).

## What is deliberately not attempted this phase

**Ocean/coast (Section 9.4).** `water.coastline` is parsed since Phase 1 and still unconsumed --
the same "reserved, unconsumed" state `terrain.interior_cliff_density` was in before Phase 5.
Placing a coastal body correctly (which edge, staying well inside the cliff boundary, not
overlapping the approach zone) is real additional design work this generator does not attempt;
Section 9.4's own text calls a coastline optional.

**True watershed delineation.** A basin's own accumulation stands in for "how much drains here";
no watershed boundary polygon is computed or stored.

## Debug drawing

`debug_layer hydrology 1` draws a river/stream's own centre line, a lake/pond's own closed ring,
and a cross at every feature's own crossing candidate, coloured by kind (yellow ford, orange
bridge, magenta route-around). `DEBUG_LAYER_HYDROLOGY` (`Code/ww3d2/debugrendersystem.h`) is new
this phase.

## What is proven, and what is not

`terrain_hydrology`/`fds_terrain_hydrology` (`Code/Commando/terrainselfcheck.cpp`,
`-terrainselfcheck hydrology`) prove, device-less, against the same synthetic uniform-slope ramp
`Check_Interior_Cliffs` uses (chosen for the identical reason: a real generated seed's terrain
gives no arithmetic guarantee a channel or a basin exists to check against): parameter refusal;
a river and a lake are both built, every structural diagnostic (no cliff-band overlap, no
climbing river station, no lake surface above its own basin rim, every feature carries a
crossing candidate) holding; the same seed and terrain reproduce the same checksum; and a
configured boundary's cliff-band chunks are never reached by a channel or its terminal basin.

What is not proven, because nothing here runs with a display or against `MacroTerrainGenerator`'s
own noise: whether a real generated seed's rivers read as rivers rather than an arbitrary set of
chunk-to-chunk lines; whether a lake sitting exactly on its own chunk's square footprint looks
like a lake rather than a tile. The reachability gap Phase 4 and Phase 5 both carried -- this
generator only running when macro terrain itself ran, which the one real caller's `fit_to_level`
profile made never -- is closed by V2 Phase 10's fitted-terrain sampling; see
`completed_features.md` and `GeneratorDeterminism.md`.
