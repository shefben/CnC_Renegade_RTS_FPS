# Unstarted Features

Backlog of every remaining roadmap work item, one line each.
**This file is delete-only.** When an item is finished, remove its line.
Do not rewrite, reformat, or add prose here.

Generated from roadmap v1.1.0 Section 53 order. 46 phases, P00-P45.
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

- [ ] Acceptance: The TT parity matrix has no `UNREVIEWED` or unexplained `MISSING` entries.

## P06: TT compatibility/cleanup/testing HARD GATE (roadmap Section 12)

- [ ] stock script catalog test
- [ ] TT script catalog test
- [ ] duplicate script-name collision test
- [ ] save/load script recreation
- [ ] TT building/factory object creation
- [ ] TT per-client visibility
- [ ] TT purchase/refill hooks
- [ ] TT damage/kill hooks
- [ ] TT chat/player hooks
- [ ] TT dialogs
- [ ] TT collision groups
- [ ] controller behavior where supported
- [ ] client/server map transition
- [ ] dedicated server
- [ ] clean startup with no scripts-related DLLs
- [ ] tool/editor build

## P07: Freeze Zero Hour donor; capability/port matrices (roadmap Section 13)

- [ ] TheSuperHackers Zero Hour donor SHA frozen and documented
- [ ] `Core + GeneralsMD` explicitly documented as the sole SAGE implementation donor
- [ ] `Generals/` excluded from the implementation workflow
- [ ] all relevant Zero Hour engine areas classified
- [ ] all 25 known macro-systems mapped to actual `Core`/`GeneralsMD` sources
- [ ] additional reusable Zero Hour capabilities discovered by the audit are added to the port matrix
- [ ] relevant TheSuperHackers fixes/enhancements for selected subsystems are classified
- [ ] every rejected engine-looking capability has a written reason
- [ ] no plan exists to build or stage through a predecessor-game implementation before the Zero Hour implementation

## P08: Asset exclusion / residency (roadmap Section 14)

- [ ] Acceptance: Repeated map/world load/unload does not invalidate retained assets or leak unbounded resources.

## P09: Shader manager architecture (roadmap Section 15)

- [ ] Acceptance: Existing W3D materials remain functional and new donor systems share one state/shader management layer.

## P10: Spatial query/index layer (roadmap Section 16)

- [ ] Acceptance: Common large-world queries no longer require scanning the complete global object list.

## P11: Hybrid heightmap/terrain framework (roadmap Section 17)

- [ ] Acceptance: FPS and vehicle gameplay works on runtime-created heightfield terrain while arbitrary W3D geometry remains supported.

## P12: Far/background terrain LOD (roadmap Section 34)

- [ ] Acceptance: - no visible cracks at near/far boundaries under normal camera ranges; - far terrain uses substantially fewer vertices/draw submissions than near terrain; - terrain edits invalidate the correct background regions; - one canonical terrain query/collision representation remains authoritative.

## P13: Terrain texture system (roadmap Section 18)

- [ ] Acceptance: A generated heightfield can obtain coherent terrain materials entirely from runtime data/masks.

## P14: Runtime road system (roadmap Section 19)

- [ ] Acceptance: Road geometry can be generated at runtime from centerlines without a unique modeled mesh for every road segment.

## P15: Sectional bridge system (roadmap Section 20)

- [ ] Acceptance: Bridges support variable length, damage, broken spans, collision, and multiplayer state.

## P16: Tree/foliage buffer (roadmap Section 21)

- [ ] Acceptance: Large forests render with substantially fewer submissions than individual `RenderObj` instances.

## P17: Reflective water / river system (roadmap Section 22)

- [ ] Acceptance: Water works visually and physically without a competing collision system.

## P18: Terrain tracks / surface ribbons (roadmap Section 23)

- [ ] Acceptance: Vehicles can leave stable marks without unbounded allocations or per-mark heavyweight objects.

## P19: Surface smudge / decal manager (roadmap Section 35)

- [ ] Acceptance: Hundreds or thousands of marks can exist without one draw call or one network object per mark, and mark lifetime/eviction cannot grow memory without bound.

## P20: Projected / cached shadows (roadmap Section 24)

- [ ] Acceptance: There is one shadow implementation serving both TT-facing APIs and new world systems.

## P21: Dynamic-light filtering (roadmap Section 25)

- [ ] Acceptance: Lighting cost scales mainly with nearby/relevant lights instead of total world light count.

## P22: Particle batching (roadmap Section 26)

- [ ] Acceptance: Large firefights do not explode draw calls or allocations linearly with particle count.

## P23: Weather / environment particle layer (roadmap Section 36)

- [ ] Acceptance: Weather can cover a large outdoor scene at a stable bounded particle count without creating a `GameObj` per particle.

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

