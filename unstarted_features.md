# Unstarted Features

Backlog of every remaining roadmap work item, one line each.
**This file is delete-only.** When an item is finished, remove its line.
Do not rewrite, reformat, or add prose here.

Generated from roadmap v1.1.0 Section 53 order. 66 phases, P00-P65.
P46-P62 are the Section 52A visual-fidelity program.
P63-P65 are the Section 52B procedural-world prerequisites.
P01-P06 gate everything after them (directive 0.3).

---

## P-VALIDATOR: feature-state bookkeeping validator (roadmap 0A.11)

- [ ] Add tools/validate_feature_state.py checking all four files exist, IDs unique, no feature in two lifecycle files, Q-### references resolve, required headings present, no file accidentally empty.

## P00: Freeze/test latest OpenW3D (roadmap Section 6)

- [ ] Run stock Renegade content.
- [ ] Load at least one campaign map.
- [ ] Load at least one multiplayer map.
- [ ] player spawn
- [ ] infantry movement
- [ ] vehicle entry/exit
- [ ] purchase terminal
- [ ] building damage/destruction
- [ ] script creation
- [ ] map transition
- [ ] save/load if supported in tested mode
- [ ] listen/network play
- [ ] dedicated server startup
- [ ] Capture baseline screenshots and performance counters.

## P01: Import + inventory TT 4.8.4 (roadmap Section 7)


## P02: Reconcile TT public engine classes into canonical OpenW3D classes (roadmap Section 8)


## P04: Native stock + TT script registry (roadmap Section 10)


## P05: Port remaining TT 4.8.4 engine-facing functionality (roadmap Section 11)

## P06: TT compatibility/cleanup/testing HARD GATE (roadmap Section 12)

## P07: Freeze Zero Hour donor; capability/port matrices (roadmap Section 13)

## P08: Asset exclusion / residency (roadmap Section 14)

## P09: Shader manager architecture (roadmap Section 15)


## P10: Spatial query/index layer (roadmap Section 16)

- [ ] Consumers moved onto the service: Commander building placement, nearby lights, foliage cells, roads/bridges/water, future procedural world generation.
- [ ] Acceptance: Common large-world queries no longer require scanning the complete global object list.

## P11: Hybrid heightmap/terrain framework (roadmap Section 17)


## P12: Far/background terrain LOD (roadmap Section 34)


## P13: Terrain texture system (roadmap Section 18)


## P14: Runtime road system (roadmap Section 19)


## P15: Sectional bridge system (roadmap Section 20)


## P16: Tree/foliage buffer (roadmap Section 21)


## P17: Reflective water / river system (roadmap Section 22)


## P18: Terrain tracks / surface ribbons (roadmap Section 23)


## P19: Surface smudge / decal manager (roadmap Section 35)


## P20: Projected / cached shadows (roadmap Section 24)


## P21: Dynamic-light filtering (roadmap Section 25)


## P22: Particle batching (roadmap Section 26)

## P23: Weather / environment particle layer (roadmap Section 36)

## P24: Tracer / beam / projectile render modules (roadmap Section 27)

- [ ] Acceptance: Weapons can use pooled/batched render modules without script-spawned effect objects for every tracer.

## P25: Debris rendering (roadmap Section 28)

- [ ] Acceptance: Large destruction events do not require every visible fragment to be a complete networked `GameObj`.

## P26: Modular draw components + composite attachments (roadmap Section 29)

- [ ] Acceptance: New rendering behavior can be attached without continuously expanding every `GameObj` subclass.

## P27: Shroud / fog of war + sensor-source API (roadmap Section 30)

- [ ] Acceptance: - Commander camera movement alone reveals nothing; - Commander/tactical intelligence is independent of normal FPS physical rendering; - strategically hidden information does not leak into ordinary FPS targeting/HUD behavior; - sensor sources can be added/removed dynamically.

## P28: Shared radar / tactical intelligence (roadmap Section 31)

- [ ] Acceptance: - FPS HUD radar and Commander presentation use consistent canonical intelligence state; - powered communications buildings can expand configured strategic coverage; - disabling/destroying those buildings removes their coverage; - camera movement does not create radar contacts.

## P29: Waypoint / status-circle renderers (roadmap Section 32)

- [ ] Acceptance: Large numbers of markers are batched and respect shroud/per-player visibility.

## P30: AI state/path/guard-retaliation + human-yield hooks (roadmap Section 37)

- [ ] Acceptance: AI behavior gains the selected Zero Hour state/path improvements without introducing the SAGE `Thing`/AIPlayer world model as a second game simulation.

## P31: RTS telemetry / statistics instrumentation (roadmap Section 38)

- [ ] Acceptance: The engine can produce a structured local/session statistics snapshot useful for tests and balancing without carrying Zero Hour's Academy UI or advice rules.

## P32: Debug rendering tools (roadmap Section 33)

- [ ] Acceptance: Every new world/Commander subsystem has enough visualization to diagnose placement, culling, transform, and rendering errors.

## P33: Building W3D semantic JSON + live descriptor reload (roadmap Section 39)

- [ ] Acceptance: - W3D + JSON resolves to one validated semantic descriptor; - power/capability values are queryable from that descriptor; - valid runtime reload changes live tuning without recompilation/restart; - invalid reload preserves last known-good values; - core Commander-buildable definitions have valid descriptors.

## P34: Finite team power grid + comms/Silo capabilities (roadmap Section 40)

- [ ] Acceptance: - finite JSON-defined generation/demand works; - overload deterministically sheds buildings; - new generation or reduced demand restores them; - destroying the final generator powers down every dependent building; - canonical Renegade powered-down behavior is used; - live JSON reload recalculates correctly; - communications coverage follows online state; - Silo wallet bonuses stack for every team player; - manual priority/on-off control is authoritative.

## P35: Embodied Commander Mode camera/view (roadmap Section 41)

- [ ] Acceptance: - FPS -> Commander -> FPS works in one runtime; - same player body remains physical/vulnerable; - death exits Commander Mode; - camera movement alone reveals no hidden enemy; - FPS behavior outside Commander Mode is unchanged.

## P36: Commander sidebar + BUILDINGS/GROUND/AIR/INFANTRY + power bars (roadmap Section 42)

- [ ] Acceptance: - BUILDINGS sidebar appears on Commander entry; - visible production/building area is two rows with paging; - power bars reflect server state; - click ground factory -> GROUND_VEHICLES; - click Air Pad -> AIR_VEHICLES; - click Barracks/Hand -> INFANTRY; - return to BUILDINGS is immediate; - existing PT icons are reused where possible; - dead/offline selected facilities cannot produce.

## P37: Ghost building + server placement validation (roadmap Section 43)

- [ ] Acceptance: Real ghost follows cursor, invalid overlap turns it red, and server independently rejects the same bad placement.

## P38: Authoritative construction + procedural blocks + auto PCT/MCT (roadmap Section 44)

- [ ] Acceptance: - building constructs visibly from bottom-to-top procedural blocks; - no new authored construction asset is required; - blocks are batched/non-physical; - network/save/load reconstructs deterministic stage; - finished behavior remains disabled until completion; - final collision activates safely; - PCT/MCT nodes work; - completed Power Plant enters grid only at completion; - cancellation/destruction leaves no temporary state.

## P39: Commander AI infantry/ground/air production + control (roadmap Section 45)

- [ ] Acceptance: - Barracks/Hand produces AI infantry that exits and holds; - ground factory produces AI vehicle that clears exit and holds; - Air Pad produces AI aircraft that clears pad; - AI consumes no fake human player slots; - Commander can select/order units; - orders use canonical AI; - dead/offline facilities cannot produce invalid units; - production is bounded and does not persistently block human players.

## P40: Full save/load + multiplayer pass (roadmap Section 46)

- [ ] Acceptance: Save/load and late join preserve/reconstruct the complete RTS state without serializing renderer-only implementation details or granting clients gameplay authority.

## P41: Dedicated-server pass (roadmap Section 47)

- [ ] Acceptance: - dedicated server can host a complete Commander/RTS match; - power, production, AI orders, construction timing, placement validation, wallet caps, and tactical intelligence remain authoritative; - no renderer/UI dependency is pulled into server code merely to support Commander Mode.

## P42: Tool/editor/descriptor-validation pass (roadmap Section 48)

- [ ] Implement per roadmap Section 48.

## P43: Performance/regression pass (roadmap Section 49)

- [ ] Implement per roadmap Section 49.

## P44: Remove migration/donor-version scaffolding (roadmap Section 51)

- [ ] Implement per roadmap Section 51.

## P45: Final provenance/documentation/state-file reconciliation (roadmap Section 50, 52)

- [ ] Implement per roadmap Section 50.
- [ ] Implement per roadmap Section 52.

## P46: Sampler/filtering/antialias quality defaults (roadmap Section 52A)

- [ ] Anisotropic/trilinear/mip-bias/multisample defaults chosen from device capability in dx8wrapper, texture reduction off.
- [ ] Acceptance: Stock textures resolve visibly further at grazing angles, and stock-equivalent settings reproduce the current image.

## P47: Level-of-detail and far-terrain cross-fade (roadmap Section 52A)

- [ ] Distance-band dissolve in distlod, same blend at the Section 34 far/near terrain block handover.
- [ ] Acceptance: No single-frame LOD pop, and the far/near handover is not locatable by eye.

## P48: Server-authoritative sky clock with moving sun and moon (roadmap Section 52A)

- [ ] Replicated clock behind SkyClass::Set_Time_Of_Day/Set_Light_Direction, feeding the scene light environment and WorldShadowManager caster direction.
- [ ] Acceptance: Sun crosses the sky over a match, shadows rotate and lengthen with it, night falls, clients agree on the hour, and a zero rate reproduces authored appearance.

## P49: Time-of-day relighting of baked static lighting (roadmap Section 52A)

- [ ] Second per-vertex sun-visibility scalar beside the existing lightsolve bake, composed at runtime with current sun colour; derived at load for stock meshes.
- [ ] Acceptance: Outdoor baked lighting tracks the sun, indoor lighting does not change, and no .w3d is re-exported.

## P50: Animated static lights (roadmap Section 52A)

- [ ] Steady/flicker/pulse/warm-up/fail curves on the light itself, fail wired to Section 40 power and damage state.
- [ ] Acceptance: Powered-down or destroyed buildings go dark through the existing grid state, and steady lights render as they do today.

## P51: Height fog tied to the horizon colour (roadmap Section 52A)

- [ ] Height-based fog term with colour taken from HazeClass::Get_Horizon_Color so it moves with the sky clock.
- [ ] Acceptance: Distance fades into the current horizon colour at every hour, low ground holds fog high ground does not, and the term disabled restores authored linear fog.

## P52: Terrain and contact ambient occlusion (roadmap Section 52A)

- [ ] Per-vertex sky occlusion computed at heightfield generation into the existing patch vertex colour, plus contact darkening through the Section 24 caster path.
- [ ] Acceptance: Creases and slope interiors darken, dynamic objects are visually anchored, and no second shadow implementation exists.

## P53: Moving cloud shadows (roadmap Section 52A)

- [ ] Procedurally generated cloud density mask scrolled through the existing texproject path, driven by the P58 wind field.
- [ ] Acceptance: Patches of shade cross terrain in the wind direction under the drawn clouds, at a cost independent of object count.

## P54: Sky-driven environment reflections (roadmap Section 52A)

- [ ] Low-rate environment map regenerated from current sky/haze/sun/cloud colour and fed to the existing metalmap mapper.
- [ ] Acceptance: Vehicle reflections change with the sky clock at bounded cost, and regeneration off uses the stock map.

## P55: Sun shafts (roadmap Section 52A)

- [ ] Depth-occluded radial blur from the SkyGlowClass on-screen sun position, as one Section 15 screen-space pipeline.
- [ ] Acceptance: Shafts appear through occluders and move with the sky clock, and nothing draws with the sun below the horizon or off screen.

## P56: Tonemapping and eye adaptation (roadmap Section 52A)

- [ ] Full-screen tonemap with slow automatic exposure and a code-generated grading ramp through the Section 15 layer, behind a setting.
- [ ] Acceptance: Sunlight-to-interior transitions adapt rather than crush, bright scenes hold sky and shadow detail together, and disabled is pixel-identical to today.

## P57: Bloom on emissive and dazzle sources (roadmap Section 52A)

- [ ] Threshold/blur/add over the sources dazzle.cpp already marks, sharing the P56 render-target owner as one post chain.
- [ ] Acceptance: Flashes and lamps bleed at night, targets are allocated once, and bloom disabled leaves the image unchanged.

## P58: Wind field applied to geometry (roadmap Section 52A)

- [ ] One replicated gusting wind vector driving height-keyed vertex sway on foliage, impostors, flags and cloth, plus particle drift and the P53 cloud scroll.
- [ ] Acceptance: Everything moves with one gusting direction, Section 21 foliage collision is unaffected, and zero wind is the current static appearance.

## P59: Wetness response (roadmap Section 52A)

- [ ] Replicated wetness scalar rising with precipitation and drying afterwards, darkening albedo, lifting specular and biasing terrain layer weights; distinct from Section 36, which owns the particles.
- [ ] Acceptance: Rain visibly wets ground, vehicles and soldiers, they dry out, sheltered surfaces stay dry, and zero wetness is the current appearance.

## P60: Surface marks age with the weather (roadmap Section 52A)

- [ ] Mark fade rate scaled by the P59 wetness scalar; Section 35 pool sizes and eviction policy unchanged.
- [ ] Acceptance: Marks laid before a storm clear sooner than dry-weather marks, the pool never grows, and zero wetness reproduces current fade timing.

## P61: Progressive damage appearance (roadmap Section 52A)

- [ ] Soot/char tint ramp and night-visible ember term driven by replicated health through the existing material effect list; tint only.
- [ ] Acceptance: Damage darkens and scorches progressively, burning objects glow at night, repair reverses it, and no new model or texture is required.

## P62: Weapon-fire camera response (roadmap Section 52A)

- [ ] Per-shot field-of-view kick and roll with fast recovery on camerashakesystem, strictly cosmetic.
- [ ] Acceptance: Firing produces a quickly recovering camera response with accuracy and hit registration provably unchanged, and zero scale is current behaviour.

## P63: World metrics service (roadmap Section 52B)

- [ ] WorldMetricsClass answering stand/crouch/prone height, envelope, step height, max climbable slope, corridor width, overhead clearance, turning radius and building footprint per locomotion class and per definition, read from the existing definitions rather than restated as constants.
- [ ] Acceptance: Terrain shaping, Commander placement, AI route checks and any later generator all get dimensions from this one service, no consumer keeps its own step height or slope limit, and changing a definition changes every answer.

## P64: Vertical world structure — cliffs, escarpments, bridge structure (roadmap Section 52B)

- [ ] Escarpment polyline with upper/lower height generating face geometry and collision beside the heightfield and dressed by the existing layer table, plus BridgeSystem deck height, required clearance, abutments and piers; absorbs the bridge structural-vocabulary gap.
- [ ] Acceptance: A vertical cliff face is representable with one canonical heightfield query for ground above and below it, collision is correct, and a bridge states a clearance checkable against the terrain instead of inferred from it.

## P65: Traversability and connectivity graph (roadmap Section 52B)

- [ ] Coarse graph over terrain, roads, bridges, water and P64 escarpments, parameterised by P63 locomotion class, answering reachability, distinct routes, chokepoints and infantry-only routes; analysis structure, not a replacement pathfinder.
- [ ] Acceptance: Reachability, distinct-route and chokepoint queries answer per locomotion class on stock and generated terrain, a vehicle-impassable crossing is absent from the vehicle graph but present in the infantry graph, and Pathfind remains the only thing that moves a unit.
