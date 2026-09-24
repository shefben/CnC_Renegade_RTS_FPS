# Commander interaction layer (D-001 priority 2)

This is the input and panel layer that sits on the Commander services: how a player gets into
Commander Mode, selects things, orders them and runs the base. The services underneath
(production, power grid, governance, orders) are documented in their own files; this page covers
what was added to make them reachable from the mouse and keyboard. D-001 in `DECISIONS.md` is the
ruling it follows.

## Entering Commander Mode (P35)

- **Key:** `INPUT_FUNCTION_COMMANDER_MODE_TOGGLE`, default `B`, edge-triggered, readable while the
  Commander UI gate is up so the same key closes it. It already existed; what was missing is now
  in place:
  - it has a row in **Controls > Multiplayer** (`renegadedialog.cpp`, `dlgcontroltabs.cpp`) and is
    listed in the remap table, so a key already bound to it counts as taken. The label is literal
    text, because the retail string table has no entry for a function it never had.
  - the `commander_mode 1` console command now goes through `CommanderUIInput::Enter_Commander_Mode`,
    the same claim-then-enter path as the key. Before, the command entered without claiming the
    seat, so every panel refused the player.
- **Camera anchor:** no building has a Commander-station bone, and none is required.
  `CommanderModeController::Resolve_Entry_Focus` opens the camera on the team's generated base
  site, else the centre of the team's registered buildings, else the soldier. It is applied on the
  first entry of a match only, so leaving and coming back keeps the player's pan. Before this, the
  camera opened on the world origin.
- Death still exits Commander Mode (`CommanderModeController::Update`, fed from `combat.cpp`).

## Selecting (P36, P39)

- **Drag box:** already worked (`Select_In_Box`), selecting living units of the local team.
- **Click:** a click that is not a drag now picks what is under the pointer
  (`CommanderModeController::Pick_Object_At_Cursor`, the ground-pick ray resolved through the hit
  object's observer). One of your own living units becomes the selection; one of your own
  buildings becomes the **selected building**; anything else clears. Units and a building are
  never selected together.
- **Control groups:** `Ctrl+1..9` stores the current unit selection as that group, `1..9` recalls
  it. Groups are local client state (`CommanderSelection::Assign_Group` / `Recall_Group`). Dead
  members drop out in `Prune`. The digits are free in Commander Mode because the UI gate
  suppresses the weapon-select functions they normally drive.

## Selection rings (P29)

`CommanderSelectionOverlays` keeps one `OVERLAY_SELECTION` status ring under each selected unit and
under the selected building, sized from the model's bounding sphere or the building's footprint.
Rings move with their object and are removed the frame the object leaves the selection or dies.
Order destinations are not ringed, because every order already raises a waypoint marker.

## The power panel (P36)

Clicking one of your buildings opens a panel in the lower right, where the order grid sits when
units are selected. It shows the building's name, draw and priority (or generation), whether it is
powered or switched off by hand, and the team's grid totals, with four buttons: **POWER ON/OFF**,
**PRIORITY -**, **PRIORITY +** (steps of 10) and **DEFAULT** (clear the override).

- A click sends `cCommanderPowerEvent`. The server accepts it only from the team's Commander, and
  only for a live building on the Commander's own team. It then calls
  `TeamPowerGridService::Set_Manual_Disabled` / `Set_Priority_Override` / `Clear_Priority_Override`.
- The panel shows what the server holds: per-building overrides now travel in `CommanderStateClass`'s
  mirror, and a client stores them with `TeamPowerGridService::Set_Replicated_Member_State`, which
  writes the fields without re-running the allocator. The client never recalculates the grid.
- A building with no descriptor is not on the Commander grid (`BaseControllerClass::Power_Base`
  still powers it). Its panel says so and offers no buttons.

## Orders: two-point patrol (P39)

`ORDER_PATROL` now patrols. The order carries a second point, which is the centre of the selection
when the order was given. `CommanderPatrolAI` sends the unit to the clicked point, then back, and
repeats until any other order arrives, whether from the player, `Dispatch_Order` or the AI
Commander. A formation keeps its shape at both ends. A leg that cannot be finished in 60 seconds is
abandoned for the other one, so a blocked unit keeps moving.

- The durable order record keeps the second point, and saves write it as an appended micro chunk.
  An older save restores a patrol with both ends at its target.
- Not changed: a loaded save records orders but does not re-dispatch them. This was already true
  for guard, and patrol behaves the same.

## Governance panel (P23)

In Commander Mode, the top-left panel shows:

- the team's Commander (by name, or YOU, NOBODY or AI COMMANDER);
- any open vote, with its type, candidate, time left, and yes and no counts.

The buttons depend on the state:

| State | Buttons |
| --- | --- |
| Vote open, you have not voted | **VOTE YES**, **VOTE NO** |
| No vote, seat vacant | **TAKE COMMAND** (a contested claim becomes an election server-side) |
| No vote, someone else commands | **VOTE TO REMOVE** |
| You command | **GIVE TO name**, one per teammate |

Each button sends the existing event (`cCommanderVoteCastEvent`, `cCommanderVoteCallEvent`,
`cCommanderClaimEvent`, `cCommanderTransferEvent`), so it reaches the server exactly as the console
commands do. The console commands remain for admin use.

To support the panel, `CommanderStateClass` now mirrors each team's vote: type, candidate, caller,
yes and no counts, and its end on the server clock, which lets a client count down between packets.
Ballots are never sent.

**Limit:** the panel lives in Commander Mode, where there is a pointer. A player in FPS mode sees no
vote prompt. They can press `B` to open the panel and vote, or use the console.

## Checks

- `shader_commandermode`: the entry anchor fallback, control groups, building/unit exclusion, the
  power panel's button requests (checked against `cCommanderPowerEvent`'s action numbering) and the
  patrol turn-around rule.
- `shader_production`: the patrol order keeps its second point in the durable record.
- `shader_governance`: the panel offers exactly the buttons each state allows, and replicated vote
  counts read back.

## Needs a live client

- the controls-screen row, and the key and console both entering claimed;
- clicking units and buildings, and control groups;
- the rings on screen;
- the power panel against a real grid, including a second client seeing the override;
- a patrol walking back and forth;
- a vote seen and cast from a second client, and a transfer.
