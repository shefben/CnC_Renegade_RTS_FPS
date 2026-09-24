/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandergovernancepanel.h            $*
 *                                                                                             *
 *	V2 roadmap Phase 23 / Section 25.1 governance, on screen: who commands the team, the vote    *
 *	in progress (what it is, who it names, time left, yes and no counts) with buttons to vote,   *
 *	a button to call a removal vote or claim a vacant seat, and -- for the sitting Commander --  *
 *	a hand-off button per teammate.                                                              *
 *                                                                                             *
 *	The model reads only what a client really has: the Commander and the vote come from        *
 *	CommanderStateClass's mirror, names from PlayerRosterClass.  A button is a request -- the     *
 *	caller (CommanderUIInput) sends the matching existing event, the server decides, and the      *
 *	panel shows the outcome when the mirror next changes.  The console commands stay for admin   *
 *	and debugging.                                                                               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERGOVERNANCEPANEL_H
#define COMMANDERGOVERNANCEPANEL_H

#include "always.h"
#include "widestring.h"

enum CommanderGovernanceActionType
{
	GOVERNANCE_ACTION_NONE = 0,
	GOVERNANCE_ACTION_VOTE_YES,
	GOVERNANCE_ACTION_VOTE_NO,
	GOVERNANCE_ACTION_CALL_REMOVE,		// a removal vote against the sitting Commander
	GOVERNANCE_ACTION_CLAIM,				// claim a vacant seat (contested claims become an election)
	GOVERNANCE_ACTION_TRANSFER,			// hand the seat to TargetId
};

struct CommanderGovernanceButton
{
	CommanderGovernanceButton(void) : Action(GOVERNANCE_ACTION_NONE),TargetId(0) { }

	CommanderGovernanceActionType	Action;
	int									TargetId;
	WideStringClass					Label;
};

struct CommanderGovernanceState
{
	enum { MAX_BUTTONS = 8 };

	CommanderGovernanceState(void) : Team(-1),LocalId(0),CommanderId(0),VoteType(0),VoteCandidate(0),
												VoteSecondsLeft(0.0f),VoteYes(0),VoteNo(0),ButtonCount(0) { }

	int							Team;
	int							LocalId;
	int							CommanderId;
	int							VoteType;			// a CommanderVoteType
	int							VoteCandidate;
	float							VoteSecondsLeft;
	int							VoteYes;
	int							VoteNo;

	int							ButtonCount;
	CommanderGovernanceButton	Buttons [MAX_BUTTONS];
};

class CommanderGovernancePanel
{
	public:

		/*
		**	The buttons follow from the state alone: an open vote offers YES/NO until the local
		**	player has voted in it; no vote offers CLAIM on a vacant seat, or CALL REMOVE against
		**	someone else's; the sitting Commander is offered a hand-off to each teammate.  Pure
		**	apart from the roster lookup for teammate names, which a null roster skips.
		*/
		static void		Build_State(int team,int local_id,CommanderGovernanceState * out);

		//	Remember that the local player voted in the vote now open, so its buttons go away.
		static void		Note_Local_Vote(int team);
		static void		Reset(void);
};

class CommanderGovernancePanelRenderer
{
	public:

		static void		Init(bool render_available = true);
		static void		Shutdown(void);

		static void		Update(void);
		static void		Render(void);

		//	The button under a point, as the action and its target.  Same inside/between-buttons
		//	contract as the other Commander panels.
		static bool		Hit_Test(float x,float y,CommanderGovernanceButton * out_button,bool * out_inside_panel);
};

#endif // COMMANDERGOVERNANCEPANEL_H
