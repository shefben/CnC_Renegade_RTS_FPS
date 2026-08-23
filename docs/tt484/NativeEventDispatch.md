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
| | `PlayerKey` | `ScriptKeyManagerClass::Key_Pressed` |
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

These have no canonical owner in OpenW3D today. They are declared because they
are named in the roadmap's required families and because the class that will
own them is already scheduled; nothing fakes an emitter for them.

| Event | Waiting on |
| --- | --- |
| `Dialog` | `ScriptedDialogClass`, which OpenW3D does not have. Matrix §5.8 assigns its 17 declaration rows to Phase 5. |
| `RenderNotify` | The shader layer, excluded by directive 0.6. The 104 `ttinit/shaderhooks.cpp` sites are out of scope. |
| `HostMessage` | Console-originated chat. Reaches `cScTextObj` directly rather than through `cCsTextObj::Act`; folded in with the console-command merge. |

---

## 3A. Script keys — the `PlayerKey` emitter

A script key is a key a level gives a meaning to and the engine has none for.
Stock Renegade has no such thing and forwards no such message, so `PlayerKey`
needed an emitter built rather than found. It has one now, and it is the last
facility the 4.8.4 script library was waiting on.

| Piece | Home | What it does |
| --- | --- | --- |
| the bindings | `Code/Combat/input.cpp`, `[Script Keys]` | logical name to key, in the player's own input configuration alongside every other binding, saved and loaded by the same two functions |
| the poll | `Input::Update_Script_Keys` | edge triggered, suppressed while the menu or console is up |
| the seam | `cCsScriptKeyEvent` | one name, client to server, reliable |
| the emitter | `ScriptKeyManagerClass::Key_Pressed` | raises `PlayerKey` |
| the script base | `KeyHookScriptClass` (`Code/Scripts/scripts.h`) | one subscription behind every hook in the level, not one each |

**The client is deliberately ignorant.** It does not know what a key does,
whether anything is listening, or whether the press was acted on; it forwards
the name and nothing else. That is what lets a level invent a key without
shipping client code, and it is why the decision is never taken client side.
The one thing the client does decide is that the menu and the console are not
script keys, because the player is typing.

4.8.4 did this with a `keys.cfg` of its own, a configuration dialog of its own,
and a chat message addressed to a magic recipient. None of that is reproduced:
the names live in the input configuration file the engine already owns, and the
press is a network object like every other client-to-server event here.
`AddKeyHook` and `RemoveKeyHook` stay `n/a-plugin-hook` in
`TTScriptApiGap.tsv` — the facility is answered, the exported plugin entry
points are not, which is true of all twenty-two `Add*Hook` names.

**Two names are the server's, not the level's.** `SSGMManagerClass` subscribes
to `PlayerKey` and answers `C4Count`, `VehBind` and `VehBL` beside the `!c4`,
`!bind` and `!bl` chat words that do the same thing, stopping dispatch when it
does. 4.8.4 reached those through three scripts attached to every player; they
are commands of the server layer, not behaviour of a character.

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

### 5.8 The dialog and purchase-terminal fixes

**`team information and battlefield information scroll fix`** (6 sites,
`tt.cpp:1002-1007`) — **merged**. Three jump hooks replace `On_Frame_Update` on
the battle-info, team-info and server-info dialogs; three `WriteNop`s of ten
bytes each remove an identically shaped run

    6A 00              push 0
    E8 xx xx xx xx     call MouseMgrClass::Show_Cursor
    83 C4 04           add  esp, 4

sitting between `MenuDialogClass::On_Init_Dialog` and `Get_Dlg_Item(0x4B6)`,
which matches `On_Init_Dialog` in all three sources exactly.

One defect in two halves. Stock hid the cursor when the dialog opened, and its
`On_Frame_Update` override did the key-release check and returned without
calling `DialogBaseClass::On_Frame_Update()` — the function that runs
`Update_Mouse_State()` and each control's own `On_Frame_Update()`. So there was
no pointer to aim, and the list control never saw the mouse regardless. Both
halves are needed and both are merged into the three canonical dialogs.
`MenuDialogClass` does not override `On_Frame_Update`, so calling the base
directly skips nothing.

**`dead powerplant 2x cost message change`** (`0x00481D1F`, `tt.cpp:986`) —
**merged**. The 16 byte replacement decodes to `xor edx,edx` /
`cmp byte [edi+6D4h], dl` / seven nops / `jnz` reusing the original
displacement, over a stock run of `fld` / `fcomp 0.0f` / `fnstsw` /
`test ah,41h` / `jz`. `sub_481C70` is
`CNCPurchaseMenuClass::Update_Building_Health` and `0x61E` is
`IDC_COST_X2_TEXT`; field `+0x6D4` is `BasePowered`, confirmed by the companion
patch at `0x0047EEA6` which tests the same byte before storing 2.0f.

The price doubles on `Is_Base_Powered()` (`Code/Commando/vendor.cpp:401` is the
authoritative one), but the message announcing it was driven off the power
plant's health fraction. Those are different predicates — `Power_Base()` is
server driven — so the terminal could charge double in silence, or warn while
charging normal price.

Two deliberate deviations, both because native code is not held to 16 bytes:
`Show (base->Is_Base_Powered () == false)` rather than TT's one-sided
`Show (true)`, since stock never hid the text again once the power plant came
back; and the call moves out of the `if (building != nullptr)` scope, because
whether the base is powered has nothing to do with whether a power plant object
was found — a base with none at all still pays double and still got no warning.

**`dead powerplant 2x cost disable`** (`0x00481D2E`, `0x0047EEA6`,
`tt.cpp:1165-1166`) — **deferred**. Gated on TT's `DisableCostMultiplier` ini
option. A server toggle that removes a stock rule is not a correction to it;
same disposition as `VehicleOwnershipPatch` in 5.5. Owners are identified
(`dlgcncpurchasemainmenu.cpp:389` and the branch above) if it is ever wanted.

**`radar fix`** (`WriteJump(0x006EF6C0, Enable_Radar_Patch)`, `tt.cpp:983`) —
**N/A, the patch is inert.** `0x006EF6C0` is `nullsub_212`, a one byte `retn`
called twice each from `sub_478570` and `sub_479840` — a hook of opportunity,
not a function whose own semantics matter. TT's replacement (`tt.cpp:754`) is

    BaseControllerClass* base = BaseControllerClass::Find_Base_For_Star();
    if (base) base->Enable_Radar(base->Is_Radar_Enabled());

and `BaseControllerClass::Enable_Radar` is a `RENEGADE_FUNCTION`
(`scripts/engine_tt.cpp:1222`, `AT3(0x006EFD00,...)`) that calls straight back
into the stock binary. Stock `0x006EFD00` opens with

    mov al, [esp+4]
    mov esi, ecx
    cmp [esi+6D7h], al        ; IsRadarEnabled
    jz  short loc_6EFD59

so passing the field back to itself compares it against itself and returns. The
re-assert can never fire. Nothing to merge; recorded so the row is not reopened.
`IsRadarEnabled` at `+0x6D7` also independently confirms `BasePowered` at
`+0x6D4` above.

### 5.9 The purchase terminal — 17 sites

Directive 0.9 governs this cluster: canonical PT interaction has to survive.
Fourteen sites merged, three did not.

**`new unpurchasable logic`** (10 sites, `tt.cpp:295-304`) — **merged**. The
vehicle half rewrites `0x0047F870`/`0x0047F883` in `sub_47F830`
(`CNCPurchaseMainMenuClass::Refresh_Button_States`) to

    mov cl, [ebp+6D6h]                  ; CanGenerateVehicles
    test cl, cl
    mov ecx, ebp
    jz  loc_47F930                      ; unavailable
    push 2 / call Find_Building / test eax,eax / jz 0x47F8D5
    call [eax+78h]  / test eax,eax / jz 0x47F8D5
    mov cl, [eax+8B0h] / test cl,cl / jz 0x47F8D3     ; Is_Busy

and nops the destroyed test at `0x0047F8D3`/`0x0047F8DE`/`0x0047F8E6`, so a
factory object that cannot be found no longer disables anything. The soldier
half does the same at `0x0047F96D` with `[ebp+6D5h]`. `code10` at `0x004815B4`
(`xor eax,eax` over the `Find_Base_For_Star` call) blanks the copy of
`Verify_Vehicle_Purchase` inlined into `Purchase()`.

The field offsets fall out of `basecontroller.h` exactly: `+0x6D4`
`BasePowered`, `+0x6D5` `CanGenerateSoldiers`, `+0x6D6` `CanGenerateVehicles`,
`+0x6D7` `IsRadarEnabled` — the same run the 2x-cost and radar patches landed
in, now confirmed from three independent directions.

The defect is that the client was deciding what the base can build by looking
at the objects it happens to hold. `Can_Generate_*` is server state, set by the
factories' `On_Destroyed`/`On_Revived` under `I_Am_Server()` and replicated in
`BaseControllerClass::Import/Export_Occasional`
(`basecontroller.cpp:1058,1091`). `Find_Building` returns null on a client that
was never sent the factory, and the stock code read that as "no production".

Merged as one predicate, `CNCPurchaseMenuClass::Get_Production_Status`, used by
the item greying, the hot keys and the purchase itself so they cannot disagree.
Two deliberate deviations: the destroyed/busy/unavailable labels are kept
(TT collapsed them because a byte patch cannot pick a string), and
`Verify_Vehicle_Purchase` is kept rather than blanked — it now runs on the
corrected predicate and can no longer fire off a null factory. A refused
purchase with no explanation would have been worse than either side.

The `hud.ini NewUnpurchaseableLogic` flag does not survive. Directive 0.4
forbids keeping stock and TT implementations selected by a flag, so the
replicated predicate is simply the predicate.

**`PT chatbox fix`** (3 sites, `tt.cpp:995-997`) — **merged**. `WriteNop
0x00480122, 7` removes `mov ecx,edi / call sub_4F3BC0` from the tail of
`sub_47FF10` = `CNCPurchaseMainMenuClass::Refresh_Message_Log`, which is its
`Scroll_To_End()`. Stock rebuilds the whole list on every new message —
`Delete_All_Entries` resets `ScrollPos` to 0 and `CurrSel` to -1 — then scrolls
to the bottom, so while the terminal is open you cannot read back through the
log at all and any selection is thrown away every time somebody talks.

TT's wrapper saves position and selection, rebuilds, and follows the tail only
if the view was already at the tail. Merged with three small public methods on
`ListCtrlClass` (`Get_Scroll_Pos`, `Is_Scrolled_To_End`, `Scroll_To`) rather
than TT's route of making `ScrollBarCtrl` public. `Is_Scrolled_To_End` uses
`>=`, which is true for an empty list and so subsumes TT's third site — the
`Set_Range(0,0)` prepended to the `Add_Column` call at `0x0047EB7A`.
`Scroll_To_End` picked up the `Set_Range` that TT's `listctrl.cpp` adds and
stock lacks: entries added since the last visibility pass left the bar with a
stale maximum, and `Set_Pos` clamps against it.

**`PT keypress fix`** (4 sites, `tt.cpp:998-1001`) — **merged**.
`hookAsCall2(0x00481C45, …, 5)` replaces `call sub_4814F0` inside `sub_481A80`
(`CNCPurchaseMenuClass::On_Key_Down`) *and nops the five bytes after it*, which
are `call sub_404D80` = `GameInitMgrClass::Continue_Game` — so the replacement
owns both. `Add_Item_To_Shopping_Cart` refuses silently when the slot is empty
or unaffordable, and stock went on to `Purchase()` — which ends the dialog —
and `Continue_Game()` regardless. A number key aimed at an empty or
unaffordable slot therefore shut the terminal and bought nothing. `Purchase()`
now reports whether it purchased, and only ends the dialog when it did.

`hookAsJump 0x00481FF6` extends the missing-control skip in `sub_481F90`
(`Update_Enabled_Status`) into a production test that routes to the
`ctrl->Enable(false)` arm at `0x004822A9`. Stock greyed on money and
`IsProductionDisabled` only, never on the factory.

The two `WriteNop`s at `0x0047FE5F`/`0x0047FE86` remove the
`cmp [eax+3Bh], bl / jz` pairs behind the `6` and `7` hot keys — the
`Is_Enabled()` guards on the characters and vehicles buttons. Opening the page
is safe now that the page itself carries the refusal, where stock just
swallowed the key. TT left the `8` beacon key and the mouse path guarded, so a
disabled button still cannot be clicked and the enable state stays as the cue.

**`enable secret PT pages`** (2 sites, `tt.cpp:283-284`) — **declined**, ini
gated. `0x0047EDA6` turns the laddered-game test in `sub_47ED90` from `jnz`
into `jmp` so `ExtrasEnabled` is never force-cleared, and `0x00427C80` writes
`retn 4` over the first instruction of the `extras` console command so it can
no longer clear the flag either. Gated on `hud.ini UseExtraPTPages`, default
off, and it makes content the stock game gates available in laddered games.
That is a content policy change, not a correction; same disposition as
`DisableCostMultiplier` in 5.8.

**`purchase terminal "building" message change`**
(`WriteCall(0x0047F899, Building_Msg_Check, 1)`) — **N/A**. The replacement is
`cl = factory->IsBusy || CurrentlyBuilding`, and `CurrentlyBuilding` only ever
moves under TT's `VehicleBuildingDisable` production model
(`engine2.cpp:121`, `BaseControllerClass.cpp:24-40`), which replaces the
factory-busy model with a per-team flag pushed to clients over TT's text
channel. OpenW3D has no such model and the roadmap does not ask for one, so
there is nothing to merge into.

### 5.10 Still open

The `wwnet` remainder, then `Commando/cnetwork.cpp` (32), then combat objects
and UI per section 4.
