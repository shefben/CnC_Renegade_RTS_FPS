# WIP Features

Two or three sentences per in-flight item, ending with the next exact action.
Detail lives in `docs/`.

---

## P04: the 4.8.4 script library, natively

The stock half, the registry and all thirteen replacements are done (P04-A,
P04-B, P04-C, P04-C1 in `completed_features.md`). What is left is the in-scope
donor-only library: 861 scripts across 25 files, absorbing the backlog lines
"Compile unchanged stock scripts only when TT does not supersede them" and
"Compile TT-only new scripts as additional canonical scripts". Their cost is
now measured rather than guessed (P04-D): the engine can already answer 6740 of
their calls, 1241 more need SDK functions whose source exists, and 270 are
blocked on one thing -- a script cannot address a single client or team.

Next exact action: build that one thing first. Add a `GameEventBus` channel
Commando *acts on* rather than observes -- `ScriptEngine::Send_Message_Player`
and its siblings raise it, a listener registered by Commando at startup builds
the `cScTextObj` or calls `VendorClass::Grant_Supplies`, and the editor
registers nothing so the call is a no-op there. See
`docs/tt484/NativeScriptRegistry.md` 4.3. It closes `Show_Message`,
`Grant_Refill`, the 270 per-client calls and the Phase 3 powerup-grant sound
together. Then port the 129 portable SDK functions into `ScriptEngine` and
convert the 25 files, smallest first (`jfwpow.cpp` is 163 lines and 13
scripts), registering each as `SCRIPT_SOURCE_TT`.

---

P01 and P02 are complete (see `completed_features.md`). The backlog is in
`unstarted_features.md`.

---

## Reference: pinned baselines (not a lifecycle entry)

OpenW3D `b970f592282f89c7a4a2f86d54c42c1dafe7c388` on `main` (user-authorized
deviation from directive 0.2). TT donor `tt_4.8.4/`. Zero Hour donor submodule
`9fee97a1792efc50edfe27300581970b4c022951`, `Core` + `GeneralsMD` only. Canonical
build `build/win32` (x86); `build/win` (x64) secondary. Game data at
`C:\Westwood\Renegade_full` — assets only, its binaries are out of scope.
Builds must run through `vcvarsall.bat x86`; this machine's system-wide `INCLUDE`
points at Visual Studio 6 and a plain shell compiles against the VC98 CRT, whose
errors look like broken source rather than a bad environment.
