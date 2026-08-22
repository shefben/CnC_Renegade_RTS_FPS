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

One thing did not land. `Test_Cinematic`'s fourth new command, `Show_Message`,
writes a coloured line into every player's chat, and a script cannot say that:
the message classes live in `Code/Commando`, and `Code/Scripts` does not link
it -- the editor builds the same sources as `scriptse` without Commando at all.
It is the same seam that blocked the powerup-grant sound in Phase 3. See 4.3.

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
whether the canonical engine can already answer it.
`tools/tt484/apigap_tsv.py` writes the same result per name to
`docs/tt484/TTScriptApiGap.tsv`.

| | Names | Calls |
| --- | --- | --- |
| **Already answerable** -- `Commands->X` where `ScriptEngine::X` exists | 154 | **6740** |
| Free SDK functions with portable source in `engine_*.cpp` | 129 | 1241 |
| Free SDK functions needing engine work | 35 | 188 |
| Blocked on per-client delivery (4.3) | 22 | 270 |
| N/A -- plugin hooks (directive 0.5) | 22 | 26 |
| N/A -- `REF_DECL` data binding | 1 | 5 |

**All 148 `Commands->` methods these files call already exist in
`ScriptEngine`.** That is the headline: the 6599 `Commands->X(...)` call sites
are a mechanical rename, not a port. This is a large correction to what this
document previously recorded -- it said the SDK's binding to the closed binary
made the bulk of the work wait on Phases 2 and 3. It does not. What is left is
1241 calls to functions whose source is right there in `engine_*.cpp`, and 188
calls to 35 functions that need writing.

The plugin-hook family (`AddChatHook`, `AddPlayerJoinHook`, `AddThinkHook` and
the rest, 22 names, 26 calls) is declined under directive 0.5. Natively the
same notifications come off the event bus; see `NativeEventDispatch.md`.

### 4.3 The one real blocker: a script cannot address one client

Twenty-two names and 270 calls -- `Create_2D_Sound_Team`, `Send_Message_Player`,
`Create_2D_WAV_Sound_Player`, `Set_HUD_Help_Text_Player`,
`Set_Screen_Fade_Color_Player`, `Force_Camera_Look_Player` and the rest -- all
want the same thing: say something to one player, or to one team, rather than
to everyone. `Grant_Refill` and `Test_Cinematic`'s `Show_Message` want it too,
and so did the powerup-grant sound in Phase 3. 4.8.4 does it by writing a line
onto its scripts text channel, which directive 0.5 declines.

Natively it means a network event or a purchase-side call, and both live in
`Code/Commando`: `cScTextObj`, `VendorClass::Grant_Supplies`. `Code/Combat`
does not reference `Code/Commando`, and `Code/Scripts` does not link it -- the
editor builds the same sources as `scriptse` without Commando at all -- so
neither the engine's script interface nor a script can reach them.

**The tree already has the shape of the answer.** `GameEventBus`
(`Code/Combat/gameeventbus.h`) is declared in Combat and its listeners are
registered by Commando; that is exactly the direction needed, and
`GameEventBus::Raise_Refill` already crosses it, only the other way round --
Commando raises, listeners observe. What is missing is a channel Combat raises
and Commando *acts on*: `ScriptEngine::Send_Message_Player(...)` raises,
a Commando-side listener installed at startup builds the `cScTextObj`. The
editor registers no listener, so the call is a no-op there, which is the right
answer for a tool with no clients.

That makes this a Phase 4 item and not a Phase 5 one. It is the first thing to
build, because it unblocks about a sixth of the donor library and closes the
Phase 3 powerup sound at the same time.

### 4.4 Registry size

1639 built-in scripts today, no duplicate names -- one fewer than the 1640 this
document used to quote, because the checker was counting the registration
macros themselves as a script called `x`; it skips macro bodies now. Adding the
861 donor-only in-scope scripts takes it to 2500. The 1259 out-of-scope
registrations are not counted and not ported; if a mod pack is ever wanted it
re-enters through the same registry with provenance `SCRIPT_SOURCE_TT`,
needing no change here.
