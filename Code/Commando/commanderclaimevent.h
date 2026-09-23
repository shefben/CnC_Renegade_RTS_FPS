/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderclaimevent.h               $*
 *                                                                                             *
 *	Roadmap Section 45 -- how a player becomes their team's Commander.                          *
 *                                                                                             *
 *	`CommanderProductionService` has always had `Set_Team_Commander`, and every authority gate    *
 *	in the Commander layer reads it -- production, placement and orders all refuse a request      *
 *	whose sender is not the recorded Commander for that team.  Nothing ever called the setter      *
 *	except the client-side state mirror, the save-game loader and the self checks, so on a live     *
 *	server `_Commanders[]` stayed zero for the whole match and every one of those gates failed      *
 *	closed.  The sidebar could be opened and nothing it offered could ever be bought.               *
 *                                                                                             *
 *	This is the missing half: the one client->server message that asks for the job, and the        *
 *	server-side rule that grants or refuses it.  A vacant seat with no other connected teammate     *
 *	grants instantly -- there is no one to contest it, which is also why a solo or skirmish match   *
 *	has exactly one claimant and the player is always their team's Commander there without a       *
 *	special case.  A vacant seat with a real teammate on the same team instead opens a Roadmap      *
 *	Section 25.1 governance vote (`CommanderProductionService::Call_Commander_Vote`, see            *
 *	commanderproduction.h) rather than granting on the spot -- Section 25.1's own "no                *
 *	first-player-wins permanent rule."  Releasing, and re-claiming a seat already held, remain      *
 *	instant; removing someone else's held command goes through `cCommanderVoteCallEvent`.           *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERCLAIMEVENT_H
#define COMMANDERCLAIMEVENT_H

#include "always.h"
#include "netevent.h"
#include "netclassids.h"


class cCommanderClaimEvent : public cNetEvent
{
	public:

		cCommanderClaimEvent(void);

		//	`want_command` true asks for the job, false gives it up.
		void	Init(bool want_command);

		virtual void	Export_Creation(BitStreamClass & packet) override;
		virtual void	Import_Creation(BitStreamClass & packet) override;

		virtual uint32	Get_Network_Class_ID(void) const override
			{ return NETCLASSID_COMMANDERCLAIMEVENT; }

	private:

		virtual void	Act(void) override;

		int	SenderId;
		int	WantCommand;
};

#endif // COMMANDERCLAIMEVENT_H
