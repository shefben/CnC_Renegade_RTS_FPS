# Bridge section models

Roadmap Section 20 (sectional bridge system). Sixty model names, in three sets of twenty,
and none of them exists.

`BridgeSystem::Build_Section_Geometry` in `Code/WWPhys/bridgesystem.cpp` builds every
section of every bridge by asking `WW3DAssetManager::Get_Instance()->Create_Render_Obj(name)`
for the model the definition names for that section's kind in that section's current state.
The names live in the definition table, `BridgeSystem::Define_Default_Bridges`, and today
every one of them is empty.

## What happens without them, which is more than usual

A missing texture leaves geometry untextured and a missing decoration leaves nothing on the
parapet, and both are fine. A missing *deck* is not fine: a bridge is the only surface over
the gap it crosses, so a bridge that draws nothing is a hole that a soldier falls through,
and Section 20's acceptance is collision and traversal before it is anything else.

So a section whose model is unnamed, or named and not in the mix files, is built as a **box
of exactly the size the definition declares** — the deck's length, the deck's width, the
deck's thickness, or a pier's footprint and drop. It goes into the physics scene with the
same collision bits a real deck would have. You can walk it, drive it, shell it and break
it. `BridgeSystem::Get_Missing_Model_Count` reports how many sections came out that way, and
`bridge_test` at the console prints it.

That makes these models a **replacement for a working thing**, not the thing that makes it
work. Dropping the models in changes what a bridge looks like and nothing else.

The one exception is `OPTIONAL_DECORATION`, which is genuinely optional: a section of that
kind is only laid out at all when its definition names a model *and* a decoration spacing.
A railing standing in as a row of boxes down both sides of every bridge is worse than no
railing, so absent decoration is absent rather than approximated.

## The five kinds

Every definition has a model slot for each kind in each of the four damage states, so a set
is twenty names. The kinds are `BridgeSectionKindType` in
`Code/WWPhys/bridgesection.h`:

| Kind | What it is | Local origin |
|------|-----------|--------------|
| `START_CAP` | the abutment the road arrives at | centre of the section |
| `SPAN` | the piece repeated to fill the gap | centre of the section |
| `END_CAP` | the abutment the road leaves from | centre of the section |
| `SUPPORT` | a pier under a span joint, down to the ground | centre of the joint, at deck height |
| `OPTIONAL_DECORATION` | lamps, railings, cable stays | the point it stands at, at deck height |

## Constraints that apply to every one of them

**The section's own space is X along the bridge, Y across it to the left, Z up.** The
transform is built in `BridgeSystem::Place_Section` and it is orthonormal, so models are
never sheared or scaled.

**The deck surface is Z = 0.** This is the important one. Whatever height a bridge's two
endpoints were given is the height a soldier stands at, so the walkable top face of a cap or
a span must be at the model's origin plane, with the structure hanging below it. A deck
modelled sitting on its origin will float by its own thickness, and every bridge in the game
will have a step at each end.

**Length along X is the definition's, and it is not exact.** A bridge's spans are stretched
to divide its gap exactly — `Layout_Bridge` divides what is left after the two abutments by
the number of spans that best fits — so a definition with an 8 m span will place spans
between roughly 6 m and 10 m long, and the caps are scaled down together on a bridge too
short to hold them. Models are placed, not stretched, so **a span model must tile against
itself at its nominal length and tolerate a joint that is a metre or so out**. Deck plate
that reads as continuous surface does this; a truss whose diagonals must meet exactly at the
joint does not, and wants a longer nominal span so the error is a smaller share of it.

**Width across Y is the definition's width**, centred on the origin. The three default
definitions are 4 m, 8 m and 16 m wide, and those match Section 19's three road classes so
that a road runs onto a bridge without a step in width. See
[RoadSurfaces.md](RoadSurfaces.md).

**A pier's model hangs below the deck**, from `-deck thickness` down to however far the
ground turned out to be — the drop is measured per pier at layout time and is not known when
the model is made. So a pier model must either be built to be **cut off at the bottom by the
ground**, or be modelled at its nominal height and accept that it will sometimes stand in
the air or sink. The slab standing in for it today is built to the measured drop exactly,
which is the behaviour to aim at: a pier that reads correctly when its bottom is buried.

**Damage states change the model, not the transform.** A `DAMAGED` span occupies the same
place as the `PRISTINE` one it replaced. A `BROKEN` one is not drawn at all — the rule for
that state says it neither collides nor is traversable, and a section that does neither and
names no model is skipped, so a broken span is a genuine hole with nothing in it.

## The three sets

### `ow_bridge_timber` — a trestle over a stream

4 m wide, 0.35 m deck, 5 m spans, piers every 10 m, 1 m pier footprint. What a track
(`ROAD_CLASS_TRACK`) crosses on. Should look like it goes down if you look at it hard.

```
ow_bridge_timber_start_cap        ow_bridge_timber_start_cap_damaged
ow_bridge_timber_span             ow_bridge_timber_span_damaged
ow_bridge_timber_end_cap          ow_bridge_timber_end_cap_damaged
ow_bridge_timber_support          ow_bridge_timber_support_damaged
                                  ...and _critical for each of the four
```

`BROKEN` names nothing, for all three sets: a broken section is a gap.

### `ow_bridge_concrete` — an ordinary road bridge

8 m wide, 0.6 m deck, 8 m spans, piers every 24 m, 2 m pier footprint. What a street
(`ROAD_CLASS_STREET`) crosses on, and the default `bridge_test` builds.

```
ow_bridge_concrete_start_cap      ow_bridge_concrete_start_cap_damaged
ow_bridge_concrete_span           ow_bridge_concrete_span_damaged
ow_bridge_concrete_end_cap        ow_bridge_concrete_end_cap_damaged
ow_bridge_concrete_support        ow_bridge_concrete_support_damaged
                                  ...and _critical for each of the four
```

### `ow_bridge_steel_truss` — a highway crossing

16 m wide, 0.8 m deck, 16 m spans, piers every 48 m, 3 m pier footprint. What a highway
(`ROAD_CLASS_HIGHWAY`) crosses on. The long nominal span is what makes a truss workable
here: see the tolerance note above.

```
ow_bridge_steel_truss_start_cap   ow_bridge_steel_truss_start_cap_damaged
ow_bridge_steel_truss_span        ow_bridge_steel_truss_span_damaged
ow_bridge_steel_truss_end_cap     ow_bridge_steel_truss_end_cap_damaged
ow_bridge_steel_truss_support     ow_bridge_steel_truss_support_damaged
                                  ...and _critical for each of the four
```

## Debris

`BridgeStateRuleClass` carries a debris model name and a count per kind per state, and
`BridgeSystem` records what came off and where in a pending list rather than dropping it —
there is no effect system to hand it to until Section 35. No default definition names any,
so the list is empty on purpose. When the art exists the names follow the same pattern:

```
ow_bridge_concrete_rubble
ow_bridge_timber_rubble
ow_bridge_steel_truss_rubble
```

Read by `BridgeSystem::Note_Debris`, drained through `Peek_Pending_Debris`.

## Trying it before the art exists

```
bridge_test 120 ow_bridge_concrete
bridge_break
bridge_clear
```

`bridge_test` builds a bridge across the ground in front of you at whatever length is asked
for, so asking for a different number and counting the spans is what "variable length" looks
like. `bridge_break` breaks the span nearest you and prints how much of the bridge is
reachable from each end afterwards. Every section will be a slab, and it will say so.

## Not listed here, and why

**Bridge towers** — the donor attaches four tower objects to a bridge, which are gameplay
objects with health that repair it. Those are objects, not sections, and they belong to
whatever builds gameplay objects rather than to this system.

**Scaffold models** — the donor's repair animation raises a scaffold under a broken span.
Nothing repairs a bridge yet, and when something does the scaffold is a model that system
names, not one this one does.
