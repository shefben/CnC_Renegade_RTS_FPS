/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderuiinput.cpp                $*
 *                                                                                             *
 *	Roadmap Sections 41-43, Phases 35-37.  See commanderuiinput.h for why this exists.          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderuiinput.h"

#include "camera.h"
#include "ccamera.h"
#include "combat.h"
#include "commandermodecontroller.h"
#include "commanderplacement.h"
#include "commanderclaimevent.h"
#include "commandercommandpalette.h"
#include "commandercommandpaletterenderer.h"
#include "commanderplacementevent.h"
#include "commanderorderevent.h"
#include "commanderproduction.h"
#include "commanderproductionevent.h"
#include "commanderpowerevent.h"
#include "commanderminimap.h"
#include "commanderpowerpanel.h"
#include "commandergovernancepanel.h"
#include "commandertransferevent.h"
#include "commandervotecallevent.h"
#include "commandervotecastevent.h"
#include "commanderselection.h"
#include "commandersidebar.h"
#include "commandersidebarrenderer.h"
#include "dialogmgr.h"
#include "directinput.h"
#include <dinput.h>
#include "input.h"
#include "mousemgr.h"
#include "playerdata.h"
#include "playertype.h"
#include "render2d.h"
#include "cnetwork.h"
#include "gameobjmanager.h"
#include "building.h"
#include "smartgameobj.h"
#include "soldier.h"
#include "wwmath.h"


static bool	_Available			= false;
static bool	_PointerActive		= false;		// true while this file owns the cursor
static bool	_LeftWasDown		= false;
static bool	_RightWasDown		= false;
static int	_ClickCount			= 0;
static int	_CardClickCount	= 0;

//	True between a left-press on the world and its release.  A drag is only a box while this is
//	set, so a press that started on the panel can never turn into a selection box.
static bool	_WorldPressActive	= false;


/***********************************************************************************************
 * Read_Cursor -- where the pointer is, in the space the panel's boxes are in                    *
 *                                                                                             *
 *	`DirectInput::CursorPos` and `Render2DClass::Get_Screen_Resolution` are the same space:       *
 *	`ScreenCursorClass::Render` clamps one against the other every frame and draws the cursor     *
 *	sprite there, and `CommanderSidebarRenderer` lays the panel out from the same rect.  So the   *
 *	pixel the player sees the arrow on is the pixel the hit-test is given, with no conversion.    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void	Read_Cursor(Vector2 * out_point)
{
	Vector3 position (0.0f,0.0f,0.0f);
	DirectInput::Get_Cursor_Pos (&position);

	//	Clamped here as well as in ScreenCursorClass, because a click can be read on a frame the
	//	cursor sprite has not drawn yet -- the frame Commander Mode was entered on.
	const RectClass & screen = Render2DClass::Get_Screen_Resolution ();
	out_point->X = WWMath::Clamp (position.X,screen.Left,screen.Right);
	out_point->Y = WWMath::Clamp (position.Y,screen.Top,screen.Bottom);
}


/***********************************************************************************************
 * Pick_World_Point -- the world point under the pointer, via the camera's own unprojection      *
 *                                                                                             *
 *	This is the "cursor ray" `CommanderModeController::Pick_At_Cursor` has always wanted and      *
 *	never been handed (WIP_features.md's P37 entry).  It asks the live camera to unproject the    *
 *	cursor pixel onto its view plane -- the same `Device_To_World_Space` the level editor's       *
 *	camera manager uses -- and casts from the camera through that point.                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static bool	Pick_World_Point(const Vector2 & cursor,Vector3 * out_world_point)
{
	CameraClass * camera = COMBAT_CAMERA;
	if (camera == nullptr) return false;

	Vector3 view_plane_point (0.0f,0.0f,0.0f);
	camera->Device_To_World_Space (cursor,&view_plane_point);

	Vector3 origin = camera->Get_Position ();
	Vector3 direction = view_plane_point - origin;

	return CommanderModeController::Pick_At_Cursor (origin,direction,out_world_point);
}


//	The same camera ray, resolved to the object it strikes (0 for ground or sky).
static int	Pick_Object(const Vector2 & cursor,bool * out_is_building)
{
	CameraClass * camera = COMBAT_CAMERA;
	if (camera == nullptr) return 0;

	Vector3 view_plane_point (0.0f,0.0f,0.0f);
	camera->Device_To_World_Space (cursor,&view_plane_point);

	Vector3 origin = camera->Get_Position ();
	return CommanderModeController::Pick_Object_At_Cursor (origin,view_plane_point - origin,out_is_building);
}


/***********************************************************************************************
 * Begin_Card -- what clicking a card means, by the mode it was clicked in                       *
 *                                                                                             *
 *	Both branches go through exactly the path the equivalent console command already takes --     *
 *	the model or a net event, never past either -- so a card click and a typed command cannot      *
 *	produce different results.  A card the panel drew as unbuyable is not acted on here: the       *
 *	model already decided that and drew the reason on the card, and re-deriving it would be a      *
 *	second implementation of the same rule.  The server re-validates either way.                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void	Begin_Card(const CommanderSidebarCard * card)
{
	if ((card == nullptr) || (card->Status != CARD_AVAILABLE)) {
		return;
	}

	_CardClickCount++;

	if (CommanderSidebarModel::Get_Mode () == SIDEBAR_MODE_BUILDINGS) {

		/*
		**	A building card does not buy anything: it arms a placement ghost, which the next
		**	world click commits.  It starts at the star's feet, because clicking a card means the
		**	pointer is over the panel and there is no world point under it yet -- from the next
		**	frame on Update moves it to wherever the pointer actually is.
		*/
		if (card->DescriptorName.Get_Length () == 0) return;

		CommanderPlacementValidator::Begin_Ghost (card->DescriptorName.Peek_Buffer (),
																CommanderSidebarModel::Get_Team ());

		Vector3 position (0.0f,0.0f,0.0f);
		if (COMBAT_STAR != nullptr) {
			COMBAT_STAR->Get_Position (&position);
		}
		CommanderPlacementValidator::Move_Ghost (position);
		return;
	}

	/*
	**	A unit card queues at whatever facility the panel is bound to.  The bound facility's own
	**	state is what the model already used to decide the card was buyable, so there is nothing
	**	further to check here; the server looks the facility up again regardless.
	*/
	const CommanderFacilityState & facility = CommanderSidebarModel::Get_Facility_State ();
	if (!facility.Is_Operational () || (card->DefinitionId == 0)) {
		return;
	}

	cCommanderProductionEvent * event = new cCommanderProductionEvent;
	event->Init (facility.MemberId,card->DefinitionId);
}


/***********************************************************************************************
 * Local_Team -- the team the local player is on, or neutral                                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static int	Local_Team(void)
{
	if (COMBAT_STAR == nullptr) return PLAYERTYPE_NEUTRAL;
	return COMBAT_STAR->Get_Player_Type ();
}


/***********************************************************************************************
 * Select_In_Box -- everything of the local team inside a screen rectangle                        *
 *                                                                                               *
 *	Walks `GameObjManager::Get_Smart_Game_Obj_List`, which is the list of soldiers and vehicles   *
 *	and nothing else -- `BuildingGameObj` derives from `DamageableGameObj`, not `SmartGameObj`,   *
 *	so "a box never selects a building" holds because a building cannot appear in the list        *
 *	being walked, not because of a test that could be forgotten.                                  *
 *                                                                                               *
 *	Human-driven units are included deliberately.  A Commander needs to see and count their own   *
 *	team; what a human's unit will not do is take the order, and that refusal lives on the server *
 *	in `cCommanderOrderEvent::Act`, where it cannot be bypassed by a client that selects anyway.  *
 *                                                                                               *
 *	Selection is by the unit's origin projected to screen rather than by its full bounds.  It is  *
 *	the cheaper test and, more importantly, the predictable one: a box that catches a tank        *
 *	because one corner of its bounding box clipped the edge is a box the player did not draw.     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static int	Select_In_Box(const Vector2 & minimum,const Vector2 & maximum)
{
	CameraClass * camera = COMBAT_CAMERA;
	if (camera == nullptr) return 0;

	const RectClass & screen = Render2DClass::Get_Screen_Resolution ();
	int team = Local_Team ();
	int added = 0;

	for (SLNode<SmartGameObj> * node = GameObjManager::Get_Smart_Game_Obj_List ()->Head ();
		  node != nullptr;
		  node = node->Next ()) {

		SmartGameObj * unit = node->Data ();
		if (unit == nullptr) continue;
		if (unit->Is_Delete_Pending ()) continue;
		if (unit->Get_Player_Type () != team) continue;

		//	A dead body is still a SmartGameObj for a while.  Ordering one is a silent no-op the
		//	player cannot see the reason for, so it never enters the selection.
		DamageableGameObj * damageable = (DamageableGameObj *)unit;
		if ((damageable->Get_Defense_Object () == nullptr) ||
			 (damageable->Get_Defense_Object ()->Get_Health () <= 0.0f)) {
			continue;
		}

		Vector3 position (0.0f,0.0f,0.0f);
		unit->Get_Position (&position);

		Vector3 projected (0.0f,0.0f,0.0f);
		if (camera->Project (projected,position) != CameraClass::INSIDE_FRUSTUM) continue;

		float x = screen.Left + (projected.X + 1.0f) * 0.5f * screen.Width ();
		float y = screen.Top  + (1.0f - projected.Y) * 0.5f * screen.Height ();

		if ((x < minimum.X) || (x > maximum.X)) continue;
		if ((y < minimum.Y) || (y > maximum.Y)) continue;

		if (CommanderSelection::Add (unit->Get_ID ())) {
			added++;
		}
	}

	if (added > 0) {
		CommanderSelection::Set_Team (team);
	}
	return added;
}


/***********************************************************************************************
 * Issue_Order -- send the armed command to the whole selection                                  *
 *                                                                                               *
 *	One packet for the group rather than one per unit: the formation the server applies is a      *
 *	property of the group, and it can only compute it from a list it received whole.              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void	Issue_Order(CommanderPaletteCommandType command,const Vector3 & target_point,int target_id)
{
	int ids [cCommanderOrderEvent::MAX_UNITS];
	int count = CommanderSelection::Get_Unit_Ids (ids,cCommanderOrderEvent::MAX_UNITS);
	if (count <= 0) return;

	//	A patrol runs from where the group stands now to the clicked point and back, so its
	//	other end is the selection's own centre at the moment of ordering.
	CommanderUnitOrderType order = CommanderCommandPalette::Get_Order (command);
	Vector3 second_point = target_point;
	if (order == ORDER_PATROL) {
		Vector3 sum (0.0f,0.0f,0.0f);
		int counted = 0;
		for (int i = 0; i < count; i++) {
			PhysicalGameObj * unit = GameObjManager::Find_PhysicalGameObj (ids [i]);
			if (unit == nullptr) continue;
			Vector3 position;
			unit->Get_Position (&position);
			sum += position;
			counted++;
		}
		if (counted > 0) second_point = sum / (float)counted;
	}

	cCommanderOrderEvent * event = new cCommanderOrderEvent;
	event->Init ((int)order,target_point,target_id,ids,count,
					 (int)CommanderCommandPalette::Get_Formation (command),&second_point);

	CommanderCommandPalette::Note_Issued ();
}


/***********************************************************************************************
 * Handle_Palette_Click -- one of the nine buttons                                               *
 *                                                                                               *
 *	A command that needs a point is armed and waits; one that does not is sent immediately at     *
 *	the selection's own position, since "stand where you are" has no destination to name.         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void	Handle_Palette_Click(CommanderPaletteCommandType command)
{
	if (CommanderCommandPalette::Needs_Target (command)) {
		CommanderCommandPalette::Arm (command);
		return;
	}

	//	Target point is the first selected unit's own position: HOLD and STOP are both "stay
	//	here", and the server ignores the point for them anyway -- it is sent so the waypoint
	//	marker the order raises lands somewhere meaningful rather than at the world origin.
	Vector3 point (0.0f,0.0f,0.0f);
	PhysicalGameObj * first = GameObjManager::Find_PhysicalGameObj (CommanderSelection::Get_Unit_Id (0));
	if (first != nullptr) {
		first->Get_Position (&point);
	}

	Issue_Order (command,point,0);
	CommanderCommandPalette::Disarm ();
}


/***********************************************************************************************
 * Handle_Left_Click -- one left-button press, resolved against the panel first                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void	Handle_Left_Click(const Vector2 & cursor)
{
	/*
	**	The command grid is tested before the sidebar because it is drawn over it: it sits in the
	**	lower right, and at small resolutions its panel and the sidebar's share an edge.  Whatever
	**	is on top has to win the click, or the player clicks a button and something behind it
	**	responds.
	*/
	{
		CommanderPaletteCommandType command = PALETTE_MOVE;
		bool inside_panel = false;

		if (CommanderCommandPaletteRenderer::Hit_Test (cursor.X,cursor.Y,&command,&inside_panel)) {
			_ClickCount++;
			Handle_Palette_Click (command);
			return;
		}

		//	Inside the grid's panel but between buttons: swallowed, not passed to the world.
		if (inside_panel) {
			_ClickCount++;
			return;
		}
	}

	/*
	**	The governance panel (top left): each button is one of the existing governance events,
	**	so a click here and the matching console command reach the server the same way.
	*/
	{
		CommanderGovernanceButton button;
		bool inside_panel = false;

		if (CommanderGovernancePanelRenderer::Hit_Test (cursor.X,cursor.Y,&button,&inside_panel)) {
			_ClickCount++;
			switch (button.Action) {
				case GOVERNANCE_ACTION_VOTE_YES:
				case GOVERNANCE_ACTION_VOTE_NO: {
					cCommanderVoteCastEvent * event = new cCommanderVoteCastEvent;
					event->Init (button.Action == GOVERNANCE_ACTION_VOTE_YES);
					CommanderGovernancePanel::Note_Local_Vote (Local_Team ());
					break;
				}
				case GOVERNANCE_ACTION_CALL_REMOVE: {
					cCommanderVoteCallEvent * event = new cCommanderVoteCallEvent;
					event->Init (true,button.TargetId);
					break;
				}
				case GOVERNANCE_ACTION_CLAIM: {
					cCommanderClaimEvent * event = new cCommanderClaimEvent;
					event->Init (true);
					break;
				}
				case GOVERNANCE_ACTION_TRANSFER: {
					cCommanderTransferEvent * event = new cCommanderTransferEvent;
					event->Init (button.TargetId);
					break;
				}
				default:
					break;
			}
			return;
		}

		if (inside_panel) {
			_ClickCount++;
			return;
		}
	}

	/*
	**	The minimap (V2 Phase 16): a click on the map moves the Commander camera there; a click on
	**	its frame is swallowed like any other panel's.
	*/
	{
		Vector3 world (0.0f,0.0f,0.0f);
		bool inside_panel = false;
		if (CommanderMinimapRenderer::Hit_Test (cursor.X,cursor.Y,&world,&inside_panel)) {
			_ClickCount++;
			CommanderModeController::Set_Focus (world);
			return;
		}
		if (inside_panel) {
			_ClickCount++;
			return;
		}
	}

	/*
	**	The selected building's power panel, which occupies the grid's corner while a building
	**	rather than units is selected.  A click becomes a request; what the panel shows next is
	**	whatever the server answers through the Commander mirror.
	*/
	{
		CommanderPowerPanelButtonType button = POWER_PANEL_TOGGLE;
		bool inside_panel = false;

		if (CommanderPowerPanelRenderer::Hit_Test (cursor.X,cursor.Y,&button,&inside_panel)) {
			_ClickCount++;

			CommanderPowerPanelState state;
			int action = 0, value = 0;
			if (CommanderPowerPanel::Build_State (CommanderSelection::Get_Selected_Building (),&state) &&
				 CommanderPowerPanel::Get_Request (button,state,&action,&value)) {
				cCommanderPowerEvent * event = new cCommanderPowerEvent;
				event->Init (state.BuildingId,action,value);
			}
			return;
		}

		if (inside_panel) {
			_ClickCount++;
			return;
		}
	}

	_ClickCount++;

	int index = -1;
	CommanderSidebarHitType hit = CommanderSidebarRenderer::Hit_Test (cursor.X,cursor.Y,&index);

	switch (hit) {

		case SIDEBAR_HIT_MODE_TAB:
			if ((index >= 0) && (index < SIDEBAR_MODE_COUNT)) {
				CommanderSidebarModel::Set_Mode ((CommanderSidebarModeType)index);
			}
			return;

		case SIDEBAR_HIT_PAGE_PREV:
			CommanderSidebarModel::Prev_Page ();
			return;

		case SIDEBAR_HIT_PAGE_NEXT:
			CommanderSidebarModel::Next_Page ();
			return;

		case SIDEBAR_HIT_CARD:
			Begin_Card (CommanderSidebarRenderer::Get_Card_At (cursor.X,cursor.Y));
			return;

		case SIDEBAR_HIT_PANEL:
			//	On the strip but not on a control.  Swallowed: the world behind the panel must not
			//	receive a click the player aimed at the panel.
			return;

		case SIDEBAR_HIT_NONE:
		default:
			break;
	}

	/*
	**	A click on the world.  With a ghost armed it commits the placement at the pointer; that
	**	is the aiming half Section 43 has been missing, and it goes through the same `Move_Ghost`
	**	the console command does, so the client's own red/green verdict is computed from the
	**	position actually committed rather than a stale one.
	**
	**	Sent even when the client believes the spot is illegal, matching `commander_place`: the
	**	server re-runs every check, and a refusal it reports is worth more than one the client
	**	swallows silently.
	*/
	if (CommanderPlacementValidator::Is_Ghost_Active ()) {

		Vector3 world_point (0.0f,0.0f,0.0f);
		if (!Pick_World_Point (cursor,&world_point)) {
			return;					// pointing at the sky: no ground to place on, so nothing happens
		}

		CommanderPlacementValidator::Move_Ghost (world_point);

		const CommanderPlacementRequest & ghost = CommanderPlacementValidator::Get_Ghost ();

		cCommanderPlacementEvent * event = new cCommanderPlacementEvent;
		event->Init (ghost.DescriptorName.Peek_Buffer (),ghost.Position,ghost.RotationDegrees,
						 ghost.SequenceId);

		CommanderPlacementValidator::Cancel_Ghost ();
		return;
	}

	/*
	**	A command armed from the grid consumes this click as its target.  Checked before the drag
	**	does anything, because arming ATTACK and then clicking a tank must attack it rather than
	**	start a selection box over it.
	*/
	if (CommanderCommandPalette::Is_Armed () && (CommanderSelection::Get_Count () > 0)) {

		CommanderPaletteCommandType command = CommanderCommandPalette::Get_Armed ();

		Vector3 world_point (0.0f,0.0f,0.0f);
		if (!Pick_World_Point (cursor,&world_point)) return;

		Issue_Order (command,world_point,0);
		CommanderCommandPalette::Disarm ();
		return;
	}

	/*
	**	Nothing else wanted the click, so it begins a selection box.  Whether it turns out to be
	**	a box or a bare click is decided on release: a press that never moves clears the
	**	selection, which is what clicking empty ground means in every RTS this is modelled on.
	*/
	_WorldPressActive = true;
	CommanderSelection::Begin_Drag (cursor);
}


/***********************************************************************************************
 * Handle_Left_Release -- what the press turned out to be                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void	Handle_Left_Release(const Vector2 & cursor)
{
	if (!_WorldPressActive) return;
	_WorldPressActive = false;

	CommanderSelection::Update_Drag (cursor);

	bool was_box = CommanderSelection::Is_Drag_Significant ();

	Vector2 minimum (0.0f,0.0f);
	Vector2 maximum (0.0f,0.0f);
	CommanderSelection::Get_Drag_Rect (&minimum,&maximum);

	CommanderSelection::End_Drag ();

	//	A fresh box replaces the selection rather than adding to it.  Additive selection wants a
	//	modifier key, and the input gate this file installs suppresses every modifier that is not
	//	on its allow-list -- so offering it here would be offering something that cannot work.
	CommanderSelection::Clear ();
	CommanderSelection::Clear_Selected_Building ();

	if (was_box) {
		Select_In_Box (minimum,maximum);
		return;
	}

	/*
	**	A click, not a box: whatever of the player's own team is under the pointer.  A building
	**	opens its power panel; a living unit becomes a one-unit selection.  Anything else --
	**	ground, an enemy, a corpse -- leaves the selection empty, which is what clicking empty
	**	ground means.
	*/
	bool is_building = false;
	int picked = Pick_Object (cursor,&is_building);
	if (picked == 0) return;

	int team = Local_Team ();

	if (is_building) {
		ScriptableGameObj * scriptable = GameObjManager::Find_ScriptableGameObj (picked);
		BuildingGameObj * building = (scriptable != nullptr) ? scriptable->As_BuildingGameObj () : nullptr;
		if ((building != nullptr) && (building->Get_Player_Type () == team) && !building->Is_Destroyed ()) {
			CommanderSelection::Select_Building (picked);
		}
		return;
	}

	PhysicalGameObj * unit = GameObjManager::Find_PhysicalGameObj (picked);
	if ((unit == nullptr) || (unit->Get_Player_Type () != team)) return;
	if (unit->As_SmartGameObj () == nullptr) return;		// a unit, not a crate or a prop

	DamageableGameObj * damageable = (DamageableGameObj *)unit;
	if ((damageable->Get_Defense_Object () == nullptr) ||
		 (damageable->Get_Defense_Object ()->Get_Health () <= 0.0f)) {
		return;
	}

	if (CommanderSelection::Add (picked)) {
		CommanderSelection::Set_Team (team);
	}
}


/***********************************************************************************************
 * Handle_Right_Click -- cancel                                                                  *
 *                                                                                             *
 *	Right-click cancels the armed ghost, which is what every RTS this panel is modelled on does  *
 *	and is the only way out of a placement that does not require committing one.                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void	Handle_Right_Click(const Vector2 & cursor)
{
	//	Cancelling comes first, in the order a player expects to be able to back out of things:
	//	the ghost they are placing, then the command they armed, and only then does right click
	//	mean anything positive.
	if (CommanderPlacementValidator::Is_Ghost_Active ()) {
		CommanderPlacementValidator::Cancel_Ghost ();
		return;
	}

	if (CommanderCommandPalette::Is_Armed ()) {
		CommanderCommandPalette::Disarm ();
		return;
	}

	/*
	**	The default order.  Right-clicking the ground with a selection is "go there" in every RTS
	**	this panel is modelled on, and it is the fastest way for a Commander to set the waypoint
	**	their AI will obey and their human teammates will see a marker for.  A right click over
	**	the panels does nothing: those are hit-tested for the left button only, and an order
	**	aimed at a point behind the sidebar is not a point the player chose.
	*/
	if (CommanderSelection::Get_Count () <= 0) return;

	bool inside_panel = false;
	CommanderPaletteCommandType ignored = PALETTE_MOVE;
	if (CommanderCommandPaletteRenderer::Hit_Test (cursor.X,cursor.Y,&ignored,&inside_panel) ||
		 inside_panel) {
		return;
	}
	{
		bool inside_power = false, inside_governance = false, inside_minimap = false;
		CommanderPowerPanelRenderer::Hit_Test (cursor.X,cursor.Y,nullptr,&inside_power);
		CommanderGovernancePanelRenderer::Hit_Test (cursor.X,cursor.Y,nullptr,&inside_governance);
		CommanderMinimapRenderer::Hit_Test (cursor.X,cursor.Y,nullptr,&inside_minimap);
		if (inside_power || inside_governance || inside_minimap) return;
	}
	if (CommanderSidebarRenderer::Hit_Test (cursor.X,cursor.Y,nullptr) != SIDEBAR_HIT_NONE) {
		return;
	}

	Vector3 world_point (0.0f,0.0f,0.0f);
	if (!Pick_World_Point (cursor,&world_point)) return;

	Issue_Order (PALETTE_MOVE,world_point,0);
}


/***********************************************************************************************
 * CommanderUIInput::Toggle_Commander_Mode -- the `b` key                                        *
 *                                                                                               *
 *	Claiming before entering is the whole reason this is not a direct call to                     *
 *	`CommanderModeController`.  Every authority gate in the Commander layer tests the sender      *
 *	against `CommanderProductionService::Get_Team_Commander`, and until this existed nothing in   *
 *	the tree ever set it on a live server -- so the mode opened onto a panel whose every card     *
 *	would have been refused.                                                                      *
 *                                                                                               *
 *	The claim is sent every time rather than only when the team looks vacant: the client's copy   *
 *	of who the Commander is arrives by replication and can be a frame or two stale, and the       *
 *	server refuses a contested claim harmlessly.  Asking and being told no costs one small packet; *
 *	not asking because of a stale local value costs the player the whole feature.                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void CommanderUIInput::Toggle_Commander_Mode(void)
{
	if (CommanderModeController::Is_Active ()) {

		CommanderModeController::Exit_Commander_Mode ();

		//	Leaving the mode is not resigning.  The job is kept, so a Commander can drop back to
		//	their body to fight and return to the panel without re-claiming -- and without the
		//	team being briefly Commander-less every time they do.
		return;
	}

	Enter_Commander_Mode ();
}


bool CommanderUIInput::Enter_Commander_Mode(void)
{
	if (CommanderModeController::Is_Active ()) return false;
	if (COMBAT_STAR == nullptr) return false;

	cCommanderClaimEvent * claim = new cCommanderClaimEvent;
	claim->Init (true);

	/*
	**	Entered without waiting for the grant.  On a listen server or in a solo/skirmish match the
	**	claim above has already been applied by the time this runs, and on a remote client the
	**	camera is local state that costs nothing if the claim turns out to be refused -- whereas
	**	blocking entry on a round trip would make the key feel broken on any real connection.
	*/
	return CommanderModeController::Enter_Commander_Mode (COMBAT_STAR);
}


bool CommanderUIInput::Is_Local_Player_Commander(void)
{
	if (COMBAT_STAR == nullptr) return false;

	int team = COMBAT_STAR->Get_Player_Type ();
	return (CommanderProductionService::Get_Team_Commander (team) == cNetwork::Get_My_Id ());
}


void CommanderUIInput::Init(bool render_available)
{
	_Available			= render_available;
	_PointerActive		= false;
	_LeftWasDown		= false;
	_RightWasDown		= false;
	_ClickCount			= 0;
	_CardClickCount	= 0;
}


void CommanderUIInput::Shutdown(void)
{
	//	Put the cursor and the input gate back however this file found them, whatever state
	//	Commander Mode was left in -- a shutdown part-way through Commander Mode must not leave
	//	the FPS controls gated for whatever runs next.
	if (_PointerActive) {
		Input::Commander_UI_Enable (false);
		if (DialogMgrClass::Get_Dialog_Count () == 0) {
			MouseMgrClass::Show_Cursor (false);
		}
	}

	_Available		= false;
	_PointerActive	= false;
}


void CommanderUIInput::Update(void)
{
	if (!_Available) return;

	bool want_pointer = CommanderModeController::Is_Active ();

	/*
	**	Cursor ownership.  Re-asserted every frame rather than only on the transition, because
	**	`DialogMgrClass::On_Dialog_Removed` hides the cursor unconditionally when its own list
	**	empties (Code/wwui/dialogmgr.cpp) -- opening and closing the in-game menu while in
	**	Commander Mode would otherwise take the pointer away and never give it back.  Show_Cursor
	**	is a no-op when the cursor is already in the asked-for state, so this costs nothing.
	*/
	if (want_pointer) {
		MouseMgrClass::Show_Cursor (true);
	} else if (_PointerActive && (DialogMgrClass::Get_Dialog_Count () == 0)) {
		//	Only hide what this file showed, and only when no dialog wants it -- a dialog opened
		//	over Commander Mode and still up when Commander Mode exits keeps its own cursor.
		MouseMgrClass::Show_Cursor (false);
	}

	//	The FPS gate.  Self-guarding against repeats, so this is safe to call every frame.
	Input::Commander_UI_Enable (want_pointer);

	_PointerActive = want_pointer;

	if (!want_pointer) {
		_LeftWasDown		= false;
		_RightWasDown		= false;
		_WorldPressActive	= false;

		//	Leaving Commander Mode drops the selection and the grid with it.  Keeping them would
		//	mean returning to a panel full of units that may have died in the meantime, and an
		//	armed command waiting for a click that will now be a weapon trigger.
		CommanderSelection::End_Drag ();
		CommanderSelection::Clear ();
		CommanderCommandPalette::Refresh (0);
		return;
	}

	/*
	**	A dialog on screen owns the mouse outright: its own controls are being hit-tested against
	**	the same cursor, and reading the button here as well would fire a card and a dialog button
	**	from one press.  The held-state flags are set rather than cleared so the press that
	**	dismisses the dialog cannot read as a fresh press to the panel on the frame after.
	*/
	if (DialogMgrClass::Get_Dialog_Count () > 0) {
		_LeftWasDown		= true;
		_RightWasDown		= true;
		_WorldPressActive	= false;
		CommanderSelection::End_Drag ();
		return;
	}

	Vector2 cursor (0.0f,0.0f);
	Read_Cursor (&cursor);

	//	The ghost follows the pointer while one is armed, so what the player is about to commit
	//	is what they are looking at.  The other half of Section 43's aiming gap.
	if (CommanderPlacementValidator::Is_Ghost_Active ()) {
		Vector3 world_point (0.0f,0.0f,0.0f);
		if (Pick_World_Point (cursor,&world_point)) {
			CommanderPlacementValidator::Move_Ghost (world_point);
		}
	}

	/*
	**	Prune before anything reads the selection this frame, so the count on the grid and the
	**	ids an order would carry are the same list -- a unit that died since last frame must not
	**	be counted in one and missing from the other.
	*/
	CommanderSelection::Prune ();

	//	Edge triggered: a held button is one click, not one per frame.
	bool left_down = Input::Is_Button_Down (DirectInput::BUTTON_MOUSE_LEFT);

	if (left_down && !_LeftWasDown) {
		Handle_Left_Click (cursor);
	} else if (left_down && _WorldPressActive) {
		//	Held and dragging: the box follows the pointer.
		CommanderSelection::Update_Drag (cursor);
	} else if (!left_down && _LeftWasDown) {
		Handle_Left_Release (cursor);
	}

	_LeftWasDown = left_down;

	bool right_down = Input::Is_Button_Down (DirectInput::BUTTON_MOUSE_RIGHT);
	if (right_down && !_RightWasDown) {
		Handle_Right_Click (cursor);
	}
	_RightWasDown = right_down;

	/*
	**	Control groups, the conventional way: Ctrl+digit stores the selection as that group, a
	**	bare digit recalls it.  Read straight from the keyboard state rather than through an
	**	input function: the digits are the weapon-select functions, which the Commander UI gate
	**	suppresses, and that suppression is exactly why they are free to mean something here.
	**	Local only -- a recalled group is ordered like any other selection.
	*/
	{
		static const int DIGIT_KEYS [CommanderSelection::MAX_CONTROL_GROUPS] =
			{ DIK_0,DIK_1,DIK_2,DIK_3,DIK_4,DIK_5,DIK_6,DIK_7,DIK_8,DIK_9 };

		bool control_held = ((DirectInput::Get_Keyboard_Button (DIK_LCONTROL) & DirectInput::DI_BUTTON_HELD) != 0) ||
								  ((DirectInput::Get_Keyboard_Button (DIK_RCONTROL) & DirectInput::DI_BUTTON_HELD) != 0);

		for (int group = 1; group < CommanderSelection::MAX_CONTROL_GROUPS; group++) {
			if ((DirectInput::Get_Keyboard_Button (DIGIT_KEYS [group]) & DirectInput::DI_BUTTON_HIT) == 0) continue;

			if (control_held) {
				CommanderSelection::Assign_Group (group);
			} else if (CommanderSelection::Recall_Group (group) > 0) {
				CommanderSelection::Set_Team (Local_Team ());
				CommanderSelection::Prune ();
			}
		}
	}

	//	The grid exists exactly when there is something to command.  Driven from the selection
	//	rather than toggled, so an emptied selection cannot leave it behind.
	CommanderCommandPalette::Refresh (CommanderSelection::Get_Count ());

	/*
	**	Re-read the model, last, so a click handled above is already reflected in what gets drawn
	**	this frame rather than the frame after.
	**
	**	Until there was a pointer, the only thing that ever refreshed the model was the
	**	`commander_sidebar` console command -- which was fine while typing that command was also
	**	the only way to change what the panel showed.  A clicked tab has no such moment: without
	**	this, switching mode would leave the previous mode's cards on screen, a card would stay
	**	affordable after the credits were spent, and a facility that lost power would keep
	**	offering units.  Section 42 requires the opposite of all three -- "immediately
	**	disable/update the contextual panel".
	**
	**	Refresh rebuilds the card list and re-reads the bound facility state.  The renderer's own
	**	Update runs later in the same frame (`CombatManager::Think`) and only rebuilds geometry
	**	when the result actually changed, so a frame in which nothing moved still costs no draw
	**	work.  The hit-test above deliberately runs against the boxes the *previous* rebuild
	**	recorded, which are the boxes the player was actually looking at when they clicked.
	*/
	int credits = 0;
	if ((COMBAT_STAR != nullptr) && (COMBAT_STAR->Get_Player_Data () != nullptr)) {
		credits = (int)COMBAT_STAR->Get_Player_Data ()->Get_Money ();
	}
	CommanderSidebarModel::Refresh (credits);
}


bool	CommanderUIInput::Is_Pointer_Active(void)		{ return _PointerActive; }
int	CommanderUIInput::Get_Click_Count(void)			{ return _ClickCount; }
int	CommanderUIInput::Get_Card_Click_Count(void)	{ return _CardClickCount; }
