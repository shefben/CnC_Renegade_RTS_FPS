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

## 4. What remains of Phase 4

The stock half is done. The donor half is not, and is blocked on work that
belongs to earlier phases.

**Done.** The registry itself; the provenance and alias fields; the OBJECT
library; generate-time and startup duplicate detection; `Create_Script` and
script destruction through the registry; save/load compatibility;
`add_dependencies(combat scripts)` removed; `ScriptManager::Init` no longer
loads a DLL; the editor on the same catalog; the `ScriptCommands` table deleted
with no facade left behind.

**Outstanding.**

- **The 13 replacements** (matrix §3.1) — script names the donor defines that
  the stock catalog also defines. Each needs the donor implementation merged
  into the canonical script and the stock one deleted, then registered as
  `SCRIPT_SOURCE_STOCK_MERGED`. `agtfix.cpp` and the `M00_*` cluster in
  `jfwws.cpp` are behaviour corrections, so the donor side wins.

  **1 of 13 merged.** `M00_Advanced_Guard_Tower` (`agtfix.cpp`): the four gun
  mounts are corners of the tower, so their offsets belong to the building's
  own frame; stock added them unrotated, which only produced the right result
  for a tower at facing zero. The facing has to come off the MCT — the
  controller is a `BuildingGameObj`, which derives from `DamageableGameObj`,
  not `PhysicalGameObj`, so it has no transform and `Get_Facing` returns zero.
  That path also carries the donor's height correction. `Killed` told the guns
  the tower was dead and left them standing; they are destroyed now.

  Not taken from that one: the optional `MissileDef`/`GunDef` weapon override
  needs `Grant_Weapon`, and the gun target test excludes harvesters via
  `Is_Harvester`. `Is_Harvester` has portable source; `Grant_Weapon` does not.
  See below.

- **The 2142 donor-only scripts** (matrix §3.2) — these compile against the
  donor's own engine SDK (`scripts/engine_*.h`).

  **Correction.** This document previously said that SDK "binds to the closed
  binary through `REF_DEF2`/`REF_DECL2`" and so had to wait on Phase 2 and
  Phase 3 entirely. That overstates it. The SDK declares 690 functions across
  20 headers, and the split is lopsided:

  | | Count | Where |
  | --- | --- | --- |
  | Declared with portable C++ source in `engine_*.cpp` | **406** | 18 of the 20 headers |
  | `SCRIPTS_API extern` — a pointer resolved into the closed binary, no source | **284** | `engine_tt.h` alone |

  Every other header — `engine_obj.h`, `engine_game.h`, `engine_weap.h`,
  `engine_player.h`, `engine_dmg.h` and the rest — has **zero** extern
  bindings. Their implementations are ordinary code: `Get_Object_Type` is four
  lines around `DamageableGameObj::Get_Player_Type`, `Is_Harvester` compares
  against each base's harvester vehicle. The implemented units carry only 22
  `REF_DEF`/`REF_DECL` data bindings between them.

  So the blocker is narrower than recorded: **`engine_tt.h`'s 284 externs**,
  not the SDK. The 406 with source can be ported natively without waiting on
  anything, and doing so is what unblocks the bulk of the 2142.

  The registry is ready for them: provenance `SCRIPT_SOURCE_TT`, and the
  checker already recognises the registration form they use.

Registry size when both land: 1640 + 13 + 2142, no duplicate names.
