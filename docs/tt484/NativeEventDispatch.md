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

### 5.4 The `UDP fixes` cluster — resolved

This document previously recorded the six `WriteMemory` patches commented
`UDP fixes` (`tt.cpp:1810-1815`) as unreadable without a disassembly the project
does not keep. They have now been read.

**The binary.** TT picks its address table by testing 19 bytes at a fixed
address (`scripts/engine_common.cpp:80,95-97`); `0x0078CE49` selects `Exe == 0`,
the game client. The image that satisfies it is **`Game.exe`, 4,399,104 bytes,
md5 `26a203bdc1e58909644dc74694c16fde`**. `RenegadeR.exe` from a TT r3474 install
is *not* it — it carries the same signature at `0x00765737`, so every TT 4.8.4
address lands mid-instruction there and decodes as nonsense. Anyone re-running
this work must check the signature before trusting an address.

**What the six patches say.** All of them are the same edit at six `recvfrom()`
call sites: the immediate of a `cmp reg, -1` becomes `4` and the following
branch is retargeted or resized to match.

    n = recvfrom(...);
    if (n == SOCKET_ERROR) discard;      ->      if (n < 4) discard;

Stock rejected only `SOCKET_ERROR`, so a datagram of 0-3 bytes went straight
into code that subtracts a four byte header from the length. The name is
accurate after all; the addresses are spread out because the receive loop is
duplicated across four subsystems.

| Patch | Stock site | Stock code | Native owner | Status |
| --- | --- | --- | --- | --- |
| `UDP1` | `0x0046A76E` | `cmp edi,-1` / `jz` | `Code/Commando/natsock.cpp` — `SocketHandlerClass` | merged |
| `UDP5` | `0x007725B2` | `cmp edi,-1` / `jz` | `Code/SControl/servercontrolsocket.cpp` — `ServerControlSocketClass` | merged |
| `UDP4` | `0x0061F99B` | `cmp esi,-1` / `jnz` | `Code/wwnet/netutil.cpp` — `cNetUtil::Lan_Servicing` | merged |
| `UDP6` | `0x00773843` | `cmp eax,-1` / `jz` | GameSpy SDK | N/A — not in tree |
| `UDP7` | `0x007757F2` | `cmp eax,-1` / `jz` | GameSpy query-and-reporting SDK | N/A — not in tree |
| `UDP8` | `0x00775EED` | `jz` → `nop; jmp` | same GameSpy dispatcher | N/A — not in tree |

`UDP1` and `UDP5` are the serious ones. Both loops do

    packet->BufferLen = result - sizeof(packet->CRC);
    memcpy(packet->Buffer, ReceiveBuffer + sizeof(packet->CRC), packet->BufferLen);

with `BufferLen` an `int` and the memcpy count unsigned, so a zero byte datagram
requests a copy of about 4GB out of a 640 byte stack buffer. One packet, no
authentication, no session. The two sites are the only two
`RECEIVE_BUFFER_LEN 640` handlers in the tree and the stock image carries
exactly two copies of the loop.

`UDP4` is milder: a runt broadcast reached the LAN callback with a bit length
smaller than the fields that callback reads.

The merged form differs from TT in one place. TT sends a runt packet down the
same path as `SOCKET_ERROR`, which **breaks out of the receive loop** for that
tick — so a stream of one byte packets stops the client draining real ones. The
native version discards and continues; the loop head re-reads `FIONREAD` and is
already bounded by its own retry counter, so continuing is safe and strictly
better.

`UDP6`/`UDP7`/`UDP8` land in the GameSpy SDK statically linked into the stock
binary — `UDP8`'s function formats `\queryid\%d.%d`, and `UDP8` itself is not a
bounds fix at all: it forces the `;`-prefixed remote command dispatch through
`vtbl+0xFC` to be skipped unconditionally. That SDK is not part of this tree and
its master server is gone, so the three have no native destination.

### 5.5 Other opaque patches read from the same image

- **`SlidePrecision`** (`0x007E1880`, "wall lag fix") — stock `1.01f`, TT writes
  `1.0f`. Read at `0x0065B32A` in the contact-normal loop of
  `Code/wwphys/phys3.cpp`. Scaling the removal of the inward velocity component
  by 1.01 does not cancel that component, it reverses a little of it, so a body
  sliding along a wall is pushed off every frame. **Merged.**
- **`StartBugFix`** (`0x004C188C`, "start button bug fix") — stock
  `jl short 0x004C18B0`, TT writes `jmp 0x004C1933`, skipping a sample-append
  and the averaging loop that follows it. The containing function
  (`sub_4C16D0`) looks up a 280 byte record by name with `_strcmpi`, keeps
  timestamps in a rolling 4000 ms window, and returns a count through its second
  argument — a named-statistics collector. Its OpenW3D owner is **not yet
  identified**; recorded here so the next pass starts from the evidence rather
  than the opcode.
- **`VehicleOwnershipPatch`** (`hookAsCall 0x0073F22D`) — the call site is
  `sub_73F0C0` calling `sub_708AF0`; it is gated on the `VehicleOwnershipDisable`
  ini option, so it is a TT feature toggle rather than a fix. Deferred with the
  rest of the vehicle-ownership work.

### 5.6 Byte-patch triage

Every byte-patch row in `TTHookSites.tsv` (135 with an address: 100
`WriteMemory`, 27 `WriteNop`, 12 `WriteJump`, and the rest) was disassembled in
one pass against the stock image. 124 land inside a function. The categories:

| Category | Sites | Disposition |
| --- | --- | --- |
| `PacketManager` singleton redirect (`mov ecx, offset unk_854708`) | ~28 | already merged in 5.1 |
| `cConnection`/`cRemoteHost` region, no intent comment | ~15 | already merged in 5.2 |
| `recvfrom` length checks (`UDP fixes`) | 6 | merged, 5.4 |
| Renderer/shader hooks — vertex declarations, `SetTransform`, `CreateIndexBuffer` | ~6 | out of scope, directive 0.6 |
| TT subsystem wiring — weather manager, screenshot, custom keys, RenLogMon, `disable OverlayGameModeClass`, `remove call to X::Initialize` | ~20 | **declined**: installs TT's own subsystems, not a stock behaviour to merge |
| Purchase terminal — `new unpurchasable logic` (10), `PT keypress fix` (2), `PT chatbox fix`, `enable secret PT pages` (2), `"building" message change` | 16 | open, directive 0.9, highest behavioural risk |
| Remaining discrete gameplay/UI fixes | ~25 | open, see below |

Two matrix guesses were wrong and are corrected in the TSV: `UDP fixes` was
attributed to `Code/wwnet/connect.cpp` (real owners in 5.4) and `wall lag fix`
to `Code/wwphys/humanphys.cpp` (real owner `phys3.cpp`).

**Declined with reason.** `Do not load all .mix files at startup`
(`WriteJump(0x0043907C, 0x00439192)`) does not trim the `data/*.mix` scan — it
jumps over the whole file-factory mount block, `Always2.dat`, `Always.dbs`,
`Always.dat` and the scan alike, because TT substitutes its own VFS. That is TT
infrastructure, not a fix to the stock path, and OpenW3D keeps its own file
factory (`Code/Commando/init.cpp:766-791`).

**Merged from this pass.** `make vehicles not die when they flip over`
(`0x0062D019`, `jz` → `jmp`): the rolled-over countdown in
`VehiclePhysClass::Timestep` is removed along with `ExpireTimer` and
`EXPIRE_SECONDS`, which had no other reader. A vehicle resting upside down no
longer destroys itself after four seconds.

To regenerate the raw windows, run the batch dump over `Game.exe` with the
headless IDA session wrapper; the address column of `TTHookSites.tsv` is the
input and only the first address of each pair is game-side.

### 5.7 Still open

The `wwnet` remainder, then `Commando/cnetwork.cpp` (32), then the purchase
terminal, combat objects and UI per section 4.
