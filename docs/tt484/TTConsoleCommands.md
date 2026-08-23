# TT 4.8.4 console commands, natively

Roadmap Section 11.7: *"Port engine-relevant console commands and debugging/admin
functions that exist in the 4.8.4 source and are not obsolete deployment code."*

Source: `tt_4.8.4/tt/consolecommands.h` and `.cpp` — 65 commands.

Six of them (`message`/`msg`, `hud`, `rlmon`, `rlmonoff`, and the two names they
alias) already existed in `Code/Commando/consolefunction.cpp` and were left alone.
Of the remaining 59, **49 are ported** and live in `Code/Commando/ttconsole.cpp`;
**10 are not**, each for a reason given below.

## How they got here

4.8.4 could not add to the engine's console function list, so it overwrote it:
`ConsoleFunctionList.Delete(8)` followed by a run of `AddToConsoleFunctionList`
calls, installed from a DLL. Nothing of that survives. The ported commands are
ordinary `ConsoleFunctionClass` objects added to the one list every other command
lives in, through `TTConsole::Add_Console_Functions`, the same way
`SystemSettings::Add_Console_Functions` already did.

The second thing that did not survive is the transport. A 4.8.4 console command
that had to reach a client wrote a private chat message beginning `"j\n"` and a
number, and a hooked client parsed the opcode back out. Every one of those is now
the script command that already does the thing — `Create_2D_WAV_Sound_Player`,
`Set_Background_Music_Player`, `Send_Message_Player` — so a console command and a
level script asking for the same effect travel the same wire and there is one
implementation of each effect rather than two.

Two effects had no per-player script command and gained one in P05:

| Effect | Command added | Client command |
| --- | --- | --- |
| A WAV file, not a preset, played at a bone of an object | `ScriptEngine::Create_3D_WAV_Sound_At_Bone_Player` | `SCRIPT_CLIENT_CMD_CREATE_3D_WAV_SOUND_AT_BONE` |
| An emoticon over a soldier's head as one other player sees it | `ScriptEngine::Set_Emot_Icon_Player` | `SCRIPT_CLIENT_CMD_SET_EMOT_ICON` |

## Ported (49)

| Group | Commands |
| --- | --- |
| Who is playing | `id`, `pinfo`, `kick2`, `eject` |
| Teams and money | `team`, `team2`, `donate`, `win` |
| Talking to players | `pamsg`, `ppage`, `tpage`, `tmsg` |
| Coloured info-box messages | `cmsg`, `cmsgp`, `cmsgt` |
| Sound | `sndp`, `sndt`, `snda`, `snd3dp`, `snd3da`, `snd3dt` |
| Music | `musicp`, `musica`, `nomusicp`, `nomusica`, `song` |
| Emoticons | `icon`, `icon2` |
| C4 | `disarm`, `disarmp`, `disarmb` |
| Limits | `mlimit`, `mlimitd`, `mined`, `vlimit`, `vlimitd`, `plimit`, `plimitd` |
| Time | `time`, `timed`, `timel`, `timeld` |
| The map cycle | `radar`, `map`, `mod`, `mapnum`, `mlist`, `mlistc` |

Behavioural notes where the native version is not a transcription:

- **`mined`** and **`disarm*`** walk the one game object list and ask each charge who
  owns it, rather than keeping a parallel C4 list as 4.8.4 did.
- **`vlimit`/`vlimitd`** set and read `VehicleFactoryGameObj::Max_Vehicles_Per_Team`,
  which is where this tree already keeps the limit. 4.8.4 kept its own counter and
  told clients about it; the factory is the one owner here.
- **`mlist`/`mlistc`/`mapnum`** work on `cGameData::MapCycle`, the map cycle this
  tree already has, rather than 4.8.4's separate hundred-entry map list.
  `cGameData::Get_Map_Cycle_Index` was added to read the current position.
- **`disarmb`** destroys a player's beacons. 4.8.4 called its own `Disarm_Beacons`;
  `BeaconGameObj` has no disarm entry point here, and destroying the beacon is what
  that function did.
- **`team`** disarms the player's C4 and changes their team through
  `ScriptEngine::Change_Team`, which already destroys the body. 4.8.4 changed the
  team and then destroyed the object as a separate step.

## Not ported (10), with reasons

| Command | Why not |
| --- | --- |
| `version <player>`, `sversion` | Both report the version of `tt.dll` on a machine. There is no `tt.dll`: this is the engine. Nothing to report. |
| `serial <player>` | Prints the serial hash a 4.8.4 client sent during the TT handshake. That handshake was part of the DLL-injection architecture directive 0.5 declines, and no client sends one. |
| `ssurl <url>`, `sshot <player>` | Remote screenshots: the server names a URL and a client uploads a picture of its screen to it. Porting this means building screen capture and an HTTP uploader on the client and deciding what a player is told before their screen leaves their machine. It is a policy question as much as an engineering one and is not being answered by a silent port. Recorded here rather than dropped. |
| `mapch <player> <map>` | Asks one client whether it has a named map, and needs a client-to-server reply channel that does not exist yet. Belongs with the map transition and download work, not with the console. |
| `tag <player> <tag>` | Sets a custom name tag on a player and replicates it in `cPlayer`'s rare state. `cPlayer` has no such field here; adding one is a network contract change that wants to be made once, alongside whatever else the player object gains. |
| `view <w3d> <anim>` | Opens 4.8.4's model viewer dialog. That dialog is not in this tree, and it is an asset-inspection tool rather than engine behavior — `leveledit` is where a model gets looked at. |
| `log <0\|1>`, `logp` | Turn the client chat log on and off. 4.8.4 stored the setting in the registry under Westwood's key. This tree does not read that key and has no client chat-log setting to toggle; `ConsoleBox` logs to disk unconditionally. Wants a real settings entry first. |

None of these are `MISSING` in the parity sense: each has a written reason, which is
what roadmap Section 11's acceptance asks for.
