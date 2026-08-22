# Phase 3 — Native event dispatch

Roadmap Section 9. This document records what the native event layer is, where
each event is raised from, and what remains of the hook conversion.

Companion documents: `TTParityMatrix.md` §4 (the hook mechanism analysis) and
`TTHookSites.tsv` (the per-site catalog).

---

## 1. What was built

`Code/Combat/gameeventbus.h` / `.cpp`.

The 4.8.4 package reached engine behavior by overwriting executable memory at
known addresses. Section 9 asks for that need to be deleted, not relocated: the
behavior merges into the canonical OpenW3D function, and where the package was
exposing a genuine extensibility point rather than changing behavior, a typed
event takes its place.

`gmplugin.h` in the donor tree is what makes the second half concrete. Its
`EventType` enumeration is the package's own list of extension points, and it
lines up almost exactly with the roadmap's required event families. That list,
plus the roadmap's, is what the bus covers.

### Design

- `GameEventClass` — base payload. A handler that has answered definitively
  calls `Stop_Dispatch`; vetoable events carry their own decision field, so
  stopping dispatch is not by itself a veto.
- `GameEventChannelClass<EVENT>` — one family. `Register` returns a token,
  `Unregister` takes one. Handlers run in registration order on the raising
  thread. Registering or unregistering from inside a handler is legal: the
  subscription list is read once per dispatch and compacted afterwards, so a
  handler added mid-dispatch does not see the event in flight and one removed
  mid-dispatch does not run again.
- `GameEventBus` — the channels, plus a `Raise_*` helper per family so the
  payload is built in exactly one place. Helpers for events whose payload costs
  anything to assemble check `Has_Subscribers` first.

Nothing in the bus touches rendering, input or dialogs, so a dedicated server
raises the same events a listen server does minus those whose owner is
client-only.

---

## 2. Events and their canonical owners

Thirty channels. Twenty-seven are raised from a canonical owner; three are
declared but not yet raised, for the reasons given in §3.

| Family | Event | Raised from |
| --- | --- | --- |
| object creation | `ObjectCreate` | `ScriptableGameObj::Start_Observers` |
| object destruction | `ObjectDestroy` | `ScriptableGameObj::Set_Delete_Pending` |
| level load/unload | `PreLoadLevel` | `CombatManager::Pre_Load_Level` |
| | `LevelLoaded` | `CombatManager::Post_Load_Level` |
| | `LevelUnloaded` | `CombatManager::Unload_Level` |
| player join/leave | `PlayerJoin` | `cPlayerManager::Add` |
| | `PlayerLeave` | `cPlayerManager::Remove` |
| connection acceptance | `ConnectionAccept` | `cNetwork::Application_Acceptance_Handler` |
| chat | `Chat` | `cCsTextObj::Act` |
| | `HostMessage` | *(declared; see §3)* |
| radio | `Radio` | `CSAnnouncement::Act` |
| host/server lifecycle | `ServerStartup` | `cNetwork::Init_Server` |
| | `ServerShutdown` | `cNetwork::Cleanup_Server` |
| purchase | `Purchase` | `VendorClass::Purchase_Vehicle` / `_Powerup` / `_Character` |
| | `PurchaseComplete` | `VendorClass::Purchase_Item` |
| refill | `Refill` | `VendorClass::Purchase_Item`, supply branch |
| damage | `Damage` | `DamageableGameObj::Apply_Damage` |
| kill | `Kill` | `DamageableGameObj::Apply_Damage`, health-exhausted branch |
| weapon events | `WeaponFire` | `WeaponClass::Do_Fire` |
| | `WeaponChanged` | `WeaponBagClass::Select_Index` |
| | `PlayerKey` | *(declared; see §3)* |
| dialog events | `Dialog` | *(declared; see §3)* |
| console output | `ConsoleOutput` | `ConsoleModeClass::Log_To_Disk` |
| shader/render notify | `RenderNotify` | *(declared; see §3)* |
| game mode lifecycle | `GameModeStart` | `GameModeClass::Activate`, on the transition only |
| | `GameModeStop` | `GameModeClass::Deactivate` |
| | `GameOver` | `cGameData::Game_Over_Processing` |
| | `Think` | `CombatManager::Think`, last |
| network visibility / dirty bits | `NetworkVisibility` | `BaseGameObj::Set_Object_Dirty_Bit`, creation bit |
| | `NetworkDirty` | `BaseGameObj::Set_Object_Dirty_Bit` |

### Decisions worth recording

**Replication events are raised from `BaseGameObj`, not from `NetworkObjectClass`.**
`Code/wwnet` includes no `Code/Combat` header anywhere, and a game event bus is
a game concept. `BaseGameObj` is the game-side floor of `NetworkObjectClass` and
both dirty-bit setters are virtual, so overriding there reports every object a
subscriber could care about while leaving the transport layer alone.

**Per-client visibility is the creation bit.** There is no `BIT_DELETION` in
this engine; an object leaves a client through the delete-pending path, which
already raises `ObjectDestroy`. So `NetworkVisibility` reports the creation bit
going on and off, and nothing pretends there is a separate visibility channel.

**Purchase refusals are engine refusals.** The donor's `PurchaseStatus` values
were derived from Renegade's `PURCHASE_ERROR` and are numerically identical to
it. `vendor.cpp` converts between them in a `switch` rather than casting, so
that stays a stated fact. The three "allow" values are real: `ALLOW_FREE` skips
the debit, `ALLOW_NO_SPAWN` skips queueing the vehicle on the factory, and both
still return success.

**Damage is reported pre-scaling.** `Apply_Damage` raises the event before
armor and warhead multipliers, so the amount is what the weapon asked for. A
refusal returns early: no health change, no `Damaged` notification, no kill.

**`TextMessageEnum` and `AnnouncementEnum` moved.** Both were defined inside
network event headers in `Code/Commando`, which the bus cannot include. They now
live in `Code/Combat/communicationtypes.h`, defined once, with `sctextobj.h` and
`AnnounceEvent.h` including it. `TEXT_MESSAGE_TMSG` was appended for the console
TMSG path; both enumerations are wire formats, so values append and never
insert.

---

## 3. Declared but not yet raised

These three have no canonical owner in OpenW3D today. They are declared because
they are named in the roadmap's required families and because the class that
will own them is already scheduled; nothing fakes an emitter for them.

| Event | Waiting on |
| --- | --- |
| `Dialog` | `ScriptedDialogClass`, which OpenW3D does not have. Matrix §5.8 assigns its 17 declaration rows to Phase 5. |
| `PlayerKey` | The client-to-server logical key message. The donor added it; stock Renegade forwards no such message. Arrives with the input merge (`Code/Combat/input.cpp`, 4 hook sites). |
| `RenderNotify` | The shader layer, excluded by directive 0.6. The 104 `ttinit/shaderhooks.cpp` sites are out of scope. |
| `HostMessage` | Console-originated chat. Reaches `cScTextObj` directly rather than through `cCsTextObj::Act`; folded in with the console-command merge. |

---

## 4. What remains of Phase 3

The event layer discharges Section 9's "native event design" requirement and
the required-event-families audit. It does not discharge the per-site behavior
merges, which are the bulk of the phase.

From `TTHookSites.tsv`: 760 sites analysed, 104 excluded as shader work, **656
in scope**, of which 12 are `DROP` (the donor nulling out stock debug routines,
nothing to port). The named-replacement sites resolve to 507 distinct donor
symbols, each a reimplementation of the stock function it displaced; merging one
means diffing that reimplementation against the canonical OpenW3D function and
taking the difference.

Recommended order, from matrix §4.6:

1. **Netcode** — roughly 200 sites: `wwnet/packetmgr.cpp` (81, but §4.5 shows
   these are 2 logical patches), `wwnet/connect.cpp` (56),
   `Commando/cnetwork.cpp` (32), `wwnet/rhost.cpp` (19). Gates P40/P41.
2. **Purchase terminal** — 19 sites on `dlgcncpurchasemenu.cpp` and
   `dlgcncpurchasemainmenu.cpp`. Mostly raw byte patches, so these need
   behavioural reimplementation from their intent comments rather than
   mechanical translation. Highest behavioural risk; directive 0.9 requires
   canonical PT interaction to survive.
3. **Combat objects** — `Combat/soldier.cpp` (30), `Combat/combat.cpp` (20),
   `VehicleGameObj`, `HumanStateClass`, `C4GameObj`, `BeaconGameObj`.
4. **UI and presentation** — `wwui` (31), HUD, dialogs, weather.

The `PacketManagerClass` cluster is the cheapest large win: all 57 of its sites
rewrite the operand of a `mov ecx, PacketManager` instruction so it points at
the donor's own singleton, duplicated across the two executable variants. In a
source merge the donor implementation simply becomes `Code/wwnet/packetmgr.cpp`
and every reference binds at link time.

---

## 5. Merged so far

### 5.1 `PacketManagerClass` — 57 sites, done

Its delta codec is not even reimplemented in the donor tree: both halves are
declared `RENEGADE_FUNCTION` and call back into the stock binary, and the
donor's own attempt at `Build_Delta_Packet_Patch` sits commented out above them
marked "Doesn't seem to work correctly". So the whole rewrite reduces to three
behavioural differences, all merged:

- `Get_Packet` computed the wrapper CRC over `bytes - sizeof(crc)` from past
  the wrapper without checking the datagram was longer than the wrapper. Any
  peer made that length negative with a short datagram.
- `Break_Packet` bounded the receive buffers with `PACKET_MANAGER_RECEIVE_BUFFERS`
  — the client constant — rather than the runtime `NumReceiveBuffers`, so a
  server used 128 of the 2048 slots it had allocated.
- `Break_Packet` recursed once per piggybacked block over attacker-supplied
  data. It iterates now.

Two bounds in its sub-packet loop were tested after the write rather than
before: the buffer index, which the base packet could leave one past the end,
and the read cursor, which was never checked against the datagram at all.

### 5.2 `cConnection` — 49 sites, and `cRemoteHost` — 19

Most of both reimplementations match stock function for function. The stale
unreliable drop, the refusal dedupe, the per-type receive stats and the resend
timeout averaging are all already in OpenW3D. `Adjust_Resend_Timeout` reads
differently from the donor's and works out identical for every input. What was
left:

| Merged | Was |
| --- | --- |
| `Get_Remote_Host` returns null out of range | asserted the range, then indexed anyway — and `WWASSERT` compiles out of release |
| `Receive_Packet` goes through that accessor | indexed `PRHost` directly with the wire-supplied sender id, which `cPacket` decodes as a *signed* char |
| unknown packet type is a counted discard | `DIE` |
| connect request at a client, or carrying a sender id, is a counted discard | `WWASSERT` |
| `ExtendedAverageCount += NumInternalPings` | `++`, against a total summed over `NumInternalPings` samples — the lifetime average ping was inflated by the sample count |
| flood ping-spike test always runs | gated on `MaximumBps < 100000`, so it was off for any host configured above that |
| flood resend test gates on the resend count | needed >20 packets queued, but a flooded link drains its queue about as fast as it fills |

### 5.3 Still open

The `wwnet` remainder, then `Commando/cnetwork.cpp` (32), then the purchase
terminal, combat objects and UI per section 4.

One cluster is **not resolvable from the donor tree alone**: the six
`WriteMemory` byte patches commented `UDP fixes` (`tt.cpp:1810-1815`). They
rewrite comparison operands and jump opcodes at six addresses spread across
unrelated subsystems, carry no replacement symbol, and the name does not match
any UDP code — the addresses are nowhere near the netcode. Reading them needs a
disassembly of the stock binary, which this project does not keep. They are the
`low` confidence rows in the matrix.
