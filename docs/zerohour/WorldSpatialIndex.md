# WorldSpatialIndex

Roadmap Section 16 (Zero Hour / SAGE Feature 19, spatial partitioning concepts).

`Code/Combat/worldspatialindex.h` and `.cpp`.

## Why it is not a new database

Section 16's "do not" is the shape of the whole design: *do not blindly import SAGE
`PartitionManager` as a second unrelated world database.* OpenW3D already has a world
database, and it is not a small one. Every physical object registers itself with a
culling system as it is created — `PhysicalGameObj::Copy_Settings` calls
`Add_Dynamic_Object`, level geometry goes in as static — and `PhysicsSceneClass` owns
both a `StaticCullingSystem` and a `DynamicCullingSystem` over
`AABTreeCullSystemClass` and `GridCullSystemClass`.

So the missing thing was never an index. It was a *place to ask*. Code that wanted to
know what was near something either reached into `PhysicsSceneClass` and knew about
`Collect_Objects(box, static_objs, dynamic_objs, list)`, or, far more often, walked
`GameObjManager::Get_Game_Obj_List()` from head to tail and measured a distance per
object. `WorldSpatialIndex` is that place. It holds no state and owns no structure; it
delegates, and it is the one name a caller has to know.

## Two levels, because there are two kinds of caller

The physics-level queries return `PhysClass` and serve rendering, placement and
lighting:

```text
Query_Point      Query_AABox      Query_OBBox
Query_Sphere     Query_Frustum    Query_Ray
Query_Placement_Overlap           Query_Nearby_Lights
```

The game-object queries return `PhysicalGameObj` and serve area damage, proximity and
AI, which is where the whole-world scans actually were:

```text
Query_Game_Objects(AABox)   Query_Game_Objects(OBBox)   Query_Game_Objects_In_Sphere
```

The second is the first plus one cast. Every `PhysicalGameObj` makes itself its physics
object's observer when it is built, so a collected `PhysClass` reaches its game object
through `Get_Observer()`. That cast is `reinterpret_cast` to `CombatPhysObserverClass`,
matching every other place in Combat that walks a collected list — `PhysObserverClass`
has no `As_` methods to ask with.

The consequence is worth stating plainly: **an object with no physics object cannot be
found by a spatial query.** Script zones are the set that matters, and they are exactly
the set the old scans skipped by hand with `// zones have no phy obj`.
`ScriptZoneGameObj::Find_Closest_Zone` therefore still walks the object list, and
correctly.

## Names

Section 16 sketches `QueryAABB` / `QuerySphere` / `QueryFrustum`. The methods here are
`Query_AABox` / `Query_Sphere` / `Query_Frustum`: the roadmap gives the shape of the
service, and the surrounding tree gives the spelling. `AABox` rather than `AABB`
because `AABoxClass` is what the type is called in this engine.

## A query is a superset, never the answer

Culling systems index axis-aligned bounding boxes. A sphere query is therefore the
sphere's bounding box, which over-collects at the corners, deliberately: the exact test
belongs to the caller, who almost always has a sharper one than "is the bounding box
within r" — distance to a bullseye, a view arc, a line of sight. Rejecting corner cases
inside the service would cost a test per candidate and change no answer.

This is what makes the moves below safe. A caller keeps its exact test and changes only
what it runs that test over.

## Consumers moved

| Site | Was | Now |
|---|---|---|
| `explosion.cpp`, `Create_Explosion_At` | whole object list, distance per object, ray cast per candidate | sphere query at the blast, `QUERY_DYNAMIC` |
| `smartgameobj.cpp`, `Think` | every unit walked every object, once or twice a second each | sphere query at the eye, radius `SightRange * GlobalSightRangeScale` |
| `cinematicgameobj.cpp`, auto-fire | whole object list | sphere query at weapon range |
| `conversationmgr.cpp`, `doors.cpp`, `raveshawbossgameobj.cpp`, `repairbaygameobj.cpp`, `sakurabossgameobj.cpp`, `scriptcommands.cpp`, `scriptzone.cpp`, `soldier.cpp` (×2), `vehiclefactorygameobj.cpp` | `PhysicsSceneClass::Get_Instance()->Collect_Objects(...)` direct | `WorldSpatialIndex::Query_AABox` / `Query_OBBox` |

The second group was already spatial and already fast. Routing it through the service
anyway is directive 0.4: one way to ask a question, not two that happen to agree.

Two behaviour notes, both deliberate:

- The explosion's *force victim* takes its damage whether or not it is close enough to
  be in the blast, so it is applied outside the query rather than found by it, and
  skipped inside the loop. The old code got the same result by special-casing it while
  scanning.
- The cinematic auto-fire picks, among several enemies in range, whichever comes last
  out of the query rather than whichever came last in the object list. Both orders are
  arbitrary; the range test is unchanged.

## Consumers not moved, and why

- **Rendering** already goes through these culling systems: it is `PhysicsSceneClass`'s
  own visibility pass, inside the delegate. There is nothing to reroute, and routing a
  class through a facade over itself would be worse.
- **`airstripgameobj.cpp`** searches for landing pads within `LandingPositionDistance`,
  where a distance of zero means *unbounded*. A sphere cannot express that, it runs once
  at building init rather than per frame, and splitting it into a query plus a fallback
  scan would trade clarity for nothing.
- **`c4.cpp` `Maintain_C4_Limit`**, radar, team change, console listings and the
  scriptcommands that enumerate by type are not spatial questions. They want every
  object, and the object list is the right answer to that.
- **Terrain, foliage, roads/bridges/water, procedural generation** have nothing to query
  yet. They arrive with P11 and after, and `Query_AABox` / `Query_Frustum` are waiting.
- **Commander building placement** is what `Query_Placement_Overlap` exists for; the
  caller arrives with the Commander phase.
- **Nearby lights**: `Query_Nearby_Lights` wraps `Collect_Lights`, which was static
  lights only — dynamic lights were not in a culling system, as the physics scene said
  at the `// TODO: Dynamic lights!!` where it collected them. P21 closed that: the
  dynamic lights live in `WorldLightManager`'s own grid and `Collect_Lights` now honours
  its `dynamic_lights` argument, so both halves of the query are spatial. See
  `WorldLightManager.md`.

## What is not proved

Nothing here has been measured. The claim behind Section 16's acceptance line —
*common large-world queries no longer require scanning the complete global object
list* — is structural: the scans are gone from the sites listed above, and can be seen
to be gone. What a frame costs before and after, in a real level with a real object
count, wants a run and a profile, and the `WWPROFILE("See")` block already around the
sight query is where that would be read.
