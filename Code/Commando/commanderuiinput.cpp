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
#include "commanderplacementevent.h"
#include "commanderproductionevent.h"
#include "commandersidebar.h"
#include "commandersidebarrenderer.h"
#include "dialogmgr.h"
#include "directinput.h"
#include "input.h"
#include "mousemgr.h"
#include "playerdata.h"
#include "playertype.h"
#include "render2d.h"
#include "soldier.h"
#include "wwmath.h"


static bool	_Available			= false;
static bool	_PointerActive		= false;		// true while this file owns the cursor
static bool	_LeftWasDown		= false;
static bool	_RightWasDown		= false;
static int	_ClickCount			= 0;
static int	_CardClickCount	= 0;


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
 * Handle_Left_Click -- one left-button press, resolved against the panel first                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void	Handle_Left_Click(const Vector2 & cursor)
{
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
	}
}


/***********************************************************************************************
 * Handle_Right_Click -- cancel                                                                  *
 *                                                                                             *
 *	Right-click cancels the armed ghost, which is what every RTS this panel is modelled on does  *
 *	and is the only way out of a placement that does not require committing one.                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void	Handle_Right_Click(void)
{
	if (CommanderPlacementValidator::Is_Ghost_Active ()) {
		CommanderPlacementValidator::Cancel_Ghost ();
	}
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
		_LeftWasDown	= false;
		_RightWasDown	= false;
		return;
	}

	/*
	**	A dialog on screen owns the mouse outright: its own controls are being hit-tested against
	**	the same cursor, and reading the button here as well would fire a card and a dialog button
	**	from one press.  The held-state flags are set rather than cleared so the press that
	**	dismisses the dialog cannot read as a fresh press to the panel on the frame after.
	*/
	if (DialogMgrClass::Get_Dialog_Count () > 0) {
		_LeftWasDown	= true;
		_RightWasDown	= true;
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

	//	Edge triggered: a held button is one click, not one per frame.
	bool left_down = Input::Is_Button_Down (DirectInput::BUTTON_MOUSE_LEFT);
	if (left_down && !_LeftWasDown) {
		Handle_Left_Click (cursor);
	}
	_LeftWasDown = left_down;

	bool right_down = Input::Is_Button_Down (DirectInput::BUTTON_MOUSE_RIGHT);
	if (right_down && !_RightWasDown) {
		Handle_Right_Click ();
	}
	_RightWasDown = right_down;

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
