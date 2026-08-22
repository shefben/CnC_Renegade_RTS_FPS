# Phase 4 — Native script registry

Roadmap Section 10. What replaced the `SCRIPTS.DLL` bootstrap, and what is left.

---

## 1. The seam that was removed

Before: `ScriptManager::Init` called `Load_Scripts("SCRIPTS.DLL")`, which
`LoadLibrary`'d a DLL, resolved `Create_Script`, `Destroy_Script`,
`Set_Request_Destroy_Func` and `Set_Script_Commands` by name, and handed the DLL
a `ScriptCommands` struct of 202 function pointers. Every built-in script
reached the engine through that struct: `Commands->Create_Object(...)`,
25,830 call sites.

The struct carried `Size` and `Version` fields and the DLL refused to load if
they disagreed — an ABI check between two halves of one program.

All of it is gone.

---

## 2. What replaced it

### 2.1 `ScriptEngine` — the script-facing engine interface

`Code/Combat/scriptcommands.h`.

The 202 functions the table pointed at were always ordinary engine functions
with external linkage; only the indirection was needed. They are now declared
directly, and `scriptcommands.cpp` defines them, in `namespace ScriptEngine`.

The namespace is not decoration. `Get_Position`, `Find_Object`, `Get_ID`,
`Set_Facing` are too generic to sit at global scope in translation units that
also define hundreds of script classes. Call sites read
`ScriptEngine::Create_Object(...)`.

Two things fell out of the conversion:

- The table could not carry default arguments, so it recorded them in comments
  (`bool primary/* = true */`). Those are real defaults again.
- `ScriptTrace`, the flag behind the `scripttrace` console command, stayed
  outside the namespace. It is engine state that a console function toggles, not
  part of the script interface.

There is no dual path: no function table, no runtime switch, no facade.

### 2.2 `NativeScriptRegistry` — the catalog

`Code/Combat/nativescriptregistry.h` / `.cpp`.

`ScriptFactoryClass` is the registration record: name, parameter schema,
provenance (`ScriptSourceEnum`), and an optional alias for a script that was
renamed and must still load from levels saved under the old name.
`ScriptRegistrant<T>` on the script side is the only thing that knows the script
class.

Registration happens during static initialization, so the head of the list is a
raw pointer and the list is intrusive — a container with its own constructor
could not be relied on to exist yet. The sorted index is built later, from
`ScriptManager::Init`, once every translation unit has registered. Lookup is a
binary search over the sorted names, case-insensitive, because level files were
always written with whatever case the designer typed. Aliases are swept
linearly, which is the right cost for how rare they are.

`ScriptRegistrar` and `ScriptFactory` in `Code/Scripts` were deleted rather than
kept alongside it.

### 2.3 An OBJECT library, not a static one

`Code/Scripts` builds as `add_library(scripts OBJECT)`, linked into `renegade`
and `renegadeserver` through `$<TARGET_OBJECTS:scripts>`.

This is not a preference. Every script registers itself from a file-scope
`ScriptRegistrant` that nothing references; from a static library the linker
would pull in only the object files something else needs and silently drop whole
scripts. Object libraries are linked whole.

The editor links the same sources as `scriptse`, an OBJECT library built with
`PARAM_EDITING_ON` to match `combate`. `ScriptMgrClass::Initialize` in LevelEdit
now enumerates `NativeScriptRegistry` instead of `LoadLibrary`-ing the DLL, so
the editor and the game show the same catalog by construction rather than by
both happening to load the same file.

`add_dependencies(combat scripts)` is gone; the dependency now runs the other
way, which is what it always meant.

### 2.4 Failing on a duplicate name, at generate time

`tools/check_script_catalog.py`, run from `Code/Scripts/CMakeLists.txt` via
`execute_process`. A duplicate name is a `FATAL_ERROR` during configure.

The checker understands all three registration forms — `DECLARE_SCRIPT`,
`REGISTER_SCRIPT` and the raw `ScriptRegistrant<T> x("Name", ...)` template
instantiation the donor tree favours, where the string literal is the registered
name and need not equal the class name. It blanks comments and `#if 0` blocks
first, preserving line numbers, because this catalog is full of parked scripts
that still contain their `DECLARE_SCRIPT`; without that, four of its five
initial findings were false.

Current result: **1640 built-in scripts, no duplicate names.**

`NativeScriptRegistry::Build_Index` checks again at startup. That second check
is not redundant: it catches a name produced at registration rather than written
in the source, which the static scan cannot see.

### 2.5 Save/load

Unchanged and deliberately so. `ScriptManager::Save`/`Load` still write and read
the script name and its parameter string, and loading still goes through
`Create_Script`, which is now a registry lookup instead of a DLL call. A save
file written before this change loads after it.

---

## 3. Two collisions the change exposed

Linking the scripts into the program surfaced two things the DLL boundary had
been hiding:

- `Code/Scripts/strtrim.cpp` defined `strtrim`, which `Code/wwlib/trim.cpp`
  already defines. Two copies in one program is a duplicate symbol; the wwlib
  one is the canonical owner and the `Code/Scripts` copy was deleted.
- `ScriptTrace` was referenced by `Code/Commando/consolefunction.cpp` and would
  have disappeared into the `ScriptEngine` namespace. See §2.1.

---

## 4. Where Phase 4 stands

**The stock half is done. The thirteen replacements are done. The in-scope
donor-only library is not.**

### 4.1 The thirteen replacements — closed

Every script name that both catalogs define now has exactly one implementation,
the merged one, registered through `DECLARE_SCRIPT_MERGED` so the catalog can
say so. The list and what each donor side corrected is in
`TTParityMatrix.md` 3.1, which also records why the membership of that list
changed: the extractor that first produced it did not blank comments, so it
counted a parked registrant and missed `M00_Nod_Obelisk_CnC`.

Six engine capabilities landed behind them, none of them TT-specific:
`Stop_Timer`, `Has_Timer`, `Get_Vehicle`, `Is_Harvester`,
`Find_Nearest_Preset`, and `Get_Player_Type`/`Set_Player_Type` widened from
`PhysicalGameObj` to `DamageableGameObj` -- asking a building which team owned
it used to answer Nod and log a complaint, which is why anything driven by a
building had its team written into the script.

All four of 4.8.4's additions to `Test_Cinematic` are in, `Show_Message`
included -- it was blocked when 4.1 was first written and is not any more; see
4.3.

### 4.2 The in-scope donor library -- 861 scripts, 25 files

Scope, set by the user: only the original TT script library is ported, not the
community gameplay-mode packs. That is `jfw*.cpp` (17), `gm*.cpp` (4),
`agtfix.cpp`, `obelfix.cpp` and `dan.cpp` -- 25 files, 873 registrations, of
which the 13 above are already absorbed. The 44 remaining files and 1259
registrations (`jmg*`, `renalert*`, `ra2`, `reborn`, `scud`, `survival`,
`ms*`, `dp88_*`, `DB*`, and the rest) are mod and alternate-game-mode content
and are out of scope; none of it corrects stock behaviour, so directive 0.4
does not reach it.

**What porting them actually costs, measured rather than estimated.**
`tools/tt484/apigap.py` counts every engine call the 25 files make and asks
whether the canonical engine can already answer it. `tools/tt484/apigap_tsv.py`
writes the same result per name to `docs/tt484/TTScriptApiGap.tsv`, and
`tools/tt484/readiness.py` turns it round to say which files are ready to
convert today.

| | Names | Calls |
| --- | --- | --- |
| **Answered** -- `ScriptEngine` has it, or `TTScriptApiRenames.tsv` says what it is instead | 271 | **8007** |
| Free SDK functions with portable source still to port | 34 | 277 |
| Free SDK functions needing engine work | 34 | 154 |
| Blocked -- the stealth gap list, which is a feature and not an API | 1 | 1 |
| N/A -- plugin hooks (directive 0.5) | 22 | 26 |
| N/A -- `REF_DECL` data binding | 1 | 5 |

Two corrections to what this section said before. First, all 148 `Commands->`
methods these files call already existed in `ScriptEngine`, so those 6599 call
sites were never a port -- they are a rename. Second, the 22 names and 270
calls this document called the one real blocker are not blocked any more: see
4.3. What actually remains is small and ordinary.

Fourteen SDK names are a second spelling of something the engine already does
(`Get_Object_Type` is `Get_Player_Type`). Directive 0.4 forbids keeping both,
so they are not ported; `TTScriptApiRenames.tsv` records the mapping and the
survey tools read it.

The plugin-hook family (`AddChatHook`, `AddPlayerJoinHook`, `AddThinkHook` and
the rest, 22 names, 26 calls) is declined under directive 0.5. Natively the
same notifications come off the event bus; see `NativeEventDispatch.md`.

**Conversion order.** `readiness.py` ranks the files by how many calls the
engine still cannot answer. Done so far: `jfwpow.cpp` (`TT_Powerup.cpp`, 13
scripts), `jfwws.cpp` (`TT_World.cpp`, 29 scripts and 7 aliases) and the
`gm*.cpp` SSGM scripts (`TT_SSGM.cpp`, 32 registrations). Next at zero
blockers is `jfwgun.cpp` (5015 lines, 60 registrations); everything else is
within a handful of names of ready.

Four of `gmsoldier.cpp`'s registrations are not in `TT_SSGM.cpp`:
`SSGM_Log_Key`, `SSGM_C4_Key`, `SSGM_Bind_Key` and `SSGM_BL_Key` all derive
from `JFW_Key_Hook_Base`, so they arrive with `jfwkey.cpp` rather than here.

**The aliases.** Six of `jfwws.cpp`'s registrations register a `JFW_*` class
under a second, stock name -- `M00_PCT_Pokable_DAK`, `M00_Disable_Transition`,
`M00_GrantPowerup_Created`, `M00_Play_Sound`, `Dr_Mobius_Script`,
`M00_BuildingStateSoundController`. Those stock names are already merged (4.1),
so the `JFW_*` name must become an alias registration of the merged class and
not a second copy of it. That is what 4.8.4 itself does, and it is the only
reading directive 0.4 allows.

**SSGM.** The five `gm*.cpp` files are two things at once: the scripts, and
the server-management layer around them. The user asked for both. The layer is
`SSGMSettingsClass` (`Code/Combat`), `SSGMGameLog` (`Code/Combat`, because the
scripts that write to it are compiled into the editor too) and
`SSGMManagerClass` (`Code/Commando`), which subscribes to the event bus rather
than installing hooks. Its plugin loader is declined under directive 0.5. See
`SSGM.md`.

### 4.3 A script can address one client — answered

Twenty-two names and 270 calls -- `Create_2D_Sound_Team`, `Send_Message_Player`,
`Set_HUD_Help_Text_Player`, `Force_Camera_Look_Player` and the rest -- all
wanted the same thing: say something to one player, or to one team, rather than
to everyone. `Grant_Refill` and `Test_Cinematic`'s `Show_Message` wanted it too.
4.8.4 does it by writing a line onto its scripts text channel, which directive
0.5 declines.

`cScScriptCommandEvent` (`Code/Combat/scscriptcommandevent.h`) is the native
answer: an ordinary S->C network event carrying one command and its parameters,
addressed with the per-client dirty bit that already exists for exactly this.
Its client half calls the same local `ScriptEngine` function the world-wide
version calls, so there is still one implementation of each effect. A listen
server that is itself the addressee runs the command directly rather than
sending it to nobody.

Combat can address a client by id but has no roster, and the roster lives in
Commando. `GameEventBus` gains one channel a listener *answers* rather than
observes -- who is connected, and on which team -- registered by
`GameEventListeners::Register` immediately after `CombatManager::Init` resets
the bus. The editor registers nothing, so a team-addressed command there does
nothing, which is the right answer for a tool with no clients. A false return
means "nobody can tell you", which callers must not confuse with an empty team.

`Grant_Refill` turned out not to need any of this: its whole body was Combat
classes, so it is a `ScriptEngine` command now and `VendorClass::Grant_Supplies`
calls it rather than owning it. `Kill_All_Buildings_By_Team` was misfiled by the
survey's `_Team` suffix heuristic and is plain server-side work.

### 4.4 Registry size

1720 built-in scripts today, no duplicate names: 1639 canonical -- one fewer
than the 1640 this document used to quote, because the checker was counting the
registration macros themselves as a script called `x` -- plus the first thirteen
from the 4.8.4 library. The remaining 848 in-scope
scripts take it to 2500. The 1259 out-of-scope
registrations are not counted and not ported; if a mod pack is ever wanted it
re-enters through the same registry with provenance `SCRIPT_SOURCE_TT`,
needing no change here.
