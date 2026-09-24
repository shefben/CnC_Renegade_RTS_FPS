/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandergovernancepanel.cpp          $*
 *                                                                                             *
 *	V2 roadmap Phase 23.  See commandergovernancepanel.h.                                       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commandergovernancepanel.h"

#include "combat.h"
#include "commandermodecontroller.h"
#include "commanderproduction.h"
#include "commanderstate.h"
#include "playerroster.h"
#include "render2d.h"
#include "render2dsentence.h"
#include "soldier.h"
#include "stylemgr.h"

#include <stdio.h>


/*
**	The vote the local player last voted in, by what identifies it: a new vote -- another type,
**	candidate or caller -- brings the buttons back.
*/
static int	_VotedTeam			= -1;
static int	_VotedType			= 0;
static int	_VotedCandidate	= 0;
static int	_VotedInitiator	= 0;


static void	Add_Button(CommanderGovernanceState * state,CommanderGovernanceActionType action,int target_id,
							  const WideStringClass & label)
{
	if (state->ButtonCount >= CommanderGovernanceState::MAX_BUTTONS) return;
	CommanderGovernanceButton & button = state->Buttons [state->ButtonCount++];
	button.Action		= action;
	button.TargetId	= target_id;
	button.Label		= label;
}


void CommanderGovernancePanel::Build_State(int team,int local_id,CommanderGovernanceState * out)
{
	if (out == nullptr) return;
	*out = CommanderGovernanceState ();

	out->Team			= team;
	out->LocalId		= local_id;
	out->CommanderId	= CommanderProductionService::Get_Team_Commander (team);

	if (CommanderProductionService::Is_Vote_Active (team)) {
		out->VoteType			= (int)CommanderProductionService::Get_Vote_Type (team);
		out->VoteCandidate	= CommanderProductionService::Get_Vote_Candidate (team);
		out->VoteSecondsLeft	= CommanderStateClass::Get_Vote_Time_Remaining (team);
		out->VoteYes			= CommanderProductionService::Get_Vote_Yes_Count (team);
		out->VoteNo				= CommanderProductionService::Get_Vote_No_Count (team);

		bool voted = (_VotedTeam == team) && (_VotedType == out->VoteType) &&
						 (_VotedCandidate == out->VoteCandidate) &&
						 (_VotedInitiator == CommanderProductionService::Get_Vote_Initiator (team));
		if (!voted) {
			Add_Button (out,GOVERNANCE_ACTION_VOTE_YES,0,WideStringClass ("VOTE YES",true));
			Add_Button (out,GOVERNANCE_ACTION_VOTE_NO,0,WideStringClass ("VOTE NO",true));
		}
		return;
	}

	if (out->CommanderId == 0) {
		Add_Button (out,GOVERNANCE_ACTION_CLAIM,0,WideStringClass ("TAKE COMMAND",true));
		return;
	}

	if (out->CommanderId != local_id) {
		Add_Button (out,GOVERNANCE_ACTION_CALL_REMOVE,out->CommanderId,WideStringClass ("VOTE TO REMOVE",true));
		return;
	}

	//	The sitting Commander: a hand-off to each teammate the roster knows.
	PlayerRosterInterfaceClass * roster = PlayerRosterClass::Peek_Interface ();
	if (roster == nullptr) return;

	for (int index = 0; index < roster->Get_Player_Count (); index++) {
		int player_id = roster->Get_Player_ID_By_Index (index);
		if ((player_id == 0) || (player_id == local_id)) continue;
		if (roster->Get_Player_Type (player_id) != team) continue;

		WideStringClass label ("GIVE TO ",true);
		const unichar_t * name = roster->Get_Player_Name (player_id);
		if (name != nullptr) label += name;
		Add_Button (out,GOVERNANCE_ACTION_TRANSFER,player_id,label);
	}
}


void CommanderGovernancePanel::Note_Local_Vote(int team)
{
	_VotedTeam			= team;
	_VotedType			= (int)CommanderProductionService::Get_Vote_Type (team);
	_VotedCandidate	= CommanderProductionService::Get_Vote_Candidate (team);
	_VotedInitiator	= CommanderProductionService::Get_Vote_Initiator (team);
}


void CommanderGovernancePanel::Reset(void)
{
	_VotedTeam = -1;
	_VotedType = _VotedCandidate = _VotedInitiator = 0;
}


/***********************************************************************************************
 * CommanderGovernancePanelRenderer -- top left, clear of the sidebar and the lower-right panels  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static bool							_Available	= false;
static bool							_Open			= false;
static Render2DClass *			_Panel		= nullptr;
static Render2DSentenceClass *	_Text			= nullptr;

static bool								_HitValid = false;
static RectClass						_HitPanel;
static RectClass						_HitRects [CommanderGovernanceState::MAX_BUTTONS];
static CommanderGovernanceButton	_HitButtons [CommanderGovernanceState::MAX_BUTTONS];
static int								_HitCount = 0;

static const unsigned int	COLOR_METAL			= RGBA_TO_INT32 ( 66, 66, 62,235);
static const unsigned int	COLOR_BEVEL_LIGHT	= RGBA_TO_INT32 (140,140,132,255);
static const unsigned int	COLOR_BEVEL_DARK	= RGBA_TO_INT32 ( 18, 18, 16,255);
static const unsigned int	COLOR_BUTTON		= RGBA_TO_INT32 ( 42, 42, 40,235);
static const unsigned int	COLOR_AMBER			= RGB_TO_INT32  (252,204, 92);
static const unsigned int	COLOR_AMBER_DIM	= RGB_TO_INT32  (142,116, 56);
static const unsigned int	COLOR_SHADOW		= RGB_TO_INT32  (  0,  0,  0);


static void	Add_Bevel(Render2DClass * renderer,const RectClass & rect,unsigned int fill,float thickness)
{
	renderer->Add_Quad (rect,fill);
	renderer->Add_Quad (RectClass (rect.Left,rect.Top,rect.Right,rect.Top + thickness),COLOR_BEVEL_LIGHT);
	renderer->Add_Quad (RectClass (rect.Left,rect.Top,rect.Left + thickness,rect.Bottom),COLOR_BEVEL_LIGHT);
	renderer->Add_Quad (RectClass (rect.Left,rect.Bottom - thickness,rect.Right,rect.Bottom),COLOR_BEVEL_DARK);
	renderer->Add_Quad (RectClass (rect.Right - thickness,rect.Top,rect.Right,rect.Bottom),COLOR_BEVEL_DARK);
}

static bool	Rect_Contains(const RectClass & rect,float x,float y)
{
	return (x >= rect.Left) && (x <= rect.Right) && (y >= rect.Top) && (y <= rect.Bottom);
}

static WideStringClass	Player_Label(int player_id,int local_id)
{
	if (player_id == 0) return WideStringClass ("NOBODY",true);
	if (player_id == local_id) return WideStringClass ("YOU",true);
	if (player_id < 0) return WideStringClass ("AI COMMANDER",true);

	PlayerRosterInterfaceClass * roster = PlayerRosterClass::Peek_Interface ();
	const unichar_t * name = (roster != nullptr) ? roster->Get_Player_Name (player_id) : nullptr;
	if (name != nullptr) return WideStringClass (name);

	char text [32];
	::snprintf (text,sizeof (text),"PLAYER %d",player_id);
	return WideStringClass (text,true);
}


void CommanderGovernancePanelRenderer::Init(bool render_available)
{
	Shutdown ();

	_Available = render_available;
	if (!_Available) return;

	_Panel = new Render2DClass ();
	_Panel->Enable_Texturing (false);
	StyleMgrClass::Configure_Renderer (_Panel);

	_Text = new Render2DSentenceClass ();
	StyleMgrClass::Assign_Font (_Text,StyleMgrClass::FONT_INGAME_TXT);
}


void CommanderGovernancePanelRenderer::Shutdown(void)
{
	if (_Panel != nullptr) { delete _Panel; _Panel = nullptr; }
	if (_Text  != nullptr) { delete _Text;  _Text  = nullptr; }

	_Available	= false;
	_Open			= false;
	_HitValid	= false;
	CommanderGovernancePanel::Reset ();
}


void CommanderGovernancePanelRenderer::Update(void)
{
	if (!_Available || (_Panel == nullptr) || (_Text == nullptr)) return;

	_Panel->Reset ();
	_Text->Reset ();
	_HitValid	= false;
	_Open			= false;
	_HitCount	= 0;

	if (!CommanderModeController::Is_Active ()) return;
	if (COMBAT_STAR == nullptr) return;

	CommanderGovernanceState state;
	CommanderGovernancePanel::Build_State (COMBAT_STAR->Get_Player_Type (),CombatManager::Get_My_Id (),&state);

	_Open = true;

	const float margin	= 8.0f;
	const float padding	= 4.0f;
	const float line		= 18.0f;
	const float button_h	= 24.0f;
	const float width		= 280.0f;

	int text_lines = (state.VoteType != 0) ? 3 : 1;
	float height = padding + (line * (float)text_lines) + padding +
						((float)state.ButtonCount * (button_h + padding)) + padding;

	RectClass panel (margin,margin,margin + width,margin + height);
	Add_Bevel (_Panel,panel,COLOR_METAL,3.0f);
	_HitPanel = panel;

	float x0 = panel.Left + padding + 4.0f;
	float x1 = panel.Right - padding;
	float y = panel.Top + padding;

	WideStringClass commander ("COMMANDER: ",true);
	commander += Player_Label (state.CommanderId,state.LocalId);
	StyleMgrClass::Render_Text (commander,_Text,COLOR_AMBER,COLOR_SHADOW,RectClass (x0,y,x1,y + line),true,true);
	y += line;

	if (state.VoteType != 0) {
		WideStringClass vote ((state.VoteType == (int)COMMANDER_VOTE_REMOVE) ? "VOTE: REMOVE " : "VOTE: ELECT ",true);
		vote += Player_Label (state.VoteCandidate,state.LocalId);
		StyleMgrClass::Render_Text (vote,_Text,COLOR_AMBER,COLOR_SHADOW,RectClass (x0,y,x1,y + line),true,true);
		y += line;

		char counts [64];
		::snprintf (counts,sizeof (counts),"%.0fs LEFT   YES %d   NO %d",state.VoteSecondsLeft,state.VoteYes,state.VoteNo);
		StyleMgrClass::Render_Text (WideStringClass (counts,true),_Text,COLOR_AMBER_DIM,COLOR_SHADOW,
											 RectClass (x0,y,x1,y + line),true,true);
		y += line;
	}

	y += padding;
	for (int index = 0; index < state.ButtonCount; index++) {
		RectClass cell (panel.Left + padding,y,panel.Right - padding,y + button_h);
		Add_Bevel (_Panel,cell,COLOR_BUTTON,2.0f);
		StyleMgrClass::Render_Text (state.Buttons [index].Label,_Text,COLOR_AMBER,COLOR_SHADOW,cell,true,true,
											 StyleMgrClass::CENTER_JUSTIFY,true);
		_HitRects [_HitCount]	= cell;
		_HitButtons [_HitCount]	= state.Buttons [index];
		_HitCount++;
		y += button_h + padding;
	}

	_HitValid = true;
}


void CommanderGovernancePanelRenderer::Render(void)
{
	if (!_Available || !_Open || (_Panel == nullptr) || (_Text == nullptr)) return;

	_Panel->Render ();
	_Text->Render ();
}


bool CommanderGovernancePanelRenderer::Hit_Test(float x,float y,CommanderGovernanceButton * out_button,
																bool * out_inside_panel)
{
	if (out_inside_panel != nullptr) *out_inside_panel = false;

	if (!_Available || !_Open || !_HitValid) return false;
	if (!Rect_Contains (_HitPanel,x,y)) return false;

	if (out_inside_panel != nullptr) *out_inside_panel = true;

	for (int index = 0; index < _HitCount; index++) {
		if (!Rect_Contains (_HitRects [index],x,y)) continue;
		if (out_button != nullptr) *out_button = _HitButtons [index];
		return true;
	}
	return false;
}


bool CommanderGovernancePanelRenderer::Is_Open(void)	{ return _Open; }

class CommanderGovernancePanelRendererOverlayClass : public IRTSOverlayComponent
{
public:
	virtual void	Render_RTS_Overlay(RenderInfoClass & /* rinfo */) override	{ CommanderGovernancePanelRenderer::Render (); }
	virtual bool	Is_RTS_Overlay_Visible(void) const override				{ return CommanderGovernancePanelRenderer::Is_Open (); }
};

static CommanderGovernancePanelRendererOverlayClass	_OverlayComponent;

IRTSOverlayComponent & CommanderGovernancePanelRenderer::Get_Overlay_Component(void)	{ return _OverlayComponent; }
