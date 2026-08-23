# SSGM — the server-side game manager, natively

## 1. What it is

The 4.8.4 package ships two different things under one `gm*` prefix. One is a set of
scripts a level carries. The other is **SSGM**: the layer a *server operator* configures
and a level knows nothing about — which buildings are in play, whether purchases cost
anything, who owns which vehicle, what a character spawns holding, and a running
commentary on all of it down a TCP port.

The user's direction was explicit: implement the server layer as well.

## 2. What was declined, and why

4.8.4 installed SSGM by overwriting engine memory with a table of hook functions
(`AddChatHook`, `AddObjectCreateHook`, `AddThinkHook`, …) and then fanning each hook out
to plugin DLLs named in `ssgm.ini` and loaded with `LoadLibrary`.

Neither half is reproduced.

- **The hooks are already events.** Every extension point SSGM patched is raised
  natively by the canonical function that owns the behaviour — see
  `NativeEventDispatch.md`. `SSGMManagerClass` subscribes to the bus like any other
  listener.
- **The plugin loader is declined under roadmap directive 0.5.** The bus is the
  extension point now, and it does not need a DLL to reach. `RegisterEvent` /
  `UnregisterEvent` and the twenty-two `Add*Hook` names are recorded `n/a-plugin-hook`
  in `TTScriptApiGap.tsv`.

## 3. Where it lives

| Piece | Home | Why there |
| --- | --- | --- |
| `SSGMSettingsClass` | `Code/Combat/ssgmsettings.h` | the objects the settings govern are Combat objects |
| `SSGMGameLog` | `Code/Combat/ssgmgamelog.h` | the scripts that write to it compile into `leveledit`, which does not link Commando |
| `SSGMManagerClass` | `Code/Commando/ssgmmanager.h` | needs the player roster, the game data and the console |
| the `SSGM_*` scripts | `Code/Scripts/TT_SSGM.cpp` | scripts |

The manager registers on `ServerStartup` and unregisters on `ServerShutdown`, so a pure
client never has one and no handler has to ask whether it is authoritative.

## 4. Settings

`ssgm.ini` is read with SSGM's own option names and SSGM's own defaults, so a server with
no `ssgm.ini` behaves exactly as SSGM does with no `ssgm.ini`.

**Two scopes, one value.** SSGM reads `[General]` at startup and the map's own section at
every level load, and `ForceTeam`, `WeatherType` and `ExtraKillMessages` may be answered
differently per map. Rather than keep a global and a map copy of each and make every
reader remember which to consult, the global value is the fallback and what
`SSGMSettingsClass` publishes is always the answer for the level now loaded.

An operator who edits the file between maps gets the new settings without restarting: the
manager compares the file's modification time at every level load. The log port is the
one exception — a socket that tools are connected to is not moved underneath them, and
SSGM does not move it either.

**Three settings carry but match nothing yet.** `DisableTechCenters`, `DisableSpecials`
and `DisableNavalFactories` name building types this tree does not have;
`BuildingConstants::BuildingType` stops at `TYPE_BASE_DEFENSE`. They are read and applied
as soon as those building types exist. (Naval factory is already deferred to Zero Hour
Feature 7 — see `TTParityMatrix.md`.)

## 5. Vehicle ownership

SSGM has two notions and 4.8.4 could only add one of them to the engine.

- **The lock** — who may get in — is `VehicleGameObj::Set_Owner`, which 4.8.4 patched in
  and which is already merged here.
- **The claim** — whose vehicle this is — SSGM kept on a script attached to every
  vehicle, because it could not add a second field.

The claim is now `VehicleGameObj::Set_Claimant`, beside the lock. Answering "whose
vehicle is this" is a field read rather than a walk of every vehicle in the level, which
is what `Find_My_Vehicle` was.

The chat commands (`!bind`, `!lock`, `!bl`, `!unbind`, `!unlock`, `!vkick`, `!c4`) are
answered on the `Chat` event before the message is distributed, and the reply goes to the
one player as a private message — 4.8.4 round-tripped it through the console's `ppage`
command because a script could not address a client. It can now.

`!c4` reports the counts but no limit: SSGM printed a limit read from a 4.8.4 engine
value, and this tree caps nothing.

## 6. The log socket

One TCP port carries both directions. Records are a three-digit channel, a timestamp
where the channel has one, the text, and a terminating nul:

| Channel | Contents |
| --- | --- |
| `000` | general server messages, tagged with a category a tool can filter on |
| `001` | the game log — kills, purchases, buildings — only when `EnableGamelog` is set |
| `002` | whatever went to the game's own log file |
| `003` | whatever the console printed |
| `nnn` | a custom channel |

Anything a connected tool sends back is read a line at a time and raised as
`GameEventBus::ConsoleInput`, which the game subscribes to and parses. That channel is
new: it is what let the log move down into Combat.

Three corrections against 4.8.4's implementation, all of them bugs rather than
differences: a closed connection was removed from inside a forward loop, which skipped
the next connection; a full buffer with no newline in it read as a graceful close
forever; and a failed send was never noticed.

## 7. Scripts

`TT_SSGM.cpp` carries 32 registrations. Two shapes are collapsed, because in both cases
4.8.4 was writing out a table by hand:

- the twenty per-character backpacks (`SSGM_Powerup_Havoc`, `SSGM_Powerup_Sakura`, …) are
  one script under twenty names, each name fixing its character with a virtual rather
  than a script parameter — a parameter would default to zero and turn every backpack in
  the game into a stealth black hand;
- `SSGM_Powerup`'s two-hundred-line chain of preset-name tests is a table of three
  strings.

`SSGM_Log_Key` is here too, and it is a key hook: it writes a fixed line to the
server log -- `"Havoc: !vote yes"` -- when the player presses a named key, so a
moderation tool watching the log can act on a vote nobody typed. `SSGM_Soldier`
attaches it twice, for `VoteYes` and `VoteNo`.

The other three key scripts from `gmsoldier.cpp` are **not** scripts here.
`SSGM_C4_Key`, `SSGM_Bind_Key` and `SSGM_BL_Key` did nothing but call what
`!c4`, `!bind` and `!bl` call, from a script attached to every player, because
a plugin DLL had no other way in. `SSGMManagerClass` subscribes to
`GameEventBus::PlayerKey` and answers `C4Count`, `VehBind` and `VehBL` beside
the chat words themselves, stopping dispatch when it does so that a level key
of the same name cannot fire twice. Under directive 0.4 there is one owner and
one path, and the owner is the layer whose commands these are.

## 8. Script commands this needed

Eight additions to `ScriptEngine`, all of them names in `TTScriptApiGap.tsv`:

| Command | Notes |
| --- | --- |
| `Get_Translated_Preset_Name` | out-parameter, so there is no buffer to free — the allocating-string shape is what kept this family blocked |
| `Get_Translated_Definition_Name` | same |
| `Remove_Weapon` | by name, out of the weapon bag |
| `Set_Skin` | the armour type an object is wearing |
| `Set_Max_Health` / `Set_Max_Shield_Strength` | |
| `Set_Damage_Points` / `Set_Death_Points` | `DefenseObjectClass` grew the two setters beside its getters |
| `Damage_All_Objects_Area` | targets collected before any damage is applied, because applying it can empty the list being walked |
