# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P08 -- one scope left, and a check only a person can run

`PERMANENT`, `GAME_MODE` and `WORLD` all have callers now (see
`docs/zerohour/AssetResidency.md`, "What is left"). What remains of P08, still
absorbing the backlog's acceptance line *Repeated map/world load/unload does not
invalidate retained assets or leak unbounded resources*: `SECTOR` has no consumer
until something streams part of a level, which belongs to the terrain work, and
materials and generated world buffers have record kinds and no registration site for
the same reason. No prototype is claimed permanently, deliberately, until a profile
says which ones are shared.

The acceptance condition is proved as arithmetic by `asset_residency` -- sixteen
cycles leak nothing, and a texture-only permanent scope retains a record while naming
nothing -- but not against a real map. That check cannot be automated here: the client
takes no map argument, so reaching a second level means driving the menus. Next exact
action is a manual one for the user: run
`renegade --gamedir "C:\Westwood\Renegade_full"`, start a level, change to another,
and read the `WWDEBUG` `Log_Report` line, confirming the permanent texture count is
unchanged across the change and the live prototype count drops back.

---

## P09 -- the layer exists, the pipelines do not

`ShaderManagerClass` is in place and checked (see `docs/zerohour/ShaderManager.md`),
with stock W3D content registered as a program rather than bypassing the layer. What
is left of Section 15 is the pipeline list itself: terrain, terrain detail, roads,
bridges, water, foliage, projected shadows, particles, tracers and beams, status
markers, ghost building tint, debug overlays. They are enumerated and unregistered
because most of them have nothing to draw yet -- the first five wait on the terrain
framework, foliage and ghost tint and status markers on the Commander work -- and each
registers itself when its system lands.

Two things are true structurally but unverified at runtime: that existing Renegade
materials render unchanged through the layer, and the acceptance line *new donor
systems share one state/shader management layer*, which needs a second system to share
it. Next exact action: register `MATERIAL_PROGRAM_DEBUG_OVERLAY` against the existing
debug drawing in `Code/ww3d2/ww3d.cpp` (`Render_Debug_Resources`), which is the one
listed pipeline whose consumer already exists, giving the layer a second program and
the first evidence that handover works against a real device.

---

## Carried forward out of P05, P06 and P07

P05, P06 and P07 are all closed (see `completed_features.md`, and
`docs/tt484/TTPhase5Audit.md`, `docs/tt484/TTPhase6Gate.md` and the four
`docs/zerohour/` documents). Five things they named for later work must not be
lost, each already written down where the work will happen: `ssurl`/`sshot`,
the remote-screenshot pair, which is a policy question about a player's screen
leaving their machine before it is an engineering one; `mapch`, which needs a
client-to-server reply channel and belongs with the map transition and download
work; `tag`, a custom player name tag whose field wants adding to `cPlayer`
once; the **client/server map transition** manual step, which stays manual
until there is a harness that can run two processes against real game data; and
`NavalFactoryGameObj`, which remains reassigned to Zero Hour Feature 7.

One latent fault is recorded rather than fixed: `cGameData`'s constructor ends
with `TRANSLATE(IDS_SERVER_SAVELOAD_CUSTOM_DEFAULT)`, so constructing one before
`Init` has loaded `STRINGS.TDB` asserts in `translatedb.h:259`. The engine's own
order never does that; it only bites something built outside startup order.

Three capabilities in the port matrix carry an `EAValidation` note, meaning EA's
archival tree must actually be consulted before they are implemented: ZH-04 if
the community's `Cast_Ray` fixes conflict, ZH-18 before adopting the
`Drawable`/`Locomotor` merge (`4990ca933`), and ZH-24 when a pathfinder change
alters retail behaviour. EA's repository is not cloned here; only its head SHA
is recorded.

---

P01 through P07 are complete (see `completed_features.md`). The backlog is in
`unstarted_features.md`.

---

## Working notes that cost a session each

Assembling a generated `.cpp` from parts: write the parts with the Write tool
and concatenate them with Python opened `newline=''`. `cat a b > c` in this
shell silently drops blank lines and any line carrying an inline
`/*comment*/`, which produced a file that looked like a compiler bug. Bash
heredocs are worse: a `<<'PY'` heredoc carrying a Python script silently
mangled a TSV, deleting fifty-one lines it never touched. Put any non-trivial
script in a file with the Write tool and run the file. A heredoc is fine for a
short `str.replace` patch, but check it actually replaced -- one printed `ok`
having matched nothing -- and never put `\n` inside a heredoc'd Python string
meant to produce a C escape: it produces a real newline and the compiler
reports a newline in a constant. Indentation is the other heredoc trap: this
tree indents with tabs, a patch anchor typed with spaces never matches, and the
fastest recovery is to locate the line by `index` and splice rather than to
keep re-typing the anchor.

`re.compile` returns the *same object* for two identical pattern strings, so
never decide what a match means by comparing pattern identity. That cost a
debugging round in `script_catalog_manifest.py`.

`git log` in this shell truncates to 50 commits when its output is piped unless
`-n` is given explicitly. `git rev-list --count HEAD` disagreeing with
`git log | wc -l` is that, not a shallow clone. The donor has 1712 commits.

`ls` here prints a size column, so `ls dir | grep -E '\.cpp$'` matches nothing.
Pipe through `sed 's/ .*//'` first.

`grep -r` over the repository root now walks the `GeneralsGameCode/` submodule
and times out. Give it a path under `Code/`, or use the Grep tool, which is
indexed.

Two builds cannot run at once: the second dies with
`LINK : fatal error LNK1104: cannot open file 'Debug\renegade.ilk'`, which
looks like a corrupt object file and is not one. Let a backgrounded build
finish before starting another. `b.bat` builds one target, so `renegade` and
`renegadeserver` are two separate invocations, and a ctest entry that runs the
one you did not rebuild fails against a stale binary.

`docs/tt484/TTScriptApiGap.tsv` is generated by `tools/tt484/apigap_tsv.py`,
which writes the file itself -- do not redirect its stdout over it. Anything
resolved by a seam, by a rename, or by a decision not to add a command must be
written into `TTScriptApiRenames.tsv`, or the next regeneration reverts it.
`Code/Scripts/tests/ScriptCatalogManifest.h` is likewise generated, by
`tools/tt484/script_catalog_manifest.py`; regenerate it whenever a script is
added or removed or the catalog tests fail on a count. The four
`docs/zerohour/*.tsv` files are generated by the five tools in
`tools/zerohour/`; regenerate them if the donor submodule pin moves.
`docs/RejectedItems.md` is generated by `tools/rejected_items.py` -- re-run it
after any pass that declines something, and put an in-code rejection into that
tool's `PHASE_DECISIONS`, not into the document.

`b.bat` must be invoked through the PowerShell tool
(`cmd /c "...\b.bat renegade"`); through Bash the argument never reaches the
batch file, and the PowerShell tool's working directory persists between calls,
so `cd` back to the repository root before invoking it. The `vswhere.exe`
warning it prints first is harmless. Reconfiguring must be
`cmake -S . -B build/win32` with no `-A Win32`, which the existing cache
rejects.

`VectorClass` compares its elements, so any struct stored in a
`DynamicVectorClass` needs `operator ==` and `operator !=` or the error arrives
from inside `vector.h` naming `const T`. `StringClass` has no `str()` --
`Peek_Buffer()` is the accessor -- and `HashTemplateClass::Get(key, value&)` is
the lookup that reports whether it found anything.

A self check that runs before `Init` must not touch anything the engine loads
during `Init` -- the string table above all. A modal `WWDebug_Assert_Fail` box
in a ctest run hangs the suite until somebody clicks it.

---

## Reference: pinned baselines (not a lifecycle entry)

OpenW3D `b970f592282f89c7a4a2f86d54c42c1dafe7c388` on `main` (user-authorized
deviation from directive 0.2). TT donor `tt_4.8.4/`. Zero Hour donor submodule
`9fee97a1792efc50edfe27300581970b4c022951`, `Core` + `GeneralsMD` only, frozen
in `docs/zerohour/ZeroHourDonorBaseline.md`; EA's archival reference is
`0a05454d8574207440a5fb15241b98ad0b435590`, recorded but not cloned. Canonical
build `build/win32` (x86); `build/win` (x64) secondary. Game data at
`C:\Westwood\Renegade_full` -- assets only, its binaries are out of scope. Run
a built binary by hand with `--gamedir "C:\Westwood\Renegade_full"`, or the
working directory becomes the build tree, which has no `Data`, and the first
string lookup asserts. Builds must run through `vcvarsall.bat x86`; this
machine's system-wide `INCLUDE` points at Visual Studio 6 and a plain shell
compiles against the VC98 CRT, whose errors look like broken source rather than
a bad environment.

Renegade models, maps and textures must keep loading unmodified. Engine work may
augment, fix and extend them; it may never require content to be re-authored or
re-exported.

The tag `tt-native-complete` marks the commit where TT 4.8.4 is natively
implemented and the hard gate passes, before any Zero Hour work.
