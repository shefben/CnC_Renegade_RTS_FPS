# Phase 5 audit — TT 4.8.4 engine-facing features, category by category

Roadmap Section 11. Acceptance: *"The TT parity matrix has no `UNREVIEWED` or
unexplained `MISSING` entries."*

This file is where that acceptance is discharged. Each of the seven categories
Section 11 names is answered here with what was found and where it went. Nothing
is left as "not looked at": where a thing is not in the tree, the reason is written
down and the row it belongs to says so too.

## Standing evidence

| Table | State at the close of P05 |
| --- | --- |
| `TTHookSites.tsv` | 760 rows. 573 merged, 111 out of scope, 76 n/a. **No open row.** |
| `TTScriptApiGap.tsv` | 363 rows. 338 done, 21 n/a-plugin-hook, 3 n/a-sdk-helper, 1 n/a-data-binding. **No port-engine-work row.** |
| `tools/tt484/readiness.py` | Every in-scope donor file at `all`, no blocked call anywhere. |
| `tools/check_script_catalog.py` | 2498 built-in scripts, no duplicate names. |

---

## 11.1 Game-object/building extensions

Section 11.1 names six types. Their disposition was settled in P02 and is recorded
in full in `TTParityMatrix.md` §5.9; repeated here so this file stands alone.

| Type | Where it is |
| --- | --- |
| `SoldierFactoryGameObj` | Already stock — `Code/Combat/soldierfactorygameobj.*`. TT added nothing to it that was not the shared factory behavior now on `VehicleFactoryGameObj`. |
| `SamSiteGameObj` | Already stock — `Code/Combat/samsite.*`. |
| `ConstructionYardGameObj` | **New class** (P02). No counterpart existed; arrives under `BuildingGameObj` with its own chunk IDs, definition factory and `As_*` cast. |
| `SuperweaponGameObj` | **New class** (P02), same treatment. |
| `AirFactoryGameObj` | **N/A — superseded by `AirStripGameObj`.** TT re-implemented, member for member, machinery this tree already owns. What was genuinely new moved across: `Is_Disabled`/`Set_Disabled`/`Set_Busy` to `VehicleFactoryGameObj`, and landing positions, `DefaultEngineEnable` and the delivered-vehicle collision group to `AirStripGameObj`. |
| `NavalFactoryGameObj` | **Reassigned to Zero Hour Feature 7** (roadmap Section 22). It needs a water surface, naval unit physics and a berth system, none of which exist yet, and Renegade ships no naval content. Its 27 rows and `As_NavalFactoryGameObj` carry forward. |

Integration named by 11.1 — `DefinitionMgrClass`, creation, save/load, networking,
building lists, damage, radar/HUD, editor definitions — was done for the two new
types as part of adding them; `TTParityMatrix.md` §5.10 records the three findings
that came out of wiring them through `wwsaveload` (chunk IDs append, force-linking
is mandatory, TT's own numeric IDs are unrecoverable and not a target).

## 11.2 Network extensions

| 11.2 item | Where it is |
| --- | --- |
| Per-client dirty-state behavior | Merged with the hook sites that carried it. P05 added the one piece stock had only in one direction: `cNetwork::Tell_Server_About_Delete_Notifications`, called at the top of `Client_Update_Dynamic_Objects`, so an object a client has finished with reaches the server at once instead of waiting for the throttled dynamic update it may no longer be part of. |
| Object visibility controls | Merged. The per-client dirty-bit path is the mechanism, and the script-facing half is `done` throughout `TTScriptApiGap.tsv`. |
| TT network event extensions | The S→C script-command channel, `cScScriptCommandEvent`, is the native replacement for 4.8.4's habit of smuggling an opcode down the private-message channel. P05 added three commands to it: the objectives dialog, a WAV file at an object's bone, and an emoticon over a soldier's head. |
| Connection acceptance features | `cNetwork::Is_Player_Name_Allowed` / `Is_Player_Name_Valid` and the refusal in `Application_Acceptance_Handler`. Stock caught only the empty name and reported it as a version mismatch. |
| Server/client scriptable dialog behavior | `PlayerTerminalClass::Display_Objectives_Text` and `IDD_OBJECTIVES_TEXT` (P05), alongside the GDI/Nod terminals that were already there. |
| Reload / force-reload behavior | The building-JSON live reload and `SSGMSettingsClass::File_Has_Changed` are the native form; see `docs/tt484/SSGM.md`. |
| GameSpy / broadcaster changes | Merged where relevant; the rest are marked out-of-scope in `TTHookSites.tsv` as deployment code for a closed binary. |

**Not ported, with reason.** `cNetwork::Hook_Compute_Exe_Key`, `computeDataFilesCrc`
and the serial hash a 4.8.4 client sent are the client-attestation handshake of the
DLL-injection architecture directive 0.5 declines: there is no `tt.dll` whose bytes
to key, and no client sends one. `Broadcast_Object_Update` is a two-line helper with
no caller anywhere in 4.8.4. `Test_For_Team_Defaulting` is `TT_UNREACHABLE` in the
donor — a declaration with no body.

## 11.3 HUD / radar / dialog APIs

Every HUD, radar and dialog entry point 4.8.4 exposed to scripts is `done` in
`TTScriptApiGap.tsv`, acting on this tree's own `HUDClass`, radar and dialog stack.
No second UI stack was created.

`Do_Objectives_Dlg` was the last one outstanding and the last blocked call in the
whole in-scope set. It is now `ScriptEngine::Display_Objectives_Text_Player`, and
`JFW_Custom_Objectives_Dlg` is a registered script rather than a comment explaining
its absence.

## 11.4 Input / controller additions

**Nothing to port.** `tt_4.8.4/tt/Input.h` declares no member that
`Code/Combat/input.h` does not already have; its only TT-only symbols are the
`REF_DECL1` / `REF_ARR_DECL1` address-binding macros and a `__declspec`. The 1191
lines of `tt/input.cpp` are re-implementations of stock methods for the closed
binary, not new behavior — exactly the "address soup" `TTParityMatrix.md` §7
describes.

`tt/directinput.cpp` is declined on the roadmap's own instruction: 11.4 says not to
reintroduce obsolete DirectInput assumptions where OpenW3D has already abstracted
them, and it has.

## 11.5 Collision and physics-facing APIs

The two hook sites that were still open at the end of P03 are the whole of this
category, and both are now merged:

- **C4 sticking to the repair bay arc.** A charge stuck to level geometry has no
  object to name and sent ID 0, which is a real network ID on a client and belongs
  to the welding arc effects, so the charge attached itself to an arc and followed
  it about. `c4.cpp` sends `NO_STUCK_OBJECT` and only looks the ID up when it is
  not that.
- **The ladder fix.** `Disable_Collision_Detection(SOLDIER_GHOST, DEFAULT)` in
  `CombatManager::Scene_Init`. The row was held open for in-game evidence that
  dropping the pair does not break doors, elevators and crates; 4.8.4 shipped it
  globally and every TT server has run it since, which is that evidence. What it
  costs is that a ghosted soldier — one inside a ladder or elevator coordination
  zone, or squeezing past a team mate — also passes through vehicles and placed
  dynamic objects. That is the fix, not a side effect: ghosting means "do not stop
  this soldier", and a vehicle parked at the foot of a ladder was stopping him.

## 11.6 Script library

Bounded by the user's explicit scope: the original TT scripts only, not the other
gameplay modes 4.8.4 ships. That is the 25-file in-scope set — `jfw*.cpp`,
`gm*.cpp`, `agtfix.cpp`, `obelfix.cpp`, `dan.cpp` — and every one of them is ported
(P04). `gmlog.cpp` registers nothing; `gmsoldier.cpp` is 2 of 5 with the other three
recorded N/A in P04-AE; every other file carries every script it registered.

## 11.7 TT debug / console / admin facilities

`docs/tt484/TTConsoleCommands.md`. Of 65 commands, 6 already existed, **53 are
ported**, and 6 are not — each named there with a reason. The four held back at
the time of this audit have since been finished: `tag` gained its `cPlayer` field,
`mapch` gained the client-to-server reply channel it needed, and the remote
screenshot pair (`ssurl`/`sshot`) was implemented once its policy question had an
answer written into the design rather than into a comment — see
`docs/tt484/RemoteScreenshots.md`.
